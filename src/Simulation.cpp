// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/Simulation.hpp"
#include "JoSIM/AnalysisType.hpp"
#include "JoSIM/Components.hpp"
#include "JoSIM/Matrix.hpp"
#include "JoSIM/Constants.hpp"
#include "JoSIM/Model.hpp"

#include "suitesparse/klu.h"

#include <cassert>
#include <cmath>
#include <iostream>

template<JoSIM::AnalysisType AnalysisTypeValue>
void Simulation::trans_sim(Input &iObj, Matrix &mObj) {
  std::vector<double> lhsValues(mObj.Nsize, 0.0),
      RHS(mObj.Nsize, 0.0), LHS_PRE(mObj.Nsize, 0.0);
  int simSize = iObj.transSim.get_simsize();
  int saveAll = false;
  if(mObj.relevantToStore.size() == 0) saveAll = true;
  if (!saveAll) {
    for (int m = 0; m < mObj.relXInd.size(); m++)
      results.xVect.emplace_back(std::vector<double>(simSize, 0.0));
  } else {
    for (int m = 0; m < mObj.rowDesc.size(); m++)
      results.xVect.emplace_back(std::vector<double>(simSize, 0.0));
  }
  double hn_2_2e_hbar = (iObj.transSim.get_prstep() / 2) * (2 * JoSIM::Constants::PI / JoSIM::Constants::PHI_ZERO);
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
  for(int i = 0; i < simSize; ++i) {
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
              iObj.transSim.get_prstep()) * lhsValues.at(mObj.components.voltInd.at(j.index).curNRow)) - RHSvalue;
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
                                      std::distance(mObj.relXInd.begin(),
                                        std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                                          mObj.relToXMap.at(txline.negNode2R)))
                                    ).at(i - txline.k);
            else if (txline.negN2Row == -1)
              RHS.at(rowCounter) = results.xVect.at(
                                    std::distance(mObj.relXInd.begin(),
                                      std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                                        mObj.relToXMap.at(txline.posNode2R)))
                                    ).at(i - txline.k);
            else
              RHS.at(rowCounter) = results.xVect.at(
                                    std::distance(mObj.relXInd.begin(),
                                      std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                                        mObj.relToXMap.at(txline.posNode2R)))
                                    ).at(i - txline.k)
                                  - results.xVect.at(
                                    std::distance(mObj.relXInd.begin(),
                                      std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                                        mObj.relToXMap.at(txline.negNode2R)))
                                    ).at(i - txline.k);
            RHS.at(rowCounter) += txline.value 
                                  * results.xVect.at(
                                    std::distance(mObj.relXInd.begin(),
                                      std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                                        mObj.relToXMap.at(txline.curNode2R)))
                                    ).at(i - txline.k);
          }
          break; }
        case RowDescriptor::Type::VoltageTX2: {
          const auto &txline = mObj.components.txLine.at(j.index);
          if (i >= txline.k) {
            if (txline.posNRow == -1)
              RHS.at(rowCounter) = -results.xVect.at(
                                    std::distance(mObj.relXInd.begin(),
                                      std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                                        mObj.relToXMap.at(txline.negNodeR)))
                                    ).at(i - txline.k);
            else if (txline.negNRow == -1)
              RHS.at(rowCounter) = results.xVect.at(
                                    std::distance(mObj.relXInd.begin(),
                                      std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                                        mObj.relToXMap.at(txline.posNodeR)))
                                    ).at(i - txline.k);
            else
              RHS.at(rowCounter) = results.xVect.at(
                                    std::distance(mObj.relXInd.begin(),
                                      std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                                        mObj.relToXMap.at(txline.posNodeR)))
                                    ).at(i - txline.k)
                                  - results.xVect.at(
                                    std::distance(mObj.relXInd.begin(),
                                      std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                                        mObj.relToXMap.at(txline.negNodeR)))
                                    ).at(i - txline.k);
            RHS.at(rowCounter) += txline.value *
                                  results.xVect.at(
                                    std::distance(mObj.relXInd.begin(),
                                      std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                                        mObj.relToXMap.at(txline.curNode1R)))
                                  ).at(i - txline.k);
          }
          break; }
        case RowDescriptor::Type::PhaseResistor: {
          auto &presis = mObj.components.phaseRes.at(j.index);
          if(presis.posNRow == -1) presis.pn1 = -lhsValues.at(presis.negNRow);
          else if (presis.negNRow == -1) presis.pn1 = lhsValues.at(presis.posNRow);
          else presis.pn1 = lhsValues.at(presis.posNRow) - lhsValues.at(presis.negNRow);
          presis.IRn1 = lhsValues.at(presis.curNRow);
          RHS.at(rowCounter) = ((JoSIM::Constants::PI * presis.value * iObj.transSim.get_prstep()) / JoSIM::Constants::PHI_ZERO) * presis.IRn1 + presis.pn1;
          break; }
        case RowDescriptor::Type::PhaseJJ:
          RHS.at(rowCounter) = mObj.components.phaseJJ.at(j.index).pn1 + hn_2_2e_hbar * mObj.components.phaseJJ.at(j.index).vn1;
          break;
        case RowDescriptor::Type::PhaseCapacitor:
          RHS.at(rowCounter) = 
            -((2 * JoSIM::Constants::PI * iObj.transSim.get_prstep() * iObj.transSim.get_prstep()) /
              (4 * JoSIM::Constants::PHI_ZERO * mObj.components.phaseCap.at(j.index).value)) *
                mObj.components.phaseCap.at(j.index).ICn1 -
            mObj.components.phaseCap.at(j.index).pn1 -
            (iObj.transSim.get_prstep() * mObj.components.phaseCap.at(j.index).dPn1);
          break;
        case RowDescriptor::Type::PhaseVS: {
          auto &pvs = mObj.components.phaseVs.at(j.index);
          if (pvs.posNRow == -1.0) pvs.pn1 = -lhsValues.at(pvs.negNRow);
          else if (pvs.negNRow == -1.0) pvs.pn1 = lhsValues.at(pvs.posNRow);
          else pvs.pn1 = lhsValues.at(pvs.posNRow) - lhsValues.at(pvs.negNRow);
          if (i >= 1) 
            RHS.at(rowCounter) = pvs.pn1 + ((iObj.transSim.get_prstep() * JoSIM::Constants::PI) / JoSIM::Constants::PHI_ZERO) *
              (mObj.sources.at(pvs.sourceDex).at(i) + mObj.sources.at(pvs.sourceDex).at(i - 1));
          else if (i == 0)
            RHS.at(rowCounter) = pvs.pn1 + ((iObj.transSim.get_prstep() * JoSIM::Constants::PI) / JoSIM::Constants::PHI_ZERO) *
              mObj.sources.at(pvs.sourceDex).at(i);
          break; }
        case RowDescriptor::Type::PhaseTX1: {
          const auto &txline = mObj.components.txPhase.at(j.index);
          if (i > txline.k)
            RHS.at(rowCounter) = ((iObj.transSim.get_prstep() * JoSIM::Constants::PI * txline.value) / JoSIM::Constants::PHI_ZERO) *
              results.xVect.at(mObj.relToXMap.at(txline.curNode2R)).at(i - txline.k) +
              txline.p1n1 + (iObj.transSim.get_prstep() / 2) * (txline.dP1n1 + txline.dP2nk);
          break; }
        case RowDescriptor::Type::PhaseTX2: {
          const auto &txline = mObj.components.txPhase.at(j.index);
          if (i > txline.k)
            RHS.at(rowCounter) = ((iObj.transSim.get_prstep() * JoSIM::Constants::PI * txline.value) / JoSIM::Constants::PHI_ZERO) *
              results.xVect.at(mObj.relToXMap.at(txline.curNode1R)).at(i - txline.k) +
              txline.p2n1 + (iObj.transSim.get_prstep() / 2) * (txline.dP2n1 + txline.dP1nk);
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
      Errors::simulation_errors(SimulationErrors::MATRIX_SINGULAR, "");

    lhsValues = LHS_PRE;
    if(!saveAll) {
      for (int m = 0; m < mObj.relXInd.size(); ++m)
        results.xVect.at(m).at(i) = lhsValues.at(mObj.relXInd.at(m));
    } else {
      for (int m = 0; m < mObj.rowDesc.size(); ++m)
        results.xVect.at(m).at(i) = lhsValues.at(m);
    }
    
    for (int j = 0; j < jj_vector.size(); ++j) {
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
      else jj.dVn1 = (2 / iObj.transSim.get_prstep()) * (jj.vn1 - jj.vn2) - jj.dVn2;
      // Guess voltage (V0)
      jj.v0 = jj.vn1 + iObj.transSim.get_prstep() * jj.dVn1;
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
              (((2 * jj.C) / iObj.transSim.get_prstep()) * jj.vn1) + (jj.C * jj.dVn1) - jj.iT;
      // Set previous values
      jj.vn2 = jj.vn1;
      jj.dVn2 = jj.dVn1;
      jj.pn2 = jj.pn1;
      // Store current
      if(jj.storeCurrent) jj.jjCur.push_back(jj.iS);
    }

    for (int j = 0; j < mObj.components.phaseCap.size(); ++j) {
      cap_phase &c = mObj.components.phaseCap.at(j);
      c.pn2 = c.pn1;
      if (c.posNRow == -1) c.pn1 = (-lhsValues.at(c.negNRow));
      else if (c.negNRow == -1) c.pn1 = (lhsValues.at(c.posNRow));
      else c.pn1 = lhsValues.at(c.posNRow) - lhsValues.at(c.negNRow);
      c.ICn1 = lhsValues.at(c.curNRow);
      c.dPn1 = (2 / iObj.transSim.get_prstep()) * (c.pn1 - c.pn2) - c.dPn2;
      c.dPn2 = c.dPn1;
    }

    for (int j = 0; j < mObj.components.txPhase.size(); ++j) {
      tx_phase &tl = mObj.components.txPhase.at(j);
       
      if (tl.posNRow == -1) tl.p1n1 = -lhsValues.at(tl.negNRow);
      else if (tl.negNRow == -1) tl.p1n1 = lhsValues.at(tl.posNRow);
      else tl.p1n1 = lhsValues.at(tl.posNRow) - lhsValues.at(tl.negNRow);
      
      if (tl.posN2Row == -1) tl.p2n1 = -lhsValues.at(tl.negN2Row);
      else if (tl.negN2Row == -1) tl.p2n1 = lhsValues.at(tl.posN2Row);
      else tl.p2n1 = lhsValues.at(tl.posN2Row) - lhsValues.at(tl.negN2Row);

      tl.dP1n1 = (2 / iObj.transSim.get_prstep()) * (tl.p1n1 - tl.p1n2) - tl.dP1n2;
      tl.p1n2 = tl.p1n1;
      tl.dP1n2 = tl.dP1n1;

      tl.dP2n1 = (2 / iObj.transSim.get_prstep()) * (tl.p2n1 - tl.p2n2) - tl.dP2n2;
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

        tl.dP1nk = (2 / iObj.transSim.get_prstep()) * (tl.p1nk - tl.p1nk1) - tl.dP1nk1;
        tl.p1nk1 = tl.p1nk;
        tl.dP1nk1 = tl.dP1nk;

        tl.dP2nk = (2 / iObj.transSim.get_prstep()) * (tl.p2nk - tl.p2nk1) - tl.dP2nk1;
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

    results.timeAxis.push_back(i * iObj.transSim.get_prstep());
  }
  std::cout << "100%" << std::endl;
  klu_free_symbolic(&Symbolic, &Common);
  klu_free_numeric(&Numeric, &Common);
}

void Simulation::trans_sim_new(Input &iObj, Matrix &mObj) {
  std::vector<double> lhsValues( mObj.rp.size() - 1, 0.0),
      LHS_PRE(mObj.rp.size() - 1, 0.0);
  int simSize = iObj.transSim.get_simsize();
  int saveAll = false;
  if(mObj.relevantIndices.size() == 0) saveAll = true;
  results.xVector_new.resize(mObj.rp.size() - 1);
  if(!saveAll) {
    for (const auto &i : mObj.relevantIndices) {
      results.xVector_new.at(i).emplace();
    }
  } else {
    for (int i = 0; i < mObj.rp.size() - 1; ++i) {
      results.xVector_new.at(i).emplace();
    }
  }
  double hbar_he = (JoSIM::Constants::HBAR / (iObj.transSim.get_prstep() * JoSIM::Constants::EV));
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
  Symbolic = klu_analyze(mObj.rp.size() - 1, &mObj.rp.front(), &mObj.ci.front(),
                         &Common);
  Numeric = klu_factor(&mObj.rp.front(), &mObj.ci.front(),
                       &mObj.nz.front(), Symbolic, &Common);
  results.timeAxis.clear();
  std::cout << "Simulation Progress:" << std::endl;
  std::cout << "0%\r" << std::flush;
  for(int i = 0; i < simSize; ++i) {
    std::vector<double> RHS(mObj.rp.size() - 1, 0.0);
    rowCounter = 0;
    if(i == fqtr) std::cout << "25%\r" << std::flush;
    if(i == sqtr) std::cout << "50%\r" << std::flush;
    if(i == tqtr) std::cout << "75%\r" << std::flush;
    // Handle resistors
    for (const auto &j : mObj.components_new.resistorIndices) {
      const auto &temp = std::get<Resistor>(mObj.components_new.devices.at(j));
      double prevNode;
      if(temp.get_posIndex() && !temp.get_negIndex()) {
        prevNode = (LHS_PRE.at(temp.get_posIndex().value()));
      } else if(!temp.get_posIndex() && temp.get_negIndex()) {
        prevNode = (-LHS_PRE.at(temp.get_negIndex().value()));
      } else {
        prevNode = (LHS_PRE.at(temp.get_posIndex().value())
                - LHS_PRE.at(temp.get_negIndex().value()));
      }
      if(iObj.argAnal == JoSIM::AnalysisType::Voltage) {
      } else if (iObj.argAnal == JoSIM::AnalysisType::Phase) {
        // Rh/2σ Ip + φp
        RHS.at(temp.get_currentIndex()) = temp.get_value() * LHS_PRE.at(temp.get_currentIndex()) + prevNode;
      }
    }
    // Handle inductors
    for (const auto &j : mObj.components_new.inductorIndices) {
      const auto &temp = std::get<Inductor>(mObj.components_new.devices.at(j));
      double prevNode;
      if(temp.get_posIndex() && !temp.get_negIndex()) {
        prevNode = (LHS_PRE.at(temp.get_posIndex().value()));
      } else if(!temp.get_posIndex() && temp.get_negIndex()) {
        prevNode = (-LHS_PRE.at(temp.get_negIndex().value()));
      } else {
        prevNode = (LHS_PRE.at(temp.get_posIndex().value())
                - LHS_PRE.at(temp.get_negIndex().value()));
      }
      if(iObj.argAnal == JoSIM::AnalysisType::Voltage) {
        // -2L/h Ip - Vp
        RHS.at(temp.get_currentIndex()) = -temp.get_value() * LHS_PRE.at(temp.get_currentIndex()) - prevNode;
        // -2M/h Im
        for(const auto &m : temp.get_mutualInductance()) {
          RHS.at(temp.get_currentIndex()) -= (((2 * m.second) / iObj.transSim.get_prstep()) * LHS_PRE.at(std::get<Inductor>(mObj.components_new.devices.at(m.first)).get_currentIndex()));
        }
      }
    }
    // Handle capacitors
    for (const auto &j : mObj.components_new.capacitorIndices) {
      auto &temp = std::get<Capacitor>(mObj.components_new.devices.at(j));
      double prevNode;
      if(temp.get_posIndex() && !temp.get_negIndex()) {
        prevNode = (LHS_PRE.at(temp.get_posIndex().value()));
      } else if(!temp.get_posIndex() && temp.get_negIndex()) {
        prevNode = (-LHS_PRE.at(temp.get_negIndex().value()));
      } else {
        prevNode = (LHS_PRE.at(temp.get_posIndex().value())
                - LHS_PRE.at(temp.get_negIndex().value()));
      }
      if(iObj.argAnal == JoSIM::AnalysisType::Voltage) {
        // h/2C Ip + Vp
        RHS.at(temp.get_currentIndex()) = temp.get_value()
                                          * LHS_PRE.at(temp.get_currentIndex()) + prevNode;
      } else if (iObj.argAnal == JoSIM::AnalysisType::Phase) {
        double pn2 = temp.get_pn1();
        temp.set_pn1(prevNode);
        double dpn2 = temp.get_dpn1();
        temp.set_dpn1((2 / iObj.transSim.get_prstep()) * (temp.get_pn1() - pn2) - dpn2); 
        // h/2C Ip - φp - h Δφp
        RHS.at(temp.get_currentIndex()) = temp.get_value() * LHS_PRE.at(temp.get_currentIndex()) + prevNode + iObj.transSim.get_prstep() * temp.get_dpn1();
      }
    }
    // Handle junctions
    for (const auto &j : mObj.components_new.junctionIndices) {
      auto &temp = std::get<JJ>(mObj.components_new.devices.at(j));
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
        if(iObj.argAnal == JoSIM::AnalysisType::Voltage) {
          temp.set_vn1(prevNode);
          temp.set_pn1(LHS_PRE.at(temp.get_variableIndex()));
        } else if (iObj.argAnal == JoSIM::AnalysisType::Phase) {
          temp.set_vn1(LHS_PRE.at(temp.get_variableIndex()));
          temp.set_pn1(prevNode);
        }
      }
      if (i <= 3) {
        temp.set_dvn1(0.0);
      } else {
        temp.set_dvn1((2 / iObj.transSim.get_prstep()) * (temp.get_vn1() - temp.get_vn2()) - temp.get_dvn2());
      }
      // Guess voltage (V0)
      double v0 = temp.get_vn1() + iObj.transSim.get_prstep() * temp.get_dvn1();
      // -(hbar / h * e) φp - Vp 
      RHS.at(temp.get_variableIndex()) = -hbar_he * temp.get_pn1() - temp.get_vn1();
      // Phase guess (P0)
      double phi0 = temp.get_pn1() + (1 / hbar_he) * (temp.get_vn1() + v0);
      // (hR / h + 2RC) * (Ic sin φ0 - 2C / h Vp - C ΔVp)
      RHS.at(temp.get_currentIndex()) = (temp.get_nonZeros().back()) * (((JoSIM::Constants::PI * temp.get_del()) / (2 * JoSIM::Constants::EV * temp.get_rncalc())) *
                                        (sin(phi0) / sqrt(1 - model.get_transparency() * (sin(phi0 / 2) * sin(phi0 / 2)))) 
                                        * tanh((temp.get_del()) / (2 * JoSIM::Constants::BOLTZMANN * model.get_temperature()) *
                                          sqrt(1 - model.get_transparency() * (sin(phi0 / 2) * sin(phi0 / 2)))) -
                                        (((2 * model.get_capacitance()) / iObj.transSim.get_prstep()) * temp.get_vn1()) 
                                        - (model.get_capacitance() * temp.get_dvn1())) - temp.get_transitionCurrent();
      // Update junction transition
      if(model.get_resistanceType() == 1) {
        needsLU = temp.update_value(v0);
      }
      temp.set_vn2(temp.get_vn1());
      temp.set_dvn2(temp.get_dvn1());
    }
    // Handle voltage sources
    for (const auto &j : mObj.components_new.vsIndices) {
      const auto &temp = std::get<VoltageSource>(mObj.components_new.devices.at(j));
      if(iObj.argAnal == JoSIM::AnalysisType::Voltage) {
        RHS.at(temp.get_currentIndex()) = mObj.sources.at(temp.get_sourceIndex()).at(i);
      } else if (iObj.argAnal == JoSIM::AnalysisType::Phase) {
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
          RHS.at(temp.get_currentIndex()) = (iObj.transSim.get_prstep() / (2 * JoSIM::Constants::SIGMA)) * (mObj.sources.at(temp.get_sourceIndex()).at(i)) + prevNode;
        } else {
          RHS.at(temp.get_currentIndex()) = (iObj.transSim.get_prstep() / (2 * JoSIM::Constants::SIGMA)) * (mObj.sources.at(temp.get_sourceIndex()).at(i) - mObj.sources.at(temp.get_sourceIndex()).at(i-1)) + prevNode;
        }
      }
    }
    // Handle phase sources
    for (const auto &j : mObj.components_new.psIndices) {
      const auto &temp = std::get<PhaseSource>(mObj.components_new.devices.at(j));
      if (iObj.argAnal == JoSIM::AnalysisType::Phase) {
        RHS.at(temp.get_currentIndex()) = mObj.sources.at(temp.get_sourceIndex()).at(i);
      } else if(iObj.argAnal == JoSIM::AnalysisType::Voltage) {
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
          RHS.at(temp.get_currentIndex()) = ((JoSIM::Constants::SIGMA * 2) / iObj.transSim.get_prstep()) * (mObj.sources.at(temp.get_sourceIndex()).at(i)) - prevNode;
        } else {
          RHS.at(temp.get_currentIndex()) = ((JoSIM::Constants::SIGMA * 2) / iObj.transSim.get_prstep()) * (mObj.sources.at(temp.get_sourceIndex()).at(i) - mObj.sources.at(temp.get_sourceIndex()).at(i-1)) - prevNode;
        }
      }
    }
    // Handle current sources
    for (const auto &j : mObj.components_new.currentsources) {
      if(j.get_posIndex() && !j.get_negIndex()) {
          RHS.at(j.get_posIndex().value()) -= (mObj.sources.at(j.get_sourceIndex()).at(i));
        } else if(!j.get_posIndex() && j.get_negIndex()) {
          RHS.at(j.get_negIndex().value()) += (mObj.sources.at(j.get_sourceIndex()).at(i));
        } else {
          RHS.at(j.get_posIndex().value()) -= (mObj.sources.at(j.get_sourceIndex()).at(i));
          RHS.at(j.get_negIndex().value()) += (mObj.sources.at(j.get_sourceIndex()).at(i));
        }
    }
    // Handle transmission lines
    for (const auto &j : mObj.components_new.txIndices) {
      auto &temp = std::get<TransmissionLine>(mObj.components_new.devices.at(j));
      if(i >= temp.get_timestepDelay()) {
        double prevNodek, prevNode2k;
        if(temp.get_posIndex() && !temp.get_negIndex()) {
          prevNodek = results.xVector_new.at(
                          temp.get_posIndex().value()).value().at(i - temp.get_timestepDelay());
        } else if(!temp.get_posIndex() && temp.get_negIndex()) {
          prevNodek = -results.xVector_new.at(
                          temp.get_negIndex().value()).value().at(i - temp.get_timestepDelay());
        } else {
          prevNodek = results.xVector_new.at(
                          temp.get_posIndex().value()).value().at(i - temp.get_timestepDelay())
                      - results.xVector_new.at(
                          temp.get_negIndex().value()).value().at(i - temp.get_timestepDelay());
        }

        if(temp.get_posIndex2() && !temp.get_negIndex2()) {
          prevNode2k = results.xVector_new.at(
                          temp.get_posIndex2().value()).value().at(i - temp.get_timestepDelay());
        } else if(!temp.get_posIndex2() && temp.get_negIndex2()) {
          prevNode2k = -results.xVector_new.at(
                          temp.get_negIndex2().value()).value().at(i - temp.get_timestepDelay());
        } else {
          prevNode2k = results.xVector_new.at(
                          temp.get_posIndex2().value()).value().at(i - temp.get_timestepDelay())
                        - results.xVector_new.at(
                          temp.get_negIndex2().value()).value().at(i - temp.get_timestepDelay());
        }

        if(iObj.argAnal == JoSIM::AnalysisType::Voltage) {
          // IT1 = V2(n-k) + Z0 I2(n-k)
          RHS.at(temp.get_currentIndex()) = prevNode2k + temp.get_value() 
                                            * results.xVector_new.at(
                                              temp.get_currentIndex2()).value().at(
                                                i - temp.get_timestepDelay());
          // IT2 = V1(n-k) + Z0 I1(n-k)
          RHS.at(temp.get_currentIndex2()) = prevNodek + temp.get_value() 
                                            * results.xVector_new.at(
                                              temp.get_currentIndex()).value().at(
                                                i - temp.get_timestepDelay());
          
        } else if(iObj.argAnal == JoSIM::AnalysisType::Phase) {
          double prevNodeN, prevNode2N, prevNodeN1, prevNode2N1, prevNodek1, prevNode2k1;
          if(temp.get_posIndex() && !temp.get_negIndex()) {
            prevNodeN = results.xVector_new.at(temp.get_posIndex().value()).value().at(i);
            prevNodeN1 = results.xVector_new.at(temp.get_posIndex().value()).value().at(i - 1);
            if(i > temp.get_timestepDelay()) {
              prevNodek1 = results.xVector_new.at(temp.get_posIndex().value()).value().at(i - temp.get_timestepDelay() - 1);
            }
          } else if(!temp.get_posIndex() && temp.get_negIndex()) {
            prevNodeN = -results.xVector_new.at(temp.get_negIndex().value()).value().at(i);
            prevNodeN1 = -results.xVector_new.at(temp.get_negIndex().value()).value().at(i - 1);            
            if(i >= temp.get_timestepDelay()) { 
              prevNodek1 = -results.xVector_new.at(temp.get_negIndex().value()).value().at(i - temp.get_timestepDelay() - 1);
            }            
          } else {
            prevNodeN = results.xVector_new.at(temp.get_posIndex().value()).value().at(i)
                    - results.xVector_new.at(temp.get_negIndex().value()).value().at(i);
            prevNodeN1 = results.xVector_new.at(temp.get_posIndex().value()).value().at(i - 1)
                    - results.xVector_new.at(temp.get_negIndex().value()).value().at(i - 1);
            if(i >= temp.get_timestepDelay()) {
              prevNodek1 = results.xVector_new.at(temp.get_posIndex().value()).value().at(i - temp.get_timestepDelay() - 1)
                    - results.xVector_new.at(temp.get_negIndex().value()).value().at(i - temp.get_timestepDelay() - 1);
            }
          }

          if(temp.get_posIndex2() && !temp.get_negIndex2()) {
            prevNode2N = results.xVector_new.at(temp.get_posIndex2().value()).value().at(i);
            prevNode2N1 = results.xVector_new.at(temp.get_posIndex2().value()).value().at(i - 1);
            if(i >= temp.get_timestepDelay()) {
              prevNode2k1 = results.xVector_new.at(temp.get_posIndex2().value()).value().at(i - temp.get_timestepDelay() - 1);
            }
          } else if(!temp.get_posIndex2() && temp.get_negIndex2()) {
            prevNode2N = -results.xVector_new.at(temp.get_negIndex2().value()).value().at(i);
            prevNode2N1 = -results.xVector_new.at(temp.get_negIndex2().value()).value().at(i - 1);
            if(i >= temp.get_timestepDelay()) {
              prevNode2k1 = -results.xVector_new.at(temp.get_negIndex2().value()).value().at(i - temp.get_timestepDelay() - 1);
            }
          } else {
            prevNode2N = results.xVector_new.at(temp.get_posIndex2().value()).value().at(i)
                    - results.xVector_new.at(temp.get_negIndex2().value()).value().at(i);
            prevNode2N1 = results.xVector_new.at(temp.get_posIndex2().value()).value().at(i - 1)
                    - results.xVector_new.at(temp.get_negIndex2().value()).value().at(i - 1);
            if(i >= temp.get_timestepDelay()) {
              prevNode2k1 = results.xVector_new.at(temp.get_posIndex2().value()).value().at(i - temp.get_timestepDelay() - 1)
                    - results.xVector_new.at(temp.get_negIndex2().value()).value().at(i - temp.get_timestepDelay() - 1);
            }
          }

          temp.set_dv1n1((iObj.transSim.get_prstep() / 2) * (prevNodeN - prevNodeN1) - temp.get_dv1n2());
          temp.set_dv2n1((iObj.transSim.get_prstep() / 2) * (prevNode2N - prevNode2N1) - temp.get_dv2n2());
          if(i < (temp.get_timestepDelay() + 1)) {
            temp.set_dv1nk((iObj.transSim.get_prstep() / 2) * (prevNodek));
            temp.set_dv2nk((iObj.transSim.get_prstep() / 2) * (prevNode2k));
          } else {
            temp.set_dv1nk((iObj.transSim.get_prstep() / 2) * (prevNodek - prevNodek1) - temp.get_dv1nk1());
            temp.set_dv2nk((iObj.transSim.get_prstep() / 2) * (prevNode2k - prevNode2k1) - temp.get_dv2nk1());
          }
          RHS.at(temp.get_currentIndex()) = ((iObj.transSim.get_prstep() * temp.get_value()) / (2 * JoSIM::Constants::SIGMA))
                                            * results.xVector_new.at(temp.get_currentIndex2()).value().at(i - temp.get_timestepDelay()) + prevNodeN
                                            + (iObj.transSim.get_prstep() / 2) * (temp.get_dv1n1() + temp.get_dv2nk());
          RHS.at(temp.get_currentIndex2()) = ((iObj.transSim.get_prstep() * temp.get_value()) / (2 * JoSIM::Constants::SIGMA))
                                            * results.xVector_new.at(temp.get_currentIndex()).value().at(i - temp.get_timestepDelay()) + prevNode2N
                                            + (iObj.transSim.get_prstep() / 2) * (temp.get_dv2n1() + temp.get_dv1nk());
          temp.set_dv1n2(temp.get_dv1n1());
          temp.set_dv2n2(temp.get_dv2n1());
          temp.set_dv1nk1(temp.get_dv1nk());
          temp.set_dv2nk1(temp.get_dv2nk());
          
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
        results.xVector_new.at(mObj.relevantIndices.at(m)).value().emplace_back(lhsValues.at(mObj.relevantIndices.at(m)));
      }
    } else {
      for (int m = 0; m < mObj.rp.size() - 1; ++m) {
        results.xVector_new.at(m).value().emplace_back(lhsValues.at(m));
      }
    }
    
    if (needsLU) {
      mObj.create_nz();
      klu_free_numeric(&Numeric, &Common);
      Numeric = klu_factor(&mObj.rp.front(), &mObj.ci.front(),
                           &mObj.nz.front(), Symbolic, &Common);
      needsLU = false;
    }

    results.timeAxis.emplace_back(i * iObj.transSim.get_prstep());
  }
  std::cout << "100%" << std::endl;
  klu_free_symbolic(&Symbolic, &Common);
  klu_free_numeric(&Numeric, &Common);
}

// Compile template for classes
template void Simulation::trans_sim<JoSIM::AnalysisType::Voltage>(Input &iObj, Matrix &mObj);
template void Simulation::trans_sim<JoSIM::AnalysisType::Phase>(Input &iObj, Matrix &mObj);
