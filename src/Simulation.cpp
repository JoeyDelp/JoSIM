// Copyright (c) 2025 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/Simulation.hpp"

#include "JoSIM/AnalysisType.hpp"
#include "JoSIM/Components.hpp"
#include "JoSIM/Constants.hpp"
#include "JoSIM/Matrix.hpp"
#include "JoSIM/Model.hpp"
#include "JoSIM/ProgressBar.hpp"
#include "JoSIM/Rng.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <new>
#include <stdexcept>

using namespace JoSIM;

namespace {
// Local ownership also covers exceptions and retries during construction.
struct KluResources {
    klu_l_symbolic*& symbolic;
    klu_l_numeric*& numeric;
    klu_l_common& common;
    ~KluResources() {
        klu_l_free_numeric(&numeric, &common);
        klu_l_free_symbolic(&symbolic, &common);
    }
};

[[noreturn]] void solver_failure(const klu_l_common& common) {
    if (common.status == KLU_SINGULAR) {
        Errors::simulation_errors(SimulationErrors::MATRIX_SINGULAR);
    }
    Errors::simulation_errors(SimulationErrors::SOLVER_FAILURE,
                              "Status " + std::to_string(common.status));
}

void validate_matrix(const Matrix& matrix) {
    const auto n = matrix.branchIndex;
    if (n <= 0 || matrix.rp.size() != static_cast<size_t>(n) + 1
        || matrix.ci.empty() || matrix.ci.size() != matrix.nz.size()
        || matrix.rp.front() != 0 || matrix.rp.back() != static_cast<int64_t>(matrix.nz.size())) {
        Errors::simulation_errors(SimulationErrors::INVALID_MATRIX, "Inconsistent dimensions or empty circuit.");
    }
    for (size_t i = 1; i < matrix.rp.size(); ++i) {
        if (matrix.rp[i] < matrix.rp[i - 1] || matrix.rp[i] > matrix.rp.back()) {
            Errors::simulation_errors(SimulationErrors::INVALID_MATRIX, "Invalid row offsets.");
        }
    }
    for (size_t i = 0; i < matrix.ci.size(); ++i) {
        if (matrix.ci[i] < 0 || matrix.ci[i] >= n || !std::isfinite(matrix.nz[i])) {
            Errors::simulation_errors(SimulationErrors::INVALID_MATRIX, "Invalid column index or non-finite coefficient.");
        }
    }
}

void validate_solution(const std::vector<double>& solution) {
    if (!std::all_of(solution.begin(), solution.end(), [](double value) { return std::isfinite(value); })) {
        Errors::simulation_errors(SimulationErrors::SOLVER_FAILURE, "Non-finite solution; model or numerical range exceeded");
    }
}
} // namespace

Simulation::Simulation(Input& iObj, Matrix& mObj) {
    while (needsTR_) {
        validate_matrix(mObj);
        // Do generic simulation setup for given step size
        setup(iObj, mObj);
        // Do solver setup
        // KLU setup
        simOK_ = klu_l_defaults(&Common_);
        if (!simOK_) { solver_failure(Common_); }
        KluResources resources{Symbolic_, Numeric_, Common_};
        Symbolic_ = klu_l_analyze(mObj.rp.size() - 1, &mObj.rp.front(), &mObj.ci.front(), &Common_);
        if (!Symbolic_) { solver_failure(Common_); }
        Numeric_  = klu_l_factor(&mObj.rp.front(), &mObj.ci.front(), &mObj.nz.front(), Symbolic_, &Common_);
        if (!Numeric_) { solver_failure(Common_); }

        // Run transient simulation
        trans_sim(mObj);
        // If step size is too large, reduce and try again
        if (needsTR_) { reduce_step(iObj, mObj); }

    }
}

void Simulation::setup(Input& iObj, Matrix& mObj) {
    // Simulation setup
    simSize_  = iObj.transSim.simsize();
    atyp_     = iObj.argAnal;
    minOut_   = iObj.argMin;
    needsLU_  = false;
    needsTR_  = false;
    stepSize_ = iObj.transSim.tstep();
    prstep_   = iObj.transSim.prstep();
    prstart_  = iObj.transSim.prstart();
    startup_  = iObj.transSim.startup();
    x_.clear();
    x_.resize(mObj.branchIndex, 0.0);
    results.sourceCurrents.clear();
    results.phaseBefore.clear();
    const auto capture_source = [&](const RelevantTrace& trace) {
        if (trace.storageType == StorageType::Current && trace.sourceIndex) {
            if (results.sourceCurrents.empty()) { results.sourceCurrents.resize(mObj.sourcegen.size()); }
            results.sourceCurrents.at(trace.sourceIndex.value()).emplace();
        }
    };
    for (const auto& trace : mObj.relevantTraces) {
        capture_source(trace);
        if (trace.subtract) { capture_source(*trace.subtract); }
    }
    {
        const auto converted = atyp_ == AnalysisType::Phase ? StorageType::Voltage : StorageType::Phase;
        const auto capture_phase = [&](const RelevantTrace& trace) {
            if (trace.storageType == converted && !trace.variableIndex) {
                if (trace.index1) { results.phaseBefore.emplace(trace.index1.value(), std::array<double, 2>{}); }
                if (trace.index2) { results.phaseBefore.emplace(trace.index2.value(), std::array<double, 2>{}); }
            }
        };
        if (mObj.relevantTraces.empty()) {
            // Preserve all nodal history with the default all-unknown result set.
            for (const auto& node : mObj.nm) { results.phaseBefore.emplace(node.second, std::array<double, 2>{}); }
        } else {
            for (const auto& trace : mObj.relevantTraces) {
                capture_phase(trace);
                if (trace.subtract) { capture_phase(*trace.subtract); }
            }
        }
    }
    if (!mObj.relevantTraces.empty()) {
        results.xVector.resize(mObj.branchIndex);
        for (const auto& i : mObj.relevantIndices) { results.xVector.at(i).emplace(); }
    } else {
        results.xVector.resize(mObj.branchIndex, std::vector<double>(0));
    }
    // The complete internal grid is retained, including when output is decimated.
    // Reserve only histories that are actually requested (or the default full set).
    try {
        const auto samples = static_cast<size_t>(simSize_);
        results.timeAxis.reserve(samples);
        for (auto& history : results.xVector) {
            if (history) { history.value().reserve(samples); }
        }
        for (auto& history : results.sourceCurrents) {
            if (history) { history.value().reserve(samples); }
        }
    } catch (const std::bad_alloc&) {
        Errors::simulation_errors(SimulationErrors::STORAGE_ALLOCATION_FAILED, std::to_string(simSize_));
    } catch (const std::length_error&) {
        Errors::simulation_errors(SimulationErrors::STORAGE_ALLOCATION_FAILED, std::to_string(simSize_));
    }
}

void Simulation::trans_sim(Matrix& mObj) {
    // Ensure time axis is cleared
    results.timeAxis.clear();
    ProgressBar bar;
    if (!minOut_) {
        bar.set_bar_width(30);
        bar.fill_bar_progress_with("O");
        bar.fill_bar_remainder_with(" ");
        bar.set_status_text("Simulating");
        bar.set_total((float) simSize_);
        bar.create_thread();
    }
    // Initialize the b matrix
    b_.resize(mObj.rp.size(), 0.0);
    capacitorIC_ = initialize_capacitors(mObj);
    if (startup_) {
        // Stabilize the simulation before starting at t=0
        const double proposed = 2 * pow(10, (abs(log10(stepSize_)) - 12) * 2 + 1);
        const int64_t startup = proposed >= 1000 ? 1000 : static_cast<int64_t>(proposed);
        if (!std::isfinite(static_cast<double>(startup) * stepSize_)) {
            Errors::control_errors(ControlErrors::TRANS_ERROR, "Startup interval exceeds the supported range");
        }
        for (int64_t i = -startup; i < 0; ++i) {
            // Setup the b matrix
            setup_b(mObj, i, i * stepSize_);
            if (needsTR_) { return; }
            // Assign x_prev the new b
            x_ = b_;
            // Solve Ax=b, storing the results in x_
            simOK_ = klu_l_tsolve(Symbolic_, Numeric_, mObj.rp.size() - 1, 1, &x_.front(), &Common_);
            // If anything is a amiss, complain about it
            if (!simOK_) { Errors::simulation_errors(SimulationErrors::MATRIX_SINGULAR); }
            validate_solution(x_);
            for (auto& history : results.phaseBefore) {
                const auto node = x_.at(history.first);
                const double phase = atyp_ == AnalysisType::Phase ? node
                    : (2.0 * stepSize_ / (3.0 * Constants::SIGMA)) * node
                      + (4.0 / 3.0) * history.second[0] - (1.0 / 3.0) * history.second[1];
                history.second[1] = history.second[0];
                history.second[0] = phase;
            }
        }
    }
    // Start the simulation loop
    for (int64_t i = capacitorIC_ ? 1 : 0; i < simSize_; ++i) {
        double step = i * stepSize_;
        // If not minimal printing report progress
        if (!minOut_) { bar.update(static_cast<float>(i)); }
        // Setup the b matrix
        setup_b(mObj, i, i * stepSize_);
        if (needsTR_) { return; }
        // Assign x_prev the new b
        x_ = b_;
        // Solve Ax=b, storing the results in x_
        simOK_ = klu_l_tsolve(Symbolic_, Numeric_, mObj.rp.size() - 1, 1, &x_.front(), &Common_);
        // If anything is a amiss, complain about it
        if (!simOK_) { Errors::simulation_errors(SimulationErrors::MATRIX_SINGULAR); }
        validate_solution(x_);
        // Store results (only requested, to prevent massive memory usage)
        for (auto j = 0; j < results.xVector.size(); ++j) {
            if (results.xVector.at(j)) { results.xVector.at(j).value().emplace_back(x_.at(j)); }
        }
        // Store the time step
        results.timeAxis.emplace_back(step);
    }
    if (!minOut_) {
        bar.complete();
        std::cout << "\n";
    }
}

void Simulation::reduce_step(Input& iObj, Matrix& mObj) {
    iObj.transSim.tstep(iObj.transSim.tstep() / 2);
    Rng::rewind();
    bool tempMinOut = iObj.argMin;
    if (!iObj.argMin) { iObj.argMin = true; }
    Matrix newmObj;
    mObj = newmObj;
    // Create the matrix in csr format
    for (auto& i : mObj.sourcegen) { i.clearMisc(); }
    mObj.create_matrix(iObj);
    find_relevant_traces(iObj, mObj);
    //// Dump expanded Netlist since it is no longer needed
    // iObj.netlist.expNetlist.clear();
    // iObj.netlist.expNetlist.shrink_to_fit();
    if (!tempMinOut) { iObj.argMin = tempMinOut; }
    results.xVector.clear();
    results.sourceCurrents.clear();
    results.phaseBefore.clear();
    results.timeAxis.clear();
}

void Simulation::setup_b(Matrix& mObj, int64_t i, double step, double factor) {
    // Clear b matrix and reset
    b_.clear();
    b_.resize(mObj.rp.size(), 0.0);
    // Handle jj
    handle_jj(mObj, i, step, factor);
    if (needsTR_) { return; }
    // Re-factorize the LU if any jj transitions
    if (needsLU_) {
        mObj.create_nz();
        validate_matrix(mObj);
        klu_l_free_numeric(&Numeric_, &Common_);
        Numeric_ = klu_l_factor(&mObj.rp.front(), &mObj.ci.front(), &mObj.nz.front(), Symbolic_, &Common_);
        if (!Numeric_) { solver_failure(Common_); }
        needsLU_ = false;
    }
    // Handle current sources
    handle_cs(mObj, step, i);
    // Handle resistors
    handle_resistors(mObj, step);
    // Handle inductors
    handle_inductors(mObj, factor);
    // Handle capacitors
    handle_capacitors(mObj);
    // Handle voltage sources
    handle_vs(mObj, i, step, factor);
    // Handle phase sources
    handle_ps(mObj, i, step, factor);
    // Handle ccvs
    handle_ccvs(mObj);
    // Handle vccs
    handle_vccs(mObj);
    // Handle transmission lines
    handle_tx(mObj, i, step);
}

void Simulation::handle_cs(Matrix& mObj, double& step, const int64_t& i) {
    for (const auto& j : mObj.components.currentsources) {
        const double value = mObj.sourcegen.at(j.sourceIndex_).value(step);
        if (i >= 0 && !results.sourceCurrents.empty() && results.sourceCurrents.at(j.sourceIndex_)) {
            results.sourceCurrents.at(j.sourceIndex_).value().emplace_back(value);
        }
        if (j.indexInfo.nodeConfig_ == NodeConfig::POSGND) {
            b_.at(j.indexInfo.posIndex_.value()) -= value;
        } else if (j.indexInfo.nodeConfig_ == NodeConfig::GNDNEG) {
            b_.at(j.indexInfo.negIndex_.value()) += value;
        } else if (j.indexInfo.nodeConfig_ == NodeConfig::POSNEG) {
            b_.at(j.indexInfo.posIndex_.value()) -= value;
            b_.at(j.indexInfo.negIndex_.value()) += value;
        }
    }
}

void Simulation::handle_resistors(Matrix& mObj, double& step) {
    double noiseVal;
    for (const auto& j : mObj.components.resistorIndices) {
        auto&       temp = std::get<Resistor>(mObj.components.devices.at(j));
        if (temp.thermalNoise) {
            noiseVal = temp.thermalNoise.value().value(step);
        } else {
            noiseVal = 0.0;
        }
        NodeConfig& nc   = temp.indexInfo.nodeConfig_;
        if (nc == NodeConfig::POSGND) {
            b_.at(temp.indexInfo.posIndex_.value()) -= noiseVal;
            temp.pn1_ = (x_.at(temp.indexInfo.posIndex_.value()));
        } else if (nc == NodeConfig::GNDNEG) {
            b_.at(temp.indexInfo.negIndex_.value()) += noiseVal;
            temp.pn1_ = (-x_.at(temp.indexInfo.negIndex_.value()));
        } else if (nc == NodeConfig::POSNEG) {
            b_.at(temp.indexInfo.posIndex_.value()) -= noiseVal;
            b_.at(temp.indexInfo.negIndex_.value()) += noiseVal;
            temp.pn1_ = (x_.at(temp.indexInfo.posIndex_.value()) - x_.at(temp.indexInfo.negIndex_.value()));
        } else {
            temp.pn1_ = 0.0;
        }
        if (atyp_ == AnalysisType::Phase) {
            // 4/3 φp1 - 1/3 φp2
            b_.at(temp.indexInfo.currentIndex_.value()) = (4.0 / 3.0) * temp.pn1_ - (1.0 / 3.0) * temp.pn2_;
            temp.pn4_                                   = temp.pn3_;
            temp.pn3_                                   = temp.pn2_;
            temp.pn2_                                   = temp.pn1_;
        }
    }
}

void Simulation::handle_inductors(Matrix& mObj, double factor) {
    for (const auto& j : mObj.components.inductorIndices) {
        auto& temp = std::get<Inductor>(mObj.components.devices.at(j));
        if (atyp_ == AnalysisType::Voltage) {
            // -2L/h Ip + L/2h Ip2
            b_.at(temp.indexInfo.currentIndex_.value())
                    = -(2.0 * temp.netlistInfo.value_ / (stepSize_ * factor))
                              * x_.at(temp.indexInfo.currentIndex_.value())
                      + ((temp.netlistInfo.value_ / (2.0 * (stepSize_ * factor))) * temp.In2_);
            // -2M/h Im + M/2h Im2
            for (const auto& m : temp.mutualInductances_) {
                Inductor& mi = std::get<Inductor>(mObj.components.devices.at(m.first));
                b_.at(temp.indexInfo.currentIndex_.value())
                        += (-((2 * m.second) / (stepSize_ * factor)) * x_.at(mi.indexInfo.currentIndex_.value())
                            + (m.second / (2.0 * (stepSize_ * factor))) * mi.In2_);
            }
        }
    }
    if (atyp_ == AnalysisType::Voltage) {
        // Every coupled branch must read the same preceding current histories.
        for (const auto& j : mObj.components.inductorIndices) {
            auto& temp = std::get<Inductor>(mObj.components.devices.at(j));
            temp.In4_ = temp.In3_;
            temp.In3_ = temp.In2_;
            temp.In2_ = x_.at(temp.indexInfo.currentIndex_.value());
        }
    }
}

void Simulation::handle_capacitors(Matrix& mObj) {
    for (const auto& j : mObj.components.capacitorIndices) {
        auto& temp = std::get<Capacitor>(mObj.components.devices.at(j));
        if (temp.indexInfo.posIndex_ && !temp.indexInfo.negIndex_) {
            temp.pn1_ = (x_.at(temp.indexInfo.posIndex_.value()));
        } else if (!temp.indexInfo.posIndex_ && temp.indexInfo.negIndex_) {
            temp.pn1_ = (-x_.at(temp.indexInfo.negIndex_.value()));
        } else if (temp.indexInfo.posIndex_ && temp.indexInfo.negIndex_) {
            temp.pn1_ = (x_.at(temp.indexInfo.posIndex_.value()) - x_.at(temp.indexInfo.negIndex_.value()));
        } else {
            temp.pn1_ = 0.0;
        }
        if (atyp_ == AnalysisType::Voltage) {
            // 4/3 Vp1 - 1/3 Vp2
            b_.at(temp.indexInfo.currentIndex_.value()) = (4.0 / 3.0) * temp.pn1_ - (1.0 / 3.0) * temp.pn2_;
        } else if (atyp_ == AnalysisType::Phase) {
            // (8/3)φn-1 - (22/9)φn-2 + (8/9)φn-3 - (1/9)φn-4
            b_.at(temp.indexInfo.currentIndex_.value()) = (8.0 / 3.0) * temp.pn1_ - (22.0 / 9.0) * temp.pn2_
                                                          + (8.0 / 9.0) * temp.pn3_ - (1.0 / 9.0) * temp.pn4_;
            temp.pn7_ = temp.pn6_;
            temp.pn6_ = temp.pn5_;
            temp.pn5_ = temp.pn4_;
            temp.pn4_ = temp.pn3_;
            temp.pn3_ = temp.pn2_;
        }
        temp.pn2_ = temp.pn1_;
    }
}

void Simulation::handle_jj(Matrix& mObj, int64_t& i, double& step, double factor) {
    for (const auto& j : mObj.components.junctionIndices) {
        auto&       temp  = std::get<JJ>(mObj.components.devices.at(j));
        const auto& model = temp.model_;

        // ---- Node voltage/phase "pn1_" and add thermal noise (as current injection) ----
        if (temp.indexInfo.posIndex_ && !temp.indexInfo.negIndex_) {
            temp.pn1_ = (x_.at(temp.indexInfo.posIndex_.value()));
        } else if (!temp.indexInfo.posIndex_ && temp.indexInfo.negIndex_) {
            temp.pn1_ = (-x_.at(temp.indexInfo.negIndex_.value()));
        } else if (temp.indexInfo.posIndex_ && temp.indexInfo.negIndex_) {
            temp.pn1_ = (x_.at(temp.indexInfo.posIndex_.value()) - x_.at(temp.indexInfo.negIndex_.value()));
        } else {
            temp.pn1_ = 0.0;
        }

        // Use the previous solved phase/voltage during startup and at t=0,
        // as well as later samples. Zero-state runs already start with x_=0.
        if (atyp_ == AnalysisType::Voltage) {
            temp.vn1_ = temp.pn1_;
            temp.pn1_ = x_.at(temp.variableIndex_);
        } else if (atyp_ == AnalysisType::Phase) {
            temp.vn1_ = x_.at(temp.variableIndex_);
            // temp.pn1_ already holds the node phase difference
        }

        // ---- Guess voltage (V0) and phase (phi0) ----
        double v0  = (5.0 / 2.0) * temp.vn1_ - 2.0 * temp.vn2_ + (1.0 / 2.0) * temp.vn3_;
        temp.phi0_ = (4.0 / 3.0) * temp.pn1_ - (1.0 / 3.0) * temp.pn2_
                     + ((1.0 / Constants::SIGMA) * ((2.0 * (stepSize_)) / 3.0)) * v0;

        // ---- Timestep stability check ----
        const double dphi = temp.phi0_ - temp.pn1_;
        if ((double) i / (double) simSize_ > 0.01) {
            if (std::abs(dphi) > (0.20 * Constants::TWO_PI)) {
                needsTR_ = true;
                return;
            }
        }

        // ---- RHS1 (phase/voltage equation row) ----
        if (atyp_ == AnalysisType::Voltage) {
            b_.at(temp.variableIndex_)
                    = (Constants::SIGMA) * (-(2.0 / (stepSize_)) * temp.pn1_ + (1.0 / (2.0 * (stepSize_))) * temp.pn2_);
        } else if (atyp_ == AnalysisType::Phase) {
            b_.at(temp.variableIndex_) = (4.0 / 3.0) * temp.pn1_ - (1.0 / 3.0) * temp.pn2_;
        }

        // ---- Shift history (phase + voltage) ----
        temp.pn4_ = temp.pn3_;
        temp.pn3_ = temp.pn2_;
        temp.pn2_ = temp.pn1_;

        temp.vn6_ = temp.vn5_;
        temp.vn5_ = temp.vn4_;
        temp.vn4_ = temp.vn3_;
        temp.vn3_ = temp.vn2_;

        // ---- Update junction transition (may trigger LU refactor) ----
        if (model.rtype() == 1) {
            const bool testLU = temp.update_value(v0);
            if (testLU && !needsLU_) { needsLU_ = true; }
        }
        // Apply one sample using the conductance selected for this step.
        if (temp.thermalNoise) {
            const double noise = temp.thermalNoise.value().value(step);
            if (temp.indexInfo.posIndex_) { b_.at(*temp.indexInfo.posIndex_) -= noise; }
            if (temp.indexInfo.negIndex_) { b_.at(*temp.indexInfo.negIndex_) += noise; }
        }
        double gateFactor = 1.0;
        if (temp.gateIndex_) {
            const double signal = x_.at(temp.gateIndex_.value());
            double voltage = signal;
            if (atyp_ == AnalysisType::Phase) {
                voltage = Constants::SIGMA * (3.0 * signal - 4.0 * temp.gatePn1_ + temp.gatePn2_)
                          / (2.0 * stepSize_);
                temp.gatePn2_ = temp.gatePn1_;
                temp.gatePn1_ = signal;
            }
            gateFactor = std::clamp(1.0 - std::abs(voltage) / model.gwidth(), 0.0, 1.0);
        }

        // Fractional critical-current noise: δ(t) dimensionless, generated by ICNOISE(...)
        double dIc = 0.0;
        if (temp.icNoise) { dIc = temp.icNoise.value().value(step); }
        if (dIc < -0.99) { dIc = -0.99; }

        // ---- RHS2 (junction current equation row) ----
        b_.at(temp.indexInfo.currentIndex_.value())
                = temp.matrixInfo.nonZeros_.back()
                  * (temp.supercurrent(temp.phi0_, gateFactor * (1.0 + dIc))
                     - ((2.0 * model.c()) / stepSize_) * temp.vn1_
                     + (model.c() / (2.0 * stepSize_)) * temp.vn2_ + temp.it_);

        // ---- Final history update (as in your original code) ----
        temp.vn2_ = temp.vn1_;
    }
}

void Simulation::handle_vs(Matrix& mObj, const int64_t& i, double& step, double factor) {
    for (const auto& j : mObj.components.vsIndices) {
        auto&              temp = std::get<VoltageSource>(mObj.components.devices.at(j));
        JoSIM::NodeConfig& nc   = temp.indexInfo.nodeConfig_;
        if (atyp_ == AnalysisType::Voltage) {
            // Vn
            b_.at(temp.indexInfo.currentIndex_.value()) = (mObj.sourcegen.at(temp.sourceIndex_).value(step));
        } else if (atyp_ == AnalysisType::Phase) {
            if (nc == NodeConfig::POSGND) {
                temp.pn1_ = (x_.at(temp.indexInfo.posIndex_.value()));
            } else if (nc == NodeConfig::GNDNEG) {
                temp.pn1_ = (-x_.at(temp.indexInfo.negIndex_.value()));
            } else if (nc == NodeConfig::POSNEG) {
                temp.pn1_ = (x_.at(temp.indexInfo.posIndex_.value()) - x_.at(temp.indexInfo.negIndex_.value()));
            } else {
                // This branch is only taken as a result of a GND-GND short-circuited voltage source.
                // In this case, the matrix is singular and the program will throw an error.
                // Without this branch, the program could crash due to std::bad_optional_access.
                temp.pn1_ = 0.0;
            }
            // (2e/hbar)(2h/3)Vn + (4/3)φn-1 - (1/3)φn-2
            b_.at(temp.indexInfo.currentIndex_.value()) = ((2 * stepSize_ * factor) / (3 * Constants::SIGMA))
                                                                  * (mObj.sourcegen.at(temp.sourceIndex_).value(step))
                                                          + (4.0 / 3.0) * temp.pn1_ - (1.0 / 3.0) * temp.pn2_;
            temp.pn4_ = temp.pn3_;
            temp.pn3_ = temp.pn2_;
            temp.pn2_ = temp.pn1_;
        }
    }
}

void Simulation::handle_ps(Matrix& mObj, const int64_t& i, double& step, double factor) {
    for (const auto& j : mObj.components.psIndices) {
        auto& temp = std::get<PhaseSource>(mObj.components.devices.at(j));
        const double phase = mObj.sourcegen.at(temp.sourceIndex_).value(step);
        if (atyp_ == AnalysisType::Phase) {
            // φn
            b_.at(temp.indexInfo.currentIndex_.value()) = phase;
        } else if (atyp_ == AnalysisType::Voltage) {
            b_.at(temp.indexInfo.currentIndex_.value()) = (Constants::SIGMA / (stepSize_ * factor))
                * (1.5 * phase - 2.0 * temp.pn1_ + 0.5 * temp.pn2_);
        }
        temp.pn2_ = temp.pn1_;
        temp.pn1_ = phase;
    }
}

void Simulation::handle_ccvs(Matrix& mObj) {
    for (const auto& j : mObj.components.ccvsIndices) {
        auto& temp = std::get<CCVS>(mObj.components.devices.at(j));
        if (atyp_ == AnalysisType::Phase) {
            if (temp.indexInfo.posIndex_ && !temp.indexInfo.negIndex_) {
                temp.pn1_ = (x_.at(temp.indexInfo.posIndex_.value()));
            } else if (!temp.indexInfo.posIndex_ && temp.indexInfo.negIndex_) {
                temp.pn1_ = (-x_.at(temp.indexInfo.negIndex_.value()));
            } else if (temp.indexInfo.posIndex_ && temp.indexInfo.negIndex_) {
                temp.pn1_ = (x_.at(temp.indexInfo.posIndex_.value()) - x_.at(temp.indexInfo.negIndex_.value()));
            } else {
                temp.pn1_ = 0.0;
            }
            b_.at(temp.indexInfo.currentIndex_.value()) = (4.0 / 3.0) * temp.pn1_ - (1.0 / 3.0) * temp.pn2_;
            temp.pn4_                                   = temp.pn3_;
            temp.pn3_                                   = temp.pn2_;
            temp.pn2_                                   = temp.pn1_;
        }
    }
}

void Simulation::handle_vccs(Matrix& mObj) {
    for (const auto& j : mObj.components.vccsIndices) {
        auto& temp = std::get<VCCS>(mObj.components.devices.at(j));
        if (atyp_ == AnalysisType::Phase) {
            if (temp.posIndex2_ && !temp.negIndex2_) {
                temp.pn1_ = (x_.at(temp.posIndex2_.value()));
            } else if (!temp.posIndex2_ && temp.negIndex2_) {
                temp.pn1_ = (-x_.at(temp.negIndex2_.value()));
            } else if (temp.posIndex2_ && temp.negIndex2_) {
                temp.pn1_ = (x_.at(temp.posIndex2_.value()) - x_.at(temp.negIndex2_.value()));
            } else {
                temp.pn1_ = 0.0;
            }
            b_.at(temp.indexInfo.currentIndex_.value()) = (4.0 / 3.0) * temp.pn1_ - (1.0 / 3.0) * temp.pn2_;
            temp.pn4_                                   = temp.pn3_;
            temp.pn3_                                   = temp.pn2_;
            temp.pn2_                                   = temp.pn1_;
        }
    }
}

void Simulation::handle_tx(Matrix& mObj, const int64_t& i, double& step, double factor) {
    for (const auto& j : mObj.components.txIndices) {
        auto&              temp = std::get<TransmissionLine>(mObj.components.devices.at(j));
        // Z0
        double&            Z    = temp.netlistInfo.value_;
        // Td == k
        int64_t&           k    = temp.timestepDelay_;
        // Shorthands
        JoSIM::NodeConfig &nc = temp.indexInfo.nodeConfig_, &nc2 = temp.nodeConfig2_;
        int_o &posInd = temp.indexInfo.posIndex_, &negInd = temp.indexInfo.negIndex_, &posInd2 = temp.posIndex2_,
              &negInd2  = temp.negIndex2_;
        int64_t &curInd = temp.indexInfo.currentIndex_.value(), &curInd2 = temp.currentIndex2_;
        if (atyp_ == AnalysisType::Voltage) {
            if (i >= k) {
                // φ1n-k
                if (nc == NodeConfig::POSGND) {
                    temp.nk_1_ = results.xVector.at(posInd.value()).value().at(i - k);
                } else if (nc == NodeConfig::GNDNEG) {
                    temp.nk_1_ = -results.xVector.at(negInd.value()).value().at(i - k);
                } else if (nc == NodeConfig::POSNEG) {
                    temp.nk_1_ = results.xVector.at(posInd.value()).value().at(i - k)
                                 - results.xVector.at(negInd.value()).value().at(i - k);
                } else {
                    temp.nk_1_ = 0.0;
                }
                // φ2n-k
                if (nc2 == NodeConfig::POSGND) {
                    temp.nk_2_ = results.xVector.at(posInd2.value()).value().at(i - k);
                } else if (nc2 == NodeConfig::GNDNEG) {
                    temp.nk_2_ = -results.xVector.at(negInd2.value()).value().at(i - k);
                } else if (nc2 == NodeConfig::POSNEG) {
                    temp.nk_2_ = results.xVector.at(posInd2.value()).value().at(i - k)
                                 - results.xVector.at(negInd2.value()).value().at(i - k);
                } else {
                    temp.nk_2_ = 0.0;
                }
                // I1n-k
                double& I1nk   = results.xVector.at(curInd).value().at(i - k);
                // I2n-k
                double& I2nk   = results.xVector.at(curInd2).value().at(i - k);
                // I1 = ZI2n-k + V2n-k
                b_.at(curInd)  = Z * I2nk + temp.nk_2_;
                // I2 = ZI1n-k + V1n-k
                b_.at(curInd2) = Z * I1nk + temp.nk_1_;
            }
        } else if (atyp_ == AnalysisType::Phase) {
            if (i > 0) {
                temp.n2_1_ = temp.n1_1_;
                temp.n2_2_ = temp.n1_2_;
                // φ1n-1
                if (nc == NodeConfig::POSGND) {
                    temp.n1_1_ = (x_.at(posInd.value()));
                } else if (nc == NodeConfig::GNDNEG) {
                    temp.n1_1_ = (-x_.at(negInd.value()));
                } else if (nc == NodeConfig::POSNEG) {
                    temp.n1_1_ = (x_.at(posInd.value()) - x_.at(negInd.value()));
                } else {
                    temp.n1_1_ = 0.0;
                }
                // φ2n-1
                if (nc2 == NodeConfig::POSGND) {
                    temp.n1_2_ = (x_.at(posInd2.value()));
                } else if (nc2 == NodeConfig::GNDNEG) {
                    temp.n1_2_ = (-x_.at(negInd2.value()));
                } else if (nc2 == NodeConfig::POSNEG) {
                    temp.n1_2_ = (x_.at(posInd2.value()) - x_.at(negInd2.value()));
                } else {
                    temp.n1_2_ = 0.0;
                }
            }
            if (i >= k) {
                // φ1n-k
                if (nc == NodeConfig::POSGND) {
                    temp.nk_1_ = results.xVector.at(posInd.value()).value().at(i - k);
                } else if (nc == NodeConfig::GNDNEG) {
                    temp.nk_1_ = -results.xVector.at(negInd.value()).value().at(i - k);
                } else if (nc == NodeConfig::POSNEG) {
                    temp.nk_1_ = results.xVector.at(posInd.value()).value().at(i - k)
                                 - results.xVector.at(negInd.value()).value().at(i - k);
                } else {
                    temp.nk_1_ = 0.0;
                }
                // φ2n-k
                if (nc2 == NodeConfig::POSGND) {
                    temp.nk_2_ = results.xVector.at(posInd2.value()).value().at(i - k);
                } else if (nc2 == NodeConfig::GNDNEG) {
                    temp.nk_2_ = -results.xVector.at(negInd2.value()).value().at(i - k);
                } else if (nc2 == NodeConfig::POSNEG) {
                    temp.nk_2_ = results.xVector.at(posInd2.value()).value().at(i - k)
                                 - results.xVector.at(negInd2.value()).value().at(i - k);
                } else {
                    temp.nk_2_ = 0.0;
                }
                // I1n-k
                double& I1nk = results.xVector.at(curInd).value().at(i - k);
                // I2n-k
                double& I2nk = results.xVector.at(curInd2).value().at(i - k);
                if (i == k) {
                    // I1 = Z(2e/hbar)(2h/3)I2n-k + (4/3)φ1n-1 - (1/3)φ1n-2 + φ2n-k
                    b_.at(curInd) = (Z / Constants::SIGMA) * ((2.0 * stepSize_ * factor) / 3.0) * I2nk
                                    + (4.0 / 3.0) * temp.n1_1_ - (1.0 / 3.0) * temp.n2_1_ + temp.nk_2_;
                    // I2 = Z(2e/hbar)(2h/3)I1n-k + (4/3)φ2n-1 - (1/3)φ2n-2 + φ1n-k
                    b_.at(curInd2) = (Z / Constants::SIGMA) * ((2.0 * stepSize_ * factor) / 3.0) * I1nk
                                     + (4.0 / 3.0) * temp.n1_2_ - (1.0 / 3.0) * temp.n2_2_ + temp.nk_1_;
                } else if (i == k + 1) {
                    // φ1n-k-1
                    if (nc == NodeConfig::POSGND) {
                        temp.nk1_1_ = results.xVector.at(posInd.value()).value().at(i - k - 1);
                    } else if (nc == NodeConfig::GNDNEG) {
                        temp.nk1_1_ = -results.xVector.at(negInd.value()).value().at(i - k - 1);
                    } else if (nc == NodeConfig::POSNEG) {
                        temp.nk1_1_ = results.xVector.at(posInd.value()).value().at(i - k - 1)
                                      - results.xVector.at(negInd.value()).value().at(i - k - 1);
                    } else {
                        temp.nk1_1_ = 0.0;
                    }
                    // φ2n-k-1
                    if (nc2 == NodeConfig::POSGND) {
                        temp.nk1_2_ = results.xVector.at(posInd2.value()).value().at(i - k - 1);
                    } else if (nc2 == NodeConfig::GNDNEG) {
                        temp.nk1_2_ = -results.xVector.at(negInd2.value()).value().at(i - k - 1);
                    } else if (nc2 == NodeConfig::POSNEG) {
                        temp.nk1_2_ = results.xVector.at(posInd2.value()).value().at(i - k - 1)
                                      - results.xVector.at(negInd2.value()).value().at(i - k - 1);
                    } else {
                        temp.nk1_2_ = 0.0;
                    }
                    // I1 = Z(2e/hbar)(2h/3)I2n-k + (4/3)φ1n-1 - (1/3)φ1n-2 +
                    //      φ2n-k - (4/3)φ2n-k-1
                    b_.at(curInd) = (Z / Constants::SIGMA) * ((2.0 * stepSize_ * factor) / 3.0) * I2nk
                                    + (4.0 / 3.0) * temp.n1_1_ - (1.0 / 3.0) * temp.n2_1_ + temp.nk_2_
                                    - (4.0 / 3.0) * temp.nk1_2_;
                    // I2 = Z(2e/hbar)(2h/3)I1n-k + (4/3)φ2n-1 - (1/3)φ2n-2 +
                    //      φ1n-k - (4/3)φ1n-k-1
                    b_.at(curInd2) = (Z / Constants::SIGMA) * ((2.0 * stepSize_ * factor) / 3.0) * I1nk
                                     + (4.0 / 3.0) * temp.n1_2_ - (1.0 / 3.0) * temp.n2_2_ + temp.nk_1_
                                     - (4.0 / 3.0) * temp.nk1_1_;
                } else if (i > k + 1) {
                    // φ1n-k-1
                    if (nc == NodeConfig::POSGND) {
                        temp.nk1_1_ = results.xVector.at(posInd.value()).value().at(i - k - 1);
                    } else if (nc == NodeConfig::GNDNEG) {
                        temp.nk1_1_ = -results.xVector.at(negInd.value()).value().at(i - k - 1);
                    } else if (nc == NodeConfig::POSNEG) {
                        temp.nk1_1_ = results.xVector.at(posInd.value()).value().at(i - k - 1)
                                      - results.xVector.at(negInd.value()).value().at(i - k - 1);
                    } else {
                        temp.nk1_1_ = 0.0;
                    }
                    // φ2n-k-1
                    if (nc2 == NodeConfig::POSGND) {
                        temp.nk1_2_ = results.xVector.at(posInd2.value()).value().at(i - k - 1);
                    } else if (nc2 == NodeConfig::GNDNEG) {
                        temp.nk1_2_ = -results.xVector.at(negInd2.value()).value().at(i - k - 1);
                    } else if (nc2 == NodeConfig::POSNEG) {
                        temp.nk1_2_ = results.xVector.at(posInd2.value()).value().at(i - k - 1)
                                      - results.xVector.at(negInd2.value()).value().at(i - k - 1);
                    } else {
                        temp.nk1_2_ = 0.0;
                    }
                    // φ1n-k-2
                    if (nc == NodeConfig::POSGND) {
                        temp.nk2_1_ = results.xVector.at(posInd.value()).value().at(i - k - 2);
                    } else if (nc == NodeConfig::GNDNEG) {
                        temp.nk2_1_ = -results.xVector.at(negInd.value()).value().at(i - k - 2);
                    } else if (nc == NodeConfig::POSNEG) {
                        temp.nk2_1_ = results.xVector.at(posInd.value()).value().at(i - k - 2)
                                      - results.xVector.at(negInd.value()).value().at(i - k - 2);
                    } else {
                        temp.nk2_1_ = 0.0;
                    }
                    // φ2n-k-2
                    if (nc2 == NodeConfig::POSGND) {
                        temp.nk2_2_ = results.xVector.at(posInd2.value()).value().at(i - k - 2);
                    } else if (nc2 == NodeConfig::GNDNEG) {
                        temp.nk2_2_ = -results.xVector.at(negInd2.value()).value().at(i - k - 2);
                    } else if (nc2 == NodeConfig::POSNEG) {
                        temp.nk2_2_ = results.xVector.at(posInd2.value()).value().at(i - k - 2)
                                      - results.xVector.at(negInd2.value()).value().at(i - k - 2);
                    } else {
                        temp.nk2_2_ = 0.0;
                    }
                    // I1 = Z(2e/hbar)(2h/3)I2n-k + (4/3)φ1n-1 - (1/3)φ1n-2 +
                    //      φ2n-k - (4/3)φ2n-k-1 + (1/3)φ2n-k-2
                    b_.at(curInd) = (Z / Constants::SIGMA) * ((2.0 * stepSize_ * factor) / 3.0) * I2nk
                                    + (4.0 / 3.0) * temp.n1_1_ - (1.0 / 3.0) * temp.n2_1_ + temp.nk_2_
                                    - (4.0 / 3.0) * temp.nk1_2_ + (1.0 / 3.0) * temp.nk2_2_;
                    // I2 = Z(2e/hbar)(2h/3)I1n-k + (4/3)φ2n-1 - (1/3)φ2n-2 +
                    //      φ1n-k - (4/3)φ1n-k-1 + (1/3)φ1n-k-2
                    b_.at(curInd2) = (Z / Constants::SIGMA) * ((2.0 * stepSize_ * factor) / 3.0) * I1nk
                                     + (4.0 / 3.0) * temp.n1_2_ - (1.0 / 3.0) * temp.n2_2_ + temp.nk_1_
                                     - (4.0 / 3.0) * temp.nk1_1_ + (1.0 / 3.0) * temp.nk2_1_;
                }
                if (capacitorIC_ && i <= k + 1) {
                    const auto before = [&](int_o positive, int_o negative, size_t sample) {
                        return (positive ? results.phaseBefore.at(*positive).at(sample) : 0.0)
                             - (negative ? results.phaseBefore.at(*negative).at(sample) : 0.0);
                    };
                    // Recover the launch voltage at t=0 from its consistent
                    // phase history, while retaining zero incoming waves for t<TD.
                    const auto correction = [&](int_o positive, int_o negative) {
                        return i == k ? -(4.0 / 3.0) * before(positive, negative, 0)
                                          + (1.0 / 3.0) * before(positive, negative, 1)
                                      : (1.0 / 3.0) * before(positive, negative, 0);
                    };
                    b_.at(curInd) += correction(posInd2, negInd2);
                    b_.at(curInd2) += correction(posInd, negInd);
                }
            } else {
                // I1 = (4/3)φ1n-1 - (1/3)φ1n-2
                b_.at(curInd)  = (4.0 / 3.0) * temp.n1_1_ - (1.0 / 3.0) * temp.n2_1_;
                // I2 = (4/3)φ2n-1 - (1/3)φ2n-2
                b_.at(curInd2) = (4.0 / 3.0) * temp.n1_2_ - (1.0 / 3.0) * temp.n2_2_;
            }
        }
    }
}
