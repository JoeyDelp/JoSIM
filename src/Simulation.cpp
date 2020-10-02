// Copyright (c) 2020 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/Simulation.hpp"
#include "JoSIM/AnalysisType.hpp"
#include "JoSIM/Components.hpp"
#include "JoSIM/Matrix.hpp"
#include "JoSIM/Constants.hpp"
#include "JoSIM/Model.hpp"
#include "JoSIM/ProgressBar.hpp"

#include <cmath>
#include <iostream>

using namespace JoSIM;

Simulation::Simulation(Input &iObj, Matrix &mObj) {
  // Simulation setup
  simSize_ = iObj.transSim.get_simsize();
  atyp_ = iObj.argAnal;
  minOut_ = iObj.argMin;
  needsLU_ = false;
  stepSize_ = iObj.transSim.get_prstep();
  x_.resize(mObj.branchIndex, 0.0);
  if(!mObj.relevantTraces.empty()) {
    results.xVector.resize(mObj.branchIndex);
    for (const auto &i : mObj.relevantIndices) {
      results.xVector.at(i).emplace();
    }
  } else {
    results.xVector.resize(mObj.branchIndex, std::vector<double>(0));
  }
  // KLU setup
  simOK_ = klu_defaults(&Common_);
  assert(simOK_);
  Symbolic_ = klu_analyze(
    mObj.rp.size() - 1, &mObj.rp.front(), &mObj.ci.front(), &Common_);
  Numeric_ = klu_factor(
    &mObj.rp.front(), &mObj.ci.front(), &mObj.nz.front(), 
    Symbolic_, &Common_);
  // Run transient simulation
  trans_sim(mObj);
  // KLU cleanup
  klu_free_symbolic(&Symbolic_, &Common_);
  klu_free_numeric(&Numeric_, &Common_);
}

void Simulation::trans_sim(Matrix &mObj) {
  // Ensure time axis is cleared
  results.timeAxis.clear();
  ProgressBar bar;
  if(!minOut_) {
    bar.create_thread();
    bar.set_bar_width(30);
    bar.fill_bar_progress_with("O");
    bar.fill_bar_remainder_with(" ");
    bar.set_status_text("Simulating");
  }
  float progress = 0;
  // Initialize the b matrix
  b_.resize(mObj.rp.size(), 0.0);
  // Start the simulation loop
  for(int i = 0; i < simSize_; ++i) {
    // If not minimal printing report progress
    if(!minOut_) {
      progress = (float)i / (float)simSize_ * 100;
      bar.update(progress);
    }
    // Setup the b matrix
    setup_b(mObj, i, i * stepSize_);
    // Assign x_prev the new b
    x_ = b_;
    // Solve Ax=b, storing the results in x_
    simOK_ = klu_tsolve(
      Symbolic_, Numeric_, mObj.rp.size() - 1, 1, &x_.front(), &Common_);
    // If anything is a amiss, complain about it
    if (!simOK_) Errors::simulation_errors(SimulationErrors::MATRIX_SINGULAR);
    // Store results (only requested, to prevent massive memory usage)
    for(int j = 0; j < results.xVector.size(); ++j) {
      if(results.xVector.at(j)) {
        results.xVector.at(j).value().emplace_back(x_.at(j));
      }
    }
    // Set backup the x vector in case we need to step back
    xPrev_ = x_;
    // Store the time step
    results.timeAxis.emplace_back(i * stepSize_);
  }
  if(!minOut_) {
    bar.update(100);
    bar.complete();
    std::cout << "\n\n";
  }
}

void Simulation::setup_b(
  Matrix &mObj, int i, double step, double factor) {
  // Clear b matrix and reset
  b_.clear();
  b_.resize(mObj.rp.size(), 0.0);
  // Handle current sources
  handle_cs(mObj, step, i);
  // Handle resistors
  handle_resistors(mObj);
  // Handle inductors
  handle_inductors(mObj);
  // Handle capacitors
  handle_capacitors(mObj);
  // Handle jj
  handle_jj(mObj, i, step);
  // Re-factorize the LU if any jj transitions
  if (needsLU_) {
    mObj.create_nz();
    klu_free_numeric(&Numeric_, &Common_);
    Numeric_ = klu_factor(
      &mObj.rp.front(), &mObj.ci.front(), &mObj.nz.front(), 
      Symbolic_, &Common_);
    needsLU_ = false;
  }
  // Handle voltage sources
  handle_vs(mObj, i, step);
  // Handle phase sources
  handle_ps(mObj, i, step);
  // Handle ccvs
  handle_ccvs(mObj);
    // Handle vccs
  handle_vccs(mObj);
  // Handle transmission lines
  handle_tx(mObj, i);
}

void Simulation::reduce_step(
  Matrix &mObj, double factor, 
  int &stepCount, double &currentStep) {
  // Backup the current nonzeros
  mObj.nz_orig = mObj.nz;
  // Restore the previous x
  x_ = xPrev_;
  // Update the non-zeros of each component to reflect the smaller timestep
  for (auto &j : mObj.components.devices) {
    BasicComponent &x = std::visit(
          [](auto& x) -> BasicComponent &{ return x; },
          j);
      x.update_timestep(factor);
  }
  // Recreate the non-zero matrix for the simulation
  mObj.create_nz();
  // Do a new LU decomposition
  klu_free_numeric(&Numeric_, &Common_);
  Numeric_ = klu_factor(
    &mObj.rp.front(), &mObj.ci.front(), &mObj.nz.front(), 
    Symbolic_, &Common_);
  int smallSteps = static_cast<int>(stepSize_ / (factor * stepSize_));
  // Split the current step into smaller steps
  for(int i = 1; i < smallSteps; ++i) {
    // Setup the b matrix
    setup_b(mObj, stepCount - 1, (currentStep + i * (factor * stepSize_)), factor);
    // Assign x_prev the new b
    x_ = b_;
    // Solve Ax=b, storing the results in x_
    simOK_ = klu_tsolve(
      Symbolic_, Numeric_, mObj.rp.size() - 1, 1, &x_.front(), &Common_);
    // If anything is a amiss, complain about it
    if (!simOK_) Errors::simulation_errors(SimulationErrors::MATRIX_SINGULAR);
  }
  // Restor nonzeros from backup
  mObj.nz = mObj.nz_orig;
  // Recreate the non-zero matrix for the simulation
  mObj.create_nz();
  // Do a new LU decomposition
  klu_free_numeric(&Numeric_, &Common_);
  Numeric_ = klu_factor(
    &mObj.rp.front(), &mObj.ci.front(), &mObj.nz.front(), 
    Symbolic_, &Common_);
}


void Simulation::handle_cs(Matrix &mObj, double &step, const int &i) {
  for (const auto &j : mObj.components.currentsources) {
    if(j.indexInfo.posIndex_ && !j.indexInfo.negIndex_) {
      b_.at(j.indexInfo.posIndex_.value()) -= 
        (mObj.sourcegen.at(j.sourceIndex_).value(step));
    } else if(!j.indexInfo.posIndex_ && j.indexInfo.negIndex_) {
      b_.at(j.indexInfo.negIndex_.value()) += 
        (mObj.sourcegen.at(j.sourceIndex_).value(step));
    } else {
      b_.at(j.indexInfo.posIndex_.value()) -= 
        (mObj.sourcegen.at(j.sourceIndex_).value(step));
      b_.at(j.indexInfo.negIndex_.value()) += 
        (mObj.sourcegen.at(j.sourceIndex_).value(step));
    }
  }
}

void Simulation::handle_resistors(Matrix &mObj) {
  for (const auto &j : mObj.components.resistorIndices) {
    auto &temp = std::get<Resistor>(mObj.components.devices.at(j));
    double prevNode;
    if(temp.indexInfo.posIndex_ && !temp.indexInfo.negIndex_) {
      prevNode = (x_.at(temp.indexInfo.posIndex_.value()));
    } else if(!temp.indexInfo.posIndex_ && temp.indexInfo.negIndex_) {
      prevNode = (-x_.at(temp.indexInfo.negIndex_.value()));
    } else {
      prevNode = (x_.at(temp.indexInfo.posIndex_.value())
              - x_.at(temp.indexInfo.negIndex_.value()));
    }
    if (atyp_ == AnalysisType::Phase) {
      // 4/3 φp1 - 1/3 φp2
      b_.at(temp.indexInfo.currentIndex_.value()) = 
        (4.0 / 3.0) * prevNode - (1.0 / 3.0) * temp.pn2_.value();
      temp.pn2_ = prevNode;
    }
  }
}

void Simulation::handle_inductors(Matrix &mObj, double factor) {
  for (const auto &j : mObj.components.inductorIndices) {
    auto &temp = std::get<Inductor>(mObj.components.devices.at(j));
    if(atyp_ == AnalysisType::Voltage) {
      // -2L/h Ip + L/2h Ip2
      b_.at(temp.indexInfo.currentIndex_.value()) = 
        -(2.0 * temp.netlistInfo.value_/(stepSize_ * factor)) * 
          x_.at(temp.indexInfo.currentIndex_.value()) + 
          ((temp.netlistInfo.value_/(2.0 * (stepSize_ * factor))) * 
          temp.In2_);
      // -2M/h Im + M/2h Im2
      for(const auto &m : temp.get_mutualInductance()) {
        Inductor &mi = 
          std::get<Inductor>(mObj.components.devices.at(m.first));
        b_.at(temp.indexInfo.currentIndex_.value()) += 
          (-((2 * m.second) / (stepSize_ * factor)) * 
            x_.at(mi.indexInfo.currentIndex_.value()) + 
            (m.second / (2.0 * (stepSize_ * factor))) * mi.In2_);
      }
      temp.In2_ = x_.at(temp.indexInfo.currentIndex_.value());
    }
  }
}

void Simulation::handle_capacitors(Matrix &mObj) {
  for (const auto &j : mObj.components.capacitorIndices) {
    auto &temp = std::get<Capacitor>(mObj.components.devices.at(j));
    double prevNode;
    if(temp.indexInfo.posIndex_ && !temp.indexInfo.negIndex_) {
      prevNode = (x_.at(temp.indexInfo.posIndex_.value()));
    } else if(!temp.indexInfo.posIndex_ && temp.indexInfo.negIndex_) {
      prevNode = (-x_.at(temp.indexInfo.negIndex_.value()));
    } else {
      prevNode = (x_.at(temp.indexInfo.posIndex_.value())
              - x_.at(temp.indexInfo.negIndex_.value()));
    }
    if(atyp_ == AnalysisType::Voltage) {
      // 4/3 Vp1 - 1/3 Vp2
      b_.at(temp.indexInfo.currentIndex_.value()) = 
        (4.0/3.0) * prevNode - (1.0/3.0) * temp.pn1_;
    } else if (atyp_ == AnalysisType::Phase) {
      // 8/3 φp1 + 10/9 φp2 -1/9 φp3
      b_.at(temp.indexInfo.currentIndex_.value()) = 
        (8.0/3.0) * prevNode + (10.0/9.0) * temp.pn1_ - (1.0/9.0) * temp.pn3_;
      temp.pn3_ = temp.pn2_;
      temp.pn2_ = temp.pn1_;
    }
    temp.pn1_ = prevNode;
  }
}

void Simulation::handle_jj(
  Matrix &mObj, int &i, double &step, double factor) {
  for (const auto &j : mObj.components.junctionIndices) {
    auto &temp = std::get<JJ>(mObj.components.devices.at(j));
    const auto &model = temp.model_;
    double prevNode;
    if(temp.indexInfo.posIndex_ && !temp.indexInfo.negIndex_) {
      prevNode = (x_.at(temp.indexInfo.posIndex_.value()));
    } else if(!temp.indexInfo.posIndex_ && temp.indexInfo.negIndex_) {
      prevNode = (-x_.at(temp.indexInfo.negIndex_.value()));
    } else {
      prevNode = (x_.at(temp.indexInfo.posIndex_.value())
              - x_.at(temp.indexInfo.negIndex_.value()));
    }
    if(i > 0) {
      if(atyp_ == AnalysisType::Voltage) {
        temp.vn1_ = prevNode;
        temp.pn1_ = x_.at(temp.variableIndex_);
      } else if (atyp_ == AnalysisType::Phase) {
        temp.vn1_ = x_.at(temp.variableIndex_);
        temp.pn1_ = prevNode;
      }
    }
    // Ensure timestep is not too large
    if ((double)i/(double)simSize_ > 0.01) {
      if (abs(temp.phi0_ - temp.pn1_) > (0.25 * 2 * Constants::PI)) {
        // reduce_step(mObj, (0.25E-12 / (stepSize_ * factor)), i, step);
        Errors::simulation_errors(
          SimulationErrors::PHASEGUESS_TOO_LARGE, temp.netlistInfo.label_);
      }
    }
    // Guess voltage (V0)
    double v0 = 
      (5.0/2.0) * temp.vn1_ - 2.0 * temp.vn2_ + (1.0 / 2.0) * temp.vn3_;
    // Phase guess (P0)
    temp.phi0_ = (4.0/3.0) * temp.pn1_ - (1.0/3.0) * temp.pn2_ + 
      ((1.0 / Constants::SIGMA) * 
        ((2.0 * (stepSize_ * factor)) / 3.0)) * v0;
    // (hbar / 2 * e) ( -(2 / h) φp1 + (1 / 2h) φp2 )
    if(atyp_ == AnalysisType::Voltage) {
      b_.at(temp.variableIndex_) = 
        (Constants::SIGMA) * (-(2.0 / (stepSize_ * factor)) * 
          temp.pn1_ + (1.0 / (2.0 * (stepSize_ * factor))) * temp.pn2_);
    // (4 / 3) φp1 - (1/3) φp2 
    } else if (atyp_ == AnalysisType::Phase) {
      b_.at(temp.variableIndex_) = 
        (4.0 / 3.0) * temp.pn1_ - (1.0 / 3.0) * temp.pn2_;
    }
    temp.pn2_ = temp.pn1_;
    temp.vn3_ = temp.vn2_;
    // Update junction transition
    if(model.value().get_resistanceType() == 1) {
      auto testLU = temp.update_value(v0);
      if(testLU && !needsLU_) {
        needsLU_ = true;
      }
    }
    // -(hR / h + 2RC) * (Ic sin φ0 - 2C / h Vp1 + C/2h Vp2 + It) 
    b_.at(temp.indexInfo.currentIndex_.value()) = 
      (temp.matrixInfo.nonZeros_.back()) * ((((Constants::PI * temp.del_) / 
        (2 * Constants::EV * temp.rncalc_)) * (sin(temp.phi0_) / 
          sqrt(1 - model.value().get_transparency() * (sin(temp.phi0_ / 2) * 
          sin(temp.phi0_ / 2)))) * tanh((temp.del_) / 
        (2 * Constants::BOLTZMANN * model.value().get_temperature()) *
        sqrt(1 - model.value().get_transparency() * 
          (sin(temp.phi0_ / 2) * sin(temp.phi0_ / 2))))) -
        (((2 * model.value().get_capacitance()) / 
          (stepSize_ * factor)) * temp.vn1_) + 
        ((model.value().get_capacitance() / 
          (2.0 * (stepSize_ * factor))) * temp.vn2_) + 
        temp.transitionCurrent_);
    temp.vn2_ = temp.vn1_;
  }
}

void Simulation::handle_vs(
  Matrix &mObj, const int &i, double &step, double factor) {
  for (const auto &j : mObj.components.vsIndices) {
    auto &temp = std::get<VoltageSource>(mObj.components.devices.at(j));
    if(temp.netlistInfo.label_.at(0) == 'V') {
      b_.at(temp.indexInfo.currentIndex_.value()) = 
        (mObj.sourcegen.at(temp.sourceIndex_).value(step));
    } else if (temp.netlistInfo.label_.at(0) == 'P') {
      b_.at(temp.indexInfo.currentIndex_.value()) = 
        (3 * Constants::SIGMA) / (2 * stepSize_ * factor) *
        (mObj.sourcegen.at(temp.sourceIndex_).value(step)) + 
        (4.0 / 3.0) * temp.pn1_ - (1.0 / 3.0) * temp.pn2_;
      temp.pn2_ = temp.pn1_;
      temp.pn1_ = (mObj.sourcegen.at(temp.sourceIndex_).value(step));
    }
  }
}

void Simulation::handle_ps(
  Matrix &mObj, const int &i, double &step, double factor) {
  for (const auto &j : mObj.components.psIndices) {
    auto &temp = std::get<PhaseSource>(mObj.components.devices.at(j));
    if (temp.netlistInfo.label_.at(0) == 'P') {
      b_.at(temp.indexInfo.currentIndex_.value()) = 
        (mObj.sourcegen.at(temp.sourceIndex_).value(step));
    } else if(temp.netlistInfo.label_.at(0) == 'V') {
      if(temp.indexInfo.posIndex_ && !temp.indexInfo.negIndex_) {
        temp.pn1_ = (x_.at(temp.indexInfo.posIndex_.value()));
      } else if(!temp.indexInfo.posIndex_ && temp.indexInfo.negIndex_) {
        temp.pn1_ = (-x_.at(temp.indexInfo.negIndex_.value()));
      } else {
        temp.pn1_ = (x_.at(temp.indexInfo.posIndex_.value())
                - x_.at(temp.indexInfo.negIndex_.value()));
      }
      b_.at(temp.indexInfo.currentIndex_.value()) = 
        (2 * stepSize_ * factor) / (3 * Constants::SIGMA) * 
        (mObj.sourcegen.at(temp.sourceIndex_).value(step)) + 
        (4.0/3.0) * temp.pn1_ - (1.0 / 3.0) * temp.pn2_;
      temp.pn2_ = temp.pn1_;
    }
  }
}

void Simulation::handle_ccvs(Matrix &mObj) {
  for (const auto &j : mObj.components.ccvsIndices) {
    auto &temp = std::get<CCVS>(mObj.components.devices.at(j));
    if (atyp_ == AnalysisType::Phase) {
      double prevNode;
      if(temp.indexInfo.posIndex_ && !temp.indexInfo.negIndex_) {
        prevNode = (x_.at(temp.indexInfo.posIndex_.value()));
      } else if(!temp.indexInfo.posIndex_ && temp.indexInfo.negIndex_) {
        prevNode = (-x_.at(temp.indexInfo.negIndex_.value()));
      } else {
        prevNode = (x_.at(temp.indexInfo.posIndex_.value())
                - x_.at(temp.indexInfo.negIndex_.value()));
      }
      b_.at(temp.indexInfo.currentIndex_.value()) = 
        (4.0 / 3.0) * prevNode - (1.0 / 3.0) * temp.pn2_.value(); 
      temp.pn2_ = prevNode;
    }
  }
}

void Simulation::handle_vccs(Matrix &mObj) {
  for (const auto &j : mObj.components.vccsIndices) {
    auto &temp = std::get<VCCS>(mObj.components.devices.at(j));
    if (atyp_ == AnalysisType::Phase) {
      double prevNode;
      if(temp.posIndex2_ && !temp.negIndex2_) {
        prevNode = (x_.at(temp.posIndex2_.value()));
      } else if(!temp.posIndex2_ && temp.negIndex2_) {
        prevNode = (-x_.at(temp.negIndex2_.value()));
      } else {
        prevNode = (x_.at(temp.posIndex2_.value())
                - x_.at(temp.negIndex2_.value()));
      }
      b_.at(temp.indexInfo.currentIndex_.value()) = 
        (4.0 / 3.0) * prevNode - (1.0 / 3.0) * temp.pn2_.value(); 
      temp.pn2_ = prevNode;
    }
  }
}

void Simulation::handle_tx(Matrix &mObj, const int &i) {
  for (const auto &j : mObj.components.txIndices) {
    auto &temp = std::get<TransmissionLine>(mObj.components.devices.at(j));
    if(atyp_ == AnalysisType::Voltage) {
      if(i >= temp.timestepDelay_) {
        double prevNodek, prevNode2k;
        // φ1n-k
        if(temp.indexInfo.posIndex_ && !temp.indexInfo.negIndex_) {
          prevNodek = results.xVector.at(
            temp.indexInfo.posIndex_.value()).value().at(
              i - temp.timestepDelay_);
        } else if(!temp.indexInfo.posIndex_ && temp.indexInfo.negIndex_) {
          prevNodek = -results.xVector.at(
            temp.indexInfo.negIndex_.value()).value().at(
              i - temp.timestepDelay_);
        } else {
          prevNodek = results.xVector.at(
            temp.indexInfo.posIndex_.value()).value().at(
              i - temp.timestepDelay_) - results.xVector.at(
            temp.indexInfo.negIndex_.value()).value().at(
              i - temp.timestepDelay_);
        }
        // φ2n-k
        if(temp.posIndex2_ && !temp.negIndex2_) {
          prevNode2k = results.xVector.at(
            temp.posIndex2_.value()).value().at(i - temp.timestepDelay_);
        } else if(!temp.posIndex2_ && temp.negIndex2_) {
          prevNode2k = -results.xVector.at(
            temp.negIndex2_.value()).value().at(i - temp.timestepDelay_);
        } else {
          prevNode2k = results.xVector.at(
              temp.posIndex2_.value()).value().at(i - temp.timestepDelay_) - 
            results.xVector.at(
              temp.negIndex2_.value()).value().at(i - temp.timestepDelay_);
        }
        // IT1 = V2(n-k) + Z0 I2(n-k)
        b_.at(temp.indexInfo.currentIndex_.value()) = 
          prevNode2k + temp.netlistInfo.value_ * results.xVector.at(
            temp.currentIndex2_).value().at(i - temp.timestepDelay_);
        // IT2 = V1(n-k) + Z0 I1(n-k)
        b_.at(temp.currentIndex2_) = 
          prevNodek + temp.netlistInfo.value_ * results.xVector.at(
            temp.indexInfo.currentIndex_.value()).value().at(
              i - temp.timestepDelay_);
      }
    } else if(atyp_ == AnalysisType::Phase) {
      if (i > 0) {
        // φ1n-1, φ2n-1, φ1n-k-1, φ2n-k-1
        double prevNodeN, prevNode2N;
        // φ1n-1
        if(temp.indexInfo.posIndex_ && !temp.indexInfo.negIndex_) {
          prevNodeN = results.xVector.at(
              temp.indexInfo.posIndex_.value()).value().at(i - 1);
        } else if(!temp.indexInfo.posIndex_ && temp.indexInfo.negIndex_) {
          prevNodeN = -results.xVector.at(
              temp.indexInfo.negIndex_.value()).value().at(i - 1);
        } else {
          prevNodeN = results.xVector.at(
              temp.indexInfo.posIndex_.value()).value().at(i - 1) - 
            results.xVector.at(
              temp.indexInfo.negIndex_.value()).value().at(i - 1);
        }
        // φ2n-1
        if(temp.posIndex2_ && !temp.negIndex2_) {
          prevNode2N = results.xVector.at(
              temp.posIndex2_.value()).value().at(i - 1);
        } else if(!temp.posIndex2_ && temp.negIndex2_) {
          prevNode2N = -results.xVector.at(
              temp.negIndex2_.value()).value().at(i - 1);
        } else {
          prevNode2N = results.xVector.at(
              temp.posIndex2_.value()).value().at(i - 1) - 
            results.xVector.at(
              temp.negIndex2_.value()).value().at(i - 1);
        }
        if(i >= temp.timestepDelay_) {
          double prevNodek, prevNode2k, prevNodek1, prevNode2k1;
          prevNodek1 = 0.0;
          prevNode2k1 = 0.0;
          // φ1n-k
          if(temp.indexInfo.posIndex_ && !temp.indexInfo.negIndex_) {
            prevNodek = results.xVector.at(
                temp.indexInfo.posIndex_.value()).value().at(
                  i - temp.timestepDelay_);
            // φ1n-k-1
            if(i > temp.timestepDelay_ + 1) {
              prevNodek1 = results.xVector.at(
                  temp.indexInfo.posIndex_.value()).value().at(
                    i - temp.timestepDelay_ - 1);
            }
          } else if(!temp.indexInfo.posIndex_ && temp.indexInfo.negIndex_) {
            prevNodek = -results.xVector.at(
                temp.indexInfo.negIndex_.value()).value().at(
                  i - temp.timestepDelay_);
            // φ1n-k-1           
            if(i >= temp.timestepDelay_ + 1) { 
              prevNodek1 = -results.xVector.at(
                  temp.indexInfo.negIndex_.value()).value().at(
                    i - temp.timestepDelay_ - 1);
            } 
          } else {
            prevNodek = results.xVector.at(
                temp.indexInfo.posIndex_.value()).value().at(
                  i - temp.timestepDelay_) - results.xVector.at(
                temp.indexInfo.negIndex_.value()).value().at(
                  i - temp.timestepDelay_);
            // φ1n-k-1
            if(i >= temp.timestepDelay_ + 1) {
              prevNodek1 = results.xVector.at(
                  temp.indexInfo.posIndex_.value()).value().at(
                    i - temp.timestepDelay_ - 1) - results.xVector.at(
                  temp.indexInfo.negIndex_.value()).value().at(
                    i - temp.timestepDelay_ - 1);
            }
          }
          // φ2n-k
          if(temp.posIndex2_ && !temp.negIndex2_) {
            prevNode2k = results.xVector.at(
                temp.posIndex2_.value()).value().at(i - temp.timestepDelay_);
            // φ2n-k-1
            if(i >= temp.timestepDelay_ + 1) {
              prevNode2k1 = results.xVector.at(
                  temp.posIndex2_.value()).value().at(
                    i - temp.timestepDelay_ - 1);
            }
          } else if(!temp.posIndex2_ && temp.negIndex2_) {
            prevNode2k = -results.xVector.at(
                temp.negIndex2_.value()).value().at(
                  i - temp.timestepDelay_);
            // φ2n-k-1
            if(i >= temp.timestepDelay_ + 1) {
              prevNode2k1 = -results.xVector.at(
                  temp.negIndex2_.value()).value().at(
                    i - temp.timestepDelay_ - 1);
            }
          } else {
            prevNode2k = results.xVector.at(
                temp.posIndex2_.value()).value().at(
                  i - temp.timestepDelay_) - results.xVector.at(
                temp.negIndex2_.value()).value().at(i - temp.timestepDelay_);
            // φ2n-k-1
            if(i >= temp.timestepDelay_ + 1) {
              prevNode2k1 = results.xVector.at(
                  temp.posIndex2_.value()).value().at(
                    i - temp.timestepDelay_ - 1) - results.xVector.at(
                  temp.negIndex2_.value()).value().at(
                    i - temp.timestepDelay_ - 1);
            }
          }
          if(i == temp.timestepDelay_) {
            // IT1 = (hZ0/2σ) * IT2n-k - (4/3) φ1n-1 - (1/3) φ1n-2 - φ2n-k
            b_.at(temp.indexInfo.currentIndex_.value()) = 
              temp.netlistInfo.value_ * results.xVector.at(
                temp.currentIndex2_).value().at(i - temp.timestepDelay_) -
              (4.0/3.0) * prevNodeN - (1.0/3.0) * temp.p1n2_ - prevNode2k;
            // IT2 = (hZ0/2σ) * IT1n-k - (4/3) φ2n-1 - (1/3) φ2n-2 - φ1n-k                          
            b_.at(temp.currentIndex2_) = 
              temp.netlistInfo.value_ * results.xVector.at(
                temp.indexInfo.currentIndex_.value()).value().at(
                  i - temp.timestepDelay_) - 
              (4.0/3.0) * prevNode2N - (1.0/3.0) * temp.p2n2_ - prevNodek;
          } else {
            // IT1 = (hZ0/2σ) * IT2n-k - (4/3) φ1n-1 - (1/3) φ1n-2 - φ2n-k - 
            //        (4/3) φ2n-k-1 + (1/3) φ2n-k-2
            b_.at(temp.indexInfo.currentIndex_.value()) = 
              temp.netlistInfo.value_ * results.xVector.at(
                temp.currentIndex2_).value().at(i - temp.timestepDelay_) - 
              (4.0/3.0) * prevNodeN - (1.0/3.0) * temp.p1n2_ - prevNode2k -
              (4.0/3.0) * prevNode2k1 + (1.0/3.0) * temp.p2nk2_;
            // IT2 = (hZ0/2σ) * IT1n-k - (4/3) φ2n-1 - (1/3) φ2n-2 - φ1n-k - 
            //        (4/3) φ1n-k-1 + (1/3) φ1n-k-2                              
            b_.at(temp.currentIndex2_) = 
              temp.netlistInfo.value_ * results.xVector.at(
                temp.indexInfo.currentIndex_.value()).value().at(
                  i - temp.timestepDelay_) - (4.0/3.0) * prevNode2N - 
              (1.0/3.0) * temp.p2n2_ - prevNodek - (4.0/3.0) * prevNodek1 + 
              (1.0/3.0) * temp.p1nk2_;
          }
          temp.p1nk2_ = prevNodek1;
          temp.p2nk2_ = prevNode2k1;
        } else {
          // IT1 = -(4/3) φ1n-1 - (1/3) φ1n-2
          b_.at(temp.indexInfo.currentIndex_.value()) = 
            -(4.0/3.0) * prevNodeN - (1.0/3.0) * temp.p1n2_;
          // IT2 = -(4/3) φ2n-1 - (1/3) φ2n-2                        
          b_.at(temp.currentIndex2_) = 
            -(4.0/3.0) * prevNode2N - (1.0/3.0) * temp.p2n2_;
        }
        temp.p1n2_ = prevNodeN;
        temp.p2n2_ = prevNode2N;
      }
    }
  }
}