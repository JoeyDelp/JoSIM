// Copyright (c) 2020 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/Simulation.hpp"
#include "JoSIM/AnalysisType.hpp"
#include "JoSIM/Components.hpp"
#include "JoSIM/IntegrationType.hpp"
#include "JoSIM/Matrix.hpp"
#include "JoSIM/Constants.hpp"
#include "JoSIM/Model.hpp"

#include "suitesparse/klu.h"

#include <cassert>
#include <cmath>
#include <iostream>

using namespace JoSIM;

void Simulation::trans_sim_new(Input &iObj, 
                                Matrix &mObj) {
  std::vector<double> lhsValues, LHS_PRE(mObj.rp.size() - 1, 0.0);
  int simSize = iObj.transSim.get_simsize();
  int saveAll = false;
  if(mObj.relevantTraces.size() == 0) saveAll = true;
  results.xVector.resize(mObj.rp.size() - 1);
  if(!saveAll) {
    for (const auto &i : mObj.relevantIndices) {
      results.xVector.at(i).emplace();
    }
  } else {
    for (int i = 0; i < mObj.rp.size() - 1; ++i) {
      results.xVector.at(i).emplace();
    }
  }
  double hbar_he = (Constants::HBAR / (iObj.transSim.get_prstep() * Constants::EV));
  int ok;
  int fqtr, sqtr, tqtr;
  fqtr = simSize/4;
  sqtr = simSize/2;
  tqtr = simSize/4 * 3;
  bool needsLU = false;
  klu_symbolic *Symbolic;
  klu_common Common;
  klu_numeric *Numeric;
  ok = klu_defaults(&Common);
  assert(ok);
  Symbolic = klu_analyze(mObj.rp.size() - 1, &mObj.rp.front(), &mObj.ci.front(),
                         &Common);
  Numeric = klu_factor(&mObj.rp.front(), &mObj.ci.front(),
                       &mObj.nz.front(), Symbolic, &Common);
  results.timeAxis.clear();
  if(!iObj.argMin) {
    std::cout << "Simulation Progress:" << std::endl;
    std::cout << "0%\r" << std::flush;
  }
  for(int i = 0; i < simSize; ++i) {
    std::vector<double> RHS(mObj.rp.size() - 1, 0.0);
    if(!iObj.argMin){
      if(i == fqtr) std::cout << "25%\r" << std::flush;
      if(i == sqtr) std::cout << "50%\r" << std::flush;
      if(i == tqtr) std::cout << "75%\r" << std::flush;
    }
    // Handle current sources
    for (const auto &j : mObj.components.currentsources) {
      if(j.get_posIndex() && !j.get_negIndex()) {
          RHS.at(j.get_posIndex().value()) -= (mObj.sources.at(j.get_sourceIndex()).at(i));
        } else if(!j.get_posIndex() && j.get_negIndex()) {
          RHS.at(j.get_negIndex().value()) += (mObj.sources.at(j.get_sourceIndex()).at(i));
        } else {
          RHS.at(j.get_posIndex().value()) -= (mObj.sources.at(j.get_sourceIndex()).at(i));
          RHS.at(j.get_negIndex().value()) += (mObj.sources.at(j.get_sourceIndex()).at(i));
        }
    }
    // Handle resistors
    for (const auto &j : mObj.components.resistorIndices) {
      auto &temp = std::get<Resistor>(mObj.components.devices.at(j));
      double prevNode;
      if(temp.get_posIndex() && !temp.get_negIndex()) {
        prevNode = (LHS_PRE.at(temp.get_posIndex().value()));
      } else if(!temp.get_posIndex() && temp.get_negIndex()) {
        prevNode = (-LHS_PRE.at(temp.get_negIndex().value()));
      } else {
        prevNode = (LHS_PRE.at(temp.get_posIndex().value())
                - LHS_PRE.at(temp.get_negIndex().value()));
      }
      if(iObj.argAnal == AnalysisType::Voltage) {
      } else if (iObj.argAnal == AnalysisType::Phase) {
        if(iObj.argInt == IntegrationType::Trapezoidal) {
          // Rh/2σ Ip + φp
          RHS.at(temp.get_currentIndex()) = temp.get_value() * LHS_PRE.at(temp.get_currentIndex()) + prevNode;
        } else {
          // 4/3 φp1 - 1/3 φp2
          RHS.at(temp.get_currentIndex()) = (4.0 / 3.0) * prevNode - (1.0 / 3.0) * temp.get_pn2();
          temp.set_pn2(prevNode);
        }
      }
    }
    // Handle inductors
    for (const auto &j : mObj.components.inductorIndices) {
      auto &temp = std::get<Inductor>(mObj.components.devices.at(j));
      double prevNode;
      if(temp.get_posIndex() && !temp.get_negIndex()) {
        prevNode = (LHS_PRE.at(temp.get_posIndex().value()));
      } else if(!temp.get_posIndex() && temp.get_negIndex()) {
        prevNode = (-LHS_PRE.at(temp.get_negIndex().value()));
      } else {
        prevNode = (LHS_PRE.at(temp.get_posIndex().value())
                - LHS_PRE.at(temp.get_negIndex().value()));
      }
      if(iObj.argAnal == AnalysisType::Voltage) {
        if(iObj.argInt == IntegrationType::Trapezoidal) {
          // -2L/h Ip - Vp
          RHS.at(temp.get_currentIndex()) = -temp.get_value() * LHS_PRE.at(temp.get_currentIndex()) - prevNode;
          // -2M/h Im
          for(const auto &m : temp.get_mutualInductance()) {
            RHS.at(temp.get_currentIndex()) -= (((2 * m.second) / iObj.transSim.get_prstep()) * LHS_PRE.at(std::get<Inductor>(mObj.components.devices.at(m.first)).get_currentIndex()));
          }
        } else {
          // -2L/h Ip + L/2h Ip2
          RHS.at(temp.get_currentIndex()) = -(2.0 * temp.get_inductance()/(iObj.transSim.get_prstep())) * LHS_PRE.at(temp.get_currentIndex()) + ((temp.get_inductance()/(2.0 * iObj.transSim.get_prstep()))*temp.get_in2());
          // -2M/h Im + M/2h Im2
          for(const auto &m : temp.get_mutualInductance()) {
            RHS.at(temp.get_currentIndex()) += (-((2 * m.second) / iObj.transSim.get_prstep()) * LHS_PRE.at(std::get<Inductor>(mObj.components.devices.at(m.first)).get_currentIndex()) 
                                                + (m.second / (2.0 * iObj.transSim.get_prstep())) * std::get<Inductor>(mObj.components.devices.at(m.first)).get_in2());
          }
          temp.set_in2(LHS_PRE.at(temp.get_currentIndex()));
        }
      }
    }
    // Handle capacitors
    for (const auto &j : mObj.components.capacitorIndices) {
      auto &temp = std::get<Capacitor>(mObj.components.devices.at(j));
      double prevNode;
      if(temp.get_posIndex() && !temp.get_negIndex()) {
        prevNode = (LHS_PRE.at(temp.get_posIndex().value()));
      } else if(!temp.get_posIndex() && temp.get_negIndex()) {
        prevNode = (-LHS_PRE.at(temp.get_negIndex().value()));
      } else {
        prevNode = (LHS_PRE.at(temp.get_posIndex().value())
                - LHS_PRE.at(temp.get_negIndex().value()));
      }
      if(iObj.argInt == IntegrationType::Trapezoidal) {
        if(iObj.argAnal == AnalysisType::Voltage) {
          // h/2C Ip + Vp
          RHS.at(temp.get_currentIndex()) = temp.get_value()
                                            * LHS_PRE.at(temp.get_currentIndex()) + prevNode;
        } else if (iObj.argAnal == AnalysisType::Phase) {
          double pn2 = temp.get_pn1();
          temp.set_pn1(prevNode);
          double dpn2 = temp.get_dpn1();
          temp.set_dpn1((2 / iObj.transSim.get_prstep()) * (temp.get_pn1() - pn2) - dpn2); 
          // h/2C Ip - φp - h Δφp
          RHS.at(temp.get_currentIndex()) = temp.get_value() * LHS_PRE.at(temp.get_currentIndex()) + prevNode + iObj.transSim.get_prstep() * temp.get_dpn1();
        }
      } else {
        if(iObj.argAnal == AnalysisType::Voltage) {
          // 4/3 Vp1 - 1/3 Vp2
          RHS.at(temp.get_currentIndex()) = (4.0/3.0) * prevNode - (1.0/3.0) * temp.get_pn1();
        } else if (iObj.argAnal == AnalysisType::Phase) {
          // 8/3 φp1 + 10/9 φp2 -1/9 φp3
          RHS.at(temp.get_currentIndex()) = (8.0/3.0) * prevNode + (10.0/9.0) * temp.get_pn1() - (1.0/9.0) * temp.get_pn2();
          temp.set_pn2(temp.get_pn1());
        }
        temp.set_pn1(prevNode);
      }
    }
    // Handle junctions
    for (const auto &j : mObj.components.junctionIndices) {
      auto &temp = std::get<JJ>(mObj.components.devices.at(j));
      const auto &model = temp.get_model();
      double prevNode;
      if(temp.get_posIndex() && !temp.get_negIndex()) {
        prevNode = (LHS_PRE.at(temp.get_posIndex().value()));
      } else if(!temp.get_posIndex() && temp.get_negIndex()) {
        prevNode = (-LHS_PRE.at(temp.get_negIndex().value()));
      } else {
        prevNode = (LHS_PRE.at(temp.get_posIndex().value())
                - LHS_PRE.at(temp.get_negIndex().value()));
      }
      if(i > 0) {
        if(iObj.argAnal == AnalysisType::Voltage) {
          temp.set_vn1(prevNode);
          temp.set_pn1(LHS_PRE.at(temp.get_variableIndex()));
        } else if (iObj.argAnal == AnalysisType::Phase) {
          temp.set_vn1(LHS_PRE.at(temp.get_variableIndex()));
          temp.set_pn1(prevNode);
        }
      }
      // Ensure timestep is not too large
      if ((double)i/(double)simSize > 0.01) {
        if (abs(temp.get_phi0() - temp.get_pn1()) > (0.25 * 2 * Constants::PI)) {
          Errors::simulation_errors(SimulationErrors::PHASEGUESS_TOO_LARGE, temp.get_label());
        }
      }
      if (i <= 3) {
        temp.set_dvn1(0.0);
      } else {
        temp.set_dvn1((2 / iObj.transSim.get_prstep()) * (temp.get_vn1() - temp.get_vn2()) - temp.get_dvn2());
      }
      double v0;
      if (iObj.argInt == IntegrationType::Trapezoidal) {
        // Guess voltage (V0)
        v0 = temp.get_vn1() + iObj.transSim.get_prstep() * temp.get_dvn1();
        // Phase guess (P0)
        temp.set_phi0(temp.get_pn1() + (1 / hbar_he) * (temp.get_vn1() + v0));
        // -(hbar / h * e) φp - Vp 
        if(iObj.argAnal == AnalysisType::Voltage) {
          RHS.at(temp.get_variableIndex()) = -hbar_he * temp.get_pn1() - temp.get_vn1();
        // φp + (h * e / hbar) Vp 
        } else if (iObj.argAnal == AnalysisType::Phase) {
          RHS.at(temp.get_variableIndex()) = temp.get_pn1() + (1 / hbar_he) * temp.get_vn1();
        }
        // Update junction transition
        if(model.get_resistanceType() == 1) {
          auto testLU = temp.update_value(v0);
          if(testLU && !needsLU) {
            needsLU = true;
          }
        }
        // (hR / h + 2RC) * (-Ic sin φ0 + 2C / h Vp + C ΔVp)
        RHS.at(temp.get_currentIndex()) = (-temp.get_nonZeros().back()) * (-(((Constants::PI * temp.get_del()) / (2 * Constants::EV * temp.get_rncalc())) *
                                          (sin(temp.get_phi0()) / sqrt(1 - model.get_transparency() * (sin(temp.get_phi0() / 2) * sin(temp.get_phi0() / 2)))) 
                                          * tanh((temp.get_del()) / (2 * Constants::BOLTZMANN * model.get_temperature()) *
                                            sqrt(1 - model.get_transparency() * (sin(temp.get_phi0() / 2) * sin(temp.get_phi0() / 2))))) +
                                          (((2 * model.get_capacitance()) / iObj.transSim.get_prstep()) * temp.get_vn1()) 
                                          + (model.get_capacitance() * temp.get_dvn1()) - temp.get_transitionCurrent());
        temp.set_dvn2(temp.get_dvn1());
      } else {
        // Guess voltage (V0)
        v0 = (5.0/2.0) * temp.get_vn1() - 2.0 * temp.get_vn2() + (1.0 / 2.0) * temp.get_vn3();
        // Phase guess (P0)
        temp.set_phi0((4.0/3.0) * temp.get_pn1() - (1.0/3.0) * temp.get_pn2() + ((1.0 / Constants::SIGMA) * ((2.0 * iObj.transSim.get_prstep()) / 3.0)) * v0);
        // (hbar / 2 * e) ( -(2 / h) φp1 + (1 / 2h) φp2 )
        if(iObj.argAnal == AnalysisType::Voltage) {
          RHS.at(temp.get_variableIndex()) = (Constants::SIGMA) * (-(2.0 / iObj.transSim.get_prstep()) * temp.get_pn1() + (1.0 / (2.0 * iObj.transSim.get_prstep())) * temp.get_pn2());
        // (4 / 3) φp1 - (1/3) φp2 
        } else if (iObj.argAnal == AnalysisType::Phase) {
          RHS.at(temp.get_variableIndex()) = (4.0 / 3.0) * temp.get_pn1() - (1.0 / 3.0) * temp.get_pn2();
        }
        temp.set_pn2(temp.get_pn1());
        temp.set_vn3(temp.get_vn2());
        // Update junction transition
        if(model.get_resistanceType() == 1) {
          auto testLU = temp.update_value(v0);
          if(testLU && !needsLU) {
            needsLU = true;
          }
        }
        // (hR / h + 2RC) * (-Ic sin φ0 + 2C / h Vp1 - C/2h Vp2)
        RHS.at(temp.get_currentIndex()) = (-temp.get_nonZeros().back()) * (-(((Constants::PI * temp.get_del()) / (2 * Constants::EV * temp.get_rncalc())) *
                                          (sin(temp.get_phi0()) / sqrt(1 - model.get_transparency() * (sin(temp.get_phi0() / 2) * sin(temp.get_phi0() / 2)))) 
                                          * tanh((temp.get_del()) / (2 * Constants::BOLTZMANN * model.get_temperature()) *
                                            sqrt(1 - model.get_transparency() * (sin(temp.get_phi0() / 2) * sin(temp.get_phi0() / 2))))) +
                                          (((2 * model.get_capacitance()) / iObj.transSim.get_prstep()) * temp.get_vn1()) 
                                          - ((model.get_capacitance() / (2.0 * iObj.transSim.get_prstep())) * temp.get_vn2()) - temp.get_transitionCurrent());
      }
      temp.set_vn2(temp.get_vn1());
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
        RHS.at(temp.get_currentIndex()) = mObj.sources.at(temp.get_sourceIndex()).at(i);
      } else if (iObj.argAnal == AnalysisType::Phase) {
        double prevNode;
        if(temp.get_posIndex() && !temp.get_negIndex()) {
          prevNode = (LHS_PRE.at(temp.get_posIndex().value()));
        } else if(!temp.get_posIndex() && temp.get_negIndex()) {
          prevNode = (-LHS_PRE.at(temp.get_negIndex().value()));
        } else {
          prevNode = (LHS_PRE.at(temp.get_posIndex().value())
                  - LHS_PRE.at(temp.get_negIndex().value()));
        }
        if(i < 1) {
          RHS.at(temp.get_currentIndex()) = (iObj.transSim.get_prstep() / (2 * Constants::SIGMA)) * (mObj.sources.at(temp.get_sourceIndex()).at(i)) + prevNode;
        } else {
          RHS.at(temp.get_currentIndex()) = (iObj.transSim.get_prstep() / (2 * Constants::SIGMA)) * (mObj.sources.at(temp.get_sourceIndex()).at(i) - mObj.sources.at(temp.get_sourceIndex()).at(i-1)) + prevNode;
        }
      }
    }
    // Handle phase sources
    for (const auto &j : mObj.components.psIndices) {
      const auto &temp = std::get<PhaseSource>(mObj.components.devices.at(j));
      if (iObj.argAnal == AnalysisType::Phase) {
        RHS.at(temp.get_currentIndex()) = mObj.sources.at(temp.get_sourceIndex()).at(i);
      } else if(iObj.argAnal == AnalysisType::Voltage) {
        double prevNode;
        if(temp.get_posIndex() && !temp.get_negIndex()) {
          prevNode = (LHS_PRE.at(temp.get_posIndex().value()));
        } else if(!temp.get_posIndex() && temp.get_negIndex()) {
          prevNode = (-LHS_PRE.at(temp.get_negIndex().value()));
        } else {
          prevNode = (LHS_PRE.at(temp.get_posIndex().value())
                  - LHS_PRE.at(temp.get_negIndex().value()));
        }
        if(i == 0) {
          RHS.at(temp.get_currentIndex()) = ((Constants::SIGMA * 2) / iObj.transSim.get_prstep()) * (mObj.sources.at(temp.get_sourceIndex()).at(i)) - prevNode;
        } else {
          RHS.at(temp.get_currentIndex()) = ((Constants::SIGMA * 2) / iObj.transSim.get_prstep()) * (mObj.sources.at(temp.get_sourceIndex()).at(i) - mObj.sources.at(temp.get_sourceIndex()).at(i-1)) - prevNode;
        }
      }
    }
    // Handle ccvs
    for (const auto &j : mObj.components.ccvsIndices) {
      const auto &temp = std::get<CCVS>(mObj.components.devices.at(j));
      if (iObj.argAnal == AnalysisType::Phase) {
        double prevNode;
        if(temp.get_posIndex() && !temp.get_negIndex()) {
          prevNode = (LHS_PRE.at(temp.get_posIndex().value()));
        } else if(!temp.get_posIndex() && temp.get_negIndex()) {
          prevNode = (-LHS_PRE.at(temp.get_negIndex().value()));
        } else {
          prevNode = (LHS_PRE.at(temp.get_posIndex().value())
                  - LHS_PRE.at(temp.get_negIndex().value()));
        }
        RHS.at(temp.get_currentIndex2()) = temp.get_value() * LHS_PRE.at(temp.get_currentIndex2()) + prevNode;
      }
    }
     // Handle vccs
    for (const auto &j : mObj.components.vccsIndices) {
      const auto &temp = std::get<VCCS>(mObj.components.devices.at(j));
      if (iObj.argAnal == AnalysisType::Phase) {
        double prevNode;
        if(temp.get_posIndex2() && !temp.get_negIndex2()) {
          prevNode = (LHS_PRE.at(temp.get_posIndex2().value()));
        } else if(!temp.get_posIndex2() && temp.get_negIndex2()) {
          prevNode = (-LHS_PRE.at(temp.get_negIndex2().value()));
        } else {
          prevNode = (LHS_PRE.at(temp.get_posIndex2().value())
                  - LHS_PRE.at(temp.get_negIndex2().value()));
        }
        RHS.at(temp.get_currentIndex()) = temp.get_value() * LHS_PRE.at(temp.get_currentIndex()) + prevNode;
      }
    }
    // Handle transmission lines
    for (const auto &j : mObj.components.txIndices) {
      auto &temp = std::get<TransmissionLine>(mObj.components.devices.at(j));
      if(iObj.argAnal == AnalysisType::Voltage) {
        if(i >= temp.get_timestepDelay()) {
          double prevNodek, prevNode2k;
          // φ1n-k
          if(temp.get_posIndex() && !temp.get_negIndex()) {
            prevNodek = results.xVector.at(temp.get_posIndex().value()).value().at(i - temp.get_timestepDelay());
          } else if(!temp.get_posIndex() && temp.get_negIndex()) {
            prevNodek = -results.xVector.at(temp.get_negIndex().value()).value().at(i - temp.get_timestepDelay());
          } else {
            prevNodek = results.xVector.at(temp.get_posIndex().value()).value().at(i - temp.get_timestepDelay())
                        - results.xVector.at(temp.get_negIndex().value()).value().at(i - temp.get_timestepDelay());
          }
          // φ2n-k
          if(temp.get_posIndex2() && !temp.get_negIndex2()) {
            prevNode2k = results.xVector.at(temp.get_posIndex2().value()).value().at(i - temp.get_timestepDelay());
          } else if(!temp.get_posIndex2() && temp.get_negIndex2()) {
            prevNode2k = -results.xVector.at(temp.get_negIndex2().value()).value().at(i - temp.get_timestepDelay());
          } else {
            prevNode2k = results.xVector.at(temp.get_posIndex2().value()).value().at(i - temp.get_timestepDelay())
                          - results.xVector.at(temp.get_negIndex2().value()).value().at(i - temp.get_timestepDelay());
          }
          // IT1 = V2(n-k) + Z0 I2(n-k)
          RHS.at(temp.get_currentIndex()) = prevNode2k + temp.get_value() * results.xVector.at(
                                              temp.get_currentIndex2()).value().at(i - temp.get_timestepDelay());
          // IT2 = V1(n-k) + Z0 I1(n-k)
          RHS.at(temp.get_currentIndex2()) = prevNodek + temp.get_value() * results.xVector.at(
                                              temp.get_currentIndex()).value().at(i - temp.get_timestepDelay());
        }
      } else if(iObj.argAnal == AnalysisType::Phase) {
        if (i > 0) {
          // φ1n-1, φ2n-1, φ1n-k-1, φ2n-k-1
          double prevNodeN, prevNode2N;
          // φ1n-1
          if(temp.get_posIndex() && !temp.get_negIndex()) {
            prevNodeN = results.xVector.at(temp.get_posIndex().value()).value().at(i - 1);
          } else if(!temp.get_posIndex() && temp.get_negIndex()) {
            prevNodeN = -results.xVector.at(temp.get_negIndex().value()).value().at(i - 1);
          } else {
            prevNodeN = results.xVector.at(temp.get_posIndex().value()).value().at(i - 1)
                    - results.xVector.at(temp.get_negIndex().value()).value().at(i - 1);
          }
          // φ2n-1
          if(temp.get_posIndex2() && !temp.get_negIndex2()) {
            prevNode2N = results.xVector.at(temp.get_posIndex2().value()).value().at(i - 1);
          } else if(!temp.get_posIndex2() && temp.get_negIndex2()) {
            prevNode2N = -results.xVector.at(temp.get_negIndex2().value()).value().at(i - 1);
          } else {
            prevNode2N = results.xVector.at(temp.get_posIndex2().value()).value().at(i - 1)
                    - results.xVector.at(temp.get_negIndex2().value()).value().at(i - 1);
          }
          if(i >= temp.get_timestepDelay()) {
            double prevNodek, prevNode2k, prevNodek1, prevNode2k1;
            prevNodek1 = 0.0;
            prevNode2k1 = 0.0;
            // φ1n-k
            if(temp.get_posIndex() && !temp.get_negIndex()) {
              prevNodek = results.xVector.at(temp.get_posIndex().value()).value().at(i - temp.get_timestepDelay());
              // φ1n-k-1
              if(i > temp.get_timestepDelay() + 1) {
                prevNodek1 = results.xVector.at(temp.get_posIndex().value()).value().at(i - temp.get_timestepDelay() - 1);
              }
            } else if(!temp.get_posIndex() && temp.get_negIndex()) {
              prevNodek = -results.xVector.at(temp.get_negIndex().value()).value().at(i - temp.get_timestepDelay());
              // φ1n-k-1           
              if(i >= temp.get_timestepDelay() + 1) { 
                prevNodek1 = -results.xVector.at(temp.get_negIndex().value()).value().at(i - temp.get_timestepDelay() - 1);
              } 
            } else {
              prevNodek = results.xVector.at(temp.get_posIndex().value()).value().at(i - temp.get_timestepDelay())
                          - results.xVector.at(temp.get_negIndex().value()).value().at(i - temp.get_timestepDelay());
              // φ1n-k-1
              if(i >= temp.get_timestepDelay() + 1) {
                prevNodek1 = results.xVector.at(temp.get_posIndex().value()).value().at(i - temp.get_timestepDelay() - 1)
                      - results.xVector.at(temp.get_negIndex().value()).value().at(i - temp.get_timestepDelay() - 1);
              }
            }
            // φ2n-k
            if(temp.get_posIndex2() && !temp.get_negIndex2()) {
              prevNode2k = results.xVector.at(temp.get_posIndex2().value()).value().at(i - temp.get_timestepDelay());
              // φ2n-k-1
              if(i >= temp.get_timestepDelay() + 1) {
                prevNode2k1 = results.xVector.at(temp.get_posIndex2().value()).value().at(i - temp.get_timestepDelay() - 1);
              }
            } else if(!temp.get_posIndex2() && temp.get_negIndex2()) {
              prevNode2k = -results.xVector.at(temp.get_negIndex2().value()).value().at(i - temp.get_timestepDelay());
              // φ2n-k-1
              if(i >= temp.get_timestepDelay() + 1) {
                prevNode2k1 = -results.xVector.at(temp.get_negIndex2().value()).value().at(i - temp.get_timestepDelay() - 1);
              }
            } else {
              prevNode2k = results.xVector.at(temp.get_posIndex2().value()).value().at(i - temp.get_timestepDelay())
                            - results.xVector.at(temp.get_negIndex2().value()).value().at(i - temp.get_timestepDelay());
              // φ2n-k-1
              if(i >= temp.get_timestepDelay() + 1) {
                prevNode2k1 = results.xVector.at(temp.get_posIndex2().value()).value().at(i - temp.get_timestepDelay() - 1)
                      - results.xVector.at(temp.get_negIndex2().value()).value().at(i - temp.get_timestepDelay() - 1);
              }
            }
            if(iObj.argInt == IntegrationType::Trapezoidal) {
              if(i == temp.get_timestepDelay()) {
                // IT1 = (hZ0/2σ) * (IT1n-1 + IT2n-k) + φ1n-1 + φ2n-k
                RHS.at(temp.get_currentIndex()) = temp.get_value() * (results.xVector.at(temp.get_currentIndex()).value().at(i - 1)
                                                  + results.xVector.at(temp.get_currentIndex2()).value().at(i - temp.get_timestepDelay()))
                                                  + prevNodeN + prevNode2k;
                // IT2 = (hZ0/2σ) * (IT2n-1 + IT1n-k + IT1n-k-1) + φ2n-1 + φ1n-k + φ1n-k-1                                   
                RHS.at(temp.get_currentIndex2()) = temp.get_value() * (results.xVector.at(temp.get_currentIndex2()).value().at(i - 1)
                                                  + results.xVector.at(temp.get_currentIndex()).value().at(i - temp.get_timestepDelay()))
                                                  + prevNode2N + prevNodek;
              } else {
                // IT1 = (hZ0/2σ) * (IT1n-1 + IT2n-k + IT2n-k-1) + φ1n-1 + φ2n-k - φ2n-k-1
                RHS.at(temp.get_currentIndex()) = temp.get_value() * (results.xVector.at(temp.get_currentIndex()).value().at(i - 1)
                                                  + results.xVector.at(temp.get_currentIndex2()).value().at(i - temp.get_timestepDelay())
                                                  + results.xVector.at(temp.get_currentIndex2()).value().at(i - temp.get_timestepDelay() - 1))
                                                  + prevNodeN + prevNode2k - prevNode2k1;
                // IT2 = (hZ0/2σ) * (IT2n-1 + IT1n-k + IT1n-k-1) + φ2n-1 + φ1n-k - φ1n-k-1                                   
                RHS.at(temp.get_currentIndex2()) = temp.get_value() * (results.xVector.at(temp.get_currentIndex2()).value().at(i - 1)
                                                  + results.xVector.at(temp.get_currentIndex()).value().at(i - temp.get_timestepDelay())
                                                  + results.xVector.at(temp.get_currentIndex()).value().at(i - temp.get_timestepDelay() - 1))
                                                  + prevNode2N + prevNodek - prevNodek1;
              }
            } else {
              if(i == temp.get_timestepDelay()) {
                // IT1 = (hZ0/2σ) * IT2n-k - (4/3) φ1n-1 - (1/3) φ1n-2 - φ2n-k
                RHS.at(temp.get_currentIndex()) = temp.get_value() * results.xVector.at(temp.get_currentIndex2()).value().at(i - temp.get_timestepDelay())
                                                  -(4.0/3.0) * prevNodeN - (1.0/3.0) * temp.get_p1n2() - prevNode2k;
                // IT2 = (hZ0/2σ) * IT1n-k - (4/3) φ2n-1 - (1/3) φ2n-2 - φ1n-k                          
                RHS.at(temp.get_currentIndex2()) = temp.get_value() * results.xVector.at(temp.get_currentIndex()).value().at(i - temp.get_timestepDelay())
                                                  -(4.0/3.0) * prevNode2N - (1.0/3.0) * temp.get_p2n2() - prevNodek;
              } else {
                // IT1 = (hZ0/2σ) * IT2n-k - (4/3) φ1n-1 - (1/3) φ1n-2 - φ2n-k - (4/3) φ2n-k-1 + (1/3) φ2n-k-2
                RHS.at(temp.get_currentIndex()) = temp.get_value() * results.xVector.at(temp.get_currentIndex2()).value().at(i - temp.get_timestepDelay())
                                                  -(4.0/3.0) * prevNodeN - (1.0/3.0) * temp.get_p1n2() - prevNode2k
                                                  -(4.0/3.0) * prevNode2k1 + (1.0/3.0) * temp.get_p2nk2();
                // IT2 = (hZ0/2σ) * IT1n-k - (4/3) φ2n-1 - (1/3) φ2n-2 - φ1n-k - (4/3) φ1n-k-1 + (1/3) φ1n-k-2                              
                RHS.at(temp.get_currentIndex2()) = temp.get_value() * results.xVector.at(temp.get_currentIndex()).value().at(i - temp.get_timestepDelay())
                                                  -(4.0/3.0) * prevNode2N - (1.0/3.0) * temp.get_p2n2() - prevNodek
                                                  -(4.0/3.0) * prevNodek1 + (1.0/3.0) * temp.get_p1nk2();
              }
              temp.set_p1nk2(prevNodek1);
              temp.set_p2nk2(prevNode2k1);
            }
          } else {
            if(iObj.argInt == IntegrationType::Trapezoidal) {
              // IT1 = (hZ0/2σ) * (IT1n-1) + φ1n-1
              RHS.at(temp.get_currentIndex()) = temp.get_value() * (results.xVector.at(temp.get_currentIndex()).value().at(i - 1))
                                                  + prevNodeN;
              // IT2 = (hZ0/2σ) * (IT2n-1) + φ2n-1                         
              RHS.at(temp.get_currentIndex2()) = temp.get_value() * (results.xVector.at(temp.get_currentIndex2()).value().at(i - 1))
                                                + prevNode2N;
            } else {
              // IT1 = -(4/3) φ1n-1 - (1/3) φ1n-2
              RHS.at(temp.get_currentIndex()) = -(4.0/3.0) * prevNodeN - (1.0/3.0) * temp.get_p1n2();
              // IT2 = -(4/3) φ2n-1 - (1/3) φ2n-2                        
              RHS.at(temp.get_currentIndex2()) = -(4.0/3.0) * prevNode2N - (1.0/3.0) * temp.get_p2n2();
            }
          }
          temp.set_p1n2(prevNodeN);
          temp.set_p2n2(prevNode2N);
        }
      }
    }

    LHS_PRE = RHS;

    ok =
        klu_tsolve(Symbolic, Numeric, mObj.rp.size() - 1, 1, &LHS_PRE.front(), &Common);
    if (!ok) {
      Errors::simulation_errors(SimulationErrors::MATRIX_SINGULAR, "");
    }

    lhsValues = LHS_PRE;
    if(!saveAll) {
      for (int m = 0; m < mObj.relevantIndices.size(); ++m) {
        results.xVector.at(mObj.relevantIndices.at(m)).value().emplace_back(lhsValues.at(mObj.relevantIndices.at(m)));
      }
    } else {
      for (int m = 0; m < mObj.rp.size() - 1; ++m) {
        results.xVector.at(m).value().emplace_back(lhsValues.at(m));
      }
    }

    results.timeAxis.emplace_back(i * iObj.transSim.get_prstep());
  }
  if(!iObj.argMin) {
    std::cout << "100%" << std::endl;
    std::cout << "\n";
  }
  
  klu_free_symbolic(&Symbolic, &Common);
  klu_free_numeric(&Numeric, &Common);
}
