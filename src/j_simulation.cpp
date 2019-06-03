// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#include "JoSIM/j_simulation.h"

// Linear algebra include
#include "klu.h"

#include <cassert>

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

void Simulation::transient_voltage_simulation(Input &iObj, Matrix &mObj) {
  results.xVect.clear();
  results.timeAxis.clear();
  std::vector<double> lhsValues(mObj.Nsize, 0.0),
      RHS(mObj.columnNames.size(), 0.0), LHS_PRE;
  int simSize = iObj.transSim.simsize();
  for (int m = 0; m < mObj.relXInd.size(); m++) {
    results.xVect.emplace_back(std::vector<double>(simSize, 0.0));
  }
  // std::vector<std::vector<std::string>>
  // nodeConnectionVector(mObj.rowNames.size());
  std::string txCurrent;
  double VB, RHSvalue, inductance, z0voltage;
  double hn_2_2e_hbar = (iObj.transSim.prstep / 2) * (2 * M_PI / PHI_ZERO);
  int ok, rowCounter;
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
  rowCounter = 0;
  // for (const auto &j : mObj.rowDesc) {

  // // assert(j.size() >= 3);

  // // 	if (j[2] == 'N') {
  // // 		nodeConnectionVector[rowCounter] =
  // mObj.nodeConnections[j];
  // // 	}
  // // 	rowCounter++;
  // // }
  if (sOutput)
    std::cout << "Simulating:" << std::endl;
  double increments = 100 / (double)simSize;
  double progress_increments = 30 / (double)simSize;
  double incremental_progress = 0.0;

  int progress = 0;
  int old_progress = 0;
  int imintd = 0;
  std::string pBar;
  double mutualL = 0.0;
  double CUR2 = 0.0;
  for (int i = 0; i < simSize; i++) {
#ifndef NO_PRINT
    if (sOutput)
      std::cout << '\r' << std::flush;
#endif
    RHS.clear();
    rowCounter = 0;
    for (auto j : mObj.rowDesc) {
      RHSvalue = 0.0;
      if (j.type == RowDescriptor::Type::VoltageNode) {
        for (auto k : mObj.nodeConnections.at(j.index).connections) {
          if (k.type == ComponentConnections::Type::JJP) {
            RHSvalue += mObj.components.voltJJ.at(k.index).iS;
          } else if (k.type == ComponentConnections::Type::JJN) {
            RHSvalue -= mObj.components.voltJJ.at(k.index).iS;
          } else if (k.type == ComponentConnections::Type::CSN) {
            RHSvalue += mObj.sources.at(k.index).at(i);
          } else if (k.type == ComponentConnections::Type::CSP) {
            RHSvalue -= mObj.sources.at(k.index).at(i);
          }
        }
      } else if (j.type == RowDescriptor::Type::VoltageInductor) {
        if (mObj.components.voltInd.at(j.index).posNRow == -1)
          VB = -lhsValues.at(mObj.components.voltInd.at(j.index).negNRow);
        else if (mObj.components.voltInd.at(j.index).negNRow == -1)
          VB = lhsValues.at(mObj.components.voltInd.at(j.index).posNRow);
        else
          VB = lhsValues.at(mObj.components.voltInd.at(j.index).posNRow) -
               lhsValues.at(mObj.components.voltInd.at(j.index).negNRow);
        RHSvalue =
            (-2 * mObj.components.voltInd.at(j.index).value /
             iObj.transSim.prstep) *
                lhsValues.at(mObj.components.voltInd.at(j.index).curNRow) -
            VB;
        for (const auto &m : mObj.components.voltInd.at(j.index).mut) {
          RHSvalue =
              RHSvalue -
              (m.second *
               lhsValues.at(mObj.components.voltInd.at(m.first).curNRow));
        }
      } else if (j.type == RowDescriptor::Type::VoltageJJ) {
        RHSvalue = mObj.components.voltJJ.at(j.index).pn1 +
                   hn_2_2e_hbar * mObj.components.voltJJ.at(j.index).vn1;
      } else if (j.type == RowDescriptor::Type::VoltageVS) {
        RHSvalue = mObj.sources.at(j.index).at(i);
      } else if (j.type == RowDescriptor::Type::VoltageTX1) {
        if (i >= mObj.components.txLine.at(j.index).k) {
          if (mObj.components.txLine.at(j.index).posN2Row == -1)
            VB = -results.xVect
                      .at(std::distance(
                          mObj.relXInd.begin(),
                          std::find(
                              mObj.relXInd.begin(), mObj.relXInd.end(),
                              mObj.components.txLine.at(j.index).negN2Row)))
                      .at(i - mObj.components.txLine.at(j.index).k);
          else if (mObj.components.txLine.at(j.index).negN2Row == -1)
            VB =
                results.xVect
                    .at(std::distance(
                        mObj.relXInd.begin(),
                        std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                                  mObj.components.txLine.at(j.index).posN2Row)))
                    .at(i - mObj.components.txLine.at(j.index).k);
          else
            VB =
                results.xVect
                    .at(std::distance(
                        mObj.relXInd.begin(),
                        std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                                  mObj.components.txLine.at(j.index).posN2Row)))
                    .at(i - mObj.components.txLine.at(j.index).k) -
                results.xVect
                    .at(std::distance(
                        mObj.relXInd.begin(),
                        std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                                  mObj.components.txLine.at(j.index).negN2Row)))
                    .at(i - mObj.components.txLine.at(j.index).k);
          RHSvalue =
              mObj.components.txLine.at(j.index).value *
                  results.xVect
                      .at(std::distance(
                          mObj.relXInd.begin(),
                          std::find(
                              mObj.relXInd.begin(), mObj.relXInd.end(),
                              mObj.components.txLine.at(j.index).curN2Row)))
                      .at(i - mObj.components.txLine.at(j.index).k) +
              VB;
        }
      } else if (j.type == RowDescriptor::Type::VoltageTX2) {
        if (i >= mObj.components.txLine.at(j.index).k) {
          if (mObj.components.txLine.at(j.index).posNRow == -1)
            VB =
                -results.xVect
                     .at(std::distance(
                         mObj.relXInd.begin(),
                         std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                                   mObj.components.txLine.at(j.index).negNRow)))
                     .at(i - mObj.components.txLine.at(j.index).k);
          else if (mObj.components.txLine.at(j.index).negNRow == -1)
            VB = results.xVect
                     .at(std::distance(
                         mObj.relXInd.begin(),
                         std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                                   mObj.components.txLine.at(j.index).posNRow)))
                     .at(i - mObj.components.txLine.at(j.index).k);
          else
            VB = results.xVect
                     .at(std::distance(
                         mObj.relXInd.begin(),
                         std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                                   mObj.components.txLine.at(j.index).posNRow)))
                     .at(i - mObj.components.txLine.at(j.index).k) -
                 results.xVect
                     .at(std::distance(
                         mObj.relXInd.begin(),
                         std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                                   mObj.components.txLine.at(j.index).negNRow)))
                     .at(i - mObj.components.txLine.at(j.index).k);
          RHSvalue =
              mObj.components.txLine.at(j.index).value *
                  results.xVect
                      .at(std::distance(
                          mObj.relXInd.begin(),
                          std::find(
                              mObj.relXInd.begin(), mObj.relXInd.end(),
                              mObj.components.txLine.at(j.index).curN1Row)))
                      .at(i - mObj.components.txLine.at(j.index).k) +
              VB;
        }
      }
      RHS.push_back(RHSvalue);
      rowCounter++;
    }

    LHS_PRE = RHS;
    ok =
        klu_tsolve(Symbolic, Numeric, mObj.Nsize, 1, &LHS_PRE.front(), &Common);
    if (!ok) {
      Errors::simulation_errors(MATRIX_SINGULAR, "");
    }

    lhsValues = LHS_PRE;
    for (int m = 0; m < mObj.relXInd.size(); m++) {
      results.xVect.at(m).at(i) = lhsValues.at(mObj.relXInd.at(m));
    }

    /* Guess next junction voltage */
    for (int j = 0; j < mObj.components.voltJJ.size(); j++) {
      jj_volt &thisJunction = mObj.components.voltJJ.at(j);
      if (thisJunction.posNRow == -1)
        thisJunction.vn1 = (-lhsValues.at(thisJunction.negNRow));
      else if (thisJunction.negNRow == -1)
        thisJunction.vn1 = (lhsValues.at(thisJunction.posNRow));
      else
        thisJunction.vn1 = (lhsValues.at(thisJunction.posNRow) -
                            lhsValues.at(thisJunction.negNRow));
      if (i <= 3)
        thisJunction.dVn1 = 0;
      else
        thisJunction.dVn1 =
            (2 / iObj.transSim.prstep) * (thisJunction.vn1 - thisJunction.vn2) -
            thisJunction.dVn2;
      thisJunction.v0 =
          thisJunction.vn1 + iObj.transSim.prstep * thisJunction.dVn1;
      if (thisJunction.rType == 1) {
        if (fabs(thisJunction.v0) < thisJunction.lowerB) {
          thisJunction.iT = 0.0;
          if (thisJunction.ppPtr != -1) {
            if (mObj.mElements.at(thisJunction.ppPtr).value !=
                thisJunction.subCond) {
              mObj.mElements.at(thisJunction.ppPtr).value =
                  thisJunction.subCond;
              needsLU = true;
            }
          }
          if (thisJunction.nnPtr != -1) {
            if (mObj.mElements.at(thisJunction.nnPtr).value !=
                thisJunction.subCond) {
              mObj.mElements.at(thisJunction.nnPtr).value =
                  thisJunction.subCond;
              needsLU = true;
            }
          }
          if (thisJunction.pnPtr != -1) {
            if (mObj.mElements.at(thisJunction.pnPtr).value !=
                -thisJunction.subCond) {
              mObj.mElements.at(thisJunction.pnPtr).value =
                  -thisJunction.subCond;
              needsLU = true;
            }
          }
          if (thisJunction.npPtr != -1) {
            if (mObj.mElements.at(thisJunction.npPtr).value !=
                -thisJunction.subCond) {
              mObj.mElements.at(thisJunction.npPtr).value =
                  -thisJunction.subCond;
              needsLU = true;
            }
          }
        } else if (fabs(thisJunction.v0) < thisJunction.upperB) {
          if (thisJunction.v0 < 0)
            thisJunction.iT = -thisJunction.lowerB *
                              ((1 / thisJunction.r0) - thisJunction.gLarge);
          else
            thisJunction.iT = thisJunction.lowerB *
                              ((1 / thisJunction.r0) - thisJunction.gLarge);
          if (thisJunction.ppPtr != -1) {
            if (mObj.mElements.at(thisJunction.ppPtr).value !=
                thisJunction.transCond) {
              mObj.mElements.at(thisJunction.ppPtr).value =
                  thisJunction.transCond;
              needsLU = true;
            }
          }
          if (thisJunction.nnPtr != -1) {
            if (mObj.mElements.at(thisJunction.nnPtr).value !=
                thisJunction.transCond) {
              mObj.mElements.at(thisJunction.nnPtr).value =
                  thisJunction.transCond;
              needsLU = true;
            }
          }
          if (thisJunction.pnPtr != -1) {
            if (mObj.mElements.at(thisJunction.pnPtr).value !=
                thisJunction.transCond) {
              mObj.mElements.at(thisJunction.pnPtr).value =
                  -thisJunction.transCond;
              needsLU = true;
            }
          }
          if (thisJunction.npPtr != -1) {
            if (mObj.mElements.at(thisJunction.npPtr).value !=
                thisJunction.transCond) {
              mObj.mElements.at(thisJunction.npPtr).value =
                  -thisJunction.transCond;
              needsLU = true;
            }
          }
        } else {
          if (thisJunction.v0 < 0)
            thisJunction.iT = -(thisJunction.iC / thisJunction.iCFact +
                                thisJunction.vG * (1 / thisJunction.r0) -
                                thisJunction.lowerB * (1 / thisJunction.rN));
          else
            thisJunction.iT = (thisJunction.iC / thisJunction.iCFact +
                               thisJunction.vG * (1 / thisJunction.r0) -
                               thisJunction.lowerB * (1 / thisJunction.rN));
          if (thisJunction.ppPtr != -1) {
            if (mObj.mElements.at(thisJunction.ppPtr).value !=
                thisJunction.normalCond) {
              mObj.mElements.at(thisJunction.ppPtr).value =
                  thisJunction.normalCond;
              needsLU = true;
            }
          }
          if (thisJunction.nnPtr != -1) {
            if (mObj.mElements.at(thisJunction.nnPtr).value !=
                thisJunction.normalCond) {
              mObj.mElements.at(thisJunction.nnPtr).value =
                  thisJunction.normalCond;
              needsLU = true;
            }
          }
          if (thisJunction.pnPtr != -1) {
            if (mObj.mElements.at(thisJunction.pnPtr).value !=
                thisJunction.normalCond) {
              mObj.mElements.at(thisJunction.pnPtr).value =
                  -thisJunction.normalCond;
              needsLU = true;
            }
          }
          if (thisJunction.npPtr != -1) {
            if (mObj.mElements.at(thisJunction.npPtr).value !=
                thisJunction.normalCond) {
              mObj.mElements.at(thisJunction.npPtr).value =
                  -thisJunction.normalCond;
              needsLU = true;
            }
          }
        }
      }
      thisJunction.pn1 = lhsValues.at(thisJunction.phaseNRow);
      thisJunction.phi0 = thisJunction.pn1 +
                          (hn_2_2e_hbar) * (thisJunction.vn1 + thisJunction.v0);
      // thisJunction.iS = -thisJunction.iC * sin(thisJunction.phi0) + (((2 *
      // thisJunction.C) / iObj.transSim.prstep)*thisJunction.vn1) +
      // (thisJunction.C * thisJunction.dVn1) - thisJunction.iT;
      thisJunction.iS =
          -((M_PI * thisJunction.Del) / (2 * EV * thisJunction.rNCalc)) *
              (sin(thisJunction.phi0) /
               sqrt(1 - thisJunction.D * (sin(thisJunction.phi0 / 2) *
                                          sin(thisJunction.phi0 / 2)))) *
              tanh((thisJunction.Del) / (2 * BOLTZMANN * thisJunction.T) *
                   sqrt(1 - thisJunction.D * (sin(thisJunction.phi0 / 2) *
                                              sin(thisJunction.phi0 / 2)))) +
          (((2 * thisJunction.C) / iObj.transSim.prstep) * thisJunction.vn1) +
          (thisJunction.C * thisJunction.dVn1) - thisJunction.iT;
      thisJunction.vn2 = thisJunction.vn1;
      thisJunction.dVn2 = thisJunction.dVn1;
      thisJunction.pn2 = thisJunction.pn1;
      // mObj.components.voltJJ.at(j.first) = thisJunction;
      mObj.components.voltJJ.at(j).jjCur.push_back(thisJunction.iS);
    }
    if (needsLU) {
      mObj.create_CSR();

      // TODO: Maybe use refactor
      klu_free_numeric(&Numeric, &Common);
      Numeric = klu_factor(&mObj.rowptr.front(), &mObj.colind.front(),
                           &mObj.nzval.front(), Symbolic, &Common);
      needsLU = false;
    }
    results.timeAxis.push_back(i * iObj.transSim.prstep);
    old_progress = progress;
    incremental_progress = incremental_progress + increments;
    progress = (int)(incremental_progress);
#ifndef NO_PRINT
    if (progress > old_progress) {
      if (sOutput)
        std::cout << std::setw(3) << std::right << std::fixed
                  << std::setprecision(0) << progress << "%";
      pBar = "[";
      for (int p = 0; p <= (int)(progress_increments * i); p++) {
        pBar.append("=");
      }
      if (sOutput)
        std::cout << std::setw(31) << std::left << pBar << "]" << std::flush;
    }
#endif
  }
#ifndef NO_PRINT
  if (sOutput)
    std::cout << "\r" << std::setw(3) << std::right << std::fixed
              << std::setprecision(0) << 100 << "%" << std::setw(31)
              << std::left << pBar << "]\n" << std::flush;
#else
  if (sOutput)
    std::cout << " done" << std::flush << std::endl;
#endif

  klu_free_symbolic(&Symbolic, &Common);
  klu_free_numeric(&Numeric, &Common);
}

void Simulation::transient_phase_simulation(Input &iObj, Matrix &mObj) {
  results.xVect.clear();
  results.timeAxis.clear();
  std::vector<double> lhsValues(mObj.Nsize, 0.0),
      RHS(mObj.columnNames.size(), 0.0), LHS_PRE;
  int simSize = iObj.transSim.simsize();
  for (int m = 0; m < mObj.relXInd.size(); m++) {
    results.xVect.emplace_back(std::vector<double>(simSize, 0.0));
  }
  // std::vector<std::vector<std::string>> nodeConnectionVector(
  //     mObj.rowNames.size());
  std::string currentLabel;
  double RHSvalue;
  double hn_2_2e_hbar = (iObj.transSim.prstep / 2) * (2 * M_PI / PHI_ZERO);
  int ok, rowCounter;
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
  rowCounter = 0;
  // for (auto j : mObj.rowNames) {
  // 	if (j[2] == 'N') {
  // 		nodeConnectionVector[rowCounter] = mObj.nodeConnections[j];
  // 	}
  // 	rowCounter++;
  // }
  if (sOutput)
    std::cout << "Simulating:" << std::endl;
  double increments = 100 / (double)simSize;
  double progress_increments = 30 / (double)simSize;
  double incremental_progress = 0.0;

  int progress = 0;
  int old_progress = 0;
  int imintd = 0;
  std::string pBar;
  double mutualL = 0.0;
  double CUR2 = 0.0;
  for (int i = 0; i < simSize; i++) {
#ifndef NO_PRINT
    if (sOutput)
      std::cout << '\r' << std::flush;
#endif
    RHS.clear();
    rowCounter = 0;
    for (auto j : mObj.rowDesc) {
      RHSvalue = 0.0;
      if (j.type == RowDescriptor::Type::PhaseNode) {
        for (auto k : mObj.nodeConnections.at(j.index).connections) {
          if (k.type == ComponentConnections::Type::JJP) {
            RHSvalue += mObj.components.phaseJJ.at(k.index).iS;
          } else if (k.type == ComponentConnections::Type::JJN) {
            RHSvalue -= mObj.components.phaseJJ.at(k.index).iS;
          } else if (k.type == ComponentConnections::Type::CSN) {
            RHSvalue += mObj.sources.at(k.index).at(i);
          } else if (k.type == ComponentConnections::Type::CSP) {
            RHSvalue -= mObj.sources.at(k.index).at(i);
          }
        }
      } else if (j.type == RowDescriptor::Type::PhaseResistor) {
        if (mObj.components.phaseRes.at(j.index).posNRow == -1) {
          mObj.components.phaseRes.at(j.index).pn1 =
              -lhsValues.at(mObj.components.phaseRes.at(j.index).negNRow);
        } else if (mObj.components.phaseRes.at(j.index).negNRow == -1) {
          mObj.components.phaseRes.at(j.index).pn1 =
              lhsValues.at(mObj.components.phaseRes.at(j.index).posNRow);
        } else {
          mObj.components.phaseRes.at(j.index).pn1 =
              lhsValues.at(mObj.components.phaseRes.at(j.index).posNRow) -
              lhsValues.at(mObj.components.phaseRes.at(j.index).negNRow);
        }
        mObj.components.phaseRes[j.index].IRn1 =
            lhsValues.at(mObj.components.phaseRes.at(j.index).curNRow);
        RHSvalue += ((M_PI * mObj.components.phaseRes.at(j.index).value *
                      iObj.transSim.prstep) /
                     PHI_ZERO) *
                        mObj.components.phaseRes.at(j.index).IRn1 +
                    mObj.components.phaseRes.at(j.index).pn1;
      } else if (j.type == RowDescriptor::Type::PhaseJJ) {
        RHSvalue = mObj.components.phaseJJ.at(j.index).pn1 +
                   hn_2_2e_hbar * mObj.components.phaseJJ.at(j.index).vn1;
      } else if (j.type == RowDescriptor::Type::PhaseCapacitor) {
        RHSvalue =
            -((2 * M_PI * iObj.transSim.prstep * iObj.transSim.prstep) /
              (4 * PHI_ZERO * mObj.components.phaseCap.at(j.index).value)) *
                mObj.components.phaseCap.at(j.index).ICn1 -
            mObj.components.phaseCap.at(j.index).pn1 -
            (iObj.transSim.prstep * mObj.components.phaseCap.at(j.index).dPn1);
      } else if (j.type == RowDescriptor::Type::PhaseVS) {
        if (mObj.components.phaseVs.at(j.index).posNRow == -1.0)
          mObj.components.phaseVs.at(j.index).pn1 =
              -lhsValues.at(mObj.components.phaseVs.at(j.index).negNRow);
        else if (mObj.components.phaseVs.at(j.index).negNRow == -1.0)
          mObj.components.phaseVs.at(j.index).pn1 =
              lhsValues.at(mObj.components.phaseVs.at(j.index).posNRow);
        else
          mObj.components.phaseVs.at(j.index).pn1 =
              lhsValues.at(mObj.components.phaseVs.at(j.index).posNRow) -
              lhsValues.at(mObj.components.phaseVs.at(j.index).negNRow);
        if (i >= 1)
          RHSvalue =
              mObj.components.phaseVs.at(j.index).pn1 +
              ((iObj.transSim.prstep * M_PI) / PHI_ZERO) *
                  (mObj.sources.at(mObj.components.phaseVs.at(j.index).sourceDex).at(i)
                  + mObj.sources.at(mObj.components.phaseVs.at(j.index).sourceDex).at(i - 1));
        else if (i == 0)
          RHSvalue = mObj.components.phaseVs.at(j.index).pn1 +
                     ((iObj.transSim.prstep * M_PI) / PHI_ZERO) *
                         mObj.sources.at(mObj.components.phaseVs.at(j.index).sourceDex).at(i);
      } else if (j.type == RowDescriptor::Type::PhasePS) {
        RHSvalue = mObj.sources.at(j.index).at(i);
      } else if (j.type == RowDescriptor::Type::PhaseTX1) {
        if (i > mObj.components.txPhase.at(j.index).k) {
          RHSvalue =
              ((iObj.transSim.prstep * M_PI *
                mObj.components.txPhase.at(j.index).value) /
               (PHI_ZERO)) *
                  results.xVect
                      .at(std::distance(
                          mObj.relXInd.begin(),
                          std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                                    mObj.components.txPhase.at(j.index).curN2Row)))
                      .at(i - mObj.components.txPhase.at(j.index).k) +
              mObj.components.txPhase.at(j.index).p1n1 +
              (iObj.transSim.prstep / 2) *
                  (mObj.components.txPhase.at(j.index).dP1n1 +
                   mObj.components.txPhase.at(j.index).dP2nk);
        }
      } else if (j.type == RowDescriptor::Type::PhaseTX2) {
        if (i > mObj.components.txPhase.at(j.index).k) {
          RHSvalue =
              ((iObj.transSim.prstep * M_PI *
                mObj.components.txPhase.at(j.index).value) /
               (PHI_ZERO)) *
                  results.xVect
                      .at(std::distance(
                          mObj.relXInd.begin(),
                          std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                                    mObj.components.txPhase.at(j.index).curN1Row)))
                      .at(i - mObj.components.txPhase.at(j.index).k) +
              mObj.components.txPhase.at(j.index).p2n1 +
              (iObj.transSim.prstep / 2) *
                  (mObj.components.txPhase.at(j.index).dP2n1 +
                   mObj.components.txPhase.at(j.index).dP1nk);
        }
      }
      RHS.push_back(RHSvalue);
      rowCounter++;
    }
    LHS_PRE = RHS;
    ok =
        klu_tsolve(Symbolic, Numeric, mObj.Nsize, 1, &LHS_PRE.front(), &Common);
    if (!ok) {
      Errors::simulation_errors(MATRIX_SINGULAR, "");
    }

    lhsValues = LHS_PRE;
    for (int m = 0; m < mObj.relXInd.size(); m++) {
      results.xVect.at(m).at(i) = lhsValues.at(mObj.relXInd.at(m));
    }

    for (int j = 0; j < mObj.components.phaseJJ.size(); j++) {
      jj_phase &thisJJ = mObj.components.phaseJJ.at(j);
      if (thisJJ.posNRow == -1)
        thisJJ.pn1 = (-lhsValues.at(thisJJ.negNRow));
      else if (thisJJ.negNRow == -1)
        thisJJ.pn1 = (lhsValues.at(thisJJ.posNRow));
      else
        thisJJ.pn1 =
            (lhsValues.at(thisJJ.posNRow) - lhsValues.at(thisJJ.negNRow));
      thisJJ.vn1 = (lhsValues.at(thisJJ.voltNRow));
      thisJJ.dVn1 =
          (2 / iObj.transSim.prstep) * (thisJJ.vn1 - thisJJ.vn2) - thisJJ.dVn2;
      thisJJ.vn2 = thisJJ.vn1;
      thisJJ.dVn2 = thisJJ.dVn1;
      thisJJ.v0 = thisJJ.vn1 + iObj.transSim.prstep * thisJJ.dVn1;
      thisJJ.dPn1 =
          (2 / iObj.transSim.prstep) * (thisJJ.pn1 - thisJJ.pn2) - thisJJ.dPn2;
      thisJJ.phi0 = thisJJ.pn1 + hn_2_2e_hbar * (thisJJ.vn1 + thisJJ.v0);
      thisJJ.pn2 = thisJJ.pn1;
      thisJJ.dPn2 = thisJJ.dPn1;
      if (thisJJ.rType == 1) {
        if (fabs(thisJJ.v0) < thisJJ.lower) {
          thisJJ.It = 0.0;
          if (thisJJ.pPtr != -1) {
            if (mObj.mElements[thisJJ.pPtr].value != thisJJ.subCond) {
              mObj.mElements[thisJJ.pPtr].value = thisJJ.subCond;
              needsLU = true;
            }
          }
          if (thisJJ.nPtr != -1) {
            if (mObj.mElements[thisJJ.nPtr].value != -thisJJ.subCond) {
              mObj.mElements[thisJJ.nPtr].value = -thisJJ.subCond;
              needsLU = true;
            }
          }
        } else if (fabs(thisJJ.v0) < thisJJ.upper) {
          if (thisJJ.v0 < 0)
            thisJJ.It = -thisJJ.vG * ((1 / thisJJ.r0) - thisJJ.gLarge);
          else
            thisJJ.It = thisJJ.vG * ((1 / thisJJ.r0) - thisJJ.gLarge);
          if (thisJJ.pPtr != -1) {
            if (mObj.mElements[thisJJ.pPtr].value != thisJJ.transCond) {
              mObj.mElements[thisJJ.pPtr].value = thisJJ.transCond;
              needsLU = true;
            }
          }
          if (thisJJ.nPtr != -1) {
            if (mObj.mElements[thisJJ.nPtr].value != -thisJJ.transCond) {
              mObj.mElements[thisJJ.nPtr].value = -thisJJ.transCond;
              needsLU = true;
            }
          }
        } else {
          if (thisJJ.v0 < 0)
            thisJJ.It = -((thisJJ.iC / thisJJ.iCFact) +
                          ((1 / thisJJ.r0) * thisJJ.lower) -
                          ((1 / thisJJ.rN) * thisJJ.upper));
          else
            thisJJ.It = ((thisJJ.iC / thisJJ.iCFact) +
                         ((1 / thisJJ.r0) * thisJJ.lower) -
                         ((1 / thisJJ.rN) * thisJJ.upper));
          if (thisJJ.pPtr != -1) {
            if (mObj.mElements[thisJJ.pPtr].value != thisJJ.normalCond) {
              mObj.mElements[thisJJ.pPtr].value = thisJJ.normalCond;
              needsLU = true;
            }
          }
          if (thisJJ.nPtr != -1) {
            if (mObj.mElements[thisJJ.nPtr].value != -thisJJ.normalCond) {
              mObj.mElements[thisJJ.nPtr].value = -thisJJ.normalCond;
              needsLU = true;
            }
          }
        }
      }
      // thisJJ.iS = -thisJJ.iC * sin(thisJJ.phi0) + ((2 * thisJJ.C) /
      // iFile.tsim.prstep) * thisJJ.vn1 + thisJJ.C * thisJJ.dVn1 - thisJJ.It;
      thisJJ.iS =
          -((M_PI * thisJJ.Del) / (2 * EV * thisJJ.rNCalc)) *
              (sin(thisJJ.phi0) / sqrt(1 - thisJJ.D * (sin(thisJJ.phi0 / 2) *
                                                       sin(thisJJ.phi0 / 2)))) *
              tanh((thisJJ.Del) / (2 * BOLTZMANN * thisJJ.T) *
                   sqrt(1 - thisJJ.D * (sin(thisJJ.phi0 / 2) *
                                        sin(thisJJ.phi0 / 2)))) +
          (((2 * thisJJ.C) / iObj.transSim.prstep) * thisJJ.vn1) +
          (thisJJ.C * thisJJ.dVn1) - thisJJ.It;
      mObj.components.phaseJJ.at(j).jjCur.push_back(thisJJ.iS);
    }
    /* Calculate next Cap values */
    for (int j = 0; j < mObj.components.phaseCap.size(); j++) {
      cap_phase &thisCap = mObj.components.phaseCap.at(j);
      thisCap.pn2 = thisCap.pn1;
      if (thisCap.posNRow == -1)
        thisCap.pn1 = (-lhsValues.at(thisCap.negNRow));
      else if (thisCap.negNRow == -1)
        thisCap.pn1 = (lhsValues.at(thisCap.posNRow));
      else
        thisCap.pn1 =
            (lhsValues.at(thisCap.posNRow) - lhsValues.at(thisCap.negNRow));
      thisCap.ICn1 = lhsValues.at(thisCap.curNRow);
      thisCap.dPn1 = (2 / iObj.transSim.prstep) * (thisCap.pn1 - thisCap.pn2) -
                     thisCap.dPn2;
      thisCap.dPn2 = thisCap.dPn1;
    }
    /* Calculate next TL values */
    for (int j = 0; j < mObj.components.txPhase.size(); j++) {
      tx_phase &thisTL = mObj.components.txPhase.at(j);
      if (thisTL.posNRow == -1) {
        thisTL.p1n1 = (-lhsValues.at(thisTL.negNRow));
      } else if (thisTL.negNRow == -1) {
        thisTL.p1n1 = (lhsValues.at(thisTL.posNRow));
      } else {
        thisTL.p1n1 =
            (lhsValues.at(thisTL.posNRow) - lhsValues.at(thisTL.negNRow));
      }
      if (thisTL.posN2Row == -1) {
        thisTL.p2n1 = (-lhsValues.at(thisTL.negN2Row));
      } else if (thisTL.negN2Row == -1) {
        thisTL.p2n1 = (lhsValues.at(thisTL.posN2Row));
      } else {
        thisTL.p2n1 =
            (lhsValues.at(thisTL.posN2Row) - lhsValues.at(thisTL.negN2Row));
      }
      thisTL.dP1n1 = (2 / iObj.transSim.prstep) * (thisTL.p1n1 - thisTL.p1n2) -
                     thisTL.dP1n2;
      thisTL.p1n2 = thisTL.p1n1;
      thisTL.dP1n2 = thisTL.dP1n1;
      thisTL.dP2n1 = (2 / iObj.transSim.prstep) * (thisTL.p2n1 - thisTL.p2n2) -
                     thisTL.dP2n2;
      thisTL.p2n2 = thisTL.p2n1;
      thisTL.dP2n2 = thisTL.dP2n1;
      if (i >= thisTL.k) {
        if (thisTL.posNRow == -1) {
          thisTL.p1nk = -results.xVect
                             .at(std::distance(mObj.relXInd.begin(),
                                               std::find(mObj.relXInd.begin(),
                                                         mObj.relXInd.end(),
                                                         thisTL.negNRow)))
                             .at(i - thisTL.k);
        } else if (thisTL.negNRow == -1) {
          thisTL.p1nk = results.xVect
                            .at(std::distance(mObj.relXInd.begin(),
                                              std::find(mObj.relXInd.begin(),
                                                        mObj.relXInd.end(),
                                                        thisTL.posNRow)))
                            .at(i - thisTL.k);
        } else {
          thisTL.p1nk = results.xVect
                            .at(std::distance(mObj.relXInd.begin(),
                                              std::find(mObj.relXInd.begin(),
                                                        mObj.relXInd.end(),
                                                        thisTL.posNRow)))
                            .at(i - thisTL.k) -
                        results.xVect
                            .at(std::distance(mObj.relXInd.begin(),
                                              std::find(mObj.relXInd.begin(),
                                                        mObj.relXInd.end(),
                                                        thisTL.negNRow)))
                            .at(i - thisTL.k);
        }
        if (thisTL.posN2Row == -1) {
          thisTL.p2nk = -results.xVect
                             .at(std::distance(mObj.relXInd.begin(),
                                               std::find(mObj.relXInd.begin(),
                                                         mObj.relXInd.end(),
                                                         thisTL.negN2Row)))
                             .at(i - thisTL.k);
        } else if (thisTL.negN2Row == -1) {
          thisTL.p2nk = results.xVect
                            .at(std::distance(mObj.relXInd.begin(),
                                              std::find(mObj.relXInd.begin(),
                                                        mObj.relXInd.end(),
                                                        thisTL.posN2Row)))
                            .at(i - thisTL.k);
        } else {
          thisTL.p2nk = results.xVect
                            .at(std::distance(mObj.relXInd.begin(),
                                              std::find(mObj.relXInd.begin(),
                                                        mObj.relXInd.end(),
                                                        thisTL.posN2Row)))
                            .at(i - thisTL.k) -
                        results.xVect
                            .at(std::distance(mObj.relXInd.begin(),
                                              std::find(mObj.relXInd.begin(),
                                                        mObj.relXInd.end(),
                                                        thisTL.negN2Row)))
                            .at(i - thisTL.k);
        }
        thisTL.dP1nk =
            (2 / iObj.transSim.prstep) * (thisTL.p1nk - thisTL.p1nk1) -
            thisTL.dP1nk1;
        thisTL.p1nk1 = thisTL.p1nk;
        thisTL.dP1nk1 = thisTL.dP1nk;
        thisTL.dP2nk =
            (2 / iObj.transSim.prstep) * (thisTL.p2nk - thisTL.p2nk1) -
            thisTL.dP2nk1;
        thisTL.p2nk1 = thisTL.p2nk;
        thisTL.dP2nk1 = thisTL.dP2nk;
      }
    }
    if (needsLU) {
      mObj.create_CSR();
      // TODO: Maybe use refactor
      klu_free_numeric(&Numeric, &Common);
      Numeric = klu_factor(&mObj.rowptr.front(), &mObj.colind.front(),
                           &mObj.nzval.front(), Symbolic, &Common);
      needsLU = false;
    }
    /* Add the current time value to the time axis for plotting purposes */
    results.timeAxis.push_back(i * iObj.transSim.prstep);
    old_progress = progress;
    incremental_progress = incremental_progress + increments;
    progress = (int)(incremental_progress);
#ifndef NO_PRINT
    if (progress > old_progress) {
      if (sOutput)
        std::cout << std::setw(3) << std::right << std::fixed
                  << std::setprecision(0) << progress << "%";
      pBar = "[";
      for (int p = 0; p <= (int)(progress_increments * i); p++) {
        pBar.append("=");
      }
      if (sOutput)
        std::cout << std::setw(31) << std::left << pBar << "]" << std::flush;
    }
#endif
  }
#ifndef NO_PRINT
  if (sOutput)
    std::cout << "\r" << std::setw(3) << std::right << std::fixed
              << std::setprecision(0) << 100 << "%" << std::setw(31)
              << std::left << pBar << "]\n" << std::flush;
#else
  if (sOutput)
    std::cout << " done" << std::flush << std::endl;
#endif

  klu_free_symbolic(&Symbolic, &Common);
  klu_free_numeric(&Numeric, &Common);
}
