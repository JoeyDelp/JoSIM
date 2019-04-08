// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#include "JoSIM/j_simulation.h"

#include <cassert>

void
Simulation::identify_simulation(std::vector<std::string> controls,
						double &prstep,
						double &tstop,
						double &tstart,
						double &maxtstep) {
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
				}
				else {
					prstep = Misc::modifier(simtokens[1]);
					if (simtokens.size() > 2) {
						tstop = Misc::modifier(simtokens[2]);
						if (simtokens.size() > 3) {
							tstart = Misc::modifier(simtokens[3]);
							if (simtokens.size() > 4) {
								maxtstep = Misc::modifier(simtokens[4]);
							}
							else maxtstep = 1E-12;
						}
						else {
							tstart = 0;
							maxtstep = 1E-12;
						}
					}
					else {
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
	for (int m = 0; m < mObj.Nsize; m++) {
		results.xVect.emplace_back(std::vector<double>(simSize, 0.0));
	}
	std::vector<std::vector<std::string>> nodeConnectionVector(mObj.rowNames.size());
	std::string currentLabel, txCurrent;
	double VB, RHSvalue, inductance, z0voltage;
	double hn_2_2e_hbar = (iObj.transSim.prstep / 2)*(2 * M_PI / PHI_ZERO);
	int ok, rowCounter;
	bool needsLU = false;
	klu_symbolic * Symbolic;
	klu_common Common;
	klu_numeric * Numeric;
	ok = klu_defaults(&Common);
	Symbolic = klu_analyze(mObj.Nsize, &mObj.rowptr.front(), &mObj.colind.front(), &Common);
	Numeric = klu_factor(&mObj.rowptr.front(), &mObj.colind.front(), &mObj.nzval.front(), Symbolic, &Common);
	rowCounter = 0;
	for (const auto &j : mObj.rowNames) {

    assert(j.size() >= 3);

		if (j[2] == 'N') {
			nodeConnectionVector[rowCounter] = mObj.nodeConnections[j];
		}
		rowCounter++;
	}
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
			std::cout << '\r';
	#endif
		RHS.clear();
		rowCounter = 0;
		for (auto j : mObj.rowNames) {
			RHSvalue = 0.0;
			if (j[2] == 'N') {
				for (auto k : nodeConnectionVector[rowCounter]) {
					if (k[0] == 'B') {
						if (j == mObj.components.voltJJ.at(k).posNodeR)
							RHSvalue += mObj.components.voltJJ.at(k).iS;
						else
							RHSvalue -= mObj.components.voltJJ.at(k).iS;
					}
					else if (k[0] == 'I') {
						RHSvalue += mObj.sources.at(k).at(i);
					}
					else if (k[0] == 'R') {
						if(mObj.components.voltRes.at(k).posNRow == rowCounter){
						}
						else if(mObj.components.voltRes.at(k).negNRow == rowCounter){
						}
					}
				}
			}
			else if (j[2] == 'L') {
				currentLabel = j.substr(2);
				if (mObj.components.voltInd.at(currentLabel).posNRow == -1)
					VB = -lhsValues.at(mObj.components.voltInd.at(currentLabel).negNRow);
				else if (mObj.components.voltInd.at(currentLabel).negNRow == -1)
					VB = lhsValues.at(mObj.components.voltInd.at(currentLabel).posNRow);
				else VB = lhsValues.at(mObj.components.voltInd.at(currentLabel).posNRow) -
					lhsValues.at(mObj.components.voltInd.at(currentLabel).negNRow);
				RHSvalue = (-2 * mObj.components.voltInd.at(currentLabel).value / iObj.transSim.prstep) *
					lhsValues.at(mObj.components.voltInd.at(currentLabel).curNRow) - VB;
				for (const auto& m : mObj.components.voltInd.at(currentLabel).mut) {
					RHSvalue = RHSvalue - ( m.second * lhsValues.at(mObj.components.voltInd.at(m.first).curNRow));
				}
			}
			else if (j[2] == 'B') {
				currentLabel = j.substr(2);
				RHSvalue = mObj.components.voltJJ.at(currentLabel).pn1 + hn_2_2e_hbar * mObj.components.voltJJ.at(currentLabel).vn1;
			}
			else if (j[2] == 'V') {
				currentLabel = j.substr(2);
				RHSvalue = mObj.sources.at(currentLabel).at(i);
			}
			else if (j[2] == 'T') {
				currentLabel = j.substr(2, j.size() - 5);
				txCurrent = j.substr(j.size() - 2, j.size() - 1);
				if(i >= mObj.components.txLine.at(currentLabel).k) {
					if (txCurrent == "I1") {
						if (mObj.components.txLine.at(currentLabel).posN2Row == -1)
							VB = -results.xVect.at(
								mObj.components.txLine.at(currentLabel).negN2Row).at(
									i - mObj.components.txLine.at(currentLabel).k);
						else if (mObj.components.txLine.at(currentLabel).negN2Row == -1)
							VB = results.xVect.at(
								mObj.components.txLine.at(currentLabel).posN2Row).at(
									i - mObj.components.txLine.at(currentLabel).k);
						else VB = results.xVect.at(
							mObj.components.txLine.at(currentLabel).posN2Row).at(
									i - mObj.components.txLine.at(currentLabel).k) -
							results.xVect.at(
								mObj.components.txLine.at(currentLabel).negN2Row).at(
									i - mObj.components.txLine.at(currentLabel).k);
						RHSvalue = mObj.components.txLine.at(currentLabel).value *
							results.xVect.at(
								mObj.components.txLine.at(currentLabel).curN2Row).at(
									i - mObj.components.txLine.at(currentLabel).k) + VB;
					}
					else if (txCurrent == "I2") {
						if (mObj.components.txLine.at(currentLabel).posNRow == -1)
							VB = -results.xVect.at(
								mObj.components.txLine.at(currentLabel).negNRow).at(
									i - mObj.components.txLine.at(currentLabel).k);
						else if (mObj.components.txLine.at(currentLabel).negNRow == -1)
							VB = results.xVect.at(
								mObj.components.txLine.at(currentLabel).posNRow).at(
									i - mObj.components.txLine.at(currentLabel).k);
						else VB = results.xVect.at(
							mObj.components.txLine.at(currentLabel).posNRow).at(
								i - mObj.components.txLine.at(currentLabel).k) -
							results.xVect.at(
								mObj.components.txLine.at(currentLabel).negNRow).at(
									i - mObj.components.txLine.at(currentLabel).k);
						RHSvalue = mObj.components.txLine.at(currentLabel).value *
							results.xVect.at(
								mObj.components.txLine.at(currentLabel).curN1Row).at(
									i - mObj.components.txLine.at(currentLabel).k) + VB;
					}
				}
			}
			RHS.push_back(RHSvalue);
			rowCounter++;
		}

		LHS_PRE = RHS;
		ok = klu_tsolve(Symbolic, Numeric, mObj.Nsize, 1, &LHS_PRE.front(), &Common);
		if(!ok) {
			Errors::matrix_errors(MATRIX_SINGULAR, "");
		}

		lhsValues = LHS_PRE;
		for (int m = 0; m < lhsValues.size(); m++) {
			results.xVect[m][i] = lhsValues[m];
		}

		/* Guess next junction voltage */
		for (const auto& j : mObj.components.voltJJ) {
			jj_volt &thisJunction = mObj.components.voltJJ.at(j.first);
			if (thisJunction.posNRow == -1) thisJunction.vn1 = (-lhsValues.at(thisJunction.negNRow));
			else if (thisJunction.negNRow == -1) thisJunction.vn1 = (lhsValues.at(thisJunction.posNRow));
			else thisJunction.vn1 = (lhsValues.at(thisJunction.posNRow) - lhsValues.at(thisJunction.negNRow));
			if (i <= 3 ) thisJunction.dVn1 = 0;
			else thisJunction.dVn1 = (2 / iObj.transSim.prstep) * (thisJunction.vn1 - thisJunction.vn2) - thisJunction.dVn2;
			thisJunction.v0 = thisJunction.vn1 + iObj.transSim.prstep * thisJunction.dVn1;
			if (thisJunction.rType == 1) {
				if(fabs(thisJunction.v0) < thisJunction.lowerB) {
					thisJunction.iT = 0.0;
					if(thisJunction.ppPtr != -1) {
						if(mObj.mElements.at(thisJunction.ppPtr).value != thisJunction.subCond) {
							mObj.mElements.at(thisJunction.ppPtr).value = thisJunction.subCond;
							needsLU = true;
						}
					}
					if(thisJunction.nnPtr != -1) {
						if(mObj.mElements.at(thisJunction.nnPtr).value != thisJunction.subCond) {
							mObj.mElements.at(thisJunction.nnPtr).value = thisJunction.subCond;
							needsLU = true;
						}
					}
					if(thisJunction.pnPtr != -1) {
						if(mObj.mElements.at(thisJunction.pnPtr).value != -thisJunction.subCond) {
							mObj.mElements.at(thisJunction.pnPtr).value = -thisJunction.subCond;
							needsLU = true;
						}
					}
					if(thisJunction.npPtr != -1) {
						if(mObj.mElements.at(thisJunction.npPtr).value != -thisJunction.subCond) {
							mObj.mElements.at(thisJunction.npPtr).value = -thisJunction.subCond;
							needsLU = true;
						}
					}
				}
				else if(fabs(thisJunction.v0) < thisJunction.upperB) {
					if(thisJunction.v0 < 0) thisJunction.iT = -thisJunction.lowerB*((1/thisJunction.r0) - thisJunction.gLarge);
					else thisJunction.iT = thisJunction.lowerB*((1/thisJunction.r0) - thisJunction.gLarge);
					if(thisJunction.ppPtr != -1) {
						if(mObj.mElements.at(thisJunction.ppPtr).value != thisJunction.transCond) {
							mObj.mElements.at(thisJunction.ppPtr).value = thisJunction.transCond;
							needsLU = true;
						}
					}
					if(thisJunction.nnPtr != -1) {
						if(mObj.mElements.at(thisJunction.nnPtr).value != thisJunction.transCond) {
							mObj.mElements.at(thisJunction.nnPtr).value = thisJunction.transCond;
							needsLU = true;
						}
					}
					if(thisJunction.pnPtr != -1) {
						if(mObj.mElements.at(thisJunction.pnPtr).value != thisJunction.transCond) {
							mObj.mElements.at(thisJunction.pnPtr).value = -thisJunction.transCond;
							needsLU = true;
						}
					}
					if(thisJunction.npPtr != -1) {
						if(mObj.mElements.at(thisJunction.npPtr).value != thisJunction.transCond) {
							mObj.mElements.at(thisJunction.npPtr).value = -thisJunction.transCond;
							needsLU = true;
						}
					}
				}
				else {
					if(thisJunction.v0 < 0) thisJunction.iT = -(thisJunction.iC/thisJunction.iCFact + thisJunction.vG * (1/thisJunction.r0) - thisJunction.lowerB * (1/thisJunction.rN));
					else thisJunction.iT = (thisJunction.iC/thisJunction.iCFact + thisJunction.vG * (1/thisJunction.r0) - thisJunction.lowerB * (1/thisJunction.rN));
					if(thisJunction.ppPtr != -1) {
						if(mObj.mElements.at(thisJunction.ppPtr).value != thisJunction.normalCond) {
							mObj.mElements.at(thisJunction.ppPtr).value = thisJunction.normalCond;
							needsLU = true;
						}
					}
					if(thisJunction.nnPtr != -1) {
						if(mObj.mElements.at(thisJunction.nnPtr).value != thisJunction.normalCond) {
							mObj.mElements.at(thisJunction.nnPtr).value = thisJunction.normalCond;
							needsLU = true;
						}
					}
					if(thisJunction.pnPtr != -1) {
						if(mObj.mElements.at(thisJunction.pnPtr).value != thisJunction.normalCond) {
							mObj.mElements.at(thisJunction.pnPtr).value = -thisJunction.normalCond;
							needsLU = true;
						}
					}
					if(thisJunction.npPtr != -1) {
						if(mObj.mElements.at(thisJunction.npPtr).value != thisJunction.normalCond) {
							mObj.mElements.at(thisJunction.npPtr).value = -thisJunction.normalCond;
							needsLU = true;
						}
					}
				}
			}
			thisJunction.pn1 = lhsValues.at(thisJunction.phaseNRow);
			thisJunction.phi0 = thisJunction.pn1 + (hn_2_2e_hbar)*(thisJunction.vn1 + thisJunction.v0);
			//thisJunction.iS = -thisJunction.iC * sin(thisJunction.phi0) + (((2 * thisJunction.C) / iObj.transSim.prstep)*thisJunction.vn1) + (thisJunction.C * thisJunction.dVn1) - thisJunction.iT;
			thisJunction.iS = -((M_PI * thisJunction.Del) / (2 * EV * thisJunction.rNCalc)) * (sin(thisJunction.phi0)/sqrt(1 - thisJunction.D * (sin(thisJunction.phi0 / 2)*sin(thisJunction.phi0 / 2))))
								* tanh((thisJunction.Del)/(2*BOLTZMANN*thisJunction.T) * sqrt(1-thisJunction.D * (sin(thisJunction.phi0 / 2)*sin(thisJunction.phi0 / 2))))
								+ (((2 * thisJunction.C) / iObj.transSim.prstep)*thisJunction.vn1) + (thisJunction.C * thisJunction.dVn1) - thisJunction.iT;
			thisJunction.vn2 = thisJunction.vn1;
			thisJunction.dVn2 = thisJunction.dVn1;
			thisJunction.pn2 = thisJunction.pn1;
			//mObj.components.voltJJ.at(j.first) = thisJunction;
			mObj.components.voltJJ.at(j.first).jjCur.push_back(thisJunction.iS);
		}
		if(needsLU) {
				mObj.create_CSR();

        // TODO: Maybe use refactor
        klu_free_numeric(&Numeric, &Common);
				Numeric = klu_factor(&mObj.rowptr.front(), &mObj.colind.front(), &mObj.nzval.front(), Symbolic, &Common);
				needsLU  = false;
		}
		results.timeAxis.push_back(i*iObj.transSim.prstep);
		old_progress = progress;
		incremental_progress = incremental_progress + increments;
		progress = (int)(incremental_progress);
	#ifndef NO_PRINT
		if (progress > old_progress) {
			std::cout << std::setw(3) << std::right << std::fixed << std::setprecision(0) << progress << "%";
			pBar = "[";
			for (int p = 0; p <= (int)(progress_increments * i); p++) {
				pBar.append("=");
			}
			std::cout << std::setw(31) << std::left << pBar << "]";
		}
	#endif
	}
	#ifndef NO_PRINT
	std::cout << "\r" << std::setw(3) << std::right << std::fixed << std::setprecision(0) << 100 << "%" << std::setw(31) << std::left << pBar << "]\n";
	#else
	std::cout << " done" << std::endl;
	#endif

  klu_free_symbolic(&Symbolic, &Common);
  klu_free_numeric(&Numeric, &Common);
}

void Simulation::transient_phase_simulation(Input &iObj, Matrix &mObj) {
	results.xVect.clear();
	results.timeAxis.clear();
	std::vector<double> lhsValues(mObj.Nsize, 0.0);
	int simSize = iObj.transSim.simsize();
	for (int m = 0; m < mObj.Nsize; m++) {
		results.xVect.emplace_back(std::vector<double>(simSize, 0.0));
	}
	std::vector<double> RHS(mObj.columnNames.size(), 0.0), LHS_PRE;
	std::vector<std::vector<std::string>> nodeConnectionVector(mObj.rowNames.size());
	std::string currentLabel;
	double RHSvalue;
	double hn_2_2e_hbar = (iObj.transSim.prstep / 2)*(2 * M_PI / PHI_ZERO);
	int ok, rowCounter;
	bool needsLU = false;
	klu_symbolic * Symbolic;
	klu_common Common;
	klu_numeric * Numeric;
	ok = klu_defaults(&Common);
	Symbolic = klu_analyze(mObj.Nsize, &mObj.rowptr.front(), &mObj.colind.front(), &Common);
	Numeric = klu_factor(&mObj.rowptr.front(), &mObj.colind.front(), &mObj.nzval.front(), Symbolic, &Common);
	rowCounter = 0;
	for (auto j : mObj.rowNames) {
		if (j[2] == 'N') {
			nodeConnectionVector[rowCounter] = mObj.nodeConnections[j];
		}
		rowCounter++;
	}
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
			std::cout << '\r';
	#endif
		RHS.clear();
		rowCounter = 0;
		for (auto j : mObj.rowNames) {
			RHSvalue = 0.0;
			if (j[2] == 'N') {
				for (auto k : nodeConnectionVector[rowCounter]) {
					if (k[0] == 'B') {
						if (j == mObj.components.phaseJJ.at(k).posNodeR)
							RHSvalue += mObj.components.phaseJJ.at(k).iS;
						else
							RHSvalue -= mObj.components.phaseJJ.at(k).iS;
					}
					else if (k[0] == 'I') {
						RHSvalue += mObj.sources[k][i];
					}
				}
			}
			else if (j[2] == 'R') {
				currentLabel = j.substr(2);
				if (mObj.components.phaseRes[currentLabel].posNRow == -1)
					mObj.components.phaseRes[currentLabel].pn1 = -lhsValues.at(mObj.components.phaseRes[currentLabel].negNRow);
				else if (mObj.components.phaseRes[currentLabel].negNRow == -1)
					mObj.components.phaseRes[currentLabel].pn1 = lhsValues.at(mObj.components.phaseRes[currentLabel].posNRow);
				else
					mObj.components.phaseRes[currentLabel].pn1 = lhsValues.at(mObj.components.phaseRes[currentLabel].posNRow) - lhsValues.at(mObj.components.phaseRes[currentLabel].negNRow);
				mObj.components.phaseRes[currentLabel].IRn1 = lhsValues.at(mObj.components.phaseRes[currentLabel].curNRow);
				RHSvalue += ((M_PI * mObj.components.phaseRes[currentLabel].value
							* iObj.transSim.prstep) / PHI_ZERO)
							* mObj.components.phaseRes[currentLabel].IRn1
							+ mObj.components.phaseRes[currentLabel].pn1;
			}
			else if (j[2] == 'B') {
				currentLabel = j.substr(2);
				RHSvalue = mObj.components.phaseJJ.at(currentLabel).pn1 + hn_2_2e_hbar * mObj.components.phaseJJ.at(currentLabel).vn1;
			}
			else if (j[2] == 'C') {
				currentLabel = j.substr(2);
				RHSvalue = -((2*M_PI*iObj.transSim.prstep*iObj.transSim.prstep)/(4 * PHI_ZERO
						* mObj.components.phaseCap[currentLabel].value))
						* mObj.components.phaseCap[currentLabel].ICn1
						- mObj.components.phaseCap[currentLabel].pn1
						- (iObj.transSim.prstep * mObj.components.phaseCap[currentLabel].dPn1);
			}
			else if (j[2] == 'V') {
				currentLabel = j.substr(2);
				if (mObj.components.phaseVs[currentLabel].posNRow == -1.0)
					mObj.components.phaseVs[currentLabel].pn1 = -lhsValues.at(mObj.components.phaseVs[currentLabel].negNRow);
				else if (mObj.components.phaseVs[currentLabel].negNRow == -1.0)
					mObj.components.phaseVs[currentLabel].pn1 = lhsValues.at(mObj.components.phaseVs[currentLabel].posNRow);
				else
					mObj.components.phaseVs[currentLabel].pn1 = lhsValues.at(mObj.components.phaseVs[currentLabel].posNRow) - lhsValues.at(mObj.components.phaseVs[currentLabel].negNRow);
				if (i >= 1)
					RHSvalue = mObj.components.phaseVs[currentLabel].pn1 + ((iObj.transSim.prstep * M_PI) / PHI_ZERO) * (mObj.sources[currentLabel][i] + mObj.sources[currentLabel][i-1]);
				else if (i == 0)
					RHSvalue = mObj.components.phaseVs[currentLabel].pn1 + ((iObj.transSim.prstep  * M_PI) / PHI_ZERO) * mObj.sources[currentLabel][i];
			}
			else if (j[2] == 'P') {
				currentLabel = j.substr(2);
				RHSvalue = mObj.sources.at(currentLabel).at(i);
			}
			else if (j[2] == 'T') {
				currentLabel = j.substr(2);
				currentLabel = currentLabel.substr(0, currentLabel.size() - 3);
				if(i > mObj.components.txPhase.at(currentLabel).k) {
					if(j.substr(j.size() - 3) == "-I1") {
						RHSvalue = ((iObj.transSim.prstep * M_PI *
							mObj.components.txPhase[currentLabel].value)/(PHI_ZERO)) *
							results.xVect.at(mObj.components.txPhase[currentLabel].curN2Row).at(
								i - mObj.components.txPhase[currentLabel].k) +
							mObj.components.txPhase[currentLabel].p1n1 +
							(iObj.transSim.prstep / 2) *
							(mObj.components.txPhase[currentLabel].dP1n1 +
							mObj.components.txPhase[currentLabel].dP2nk);
					}
					else if(j.substr(j.size() - 3) == "-I2") {
						RHSvalue = ((iObj.transSim.prstep * M_PI *
							mObj.components.txPhase[currentLabel].value)/(PHI_ZERO)) *
							results.xVect.at(mObj.components.txPhase[currentLabel].curN1Row).at(
								i - mObj.components.txPhase[currentLabel].k) +
							mObj.components.txPhase[currentLabel].p2n1 +
							(iObj.transSim.prstep / 2) *
							(mObj.components.txPhase[currentLabel].dP2n1 +
							mObj.components.txPhase[currentLabel].dP1nk);
					}
				}
			}
			RHS.push_back(RHSvalue);
			rowCounter++;
		}
		LHS_PRE = RHS;
		ok = klu_tsolve(Symbolic, Numeric, mObj.Nsize, 1, &LHS_PRE.front(), &Common);
		if(!ok) {
			Errors::matrix_errors(MATRIX_SINGULAR, "");
		}

		lhsValues = LHS_PRE;
		for (int m = 0; m < lhsValues.size(); m++) {
			results.xVect[m][i] = lhsValues[m];
		}

		for (const auto& j : mObj.components.phaseJJ) {
			jj_phase &thisJJ = mObj.components.phaseJJ.at(j.first);
			if (thisJJ.posNRow == -1)
				thisJJ.pn1 = (-lhsValues.at(thisJJ.negNRow));
			else if (thisJJ.negNRow == -1)
				thisJJ.pn1 = (lhsValues.at(thisJJ.posNRow));
			else
				thisJJ.pn1 = (lhsValues.at(thisJJ.posNRow) - lhsValues.at(thisJJ.negNRow));
			thisJJ.vn1 = (lhsValues.at(thisJJ.voltNRow));
			thisJJ.dVn1 = (2 / iObj.transSim.prstep)*(thisJJ.vn1 - thisJJ.vn2) - thisJJ.dVn2;
			thisJJ.vn2 = thisJJ.vn1;
			thisJJ.dVn2 = thisJJ.dVn1;
			thisJJ.v0 = thisJJ.vn1 + iObj.transSim.prstep * thisJJ.dVn1;
			thisJJ.dPn1 = (2 / iObj.transSim.prstep)*(thisJJ.pn1 - thisJJ.pn2) - thisJJ.dPn2;
			thisJJ.phi0 = thisJJ.pn1 + hn_2_2e_hbar * (thisJJ.vn1 + thisJJ.v0);
			thisJJ.pn2 = thisJJ.pn1;
			thisJJ.dPn2 = thisJJ.dPn1;
			if (thisJJ.rType == 1) {
				if(fabs(thisJJ.v0) < thisJJ.lower) {
					thisJJ.It = 0.0;
					if(thisJJ.pPtr != -1) {
						if(mObj.mElements[thisJJ.pPtr].value != thisJJ.subCond) {
							mObj.mElements[thisJJ.pPtr].value = thisJJ.subCond;
							needsLU = true;
						}
					}
					if(thisJJ.nPtr != -1) {
						if(mObj.mElements[thisJJ.nPtr].value != -thisJJ.subCond) {
							mObj.mElements[thisJJ.nPtr].value = -thisJJ.subCond;
							needsLU = true;
						}
					}
				}
				else if(fabs(thisJJ.v0) < thisJJ.upper) {
					if(thisJJ.v0 < 0) thisJJ.It = -thisJJ.vG*((1/thisJJ.r0) - thisJJ.gLarge);
					else thisJJ.It = thisJJ.vG * ((1/thisJJ.r0) - thisJJ.gLarge);
					if(thisJJ.pPtr != -1) {
						if(mObj.mElements[thisJJ.pPtr].value != thisJJ.transCond) {
							mObj.mElements[thisJJ.pPtr].value = thisJJ.transCond;
							needsLU = true;
						}
					}
					if(thisJJ.nPtr != -1) {
						if(mObj.mElements[thisJJ.nPtr].value != -thisJJ.transCond) {
							mObj.mElements[thisJJ.nPtr].value = -thisJJ.transCond;
							needsLU = true;
						}
					}
				}
				else {
					if(thisJJ.v0 < 0) thisJJ.It = -((thisJJ.iC/thisJJ.iCFact) + ((1/thisJJ.r0) * thisJJ.lower) - ((1/thisJJ.rN) * thisJJ.upper));
					else thisJJ.It = ((thisJJ.iC/thisJJ.iCFact) + ((1/thisJJ.r0) * thisJJ.lower) - ((1/thisJJ.rN) * thisJJ.upper));
					if(thisJJ.pPtr != -1) {
						if(mObj.mElements[thisJJ.pPtr].value != thisJJ.normalCond) {
							mObj.mElements[thisJJ.pPtr].value = thisJJ.normalCond;
							needsLU = true;
						}
					}
					if(thisJJ.nPtr != -1) {
						if(mObj.mElements[thisJJ.nPtr].value != -thisJJ.normalCond) {
							mObj.mElements[thisJJ.nPtr].value = -thisJJ.normalCond;
							needsLU = true;
						}
					}
				}
			}
			// thisJJ.iS = -thisJJ.iC * sin(thisJJ.phi0) + ((2 * thisJJ.C) / iFile.tsim.prstep) * thisJJ.vn1 + thisJJ.C * thisJJ.dVn1 - thisJJ.It;
			thisJJ.iS = -((M_PI * thisJJ.Del) / (2 * EV * thisJJ.rNCalc)) * (sin(thisJJ.phi0)/sqrt(1 - thisJJ.D * (sin(thisJJ.phi0 / 2)*sin(thisJJ.phi0 / 2))))
								* tanh((thisJJ.Del)/(2*BOLTZMANN*thisJJ.T) * sqrt(1-thisJJ.D * (sin(thisJJ.phi0 / 2)*sin(thisJJ.phi0 / 2))))
								+ (((2 * thisJJ.C) / iObj.transSim.prstep)*thisJJ.vn1) + (thisJJ.C * thisJJ.dVn1) - thisJJ.It;
			mObj.components.phaseJJ.at(j.first).jjCur.push_back(thisJJ.iS);
		}
		/* Calculate next Cap values */
		for (const auto& j : mObj.components.phaseCap) {
			cap_phase thisCap = mObj.components.phaseCap.at(j.first);
			thisCap.pn2 = thisCap.pn1;
			if (j.second.posNRow == -1)
				thisCap.pn1 = (-lhsValues.at(j.second.negNRow));
			else if (j.second.negNRow == -1)
				thisCap.pn1 = (lhsValues.at(j.second.posNRow));
			else
				thisCap.pn1 = (lhsValues.at(j.second.posNRow) - lhsValues.at(j.second.negNRow));
			thisCap.ICn1 = lhsValues.at(thisCap.curNRow);
			thisCap.dPn1 = (2/iObj.transSim.prstep) * (thisCap.pn1 - thisCap.pn2) - thisCap.dPn2;
			thisCap.dPn2 = thisCap.dPn1;
			mObj.components.phaseCap.at(j.first) = thisCap;
		}
		/* Calculate next TL values */
		for (const auto& j : mObj.components.txPhase) {
			tx_phase thisTL = mObj.components.txPhase.at(j.first);
			if (j.second.posNRow == -1) {
				thisTL.p1n1 = (-lhsValues.at(j.second.negNRow));
			}
			else if (j.second.negNRow == -1) {
				thisTL.p1n1 = (lhsValues.at(j.second.posNRow));
			}
			else {
				thisTL.p1n1 = (lhsValues.at(j.second.posNRow) - lhsValues.at(j.second.negNRow));
			}
			if (j.second.posN2Row == -1) {
				thisTL.p2n1 = (-lhsValues.at(j.second.negN2Row));
			}
			else if (j.second.negN2Row == -1) {
				thisTL.p2n1 = (lhsValues.at(j.second.posN2Row));
			}
			else {
				thisTL.p2n1 = (lhsValues.at(j.second.posN2Row) - lhsValues.at(j.second.negN2Row));
			}
			thisTL.dP1n1 = (2/iObj.transSim.prstep)*(thisTL.p1n1 - thisTL.p1n2) - thisTL.dP1n2;
			thisTL.p1n2 = thisTL.p1n1;
			thisTL.dP1n2 = thisTL.dP1n1;
			thisTL.dP2n1 = (2/iObj.transSim.prstep)*(thisTL.p2n1 - thisTL.p2n2) - thisTL.dP2n2;
			thisTL.p2n2 = thisTL.p2n1;
			thisTL.dP2n2 = thisTL.dP2n1;
			if(i >= thisTL.k) {
				if (thisTL.posNRow == -1) {
					thisTL.p1nk = -results.xVect[thisTL.negNRow][i-thisTL.k];
				}
				else if (thisTL.negNRow == -1) {
					thisTL.p1nk = results.xVect[thisTL.posNRow][i-thisTL.k];
				}
				else {
					thisTL.p1nk = results.xVect[thisTL.posNRow][i-thisTL.k] - results.xVect[thisTL.negNRow][i-thisTL.k];
				}
				if (thisTL.posN2Row == -1) {
					thisTL.p2nk = -results.xVect[thisTL.negN2Row][i-thisTL.k];
				}
				else if (thisTL.negN2Row == -1) {
					thisTL.p2nk = results.xVect[thisTL.posN2Row][i-thisTL.k];
				}
				else {
					thisTL.p2nk = results.xVect[thisTL.posN2Row][i-thisTL.k] - results.xVect[thisTL.negN2Row][i-thisTL.k];
				}
				thisTL.dP1nk = (2/iObj.transSim.prstep)*(thisTL.p1nk - thisTL.p1nk1) - thisTL.dP1nk1;
				thisTL.p1nk1 = thisTL.p1nk;
				thisTL.dP1nk1 = thisTL.dP1nk;
				thisTL.dP2nk = (2/iObj.transSim.prstep)*(thisTL.p2nk - thisTL.p2nk1) - thisTL.dP2nk1;
				thisTL.p2nk1 = thisTL.p2nk;
				thisTL.dP2nk1 = thisTL.dP2nk;
			}
			mObj.components.txPhase.at(j.first) = thisTL;
		}
		if(needsLU) {
				mObj.create_CSR();
        // TODO: Maybe use refactor
        klu_free_numeric(&Numeric, &Common);
				Numeric = klu_factor(&mObj.rowptr.front(), &mObj.colind.front(), &mObj.nzval.front(), Symbolic, &Common);
				needsLU = false;
		}
		/* Add the current time value to the time axis for plotting purposes */
		results.timeAxis.push_back(i*iObj.transSim.prstep);
		old_progress = progress;
		incremental_progress = incremental_progress + increments;
		progress = (int)(incremental_progress);
	#ifndef NO_PRINT
		if (progress > old_progress) {
			std::cout << std::setw(3) << std::right << std::fixed << std::setprecision(0) << progress << "%";
			pBar = "[";
			for (int p = 0; p <= (int)(progress_increments * i); p++) {
				pBar.append("=");
			}
			std::cout << std::setw(31) << std::left << pBar << "]";
		}
	#endif
	}
	#ifndef NO_PRINT
	std::cout << "\r" << std::setw(3) << std::right << std::fixed << std::setprecision(0) << 100 << "%" << std::setw(31) << std::left << pBar << "]\n";
	#else
	std::cout << " done" << std::endl;
	#endif

  klu_free_symbolic(&Symbolic, &Common);
  klu_free_numeric(&Numeric, &Common);
}
