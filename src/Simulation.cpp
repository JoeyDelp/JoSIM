// Copyright (c) 2020 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/Simulation.hpp"
#include "JoSIM/AnalysisType.hpp"
#include "JoSIM/Components.hpp"
#include "JoSIM/Matrix.hpp"
#include "JoSIM/Constants.hpp"
#include "JoSIM/Model.hpp"
#include "JoSIM/ProgressPrinter.hpp"

#include "suitesparse/klu.h"

#include <cassert>
#include <cmath>
#include <iostream>

using namespace JoSIM;

void Simulation::trans_sim_new(Input &iObj, Matrix &mObj) {
  // Create a left hand side vector (x) and a previous LHS
  std::vector<double> lhsValues, LHS_PRE(mObj.rp.size() - 1, 0.0);
  // Determine the simulation size
  int simSize = iObj.transSim.get_simsize();
  // Should all vectors be saved, default false
  int saveAll = false;
  // If nothing was specified to be saved, save everything!
  if(mObj.relevantTraces.size() == 0) saveAll = true;
  // Create an x vector where results will be stored
  results.xVector.resize(mObj.rp.size() - 1);
  // Determine the size of this x vector
  if(!saveAll) {
    for (const auto &i : mObj.relevantIndices) {
      results.xVector.at(i).emplace();
    }
  } else {
    for (int i = 0; i < mObj.rp.size() - 1; ++i) {
      results.xVector.at(i).emplace();
    }
  }
  // Get this constant value only once to be used over and over
  double hbar_he = 
    (Constants::HBAR / (iObj.transSim.get_prstep() * Constants::EV));
  // Indicator as to whether convergence was reached
  int ok;
  // Indicator as to wether a new LU decomposition is needed
  bool needsLU = false;
  // KLU variables
  klu_symbolic *Symbolic;
  klu_common Common;
  klu_numeric *Numeric;
  // Do the KLU setup
  ok = klu_defaults(&Common);
  // Make sure that everything is okay
  assert(ok);
  // Create a symbolic LU object
  Symbolic = klu_analyze(mObj.rp.size() - 1, &mObj.rp.front(), &mObj.ci.front(),
                         &Common);
  // Create a numeric LU object
  Numeric = klu_factor(&mObj.rp.front(), &mObj.ci.front(),
                       &mObj.nz.front(), Symbolic, &Common);
  // Ensure time axis is cleared
  results.timeAxis.clear();
  std::optional<BufferedProgressPrinter<TimeProgressPrinter>> buffered_printer;
  // If minimal reporting is not enabled
  if(!iObj.argMin) {
    // Begin simulation progress reporting
    std::cout << "Simulation Progress:" << std::endl;
    // Threaded printer object to move printing away from main thread
    auto printer = TimeProgressPrinter(simSize);
    buffered_printer = BufferedProgressPrinter<TimeProgressPrinter>(
      std::move(printer), 0);
  }

  // Create a seperate thread that will be used for printing progress
  std::thread printingThread;
  // Start the simulation loop
  for(int i = 0; i < simSize; ++i) {
    // If not minimal printing
    if(!iObj.argMin) {
      // Report progress
      buffered_printer.value().update(i);
    }
    // Create a new right hand size vector (b) at each loop
    std::vector<double> RHS(mObj.rp.size() - 1, 0.0);
    // Handle current sources
    for (const auto &j : mObj.components.currentsources) {
      if(j.indexInfo.posIndex_ && !j.indexInfo.negIndex_) {
          RHS.at(j.indexInfo.posIndex_.value()) -= 
            (mObj.sources.at(j.sourceIndex_).at(i));
        } else if(!j.indexInfo.posIndex_ && j.indexInfo.negIndex_) {
          RHS.at(j.indexInfo.negIndex_.value()) += 
            (mObj.sources.at(j.sourceIndex_).at(i));
        } else {
          RHS.at(j.indexInfo.posIndex_.value()) -= 
            (mObj.sources.at(j.sourceIndex_).at(i));
          RHS.at(j.indexInfo.negIndex_.value()) += 
            (mObj.sources.at(j.sourceIndex_).at(i));
        }
    }
    // Handle resistors
    for (const auto &j : mObj.components.resistorIndices) {
      auto &temp = std::get<Resistor>(mObj.components.devices.at(j));
      double prevNode;
      if(temp.indexInfo.posIndex_ && !temp.indexInfo.negIndex_) {
        prevNode = (LHS_PRE.at(temp.indexInfo.posIndex_.value()));
      } else if(!temp.indexInfo.posIndex_ && temp.indexInfo.negIndex_) {
        prevNode = (-LHS_PRE.at(temp.indexInfo.negIndex_.value()));
      } else {
        prevNode = (LHS_PRE.at(temp.indexInfo.posIndex_.value())
                - LHS_PRE.at(temp.indexInfo.negIndex_.value()));
      }
      if (iObj.argAnal == AnalysisType::Phase) {
        // 4/3 φp1 - 1/3 φp2
        RHS.at(temp.indexInfo.currentIndex_.value()) = 
          (4.0 / 3.0) * prevNode - (1.0 / 3.0) * temp.pn2_.value();
        temp.pn2_ = prevNode;
      }
    }
    // Handle inductors
    for (const auto &j : mObj.components.inductorIndices) {
      auto &temp = std::get<Inductor>(mObj.components.devices.at(j));
      if(iObj.argAnal == AnalysisType::Voltage) {
        // -2L/h Ip + L/2h Ip2
        RHS.at(temp.indexInfo.currentIndex_.value()) = 
          -(2.0 * temp.netlistInfo.value_/(iObj.transSim.get_prstep())) * 
            LHS_PRE.at(temp.indexInfo.currentIndex_.value()) + 
            ((temp.netlistInfo.value_/(2.0 * iObj.transSim.get_prstep())) * 
            temp.In2_);
        // -2M/h Im + M/2h Im2
        for(const auto &m : temp.get_mutualInductance()) {
          Inductor &mi = 
            std::get<Inductor>(mObj.components.devices.at(m.first));
          RHS.at(temp.indexInfo.currentIndex_.value()) += 
            (-((2 * m.second) / iObj.transSim.get_prstep()) * 
              LHS_PRE.at(mi.indexInfo.currentIndex_.value()) + 
              (m.second / (2.0 * iObj.transSim.get_prstep())) * mi.In2_);
        }
        temp.In2_ = LHS_PRE.at(temp.indexInfo.currentIndex_.value());
      }
    }
    // Handle capacitors
    for (const auto &j : mObj.components.capacitorIndices) {
      auto &temp = std::get<Capacitor>(mObj.components.devices.at(j));
      double prevNode;
      if(temp.indexInfo.posIndex_ && !temp.indexInfo.negIndex_) {
        prevNode = (LHS_PRE.at(temp.indexInfo.posIndex_.value()));
      } else if(!temp.indexInfo.posIndex_ && temp.indexInfo.negIndex_) {
        prevNode = (-LHS_PRE.at(temp.indexInfo.negIndex_.value()));
      } else {
        prevNode = (LHS_PRE.at(temp.indexInfo.posIndex_.value())
                - LHS_PRE.at(temp.indexInfo.negIndex_.value()));
      }
      if(iObj.argAnal == AnalysisType::Voltage) {
        // 4/3 Vp1 - 1/3 Vp2
        RHS.at(temp.indexInfo.currentIndex_.value()) = 
          (4.0/3.0) * prevNode - (1.0/3.0) * temp.pn1_;
      } else if (iObj.argAnal == AnalysisType::Phase) {
        // 8/3 φp1 + 10/9 φp2 -1/9 φp3
        RHS.at(temp.indexInfo.currentIndex_.value()) = 
          (8.0/3.0) * prevNode + (10.0/9.0) * temp.pn1_ - (1.0/9.0) * temp.pn3_;
        temp.pn3_ = temp.pn2_;
        temp.pn2_ = temp.pn1_;
      }
      temp.pn1_ = prevNode;
    }
    // Handle junctions
    for (const auto &j : mObj.components.junctionIndices) {
      auto &temp = std::get<JJ>(mObj.components.devices.at(j));
      const auto &model = temp.model_;
      double prevNode;
      if(temp.indexInfo.posIndex_ && !temp.indexInfo.negIndex_) {
        prevNode = (LHS_PRE.at(temp.indexInfo.posIndex_.value()));
      } else if(!temp.indexInfo.posIndex_ && temp.indexInfo.negIndex_) {
        prevNode = (-LHS_PRE.at(temp.indexInfo.negIndex_.value()));
      } else {
        prevNode = (LHS_PRE.at(temp.indexInfo.posIndex_.value())
                - LHS_PRE.at(temp.indexInfo.negIndex_.value()));
      }
      if(i > 0) {
        if(iObj.argAnal == AnalysisType::Voltage) {
          temp.vn1_ = prevNode;
          temp.pn1_ = LHS_PRE.at(temp.variableIndex_);
        } else if (iObj.argAnal == AnalysisType::Phase) {
          temp.vn1_ = LHS_PRE.at(temp.variableIndex_);
          temp.pn1_ = prevNode;
        }
      }
      // Ensure timestep is not too large
      if ((double)i/(double)simSize > 0.01) {
        if (abs(temp.phi0_ - temp.pn1_) > (0.25 * 2 * Constants::PI)) {
          Errors::simulation_errors(
            SimulationErrors::PHASEGUESS_TOO_LARGE, temp.netlistInfo.label_);
        }
      }
      double v0;
      // Guess voltage (V0)
      v0 = (5.0/2.0) * temp.vn1_ - 2.0 * temp.vn2_ + (1.0 / 2.0) * temp.vn3_;
      // Phase guess (P0)
      temp.phi0_ = (4.0/3.0) * temp.pn1_ - (1.0/3.0) * temp.pn2_ + 
        ((1.0 / Constants::SIGMA) * 
          ((2.0 * iObj.transSim.get_prstep()) / 3.0)) * v0;
      // (hbar / 2 * e) ( -(2 / h) φp1 + (1 / 2h) φp2 )
      if(iObj.argAnal == AnalysisType::Voltage) {
        RHS.at(temp.variableIndex_) = 
          (Constants::SIGMA) * (-(2.0 / iObj.transSim.get_prstep()) * 
            temp.pn1_ + (1.0 / (2.0 * iObj.transSim.get_prstep())) * temp.pn2_);
      // (4 / 3) φp1 - (1/3) φp2 
      } else if (iObj.argAnal == AnalysisType::Phase) {
        RHS.at(temp.variableIndex_) = 
          (4.0 / 3.0) * temp.pn1_ - (1.0 / 3.0) * temp.pn2_;
      }
      temp.pn2_ = temp.pn1_;
      temp.vn3_ = temp.vn2_;
      // Update junction transition
      if(model.value().get_resistanceType() == 1) {
        auto testLU = temp.update_value(v0);
        if(testLU && !needsLU) {
          needsLU = true;
        }
      }
      // -(hR / h + 2RC) * (Ic sin φ0 - 2C / h Vp1 + C/2h Vp2 + It) 
      RHS.at(temp.indexInfo.currentIndex_.value()) = 
        (temp.matrixInfo.nonZeros_.back()) * ((((Constants::PI * temp.del_) / 
          (2 * Constants::EV * temp.rncalc_)) * (sin(temp.phi0_) / 
            sqrt(1 - model.value().get_transparency() * (sin(temp.phi0_ / 2) * 
            sin(temp.phi0_ / 2)))) * tanh((temp.del_) / 
          (2 * Constants::BOLTZMANN * model.value().get_temperature()) *
          sqrt(1 - model.value().get_transparency() * 
            (sin(temp.phi0_ / 2) * sin(temp.phi0_ / 2))))) -
          (((2 * model.value().get_capacitance()) / 
            iObj.transSim.get_prstep()) * temp.vn1_) + 
          ((model.value().get_capacitance() / 
            (2.0 * iObj.transSim.get_prstep())) * temp.vn2_) + 
          temp.transitionCurrent_);
      temp.vn2_ = temp.vn1_;
    }
    if (needsLU) {
      mObj.create_nz();
      klu_free_numeric(&Numeric, &Common);
      Numeric = klu_factor(&mObj.rp.front(), &mObj.ci.front(),
                          &mObj.nz.front(), Symbolic, &Common);
      needsLU = false;
    }
    // Handle voltage sources
    for (const auto &j : mObj.components.vsIndices) {
      const auto &temp = std::get<VoltageSource>(mObj.components.devices.at(j));
      if(iObj.argAnal == AnalysisType::Voltage) {
        RHS.at(temp.indexInfo.currentIndex_.value()) = 
          mObj.sources.at(temp.sourceIndex_).at(i);
      } else if (iObj.argAnal == AnalysisType::Phase) {
        double prevNode;
        if(temp.indexInfo.posIndex_ && !temp.indexInfo.negIndex_) {
          prevNode = (LHS_PRE.at(temp.indexInfo.posIndex_.value()));
        } else if(!temp.indexInfo.posIndex_ && temp.indexInfo.negIndex_) {
          prevNode = (-LHS_PRE.at(temp.indexInfo.negIndex_.value()));
        } else {
          prevNode = (LHS_PRE.at(temp.indexInfo.posIndex_.value())
                  - LHS_PRE.at(temp.indexInfo.negIndex_.value()));
        }
        if(i < 1) {
          RHS.at(temp.indexInfo.currentIndex_.value()) = 
            (iObj.transSim.get_prstep() / (2 * Constants::SIGMA)) * 
            (mObj.sources.at(temp.sourceIndex_).at(i)) + prevNode;
        } else {
          RHS.at(temp.indexInfo.currentIndex_.value()) = 
            (iObj.transSim.get_prstep() / (2 * Constants::SIGMA)) * 
            (mObj.sources.at(temp.sourceIndex_).at(i) - 
              mObj.sources.at(temp.sourceIndex_).at(i-1)) + prevNode;
        }
      }
    }
    // Handle phase sources
    for (const auto &j : mObj.components.psIndices) {
      const auto &temp = std::get<PhaseSource>(mObj.components.devices.at(j));
      if (iObj.argAnal == AnalysisType::Phase) {
        RHS.at(temp.indexInfo.currentIndex_.value()) = 
          mObj.sources.at(temp.sourceIndex_).at(i);
      } else if(iObj.argAnal == AnalysisType::Voltage) {
        double prevNode;
        if(temp.indexInfo.posIndex_ && !temp.indexInfo.negIndex_) {
          prevNode = (LHS_PRE.at(temp.indexInfo.posIndex_.value()));
        } else if(!temp.indexInfo.posIndex_ && temp.indexInfo.negIndex_) {
          prevNode = (-LHS_PRE.at(temp.indexInfo.negIndex_.value()));
        } else {
          prevNode = (LHS_PRE.at(temp.indexInfo.posIndex_.value())
                  - LHS_PRE.at(temp.indexInfo.negIndex_.value()));
        }
        if(i == 0) {
          RHS.at(temp.indexInfo.currentIndex_.value()) = 
            ((Constants::SIGMA * 2) / iObj.transSim.get_prstep()) * 
            (mObj.sources.at(temp.sourceIndex_).at(i)) - prevNode;
        } else {
          RHS.at(temp.indexInfo.currentIndex_.value()) = 
            ((Constants::SIGMA * 2) / iObj.transSim.get_prstep()) * 
            (mObj.sources.at(temp.sourceIndex_).at(i) - 
              mObj.sources.at(temp.sourceIndex_).at(i-1)) - prevNode;
        }
      }
    }
    // Handle ccvs
    for (const auto &j : mObj.components.ccvsIndices) {
      auto &temp = std::get<CCVS>(mObj.components.devices.at(j));
      if (iObj.argAnal == AnalysisType::Phase) {
        double prevNode;
        if(temp.indexInfo.posIndex_ && !temp.indexInfo.negIndex_) {
          prevNode = (LHS_PRE.at(temp.indexInfo.posIndex_.value()));
        } else if(!temp.indexInfo.posIndex_ && temp.indexInfo.negIndex_) {
          prevNode = (-LHS_PRE.at(temp.indexInfo.negIndex_.value()));
        } else {
          prevNode = (LHS_PRE.at(temp.indexInfo.posIndex_.value())
                  - LHS_PRE.at(temp.indexInfo.negIndex_.value()));
        }
        RHS.at(temp.indexInfo.currentIndex_.value()) = 
          (4.0 / 3.0) * prevNode - (1.0 / 3.0) * temp.pn2_.value(); 
        temp.pn2_ = prevNode;
      }
    }
     // Handle vccs
    for (const auto &j : mObj.components.vccsIndices) {
      auto &temp = std::get<VCCS>(mObj.components.devices.at(j));
      if (iObj.argAnal == AnalysisType::Phase) {
        double prevNode;
        if(temp.posIndex2_ && !temp.negIndex2_) {
          prevNode = (LHS_PRE.at(temp.posIndex2_.value()));
        } else if(!temp.posIndex2_ && temp.negIndex2_) {
          prevNode = (-LHS_PRE.at(temp.negIndex2_.value()));
        } else {
          prevNode = (LHS_PRE.at(temp.posIndex2_.value())
                  - LHS_PRE.at(temp.negIndex2_.value()));
        }
        RHS.at(temp.indexInfo.currentIndex_.value()) = 
          (4.0 / 3.0) * prevNode - (1.0 / 3.0) * temp.pn2_.value(); 
        temp.pn2_ = prevNode;
      }
    }
    // Handle transmission lines
    for (const auto &j : mObj.components.txIndices) {
      auto &temp = std::get<TransmissionLine>(mObj.components.devices.at(j));
      if(iObj.argAnal == AnalysisType::Voltage) {
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
          RHS.at(temp.indexInfo.currentIndex_.value()) = 
            prevNode2k + temp.netlistInfo.value_ * results.xVector.at(
              temp.currentIndex2_).value().at(i - temp.timestepDelay_);
          // IT2 = V1(n-k) + Z0 I1(n-k)
          RHS.at(temp.currentIndex2_) = 
            prevNodek + temp.netlistInfo.value_ * results.xVector.at(
              temp.indexInfo.currentIndex_.value()).value().at(
                i - temp.timestepDelay_);
        }
      } else if(iObj.argAnal == AnalysisType::Phase) {
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
              RHS.at(temp.indexInfo.currentIndex_.value()) = 
                temp.netlistInfo.value_ * results.xVector.at(
                  temp.currentIndex2_).value().at(i - temp.timestepDelay_) -
                (4.0/3.0) * prevNodeN - (1.0/3.0) * temp.p1n2_ - prevNode2k;
              // IT2 = (hZ0/2σ) * IT1n-k - (4/3) φ2n-1 - (1/3) φ2n-2 - φ1n-k                          
              RHS.at(temp.currentIndex2_) = 
                temp.netlistInfo.value_ * results.xVector.at(
                  temp.indexInfo.currentIndex_.value()).value().at(
                    i - temp.timestepDelay_) - 
                (4.0/3.0) * prevNode2N - (1.0/3.0) * temp.p2n2_ - prevNodek;
            } else {
              // IT1 = (hZ0/2σ) * IT2n-k - (4/3) φ1n-1 - (1/3) φ1n-2 - φ2n-k - 
              //        (4/3) φ2n-k-1 + (1/3) φ2n-k-2
              RHS.at(temp.indexInfo.currentIndex_.value()) = 
                temp.netlistInfo.value_ * results.xVector.at(
                  temp.currentIndex2_).value().at(i - temp.timestepDelay_) - 
                (4.0/3.0) * prevNodeN - (1.0/3.0) * temp.p1n2_ - prevNode2k -
                (4.0/3.0) * prevNode2k1 + (1.0/3.0) * temp.p2nk2_;
              // IT2 = (hZ0/2σ) * IT1n-k - (4/3) φ2n-1 - (1/3) φ2n-2 - φ1n-k - 
              //        (4/3) φ1n-k-1 + (1/3) φ1n-k-2                              
              RHS.at(temp.currentIndex2_) = 
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
            RHS.at(temp.indexInfo.currentIndex_.value()) = 
              -(4.0/3.0) * prevNodeN - (1.0/3.0) * temp.p1n2_;
            // IT2 = -(4/3) φ2n-1 - (1/3) φ2n-2                        
            RHS.at(temp.currentIndex2_) = 
              -(4.0/3.0) * prevNode2N - (1.0/3.0) * temp.p2n2_;
          }
          temp.p1n2_ = prevNodeN;
          temp.p2n2_ = prevNode2N;
        }
      }
    }

    LHS_PRE = RHS;

    ok = klu_tsolve(
      Symbolic, Numeric, mObj.rp.size() - 1, 1, &LHS_PRE.front(), &Common);
    if (!ok) {
      Errors::simulation_errors(SimulationErrors::MATRIX_SINGULAR);
    }

    lhsValues = LHS_PRE;
    if(!saveAll) {
      for (int m = 0; m < mObj.relevantIndices.size(); ++m) {
        results.xVector.at(mObj.relevantIndices.at(m)).value().emplace_back(
          lhsValues.at(mObj.relevantIndices.at(m)));
      }
    } else {
      for (int m = 0; m < mObj.rp.size() - 1; ++m) {
        results.xVector.at(m).value().emplace_back(lhsValues.at(m));
      }
    }

    results.timeAxis.emplace_back(i * iObj.transSim.get_prstep());
  }
  if(!iObj.argMin) {
    buffered_printer.value().done();
    std::cout << "\n";
  }
  
  klu_free_symbolic(&Symbolic, &Common);
  klu_free_numeric(&Numeric, &Common);
}
