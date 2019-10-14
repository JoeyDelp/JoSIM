// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#include "JoSIM/j_simulation.h"

// Linear algebra include
#include "JoSIM/AnalysisType.hpp"
#include "JoSIM/j_components.h"
#include "JoSIM/j_matrix.h"
#include "JoSIM/Constants.hpp"
#include "klu.h"

#include <cassert>
#include <cmath>

void Simulation::identify_simulation(const std::vector<std::string> &controls,
                                     double &prstep, double &tstop,
                                     double &tstart, double &maxtstep) {
  std::vector<std::string> simtokens;
  bool transFound = false;
  for (const auto &i : controls) {
    if (i.find("TRAN") != std::string::npos) {
      transFound = true;
      simtokens = Misc::tokenize_delimeter(i, " ,");
      if (simtokens.at(0).find("TRAN") != std::string::npos) {
        if (simtokens.size() < 2) {
          Errors::control_errors(TRANS_ERROR, "Too few parameters: " + i);
          maxtstep = 1E-12;
          tstop = 1E-9;
          tstart = 0;
        } else {
          prstep = Misc::modifier(simtokens[1]);
          if (simtokens.size() > 2) {
            tstop = Misc::modifier(simtokens[2]);
            if (simtokens.size() > 3) {
              tstart = Misc::modifier(simtokens[3]);
              if (simtokens.size() > 4) {
                maxtstep = Misc::modifier(simtokens[4]);
              } else
                maxtstep = 1E-12;
            } else {
              tstart = 0;
              maxtstep = 1E-12;
            }
          } else {
            tstop = 1E-9;
            tstart = 0;
            maxtstep = 1E-12;
          }
        }
      }
    }
  }
  if (!transFound) {
    Errors::control_errors(NO_SIM, "");
  }
}

template<JoSIM::AnalysisType AnalysisTypeValue>
void Simulation::trans_sim(Input &iObj, Matrix &mObj) {
  std::vector<double> lhsValues(mObj.Nsize, 0.0),
      RHS(mObj.Nsize, 0.0), LHS_PRE(mObj.Nsize, 0.0);
  int simSize = iObj.transSim.simsize();
  int saveAll = false;
  if(mObj.relevantToStore.size() == 0) saveAll = true;
  if (!saveAll) {
    for (int m = 0; m < mObj.relXInd.size(); m++)
      results.xVect.emplace_back(std::vector<double>(simSize, 0.0));
  } else {
    for (int m = 0; m < mObj.rowDesc.size(); m++)
      results.xVect.emplace_back(std::vector<double>(simSize, 0.0));
  }
  double hn_2_2e_hbar = (iObj.transSim.prstep / 2) * (2 * JoSIM::Constants::PI / JoSIM::Constants::PHI_ZERO);
  double RHSvalue = 0.0;
  int ok, jjcount, rowCounter = 0;
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
  Symbolic = klu_analyze(mObj.Nsize, &mObj.rowptr.front(), &mObj.colind.front(),
                         &Common);
  Numeric = klu_factor(&mObj.rowptr.front(), &mObj.colind.front(),
                       &mObj.nzval.front(), Symbolic, &Common);

  auto& jj_vector = [&]() -> auto& {
    if constexpr(AnalysisTypeValue == JoSIM::AnalysisType::Phase)
      return mObj.components.phaseJJ;
    else
      return mObj.components.voltJJ;
  }();
  std::cout << "Simulation Progress:" << std::endl;
  std::cout << "0%\r" << std::flush;
  for(int i = 0; i < simSize; i++) {
    rowCounter = 0;
    if(i == fqtr) std::cout << "25%\r" << std::flush;
    if(i == sqtr) std::cout << "50%\r" << std::flush;
    if(i == tqtr) std::cout << "75%\r" << std::flush;
    for (auto j : mObj.rowDesc) {
      RHSvalue = 0.0;
      switch (j.type){
        case RowDescriptor::Type::VoltageNode: 
        case RowDescriptor::Type::PhaseNode:
          for (auto k : mObj.nodeConnections.at(j.index).connections) {
            switch(k.type) {
              case ComponentConnections::Type::JJP: 
                  RHSvalue += jj_vector.at(k.index).iS;
                break;
              case ComponentConnections::Type::JJN: 
                  RHSvalue -= jj_vector.at(k.index).iS;
                break;
              case ComponentConnections::Type::CSP:
                RHSvalue -= mObj.sources.at(k.index).at(i);
                break;
              case ComponentConnections::Type::CSN:
                RHSvalue += mObj.sources.at(k.index).at(i);
                break;
              default:
                break;
            }
          }
          RHS.at(rowCounter) = RHSvalue;
          break;
        case RowDescriptor::Type::VoltageInductor:
          if (mObj.components.voltInd.at(j.index).posNRow == -1)
            RHSvalue = -lhsValues.at(mObj.components.voltInd.at(j.index).negNRow);
          else if (mObj.components.voltInd.at(j.index).negNRow == -1)
            RHSvalue = lhsValues.at(mObj.components.voltInd.at(j.index).posNRow);
          else
            RHSvalue = (lhsValues.at(mObj.components.voltInd.at(j.index).posNRow) 
              - lhsValues.at(mObj.components.voltInd.at(j.index).negNRow));
          RHSvalue = ((-2 * mObj.components.voltInd.at(j.index).value /
              iObj.transSim.prstep) * lhsValues.at(mObj.components.voltInd.at(j.index).curNRow)) - RHSvalue;
          for (const auto &m : mObj.components.voltInd.at(j.index).mut) {
            RHSvalue -= (m.second * lhsValues.at(mObj.components.voltInd.at(m.first).curNRow));
          }
          RHS.at(rowCounter) = RHSvalue;
          break;
        case RowDescriptor::Type::VoltageJJ:
          RHS.at(rowCounter) = mObj.components.voltJJ.at(j.index).pn1 +
                   hn_2_2e_hbar * mObj.components.voltJJ.at(j.index).vn1;
          break;
        case RowDescriptor::Type::VoltageVS:
        case RowDescriptor::Type::PhasePS:
          RHS.at(rowCounter) = mObj.sources.at(j.index).at(i);
          break;
        case RowDescriptor::Type::VoltageTX1: {
          const auto &txline = mObj.components.txLine.at(j.index);
          if (i >= txline.k) {
            if (txline.posN2Row == -1)
                RHS.at(rowCounter) = -results.xVect.at(
                  std::distance(
                    mObj.relXInd.begin(),
                    std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                      mObj.relToXMap.at(txline.negNode2R)
                    )
                  )
                ).at(i - txline.k);
            else if (txline.negN2Row == -1)
              RHS.at(rowCounter) = results.xVect.at(
                  std::distance(
                    mObj.relXInd.begin(),
                    std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                      mObj.relToXMap.at(txline.posNode2R)
                    )
                  )
                ).at(i - txline.k);
            else
              RHS.at(rowCounter) = results.xVect.at(
                  std::distance(
                    mObj.relXInd.begin(),
                    std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                      mObj.relToXMap.at(txline.posNode2R)
                    )
                  )
                ).at(i - txline.k)
                - results.xVect.at(
                  std::distance(
                    mObj.relXInd.begin(),
                    std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                      mObj.relToXMap.at(txline.negNode2R)
                    )
                  )
                ).at(i - txline.k);
            RHS.at(rowCounter) +=
                txline.value *
                    results.xVect.at(
                  std::distance(
                    mObj.relXInd.begin(),
                    std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                      mObj.relToXMap.at(txline.curNode2R)
                    )
                  )
                ).at(i - txline.k);
          }
          break; }
        case RowDescriptor::Type::VoltageTX2: {
          const auto &txline = mObj.components.txLine.at(j.index);
          if (i >= txline.k) {
            if (txline.posNRow == -1)
              RHS.at(rowCounter) = -results.xVect.at(
                  std::distance(
                    mObj.relXInd.begin(),
                    std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                      mObj.relToXMap.at(txline.negNodeR)
                    )
                  )
                ).at(i - txline.k);
            else if (txline.negNRow == -1)
              RHS.at(rowCounter) = results.xVect.at(
                  std::distance(
                    mObj.relXInd.begin(),
                    std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                      mObj.relToXMap.at(txline.posNodeR)
                    )
                  )
                ).at(i - txline.k);
            else
              RHS.at(rowCounter) = results.xVect.at(
                  std::distance(
                    mObj.relXInd.begin(),
                    std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                      mObj.relToXMap.at(txline.posNodeR)
                    )
                  )
                ).at(i - txline.k)
                - results.xVect.at(
                  std::distance(
                    mObj.relXInd.begin(),
                    std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                      mObj.relToXMap.at(txline.negNodeR)
                    )
                  )
                ).at(i - txline.k);
            RHS.at(rowCounter) +=
                txline.value *
                    results.xVect.at(
                  std::distance(
                    mObj.relXInd.begin(),
                    std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                      mObj.relToXMap.at(txline.curNode1R)
                    )
                  )
                ).at(i - txline.k);
          }
          break; }
        case RowDescriptor::Type::PhaseResistor: {
          auto &presis = mObj.components.phaseRes.at(j.index);
          if(presis.posNRow == -1) presis.pn1 = -lhsValues.at(presis.negNRow);
          else if (presis.negNRow == -1) presis.pn1 = lhsValues.at(presis.posNRow);
          else presis.pn1 = lhsValues.at(presis.posNRow) - lhsValues.at(presis.negNRow);
          presis.IRn1 = lhsValues.at(presis.curNRow);
          RHS.at(rowCounter) = ((JoSIM::Constants::PI * presis.value * iObj.transSim.prstep) / JoSIM::Constants::PHI_ZERO) * presis.IRn1 + presis.pn1;
          break; }
        case RowDescriptor::Type::PhaseJJ:
          RHS.at(rowCounter) = mObj.components.phaseJJ.at(j.index).pn1 + hn_2_2e_hbar * mObj.components.phaseJJ.at(j.index).vn1;
          break;
        case RowDescriptor::Type::PhaseCapacitor:
          RHS.at(rowCounter) = 
            -((2 * JoSIM::Constants::PI * iObj.transSim.prstep * iObj.transSim.prstep) /
              (4 * JoSIM::Constants::PHI_ZERO * mObj.components.phaseCap.at(j.index).value)) *
                mObj.components.phaseCap.at(j.index).ICn1 -
            mObj.components.phaseCap.at(j.index).pn1 -
            (iObj.transSim.prstep * mObj.components.phaseCap.at(j.index).dPn1);
          break;
        case RowDescriptor::Type::PhaseVS: {
          auto &pvs = mObj.components.phaseVs.at(j.index);
          if (pvs.posNRow == -1.0) pvs.pn1 = -lhsValues.at(pvs.negNRow);
          else if (pvs.negNRow == -1.0) pvs.pn1 = lhsValues.at(pvs.posNRow);
          else pvs.pn1 = lhsValues.at(pvs.posNRow) - lhsValues.at(pvs.negNRow);
          if (i >= 1) 
            RHS.at(rowCounter) = pvs.pn1 + ((iObj.transSim.prstep * JoSIM::Constants::PI) / JoSIM::Constants::PHI_ZERO) *
              (mObj.sources.at(pvs.sourceDex).at(i) + mObj.sources.at(pvs.sourceDex).at(i - 1));
          else if (i == 0)
            RHS.at(rowCounter) = pvs.pn1 + ((iObj.transSim.prstep * JoSIM::Constants::PI) / JoSIM::Constants::PHI_ZERO) *
              mObj.sources.at(pvs.sourceDex).at(i);
          break; }
        case RowDescriptor::Type::PhaseTX1: {
          const auto &txline = mObj.components.txPhase.at(j.index);
          if (i > txline.k)
            RHS.at(rowCounter) = ((iObj.transSim.prstep * JoSIM::Constants::PI * txline.value) / JoSIM::Constants::PHI_ZERO) *
              results.xVect.at(mObj.relToXMap.at(txline.curNode2R)).at(i - txline.k) +
              txline.p1n1 + (iObj.transSim.prstep / 2) * (txline.dP1n1 + txline.dP2nk);
          break; }
        case RowDescriptor::Type::PhaseTX2: {
          const auto &txline = mObj.components.txPhase.at(j.index);
          if (i > txline.k)
            RHS.at(rowCounter) = ((iObj.transSim.prstep * JoSIM::Constants::PI * txline.value) / JoSIM::Constants::PHI_ZERO) *
              results.xVect.at(mObj.relToXMap.at(txline.curNode1R)).at(i - txline.k) +
              txline.p2n1 + (iObj.transSim.prstep / 2) * (txline.dP2n1 + txline.dP1nk);
          break; }
        default:
          break;
      }
      rowCounter++;
    }

    LHS_PRE = RHS;

    ok =
        klu_tsolve(Symbolic, Numeric, mObj.Nsize, 1, &LHS_PRE.front(), &Common);
    if (!ok)
      Errors::simulation_errors(MATRIX_SINGULAR, "");

    lhsValues = LHS_PRE;
    if(!saveAll) {
      for (int m = 0; m < mObj.relXInd.size(); m++)
        results.xVect.at(m).at(i) = lhsValues.at(mObj.relXInd.at(m));
    } else {
      for (int m = 0; m < mObj.rowDesc.size(); m++)
        results.xVect.at(m).at(i) = lhsValues.at(m);
    }
    
    for (int j = 0; j < jj_vector.size(); j++) {
      auto &jj = jj_vector.at(j);
      // V_n-1 or P_n-1
      if(jj.posNRow == -1) {
        if(AnalysisTypeValue == JoSIM::AnalysisType::Voltage) jj.vn1 = -lhsValues.at(jj.negNRow);
        else jj.pn1 = -lhsValues.at(jj.negNRow);
      } else if (jj.negNRow == -1) {
        if(AnalysisTypeValue == JoSIM::AnalysisType::Voltage) jj.vn1 = lhsValues.at(jj.posNRow);
        else jj.pn1 = lhsValues.at(jj.posNRow);
      } else {
        if(AnalysisTypeValue == JoSIM::AnalysisType::Voltage) jj.vn1 = lhsValues.at(jj.posNRow) - lhsValues.at(jj.negNRow);
        else jj.pn1 = lhsValues.at(jj.posNRow) - lhsValues.at(jj.negNRow);
      }
      // Phase_n-1 or V_n-1
      if(AnalysisTypeValue == JoSIM::AnalysisType::Voltage) jj.pn1 = lhsValues.at(jj.phaseNRow);
      else jj.vn1 = lhsValues.at(jj.voltNRow);
      // Prevent initial large derivitive when V_n-1 = 0
      // Otherwise: trapezoidal find dV_n-1
      if (i <= 3) jj.dVn1 = 0;
      else jj.dVn1 = (2 / iObj.transSim.prstep) * (jj.vn1 - jj.vn2) - jj.dVn2;
      // Guess voltage (V0)
      jj.v0 = jj.vn1 + iObj.transSim.prstep * jj.dVn1;
      // Handle Rtype=1
      if (jj.rType == 1) {
        if (fabs(jj.v0) < jj.lowerB) {
          jj.iT = 0.0;
          if (jj.ppPtr != -1) {
            if (mObj.mElements.at(jj.ppPtr).value != jj.subCond) {
              mObj.mElements.at(jj.ppPtr).value = jj.subCond;
              needsLU = true;
            }
          }
          if (jj.nnPtr != -1) {
            if (mObj.mElements.at(jj.nnPtr).value != jj.subCond) {
              mObj.mElements.at(jj.nnPtr).value = jj.subCond;
              needsLU = true;
            }
          }
          if (jj.pnPtr != -1) {
            if (mObj.mElements.at(jj.pnPtr).value != -jj.subCond) {
              mObj.mElements.at(jj.pnPtr).value = -jj.subCond;
              needsLU = true;
            }
          }
          if (jj.npPtr != -1) {
            if (mObj.mElements.at(jj.npPtr).value != -jj.subCond) {
              mObj.mElements.at(jj.npPtr).value = -jj.subCond;
              needsLU = true;
            }
          }
          if (jj.pPtr != -1) {
            if (mObj.mElements.at(jj.pPtr).value != jj.subCond) {
              mObj.mElements.at(jj.pPtr).value = jj.subCond;
              needsLU = true;
            }
          }
          if (jj.nPtr != -1) {
            if (mObj.mElements.at(jj.nPtr).value != -jj.subCond) {
              mObj.mElements.at(jj.nPtr).value = -jj.subCond;
              needsLU = true;
            }
          }
        } else if (fabs(jj.v0) < jj.upperB) {
          if (jj.v0 < 0) jj.iT = -jj.lowerB * ((1 / jj.r0) - jj.gLarge);
          else jj.iT = jj.lowerB * ((1 / jj.r0) - jj.gLarge);
          if (jj.ppPtr != -1) {
            if (mObj.mElements.at(jj.ppPtr).value != jj.transCond) {
              mObj.mElements.at(jj.ppPtr).value = jj.transCond;
              needsLU = true;
            }
          }
          if (jj.nnPtr != -1) {
            if (mObj.mElements.at(jj.nnPtr).value != jj.transCond) {
              mObj.mElements.at(jj.nnPtr).value = jj.transCond;
              needsLU = true;
            }
          }
          if (jj.pnPtr != -1) {
            if (mObj.mElements.at(jj.pnPtr).value != jj.transCond) {
              mObj.mElements.at(jj.pnPtr).value = -jj.transCond;
              needsLU = true;
            }
          }
          if (jj.npPtr != -1) {
            if (mObj.mElements.at(jj.npPtr).value != jj.transCond) {
              mObj.mElements.at(jj.npPtr).value = -jj.transCond;
              needsLU = true;
            }
          }
          if (jj.pPtr != -1) {
            if (mObj.mElements.at(jj.pPtr).value != jj.transCond) {
              mObj.mElements.at(jj.pPtr).value = jj.transCond;
              needsLU = true;
            }
          }
          if (jj.nPtr != -1) {
            if (mObj.mElements.at(jj.pnPtr).value != -jj.transCond) {
              mObj.mElements.at(jj.pnPtr).value = -jj.transCond;
              needsLU = true;
            }
          }
        } else {
          if (jj.v0 < 0) 
            jj.iT = -(jj.iC / jj.iCFact + jj.vG * (1 / jj.r0) - jj.lowerB * (1 / jj.rN));
          else 
            jj.iT = (jj.iC / jj.iCFact + jj.vG * (1 / jj.r0) - jj.lowerB * (1 / jj.rN));
          if (jj.ppPtr != -1) {
            if (mObj.mElements.at(jj.ppPtr).value != jj.normalCond) {
              mObj.mElements.at(jj.ppPtr).value = jj.normalCond;
              needsLU = true;
            }
          }
          if (jj.nnPtr != -1) {
            if (mObj.mElements.at(jj.nnPtr).value != jj.normalCond) {
              mObj.mElements.at(jj.nnPtr).value = jj.normalCond;
              needsLU = true;
            }
          }
          if (jj.pnPtr != -1) {
            if (mObj.mElements.at(jj.pnPtr).value != jj.normalCond) {
              mObj.mElements.at(jj.pnPtr).value = -jj.normalCond;
              needsLU = true;
            }
          }
          if (jj.npPtr != -1) {
            if (mObj.mElements.at(jj.npPtr).value != jj.normalCond) {
              mObj.mElements.at(jj.npPtr).value = -jj.normalCond;
              needsLU = true;
            }
          }
          if (jj.pPtr != -1) {
            if (mObj.mElements.at(jj.pPtr).value != jj.normalCond) {
              mObj.mElements.at(jj.pPtr).value = jj.normalCond;
              needsLU = true;
            }
          }
          if (jj.nPtr != -1) {
            if (mObj.mElements.at(jj.nPtr).value != -jj.normalCond) {
              mObj.mElements.at(jj.nPtr).value = -jj.normalCond;
              needsLU = true;
            }
          }
        }
      }
      // Phase guess (P0)
      jj.phi0 = jj.pn1 + (hn_2_2e_hbar) * (jj.vn1 + jj.v0);
      // Junction current (Is)
      jj.iS = -((JoSIM::Constants::PI * jj.Del) / (2 * JoSIM::Constants::EV * jj.rNCalc)) *
              (sin(jj.phi0) / sqrt(1 - jj.D * (sin(jj.phi0 / 2) *
                sin(jj.phi0 / 2)))) * tanh((jj.Del) / (2 * JoSIM::Constants::BOLTZMANN * jj.T) *
                sqrt(1 - jj.D * (sin(jj.phi0 / 2) * sin(jj.phi0 / 2)))) +
              (((2 * jj.C) / iObj.transSim.prstep) * jj.vn1) + (jj.C * jj.dVn1) - jj.iT;
      // Set previous values
      jj.vn2 = jj.vn1;
      jj.dVn2 = jj.dVn1;
      jj.pn2 = jj.pn1;
      // Store current
      if(jj.storeCurrent) jj.jjCur.push_back(jj.iS);
    }

    for (int j = 0; j < mObj.components.phaseCap.size(); j++) {
      cap_phase &c = mObj.components.phaseCap.at(j);
      c.pn2 = c.pn1;
      if (c.posNRow == -1) c.pn1 = (-lhsValues.at(c.negNRow));
      else if (c.negNRow == -1) c.pn1 = (lhsValues.at(c.posNRow));
      else c.pn1 = lhsValues.at(c.posNRow) - lhsValues.at(c.negNRow);
      c.ICn1 = lhsValues.at(c.curNRow);
      c.dPn1 = (2 / iObj.transSim.prstep) * (c.pn1 - c.pn2) - c.dPn2;
      c.dPn2 = c.dPn1;
    }

    for (int j = 0; j < mObj.components.txPhase.size(); j++) {
      tx_phase &tl = mObj.components.txPhase.at(j);
       
      if (tl.posNRow == -1) tl.p1n1 = -lhsValues.at(tl.negNRow);
      else if (tl.negNRow == -1) tl.p1n1 = lhsValues.at(tl.posNRow);
      else tl.p1n1 = lhsValues.at(tl.posNRow) - lhsValues.at(tl.negNRow);
      
      if (tl.posN2Row == -1) tl.p2n1 = -lhsValues.at(tl.negN2Row);
      else if (tl.negN2Row == -1) tl.p2n1 = lhsValues.at(tl.posN2Row);
      else tl.p2n1 = lhsValues.at(tl.posN2Row) - lhsValues.at(tl.negN2Row);

      tl.dP1n1 = (2 / iObj.transSim.prstep) * (tl.p1n1 - tl.p1n2) - tl.dP1n2;
      tl.p1n2 = tl.p1n1;
      tl.dP1n2 = tl.dP1n1;

      tl.dP2n1 = (2 / iObj.transSim.prstep) * (tl.p2n1 - tl.p2n2) - tl.dP2n2;
      tl.p2n2 = tl.p2n1;
      tl.dP2n2 = tl.dP2n1;
      
      if (i >= tl.k) {
        if (tl.posNRow == -1) tl.p1nk = -results.xVect.at(mObj.relToXMap.at(tl.negNodeR)).at(i - tl.k);
        else if (tl.negNRow == -1) tl.p1nk = results.xVect.at(mObj.relToXMap.at(tl.posNodeR)).at(i - tl.k);
        else tl.p1nk = results.xVect.at(mObj.relToXMap.at(tl.posNodeR)).at(i - tl.k)
          - results.xVect.at(mObj.relToXMap.at(tl.negNodeR)).at(i - tl.k);

        if (tl.posN2Row == -1) tl.p1nk = -results.xVect.at(mObj.relToXMap.at(tl.negNode2R)).at(i - tl.k);
        else if (tl.negN2Row == -1) tl.p1nk = results.xVect.at(mObj.relToXMap.at(tl.posNode2R)).at(i - tl.k);
        else tl.p1nk = results.xVect.at(mObj.relToXMap.at(tl.posNode2R)).at(i - tl.k)
          - results.xVect.at(mObj.relToXMap.at(tl.negNode2R)).at(i - tl.k);

        tl.dP1nk = (2 / iObj.transSim.prstep) * (tl.p1nk - tl.p1nk1) - tl.dP1nk1;
        tl.p1nk1 = tl.p1nk;
        tl.dP1nk1 = tl.dP1nk;

        tl.dP2nk = (2 / iObj.transSim.prstep) * (tl.p2nk - tl.p2nk1) - tl.dP2nk1;
        tl.p2nk1 = tl.p2nk;
        tl.dP2nk1 = tl.dP2nk;
      }
    }

    if (needsLU) {
      mObj.create_CSR();
      klu_free_numeric(&Numeric, &Common);
      Numeric = klu_factor(&mObj.rowptr.front(), &mObj.colind.front(),
                           &mObj.nzval.front(), Symbolic, &Common);
      needsLU = false;
    }

    results.timeAxis.push_back(i * iObj.transSim.prstep);
  }
  std::cout << "100%" << std::endl;
  klu_free_symbolic(&Symbolic, &Common);
  klu_free_numeric(&Numeric, &Common);
}

// Compile template for classes
template void Simulation::trans_sim<JoSIM::AnalysisType::Voltage>(Input &iObj, Matrix &mObj);
template void Simulation::trans_sim<JoSIM::AnalysisType::Phase>(Input &iObj, Matrix &mObj);
