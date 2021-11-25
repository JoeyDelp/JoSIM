// Copyright (c) 2021 Johannes Delport
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
  SLU = iObj.SLU;
  simSize_ = iObj.transSim.simsize();
  atyp_ = iObj.argAnal;
  minOut_ = iObj.argMin;
  needsLU_ = false;
  needsTR_ = false;
  stepSize_ = iObj.transSim.tstep();
  prstep_ = iObj.transSim.prstep();
  prstart_ = iObj.transSim.prstart();
  x_.resize(mObj.branchIndex, 0.0);
  if(!mObj.relevantTraces.empty()) {
    results.xVector.resize(mObj.branchIndex);
    for (const auto &i : mObj.relevantIndices) {
      results.xVector.at(i).emplace();
    }
  } else {
    results.xVector.resize(mObj.branchIndex, std::vector<double>(0));
  }
  if (SLU) {
    // SLU setup
    lu.create_matrix(mObj.rp.size() - 1, mObj.nz, mObj.ci, mObj.rp);
    lu.factorize();
    // Run transient simulation
    trans_sim(mObj);
    // SLU cleanup
    lu.free();
  } else {
    // KLU setup
    simOK_ = klu_l_defaults(&Common_);
    assert(simOK_);
    Symbolic_ = klu_l_analyze(
      mObj.rp.size() - 1, &mObj.rp.front(), &mObj.ci.front(), &Common_);
    Numeric_ = klu_l_factor(
      &mObj.rp.front(), &mObj.ci.front(), &mObj.nz.front(),
      Symbolic_, &Common_);
    // Run transient simulation
    trans_sim(mObj);
    // KLU cleanup
    klu_l_free_symbolic(&Symbolic_, &Common_);
    klu_l_free_numeric(&Numeric_, &Common_);
  }
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
    bar.set_total((float)simSize_);
  }
  // Initialize the b matrix
  b_.resize(mObj.rp.size(), 0.0);
  // Start the simulation loop
  for(int i = 0; i < simSize_; ++i) {
    double step = i * stepSize_;
    // If not minimal printing report progress
    if(!minOut_) {
      bar.update(static_cast<float>(i));
    }
    // Setup the b matrix
    setup_b(mObj, i, i * stepSize_);
    // Assign x_prev the new b
    x_ = b_;
    // Solve Ax=b, storing the results in x_
    if (SLU) {
      lu.solve(x_);
    } else {
      simOK_ = klu_l_tsolve(
        Symbolic_, Numeric_, mObj.rp.size() - 1, 1, &x_.front(), &Common_);
      // If anything is a amiss, complain about it
      if (!simOK_) Errors::simulation_errors(
        SimulationErrors::MATRIX_SINGULAR);
    }
    // Store results (only requested, to prevent massive memory usage)
    for(int j = 0; j < results.xVector.size(); ++j) {
      if(results.xVector.at(j)) {
        results.xVector.at(j).value().emplace_back(x_.at(j));
      }
    }
    // Store the time step
    results.timeAxis.emplace_back(step);
  }
  if(!minOut_) {
    bar.complete();
    std::cout << "\n";
  }
}

void Simulation::setup_b(
  Matrix &mObj, int i, double step, double factor) {
  // Clear b matrix and reset
  b_.clear();
  b_.resize(mObj.rp.size(), 0.0);
  // Handle jj
  handle_jj(mObj, i, step, factor);
  // Handle pjj
  handle_pijj(mObj, i, step, factor);
  if(needsTR_) return;
  // Re-factorize the LU if any jj transitions
  if (needsLU_) {
    mObj.create_nz();
    if (SLU) {
      lu.factorize(true);
    } else {
      klu_l_free_numeric(&Numeric_, &Common_);
      Numeric_ = klu_l_factor(
        &mObj.rp.front(), &mObj.ci.front(), &mObj.nz.front(), 
        Symbolic_, &Common_);
    }
    needsLU_ = false;
  }
  // Handle current sources
  handle_cs(mObj, step, i);
  // Handle resistors
  handle_resistors(mObj);
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

void Simulation::reduce_step(
  Matrix &mObj, double factor, 
  int &stepCount, double currentStep) {
  // Backup the current nonzeros
  mObj.nz_orig = mObj.nz;
  // Restore the previous x
  x_ = xn3_;
  // Remove the stored results for the larger timesteps
  for(auto &i : results.xVector){
    if(i) {
      i.value().pop_back();
      i.value().pop_back();
    }
  }
  results.timeAxis.pop_back();
  results.timeAxis.pop_back();
  // Determine how many steps to take using the reduced time step
  int smallSteps = static_cast<int>(20E-12 / (factor * stepSize_));
  // Update the non-zeros of each component to reflect the smaller timestep
  for (auto &j : mObj.components.devices) {
    BasicComponent &x = std::visit(
          [](auto& x) -> BasicComponent &{ return x; },
          j);
      x.update_timestep(factor);
      x.step_back();
  }
  // Recreate the non-zero matrix for the simulation
  mObj.create_nz();
  // Do a new LU decomposition
  klu_l_free_numeric(&Numeric_, &Common_);
  Numeric_ = klu_l_factor(
    &mObj.rp.front(), &mObj.ci.front(), &mObj.nz.front(), 
    Symbolic_, &Common_);
  for(int i = 0; i < smallSteps; ++i) {
    double smallStep = currentStep + i * (factor * stepSize_);
    // Setup the b matrix
    setup_b(
      mObj, stepCount, smallStep, factor);
    // Assign x_prev the new b
    x_ = b_;
    // Solve Ax=b, storing the results in x_
    simOK_ = klu_l_tsolve(
      Symbolic_, Numeric_, mObj.rp.size() - 1, 1, &x_.front(), &Common_);
    // If anything is a amiss, complain about it
    if (!simOK_) Errors::simulation_errors(SimulationErrors::MATRIX_SINGULAR);
    float test = smallStep / stepSize_;
    // if(test == stepCount) {
      // Store results (only requested, to prevent massive memory usage)
      for(int j = 0; j < results.xVector.size(); ++j) {
        if(smallStep >= prstart_) {
          if(results.xVector.at(j)) {
            results.xVector.at(j).value().emplace_back(x_.at(j));
          }
        }
      }
      // Store the time step
      if(smallStep >= prstart_) results.timeAxis.emplace_back(smallStep);
    if(test == stepCount) {
      stepCount++;
    }
  }
  // Restore the timestep of all components to the correct values
  for (auto &j : mObj.components.devices) {
    BasicComponent &x = std::visit(
          [](auto& x) -> BasicComponent &{ return x; },
          j);
      x.update_timestep(1/factor);
  }
  // Recreate the non-zero matrix for the simulation
  mObj.create_nz();
  // Do a new LU decomposition
  klu_l_free_numeric(&Numeric_, &Common_);
  Numeric_ = klu_l_factor(
    &mObj.rp.front(), &mObj.ci.front(), &mObj.nz.front(), 
    Symbolic_, &Common_);
}


void Simulation::handle_cs(Matrix &mObj, double &step, const int &i) {
  for (const auto &j : mObj.components.currentsources) {
    if(j.indexInfo.nodeConfig_ == NodeConfig::POSGND) {
      b_.at(j.indexInfo.posIndex_.value()) -= 
        (mObj.sourcegen.at(j.sourceIndex_).value(step));
    } else if(j.indexInfo.nodeConfig_ == NodeConfig::GNDNEG) {
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
    NodeConfig &nc = temp.indexInfo.nodeConfig_;
    if(nc == NodeConfig::POSGND) {
      temp.pn1_ = (x_.at(temp.indexInfo.posIndex_.value()));
    } else if(nc == NodeConfig::GNDNEG) {
      temp.pn1_ = (-x_.at(temp.indexInfo.negIndex_.value()));
    } else {
      temp.pn1_ = (x_.at(temp.indexInfo.posIndex_.value())
              - x_.at(temp.indexInfo.negIndex_.value()));
    }
    if (atyp_ == AnalysisType::Phase) {
      // 4/3 φp1 - 1/3 φp2
      b_.at(temp.indexInfo.currentIndex_.value()) = 
        (4.0 / 3.0) * temp.pn1_ - (1.0 / 3.0) * temp.pn2_;
      temp.pn4_ = temp.pn3_;
      temp.pn3_ = temp.pn2_;
      temp.pn2_ = temp.pn1_;
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
      temp.In4_ = temp.In3_;
      temp.In3_ = temp.In2_;
      temp.In2_ = x_.at(temp.indexInfo.currentIndex_.value());
    }
  }
}

void Simulation::handle_capacitors(Matrix &mObj) {
  for (const auto &j : mObj.components.capacitorIndices) {
    auto &temp = std::get<Capacitor>(mObj.components.devices.at(j));
    if(temp.indexInfo.posIndex_ && !temp.indexInfo.negIndex_) {
      temp.pn1_ = (x_.at(temp.indexInfo.posIndex_.value()));
    } else if(!temp.indexInfo.posIndex_ && temp.indexInfo.negIndex_) {
      temp.pn1_ = (-x_.at(temp.indexInfo.negIndex_.value()));
    } else {
      temp.pn1_ = (x_.at(temp.indexInfo.posIndex_.value())
              - x_.at(temp.indexInfo.negIndex_.value()));
    }
    if(atyp_ == AnalysisType::Voltage) {
      // 4/3 Vp1 - 1/3 Vp2
      b_.at(temp.indexInfo.currentIndex_.value()) = 
        (4.0/3.0) * temp.pn1_ - (1.0/3.0) * temp.pn2_;
    } else if (atyp_ == AnalysisType::Phase) {
      // (8/3)φn-1 - (22/9)φn-2 + (8/9)φn-3 - (1/9)φn-4
      b_.at(temp.indexInfo.currentIndex_.value()) = 
        (8.0/3.0) * temp.pn1_ - (22.0/9.0) * temp.pn2_ + 
        (8.0/9.0) * temp.pn3_ - (1.0/9.0) * temp.pn4_;
      temp.pn7_ = temp.pn6_;
      temp.pn6_ = temp.pn5_;
      temp.pn5_ = temp.pn4_;
      temp.pn4_ = temp.pn3_;
      temp.pn3_ = temp.pn2_;
    }
    temp.pn2_ = temp.pn1_;
  }
}

void Simulation::handle_jj(
  Matrix &mObj, int &i, double &step, double factor) {
  for (const auto &j : mObj.components.junctionIndices) {
    auto &temp = std::get<JJ>(mObj.components.devices.at(j));
    const auto &model = temp.model_;
    if(temp.indexInfo.posIndex_ && !temp.indexInfo.negIndex_) {
      temp.pn1_ = (x_.at(temp.indexInfo.posIndex_.value()));
    } else if(!temp.indexInfo.posIndex_ && temp.indexInfo.negIndex_) {
      temp.pn1_ = (-x_.at(temp.indexInfo.negIndex_.value()));
    } else {
      temp.pn1_ = (x_.at(temp.indexInfo.posIndex_.value())
              - x_.at(temp.indexInfo.negIndex_.value()));
    }
    if(i > 0) {
      if(atyp_ == AnalysisType::Voltage) {
        temp.vn1_ = temp.pn1_;
        temp.pn1_ = x_.at(temp.variableIndex_);
      } else if (atyp_ == AnalysisType::Phase) {
        temp.vn1_ = x_.at(temp.variableIndex_);
        temp.pn1_ = temp.pn1_;
      }
    }
    // Guess voltage (V0)
    double v0 = 
      (5.0/2.0) * temp.vn1_ - 2.0 * temp.vn2_ + (1.0 / 2.0) * temp.vn3_;
    // Phase guess (P0)
    temp.phi0_ = (4.0/3.0) * temp.pn1_ - (1.0/3.0) * temp.pn2_ + 
      ((1.0 / Constants::SIGMA) * 
        ((2.0 * (stepSize_ * factor)) / 3.0)) * v0;
    // Ensure timestep is not too large
    if ((double)i/(double)simSize_ > 0.01) {
      if (abs(temp.phi0_ - temp.pn1_) > (0.25 * 2 * Constants::PI)) {
        // needsTR_ = true;
        // return;
        Errors::simulation_errors(
          SimulationErrors::PHASEGUESS_TOO_LARGE, temp.netlistInfo.label_);
      }
    }
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
    temp.pn4_ = temp.pn3_;
    temp.pn3_ = temp.pn2_;
    temp.pn2_ = temp.pn1_;
    temp.vn6_ = temp.vn5_;
    temp.vn5_ = temp.vn4_;
    temp.vn4_ = temp.vn3_;
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

void Simulation::handle_pijj(
  Matrix &mObj, int &i, double &step, double factor) {
  for (const auto &j : mObj.components.pjunctionIndices) {
    auto &temp = std::get<JJ>(mObj.components.devices.at(j));
    const auto &model = temp.model_;
    if(temp.indexInfo.posIndex_ && !temp.indexInfo.negIndex_) {
      temp.pn1_ = (x_.at(temp.indexInfo.posIndex_.value()));
    } else if(!temp.indexInfo.posIndex_ && temp.indexInfo.negIndex_) {
      temp.pn1_ = (-x_.at(temp.indexInfo.negIndex_.value()));
    } else {
      temp.pn1_ = (x_.at(temp.indexInfo.posIndex_.value())
              - x_.at(temp.indexInfo.negIndex_.value()));
    }
    if(i > 0) {
      if(atyp_ == AnalysisType::Voltage) {
        temp.vn1_ = temp.pn1_;
        temp.pn1_ = x_.at(temp.variableIndex_);
      } else if (atyp_ == AnalysisType::Phase) {
        temp.vn1_ = x_.at(temp.variableIndex_);
        temp.pn1_ = temp.pn1_;
      }
    }
    // Guess voltage (V0)
    double v0 = 
      (5.0/2.0) * temp.vn1_ - 2.0 * temp.vn2_ + (1.0 / 2.0) * temp.vn3_;
    // Phase guess (P0)
    temp.phi0_ = (4.0/3.0) * temp.pn1_ - (1.0/3.0) * temp.pn2_ + 
      ((1.0 / Constants::SIGMA) * 
        ((2.0 * (stepSize_ * factor)) / 3.0)) * v0;
    // Ensure timestep is not too large
    if ((double)i/(double)simSize_ > 0.01) {
      if (abs(temp.phi0_ - temp.pn1_) > (0.25 * 2 * Constants::PI)) {
        // needsTR_ = true;
        // return;
        Errors::simulation_errors(
          SimulationErrors::PHASEGUESS_TOO_LARGE, temp.netlistInfo.label_);
      }
    }
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
    temp.pn4_ = temp.pn3_;
    temp.pn3_ = temp.pn2_;
    temp.pn2_ = temp.pn1_;
    temp.vn6_ = temp.vn5_;
    temp.vn5_ = temp.vn4_;
    temp.vn4_ = temp.vn3_;
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
        (2 * Constants::EV * temp.rncalc_)) * ((-1)*sin(temp.phi0_) / 
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
    JoSIM::NodeConfig &nc = temp.indexInfo.nodeConfig_;
    if(atyp_ == AnalysisType::Voltage) {
      // Vn
      b_.at(temp.indexInfo.currentIndex_.value()) = 
        (mObj.sourcegen.at(temp.sourceIndex_).value(step));
    } else if (atyp_ == AnalysisType::Phase) {
      if(nc == NodeConfig::POSGND) {
        temp.pn1_ = (x_.at(temp.indexInfo.posIndex_.value()));
      } else if(nc == NodeConfig::GNDNEG) {
        temp.pn1_ = (-x_.at(temp.indexInfo.negIndex_.value()));
      } else {
        temp.pn1_ = (x_.at(temp.indexInfo.posIndex_.value())
                - x_.at(temp.indexInfo.negIndex_.value()));
      }
      // (2e/hbar)(2h/3)Vn + (4/3)φn-1 - (1/3)φn-2
      b_.at(temp.indexInfo.currentIndex_.value()) = 
        ((2 * stepSize_ * factor) / (3 * Constants::SIGMA)) *
        (mObj.sourcegen.at(temp.sourceIndex_).value(step)) + 
        (4.0 / 3.0) * temp.pn1_ - (1.0 / 3.0) * temp.pn2_;
      temp.pn4_ = temp.pn3_;
      temp.pn3_ = temp.pn2_;
      temp.pn2_ = temp.pn1_;
    }
  }
}

void Simulation::handle_ps(
  Matrix &mObj, const int &i, double &step, double factor) {
  for (const auto &j : mObj.components.psIndices) {
    auto &temp = std::get<PhaseSource>(mObj.components.devices.at(j));
    if (atyp_ == AnalysisType::Phase) {
      // φn
      b_.at(temp.indexInfo.currentIndex_.value()) = 
        (mObj.sourcegen.at(temp.sourceIndex_).value(step));
    } else if(atyp_ == AnalysisType::Voltage) {
      if(i == 0) {
        b_.at(temp.indexInfo.currentIndex_.value()) = 
        (Constants::SIGMA / (stepSize_ * factor)) * ((3.0/2.0) * 
        (mObj.sourcegen.at(temp.sourceIndex_).value(step)));
      } else if (i == 1) {
        b_.at(temp.indexInfo.currentIndex_.value()) = 
        (Constants::SIGMA / (stepSize_ * factor)) * ((3.0/2.0) * 
        (mObj.sourcegen.at(temp.sourceIndex_).value(step)) - 2.0 * 
        (mObj.sourcegen.at(
          temp.sourceIndex_).value(step - (stepSize_*factor))));
      } else {
        b_.at(temp.indexInfo.currentIndex_.value()) = 
        (Constants::SIGMA / (stepSize_ * factor)) * ((3.0/2.0) * 
        (mObj.sourcegen.at(temp.sourceIndex_).value(step)) - 2.0 * 
        (mObj.sourcegen.at(
          temp.sourceIndex_).value(step - (stepSize_*factor))) + 0.5 * 
        (mObj.sourcegen.at(
          temp.sourceIndex_).value(step - (2 * stepSize_*factor))));
      }
      
    }
  }
}

void Simulation::handle_ccvs(Matrix &mObj) {
  for (const auto &j : mObj.components.ccvsIndices) {
    auto &temp = std::get<CCVS>(mObj.components.devices.at(j));
    if (atyp_ == AnalysisType::Phase) {
      if(temp.indexInfo.posIndex_ && !temp.indexInfo.negIndex_) {
        temp.pn1_ = (x_.at(temp.indexInfo.posIndex_.value()));
      } else if(!temp.indexInfo.posIndex_ && temp.indexInfo.negIndex_) {
        temp.pn1_ = (-x_.at(temp.indexInfo.negIndex_.value()));
      } else {
        temp.pn1_ = (x_.at(temp.indexInfo.posIndex_.value())
                - x_.at(temp.indexInfo.negIndex_.value()));
      }
      b_.at(temp.indexInfo.currentIndex_.value()) = 
        (4.0 / 3.0) * temp.pn1_ - (1.0 / 3.0) * temp.pn2_; 
      temp.pn4_ = temp.pn3_;
      temp.pn3_ = temp.pn2_;
      temp.pn2_ = temp.pn1_;
    }
  }
}

void Simulation::handle_vccs(Matrix &mObj) {
  for (const auto &j : mObj.components.vccsIndices) {
    auto &temp = std::get<VCCS>(mObj.components.devices.at(j));
    if (atyp_ == AnalysisType::Phase) {
      if(temp.posIndex2_ && !temp.negIndex2_) {
        temp.pn1_ = (x_.at(temp.posIndex2_.value()));
      } else if(!temp.posIndex2_ && temp.negIndex2_) {
        temp.pn1_ = (-x_.at(temp.negIndex2_.value()));
      } else {
        temp.pn1_ = (x_.at(temp.posIndex2_.value())
                - x_.at(temp.negIndex2_.value()));
      }
      b_.at(temp.indexInfo.currentIndex_.value()) = 
        (4.0 / 3.0) * temp.pn1_ - (1.0 / 3.0) * temp.pn2_; 
      temp.pn4_ = temp.pn3_;
      temp.pn3_ = temp.pn2_;
      temp.pn2_ = temp.pn1_;
    }
  }
}

void Simulation::handle_tx(
  Matrix &mObj, const int &i, double &step, double factor) {
  for (const auto &j : mObj.components.txIndices) {
    auto &temp = std::get<TransmissionLine>(mObj.components.devices.at(j));
    // Z0
    double &Z = temp.netlistInfo.value_;
    // Td == k
    int &k = temp.timestepDelay_;
    // Shorthands
    JoSIM::NodeConfig &nc = temp.indexInfo.nodeConfig_, 
      &nc2 = temp.nodeConfig2_;
    int_o &posInd = temp.indexInfo.posIndex_, 
      &negInd = temp.indexInfo.negIndex_,
      &posInd2 = temp.posIndex2_,
      &negInd2 = temp.negIndex2_;
    int &curInd = temp.indexInfo.currentIndex_.value(),
      &curInd2 = temp.currentIndex2_;
    if(atyp_ == AnalysisType::Voltage) {
      if(i >= k) {
        // φ1n-k
        if(nc == NodeConfig::POSGND) {
          temp.nk_1_ = results.xVector.at(posInd.value()).value().at(i - k);
        } else if(nc == NodeConfig::GNDNEG) {
          temp.nk_1_ = -results.xVector.at(negInd.value()).value().at(i - k);
        } else {
          temp.nk_1_ = results.xVector.at(posInd.value()).value().at(i - k) - 
            results.xVector.at(negInd.value()).value().at(i - k);
        }
        // φ2n-k
        if(nc2 == NodeConfig::POSGND) {
          temp.nk_2_ = results.xVector.at(posInd2.value()).value().at(i - k);
        } else if(nc2 == NodeConfig::GNDNEG) {
          temp.nk_2_ = -results.xVector.at(negInd2.value()).value().at(i - k);
        } else {
          temp.nk_2_ = results.xVector.at(posInd2.value()).value().at(i - k) - 
            results.xVector.at(negInd2.value()).value().at(i - k);
        }
        // I1n-k
        double &I1nk = results.xVector.at(curInd).value().at(i - k);
        // I2n-k
        double &I2nk = results.xVector.at(curInd2).value().at(i - k);
        // I1 = ZI2n-k + V2n-k
        b_.at(curInd) = Z * I2nk + temp.nk_2_;
        // I2 = ZI1n-k + V1n-k
        b_.at(curInd2) = Z * I1nk + temp.nk_1_;
      }
    } else if (atyp_ == AnalysisType::Phase) {
      if (i > 0) {
        temp.n2_1_ = temp.n1_1_;
        temp.n2_2_ = temp.n1_2_;
        // φ1n-1
        if(nc == NodeConfig::POSGND) {
          temp.n1_1_ = (x_.at(posInd.value()));
        } else if(nc == NodeConfig::GNDNEG) {
          temp.n1_1_ = (-x_.at(negInd.value()));
        } else {
          temp.n1_1_ = (x_.at(posInd.value()) - x_.at(negInd.value()));
        }
        // φ2n-1
        if(nc2 == NodeConfig::POSGND) {
          temp.n1_2_ = (x_.at(posInd2.value()));
        } else if(nc2 == NodeConfig::GNDNEG) {
          temp.n1_2_ = (-x_.at(negInd2.value()));
        } else {
          temp.n1_2_ = (x_.at(posInd2.value()) - x_.at(negInd2.value()));
        }
      }
      if (i >= k) {
        // φ1n-k
        if(nc == NodeConfig::POSGND) {
          temp.nk_1_ = results.xVector.at(posInd.value()).value().at(i - k);
        } else if(nc == NodeConfig::GNDNEG) {
          temp.nk_1_ = -results.xVector.at(negInd.value()).value().at(i - k);
        } else {
          temp.nk_1_ = results.xVector.at(posInd.value()).value().at(i - k) - 
            results.xVector.at(negInd.value()).value().at(i - k);
        }
        // φ2n-k
        if(nc2 == NodeConfig::POSGND) {
          temp.nk_2_ = results.xVector.at(posInd2.value()).value().at(i - k);
        } else if(nc2 == NodeConfig::GNDNEG) {
          temp.nk_2_ = -results.xVector.at(negInd2.value()).value().at(i - k);
        } else {
          temp.nk_2_ = results.xVector.at(posInd2.value()).value().at(i - k) - 
            results.xVector.at(negInd2.value()).value().at(i - k);
        }
        // I1n-k
        double &I1nk = results.xVector.at(curInd).value().at(i - k);
        // I2n-k
        double &I2nk = results.xVector.at(curInd2).value().at(i - k);
        if (i == k) {
          // I1 = Z(2e/hbar)(2h/3)I2n-k + (4/3)φ1n-1 - (1/3)φ1n-2 + φ2n-k
          b_.at(curInd) = (Z / Constants::SIGMA) * 
            ((2.0 * stepSize_ * factor) / 3.0) * I2nk + 
            (4.0 / 3.0) * temp.n1_1_ - (1.0 / 3.0) * temp.n2_1_ + temp.nk_2_;
          // I2 = Z(2e/hbar)(2h/3)I1n-k + (4/3)φ2n-1 - (1/3)φ2n-2 + φ1n-k
          b_.at(curInd2) = (Z / Constants::SIGMA) * 
            ((2.0 * stepSize_ * factor) / 3.0) * I1nk 
            + (4.0 / 3.0) * temp.n1_2_ - (1.0 / 3.0) * temp.n2_2_ + temp.nk_1_;
        } else if (i == k + 1) {
          // φ1n-k-1
          if(nc == NodeConfig::POSGND) {
            temp.nk1_1_ = results.xVector.at(
              posInd.value()).value().at(i - k - 1);
          } else if(nc == NodeConfig::GNDNEG) {
            temp.nk1_1_ = -results.xVector.at(
              negInd.value()).value().at(i - k - 1);
          } else {
            temp.nk1_1_ = results.xVector.at(
              posInd.value()).value().at(i - k - 1) - 
              results.xVector.at(negInd.value()).value().at(i - k - 1);
          }
          // φ2n-k-1
          if(nc2 == NodeConfig::POSGND) {
            temp.nk1_2_ = results.xVector.at(
              posInd2.value()).value().at(i - k - 1);
          } else if(nc2 == NodeConfig::GNDNEG) {
            temp.nk1_2_ = -results.xVector.at(
              negInd2.value()).value().at(i - k - 1);
          } else {
            temp.nk1_2_ = results.xVector.at(
              posInd2.value()).value().at(i - k - 1) - 
              results.xVector.at(negInd2.value()).value().at(i - k - 1);
          }
          // I1 = Z(2e/hbar)(2h/3)I2n-k + (4/3)φ1n-1 - (1/3)φ1n-2 +
          //      φ2n-k - (4/3)φ2n-k-1
          b_.at(curInd) = (Z / Constants::SIGMA) * 
            ((2.0 * stepSize_ * factor) / 3.0) * I2nk + 
            (4.0 / 3.0) * temp.n1_1_ - (1.0 / 3.0) * temp.n2_1_ + 
            temp.nk_2_ - (4.0 / 3.0) * temp.nk1_2_;
          // I2 = Z(2e/hbar)(2h/3)I1n-k + (4/3)φ2n-1 - (1/3)φ2n-2 +
          //      φ1n-k - (4/3)φ1n-k-1
          b_.at(curInd2) = (Z / Constants::SIGMA) * 
            ((2.0 * stepSize_ * factor) / 3.0) * I1nk +
            (4.0 / 3.0) * temp.n1_2_ - (1.0 / 3.0) * temp.n2_2_ + 
            temp.nk_1_ - (4.0 / 3.0) * temp.nk1_1_;
        } else if (i > k + 1) {
          // φ1n-k-1
          if(nc == NodeConfig::POSGND) {
            temp.nk1_1_ = results.xVector.at(
              posInd.value()).value().at(i - k - 1);
          } else if(nc == NodeConfig::GNDNEG) {
            temp.nk1_1_ = -results.xVector.at(
              negInd.value()).value().at(i - k - 1);
          } else {
            temp.nk1_1_ = results.xVector.at(
              posInd.value()).value().at(i - k - 1) - 
              results.xVector.at(negInd.value()).value().at(i - k - 1);
          }
          // φ2n-k-1
          if(nc2 == NodeConfig::POSGND) {
            temp.nk1_2_ = results.xVector.at(
              posInd2.value()).value().at(i - k - 1);
          } else if(nc2 == NodeConfig::GNDNEG) {
            temp.nk1_2_ = -results.xVector.at(
              negInd2.value()).value().at(i - k - 1);
          } else {
            temp.nk1_2_ = results.xVector.at(
              posInd2.value()).value().at(i - k - 1) - 
              results.xVector.at(negInd2.value()).value().at(i - k - 1);
          }
          // φ1n-k-2
          if(nc == NodeConfig::POSGND) {
            temp.nk2_1_ = results.xVector.at(
              posInd.value()).value().at(i - k - 2);
          } else if(nc == NodeConfig::GNDNEG) {
            temp.nk2_1_ = -results.xVector.at(
              negInd.value()).value().at(i - k - 2);
          } else {
            temp.nk2_1_ = results.xVector.at(
              posInd.value()).value().at(i - k - 2) - 
              results.xVector.at(negInd.value()).value().at(i - k - 2);
          }
          // φ2n-k-2
          if(nc2 == NodeConfig::POSGND) {
            temp.nk2_2_ = results.xVector.at(
              posInd2.value()).value().at(i - k - 2);
          } else if(nc2 == NodeConfig::GNDNEG) {
            temp.nk2_2_ = -results.xVector.at(
              negInd2.value()).value().at(i - k - 2);
          } else {
            temp.nk2_2_ = results.xVector.at(
              posInd2.value()).value().at(i - k - 2) - 
              results.xVector.at(negInd2.value()).value().at(i - k - 2);
          }
          // I1 = Z(2e/hbar)(2h/3)I2n-k + (4/3)φ1n-1 - (1/3)φ1n-2 +
          //      φ2n-k - (4/3)φ2n-k-1 + (1/3)φ2n-k-2
          b_.at(curInd) = (Z / Constants::SIGMA) * 
            ((2.0 * stepSize_ * factor) / 3.0) * I2nk + 
            (4.0 / 3.0) * temp.n1_1_ - (1.0 / 3.0) * temp.n2_1_ + temp.nk_2_ - 
            (4.0 / 3.0) * temp.nk1_2_ + (1.0 / 3.0) * temp.nk2_2_;
          // I2 = Z(2e/hbar)(2h/3)I1n-k + (4/3)φ2n-1 - (1/3)φ2n-2 +
          //      φ1n-k - (4/3)φ1n-k-1 + (1/3)φ1n-k-2
          b_.at(curInd2) = (Z / Constants::SIGMA) * 
            ((2.0 * stepSize_ * factor) / 3.0) * I1nk + 
            (4.0 / 3.0) * temp.n1_2_ - (1.0 / 3.0) * temp.n2_2_ + temp.nk_1_ - 
            (4.0 / 3.0) * temp.nk1_1_ + (1.0 / 3.0) * temp.nk2_1_;
        } 
      } else {
        // I1 = (4/3)φ1n-1 - (1/3)φ1n-2
        b_.at(curInd) = (4.0 / 3.0) * temp.n1_1_ - (1.0 / 3.0) * temp.n2_1_;
        // I2 = (4/3)φ2n-1 - (1/3)φ2n-2
        b_.at(curInd2) = (4.0 / 3.0) * temp.n1_2_ - (1.0 / 3.0) * temp.n2_2_;
      }
    }
  }
}