// Copyright (c) 2018 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#include "j_simulation.h"

void Simulation::identify_simulation(InputFile& iFile) {
	std::vector<std::string> simtokens;
	// Identify a line if it is a simulation control
	for (const auto &i : iFile.controlPart) {
		// If transient analysis...
		if (i.find("TRAN") != std::string::npos) {
			// Set simulation type to transient
			iFile.simulationType = TRANSIENT;
			// Tokenize the string
			simtokens = Misc::tokenize_delimeter(i, " ,");
			if (simtokens.size() < 2) {
				Errors::control_errors(TRANS_ERROR, "Too few parameters: " + i);
				iFile.tsim.maxtstep = 1E-12;
				iFile.tsim.tstop = 1E-9;
				iFile.tsim.tstart = 0;
			}
			else {
				iFile.tsim.prstep = Misc::modifier(simtokens[1]);
				if (simtokens.size() > 2) {
					iFile.tsim.tstop = Misc::modifier(simtokens[2]);
					if (simtokens.size() > 3) {
						iFile.tsim.tstart = Misc::modifier(simtokens[3]);
						if (simtokens.size() > 4) {
							iFile.tsim.maxtstep = Misc::modifier(simtokens[4]);
						}
						else iFile.tsim.maxtstep = 1E-12;
					}
					else {
						iFile.tsim.tstart = 0;
						iFile.tsim.maxtstep = 1E-12;
					}
				}
				else {
					iFile.tsim.tstop = 1E-9;
					iFile.tsim.tstart = 0;
					iFile.tsim.maxtstep = 1E-12;
				}
			}
			break;
		}
		// If dc analysis...
		if (i.find("DC") != std::string::npos) {
			iFile.simulationType = DC;
			break;
		}
		// If ac analysis...
		if (i.find("AC") != std::string::npos) {
			iFile.simulationType = AC;
		}
		// If phase analysis...
		if (i.find("PHASE") != std::string::npos) {
			iFile.simulationType = PHASE;
		}
	}
	// No simulation type was specified in all the controls
	if (iFile.simulationType == 4) {
		// Error and inform user
		Errors::control_errors(NO_SIM, "");
	}
}

/*
	Perform transient voltage simulation
*/
void Simulation::transient_voltage_simulation(InputFile& iFile) {
	iFile.xVect.clear();
	iFile.timeAxis.clear();
	std::vector<double> lhsValues(iFile.matA.Nsize, 0.0),
		RHS(iFile.matA.columnNames.size(), 0.0), LHS_PRE;
	int simSize = iFile.tsim.simsize();
	for (int m = 0; m < iFile.matA.Nsize; m++) {
		iFile.xVect.emplace_back(std::vector<double>(simSize, 0.0));
	}
	std::vector<std::vector<std::string>> nodeConnectionVector(iFile.matA.rowNames.size());
	std::string currentLabel, txCurrent;
	std::vector<std::string> tokens;
	double VB, RHSvalue, inductance, z0voltage;
	double hn_2_2e_hbar = (iFile.tsim.prstep / 2)*(2 * M_PI / PHI_ZERO);
	int ok, rowCounter;
	bool needsLU = false;
	klu_symbolic * Symbolic;
	klu_common Common;
	klu_numeric * Numeric;
	ok = klu_defaults(&Common);
	Symbolic = klu_analyze(iFile.matA.Nsize, &iFile.matA.rowptr.front(), &iFile.matA.colind.front(), &Common);
	Numeric = klu_factor(&iFile.matA.rowptr.front(), &iFile.matA.colind.front(), &iFile.matA.nzval.front(), Symbolic, &Common);
	rowCounter = 0;
	for (auto j : iFile.matA.rowNames) {
		if (j[2] == 'N') {
			nodeConnectionVector[rowCounter] = iFile.matA.nodeConnections[j];
		}
		rowCounter++;
	}
	/***************/
	/** TIME LOOP **/
	/***************/
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
		for (auto j : iFile.matA.rowNames) {
			RHSvalue = 0.0;
			if (j[2] == 'N') {
				tokens.clear();
				for (auto k : nodeConnectionVector[rowCounter]) {
					if (k[0] == 'B') {
						if (j == iFile.voltJJ.at(k).posNodeR)
							RHSvalue += iFile.voltJJ.at(k).iS;
						else
							RHSvalue -= iFile.voltJJ.at(k).iS;
					}
					else if (k[0] == 'I') {
						RHSvalue += iFile.matA.sources.at(k).at(i);
					}
					else if (k[0] == 'R') {
						if(iFile.voltRes.at(k).posNRow == rowCounter){
						}
						else if(iFile.voltRes.at(k).negNRow == rowCounter){
						}
					}
				}
			}
			else if (j[2] == 'L') {
				currentLabel = j.substr(2);
				if (iFile.voltInd.at(currentLabel).posNRow == -1)
					VB = -lhsValues.at(iFile.voltInd.at(currentLabel).negNRow);
				else if (iFile.voltInd.at(currentLabel).negNRow == -1)
					VB = lhsValues.at(iFile.voltInd.at(currentLabel).posNRow);
				else VB = lhsValues.at(iFile.voltInd.at(currentLabel).posNRow) - 
					lhsValues.at(iFile.voltInd.at(currentLabel).negNRow);
				RHSvalue = (-2 * iFile.voltInd.at(currentLabel).value / iFile.tsim.prstep) * 
					lhsValues.at(iFile.voltInd.at(currentLabel).curNRow) - VB;
				for (auto m : iFile.voltInd.at(currentLabel).mut) {
					RHSvalue = RHSvalue - ( m.second * lhsValues.at(iFile.voltInd.at(m.first).curNRow));
				}
			}
			else if (j[2] == 'B') {
				currentLabel = j.substr(2);
				RHSvalue = iFile.voltJJ.at(currentLabel).pn1 + hn_2_2e_hbar * iFile.voltJJ.at(currentLabel).vn1;
			}
			else if (j[2] == 'V') {
				currentLabel = j.substr(2);
				RHSvalue = iFile.matA.sources.at(currentLabel).at(i);
			}
			else if (j[2] == 'T') {
				currentLabel = j.substr(2, j.size() - 5);
				txCurrent = j.substr(j.size() - 2, j.size() - 1);
				if(i >= iFile.txLine.at(currentLabel).k) {
					if (txCurrent == "I1") {
						if (iFile.txLine.at(currentLabel).posN2Row == -1) 
							VB = -iFile.xVect.at(
								iFile.txLine.at(currentLabel).negN2Row).at(
									i - iFile.txLine.at(currentLabel).k);
						else if (iFile.txLine.at(currentLabel).negN2Row == -1) 
							VB = iFile.xVect.at(
								iFile.txLine.at(currentLabel).posN2Row).at(
									i - iFile.txLine.at(currentLabel).k);
						else VB = iFile.xVect.at(
							iFile.txLine.at(currentLabel).posN2Row).at(
									i - iFile.txLine.at(currentLabel).k) - 
							iFile.xVect.at(
								iFile.txLine.at(currentLabel).negN2Row).at(
									i - iFile.txLine.at(currentLabel).k);
						RHSvalue = iFile.txLine.at(currentLabel).value * 
							iFile.xVect.at(
								iFile.txLine.at(currentLabel).curN2Row).at(
									i - iFile.txLine.at(currentLabel).k) + VB;
					}
					else if (txCurrent == "I2") {
						if (iFile.txLine.at(currentLabel).posNRow == -1) 
							VB = -iFile.xVect.at(
								iFile.txLine.at(currentLabel).negNRow).at(
									i - iFile.txLine.at(currentLabel).k);
						else if (iFile.txLine.at(currentLabel).negNRow == -1) 
							VB = iFile.xVect.at(
								iFile.txLine.at(currentLabel).posNRow).at(
									i - iFile.txLine.at(currentLabel).k);
						else VB = iFile.xVect.at(
							iFile.txLine.at(currentLabel).posNRow).at(
								i - iFile.txLine.at(currentLabel).k) - 
							iFile.xVect.at(
								iFile.txLine.at(currentLabel).negNRow).at(
									i - iFile.txLine.at(currentLabel).k);
						RHSvalue = iFile.txLine.at(currentLabel).value * 
							iFile.xVect.at(
								iFile.txLine.at(currentLabel).curN1Row).at(
									i - iFile.txLine.at(currentLabel).k) + VB;
					}
				}
			}
			RHS.push_back(RHSvalue);
			rowCounter++;
		}

		LHS_PRE = RHS;
		ok = klu_tsolve(Symbolic, Numeric, iFile.matA.Nsize, 1, &LHS_PRE.front(), &Common);
		if(!ok) {
			Errors::matrix_errors(MATRIX_SINGULAR, "");
		}

		lhsValues = LHS_PRE;
		for (int m = 0; m < lhsValues.size(); m++) {
			iFile.xVect[m][i] = lhsValues[m];
		}

		/* Guess next junction voltage */
		for (auto j : iFile.voltJJ) {
			jj_volt thisJunction = iFile.voltJJ.at(j.first);
			if (thisJunction.posNRow == -1) thisJunction.vn1 = (-lhsValues.at(thisJunction.negNRow));
			else if (thisJunction.negNRow == -1) thisJunction.vn1 = (lhsValues.at(thisJunction.posNRow));
			else thisJunction.vn1 = (lhsValues.at(thisJunction.posNRow) - lhsValues.at(thisJunction.negNRow));
			thisJunction.dVn1 = (2 / iFile.tsim.prstep) * (thisJunction.vn1 - thisJunction.vn2) - thisJunction.dVn2;
			thisJunction.v0 = thisJunction.vn1 + iFile.tsim.prstep * thisJunction.dVn1;
			if (thisJunction.rType == 1) {
				if(fabs(thisJunction.v0) < thisJunction.lowerB) {
					thisJunction.iT = 0.0;
					if(thisJunction.ppPtr != -1) {
						if(iFile.matA.mElements.at(thisJunction.ppPtr).value != thisJunction.subCond) {
							iFile.matA.mElements.at(thisJunction.ppPtr).value = thisJunction.subCond;
							needsLU = true;
						}
					}
					if(thisJunction.nnPtr != -1) {
						if(iFile.matA.mElements.at(thisJunction.nnPtr).value != thisJunction.subCond) {
							iFile.matA.mElements.at(thisJunction.nnPtr).value = thisJunction.subCond;
							needsLU = true;
						}
					}
					if(thisJunction.pnPtr != -1) {
						if(iFile.matA.mElements.at(thisJunction.pnPtr).value != -thisJunction.subCond) {
							iFile.matA.mElements.at(thisJunction.pnPtr).value = -thisJunction.subCond;
							needsLU = true;
						}
					}
					if(thisJunction.npPtr != -1) {
						if(iFile.matA.mElements.at(thisJunction.npPtr).value != -thisJunction.subCond) {
							iFile.matA.mElements.at(thisJunction.npPtr).value = -thisJunction.subCond;
							needsLU = true;
						}
					}
				}
				else if(fabs(thisJunction.v0) < thisJunction.upperB) {
					if(thisJunction.v0 < 0) thisJunction.iT = -thisJunction.lowerB*((1/thisJunction.r0) - thisJunction.gLarge);
					else thisJunction.iT = thisJunction.lowerB*((1/thisJunction.r0) - thisJunction.gLarge);
					if(thisJunction.ppPtr != -1) {
						if(iFile.matA.mElements.at(thisJunction.ppPtr).value != thisJunction.transCond) {
							iFile.matA.mElements.at(thisJunction.ppPtr).value = thisJunction.transCond;
							needsLU = true;
						}
					}
					if(thisJunction.nnPtr != -1) {
						if(iFile.matA.mElements.at(thisJunction.nnPtr).value != thisJunction.transCond) {
							iFile.matA.mElements.at(thisJunction.nnPtr).value = thisJunction.transCond;
							needsLU = true;
						}
					}
					if(thisJunction.pnPtr != -1) {
						if(iFile.matA.mElements.at(thisJunction.pnPtr).value != thisJunction.transCond) {
							iFile.matA.mElements.at(thisJunction.pnPtr).value = -thisJunction.transCond;
							needsLU = true;
						}
					}
					if(thisJunction.npPtr != -1) {
						if(iFile.matA.mElements.at(thisJunction.npPtr).value != thisJunction.transCond) {
							iFile.matA.mElements.at(thisJunction.npPtr).value = -thisJunction.transCond;
							needsLU = true;
						}
					}
				}
				else {
					if(thisJunction.v0 < 0) thisJunction.iT = -(thisJunction.iC/thisJunction.iCFact + thisJunction.vG * (1/thisJunction.r0) - thisJunction.lowerB * (1/thisJunction.rN));
					else thisJunction.iT = (thisJunction.iC/thisJunction.iCFact + thisJunction.vG * (1/thisJunction.r0) - thisJunction.lowerB * (1/thisJunction.rN));
					if(thisJunction.ppPtr != -1) {
						if(iFile.matA.mElements.at(thisJunction.ppPtr).value != thisJunction.normalCond) {
							iFile.matA.mElements.at(thisJunction.ppPtr).value = thisJunction.normalCond;
							needsLU = true;
						}
					}
					if(thisJunction.nnPtr != -1) {
						if(iFile.matA.mElements.at(thisJunction.nnPtr).value != thisJunction.normalCond) {
							iFile.matA.mElements.at(thisJunction.nnPtr).value = thisJunction.normalCond;
							needsLU = true;
						}
					}
					if(thisJunction.pnPtr != -1) {
						if(iFile.matA.mElements.at(thisJunction.pnPtr).value != thisJunction.normalCond) {
							iFile.matA.mElements.at(thisJunction.pnPtr).value = -thisJunction.normalCond;
							needsLU = true;
						}
					}
					if(thisJunction.npPtr != -1) {
						if(iFile.matA.mElements.at(thisJunction.npPtr).value != thisJunction.normalCond) {
							iFile.matA.mElements.at(thisJunction.npPtr).value = -thisJunction.normalCond;
							needsLU = true;
						}
					}
				}
			}
			thisJunction.pn1 = lhsValues.at(thisJunction.phaseNRow);
			thisJunction.phi0 = thisJunction.pn1 + (hn_2_2e_hbar)*(thisJunction.vn1 + thisJunction.v0);
			// thisJunction.iS = -thisJunction.iC * sin(thisJunction.phi0) + (((2 * thisJunction.C) / iFile.tsim.prstep)*thisJunction.vn1) + (thisJunction.C * thisJunction.dVn1) - thisJunction.iT;
			thisJunction.iS = -((M_PI * thisJunction.Del) / (2 * EV * thisJunction.rNCalc)) * (sin(thisJunction.phi0)/sqrt(1 - thisJunction.D * (sin(thisJunction.phi0 / 2)*sin(thisJunction.phi0 / 2))))
								* tanh((thisJunction.Del)/(2*BOLTZMANN*thisJunction.T) * sqrt(1-thisJunction.D * (sin(thisJunction.phi0 / 2)*sin(thisJunction.phi0 / 2))))
								+ (((2 * thisJunction.C) / iFile.tsim.prstep)*thisJunction.vn1) + (thisJunction.C * thisJunction.dVn1) - thisJunction.iT;
			thisJunction.vn2 = thisJunction.vn1;
			thisJunction.dVn2 = thisJunction.dVn1;
			thisJunction.pn2 = thisJunction.pn1;
			iFile.voltJJ.at(j.first) = thisJunction;
			iFile.voltJJ.at(j.first).jjCur.push_back(thisJunction.iS);
		}
		if(needsLU) {
				Matrix::csr_A_matrix(iFile);
				Numeric = klu_factor(&iFile.matA.rowptr.front(), &iFile.matA.colind.front(), &iFile.matA.nzval.front(), Symbolic, &Common);
				needsLU  = false;
		}
		iFile.timeAxis.push_back(i*iFile.tsim.prstep);
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
}

/*
	Perform transient phase simulation
*/
void Simulation::transient_phase_simulation(InputFile& iFile) {
	iFile.xVect.clear();
	iFile.timeAxis.clear();
	std::vector<double> lhsValues(iFile.matA.Nsize, 0.0);
	int simSize = iFile.tsim.simsize();
	/* Where to store the calculated values */
	for (int m = 0; m < iFile.matA.Nsize; m++) {
		iFile.xVect.emplace_back(std::vector<double>(simSize, 0.0));
	}
	std::vector<double> RHS(iFile.matA.columnNames.size(), 0.0), LHS_PRE;
	std::vector<std::vector<std::string>> nodeConnectionVector(iFile.matA.rowNames.size());
	std::string currentLabel;
	std::vector<std::string> tokens;
	double RHSvalue;
	double hn_2_2e_hbar = (iFile.tsim.prstep / 2)*(2 * M_PI / PHI_ZERO);
	int ok, rowCounter;
	bool needsLU = false;
	klu_symbolic * Symbolic;
	klu_common Common;
	klu_numeric * Numeric;
	ok = klu_defaults(&Common);
	Symbolic = klu_analyze(iFile.matA.Nsize, &iFile.matA.rowptr.front(), &iFile.matA.colind.front(), &Common);
	Numeric = klu_factor(&iFile.matA.rowptr.front(), &iFile.matA.colind.front(), &iFile.matA.nzval.front(), Symbolic, &Common);
	rowCounter = 0;
	for (auto j : iFile.matA.rowNames) {
		if (j[2] == 'N') {
			nodeConnectionVector[rowCounter] = iFile.matA.nodeConnections[j];
		}
		rowCounter++;
	}
	/***************/
	/** TIME LOOP **/
	/***************/
	/* Start a progress bar */
	std::cout << "Simulating:" << std::endl;
	double increments = 100 / (double)simSize;
	double progress_increments = 30 / (double)simSize;
	double incremental_progress = 0.0;
	int progress = 0;
	int old_progress = 0;
	int imintd = 0;
	std::string pBar;
	/* Mutual inductance variables */
	double mutualL = 0.0;
	double CUR2 = 0.0;
	/* Now the start of the LOOP */
	/*****************************/
	for (int i = 0; i < simSize; i++) {
	#ifndef NO_PRINT 
			std::cout << '\r';
	#endif
		/* Start of initialization of the B matrix */
		RHS.clear();
		rowCounter = 0;
		/* Construct RHS matrix */
		for (auto j : iFile.matA.rowNames) {
			RHSvalue = 0.0;
			/* If this row item is identified as a node row then...*/
			if (j[2] == 'N') {
				tokens.clear();
				for (auto k : nodeConnectionVector[rowCounter]) {
					/* Add junction as calculated at the end of the current loop to the RHS */
					if (k[0] == 'B') {
						if (j == iFile.phaseJJ.at(k).posNodeR)
							RHSvalue += iFile.phaseJJ.at(k).iS;
						else
							RHSvalue -= iFile.phaseJJ.at(k).iS;
					}
					/* Add the current value to the RHS in the correct row */
					else if (k[0] == 'I') {
						RHSvalue += iFile.matA.sources[k][i];
					}
				}
			}
			/* If this row item is identified as an resistor row */
			else if (j[2] == 'R') {
				currentLabel = j.substr(2);
				if (iFile.phaseRes[currentLabel].posNRow == -1)
					iFile.phaseRes[currentLabel].pn1 = -lhsValues.at(iFile.phaseRes[currentLabel].negNRow);
				else if (iFile.phaseRes[currentLabel].negNRow == -1)
					iFile.phaseRes[currentLabel].pn1 = lhsValues.at(iFile.phaseRes[currentLabel].posNRow);
				else
					iFile.phaseRes[currentLabel].pn1 = lhsValues.at(iFile.phaseRes[currentLabel].posNRow) - lhsValues.at(iFile.phaseRes[currentLabel].negNRow);
				iFile.phaseRes[currentLabel].IRn1 = lhsValues.at(iFile.phaseRes[currentLabel].curNRow);
				/* R_L = (2pi Rhn / 2Phi0)IR + PR*/
				RHSvalue += ((M_PI * iFile.phaseRes[currentLabel].value * iFile.tsim.prstep) / PHI_ZERO) * iFile.phaseRes[currentLabel].IRn1 + iFile.phaseRes[currentLabel].pn1;
			}
			/* If this row item is identified as a junction row */
			else if (j[2] == 'B') {
				currentLabel = j.substr(2);
				RHSvalue = iFile.phaseJJ.at(currentLabel).pn1 + hn_2_2e_hbar * iFile.phaseJJ.at(currentLabel).vn1;
			}
			/* If this row item is identified as a junction row */
			else if (j[2] == 'C') {
				currentLabel = j.substr(2);
				RHSvalue = -((2*M_PI*iFile.tsim.prstep*iFile.tsim.prstep)/(4 * PHI_ZERO * iFile.phaseCap[currentLabel].value)) * iFile.phaseCap[currentLabel].ICn1 - iFile.phaseCap[currentLabel].pn1 - (iFile.tsim.prstep * iFile.phaseCap[currentLabel].dPn1);
			}
			/* If this row item is identified as a voltage source row */
			else if (j[2] == 'V') {
				currentLabel = j.substr(2);
				if (iFile.phaseVs[currentLabel].posNRow == -1.0)
					iFile.phaseVs[currentLabel].pn1 = -lhsValues.at(iFile.phaseVs[currentLabel].negNRow);
				else if (iFile.phaseVs[currentLabel].negNRow == -1.0)
					iFile.phaseVs[currentLabel].pn1 = lhsValues.at(iFile.phaseVs[currentLabel].posNRow);
				else
					iFile.phaseVs[currentLabel].pn1 = lhsValues.at(iFile.phaseVs[currentLabel].posNRow) - lhsValues.at(iFile.phaseVs[currentLabel].negNRow);
				if (i >= 1)
					RHSvalue = iFile.phaseVs[currentLabel].pn1 + ((iFile.tsim.prstep * M_PI) / PHI_ZERO) * (iFile.matA.sources[currentLabel][i] + iFile.matA.sources[currentLabel][i-1]);
				else if (i == 0)
					RHSvalue = iFile.phaseVs[currentLabel].pn1 + ((iFile.tsim.prstep  * M_PI) / PHI_ZERO) * iFile.matA.sources[currentLabel][i];
			}
			else if (j[2] == 'T') {
				currentLabel = j.substr(2);
				currentLabel = currentLabel.substr(0, currentLabel.size() - 3);
				if(i > iFile.txPhase.at(currentLabel).k) {
					if(j.substr(j.size() - 3) == "-I1") {
						RHSvalue = ((iFile.tsim.prstep * M_PI * 
							iFile.txPhase[currentLabel].value)/(PHI_ZERO)) * 
							iFile.xVect.at(iFile.txPhase[currentLabel].curN2Row).at(
								i - iFile.txPhase[currentLabel].k) + 
							iFile.txPhase[currentLabel].p1n1 + 
							(iFile.tsim.prstep / 2) * 
							(iFile.txPhase[currentLabel].dP1n1 + 
							iFile.txPhase[currentLabel].dP2nk);
					}
					else if(j.substr(j.size() - 3) == "-I2") {
						RHSvalue = ((iFile.tsim.prstep * M_PI * 
							iFile.txPhase[currentLabel].value)/(PHI_ZERO)) * 
							iFile.xVect.at(iFile.txPhase[currentLabel].curN1Row).at(
								i - iFile.txPhase[currentLabel].k) + 
							iFile.txPhase[currentLabel].p2n1 + 
							(iFile.tsim.prstep / 2) * 
							(iFile.txPhase[currentLabel].dP2n1 + 
							iFile.txPhase[currentLabel].dP1nk);
					}
				}
			}
			/* Add the RHS value as determined above to the correct spot in the RHS vector */
			RHS.push_back(RHSvalue);
			rowCounter++;
		}
		/* End of the B matrix initialization */

		/* Solve Ax=b */
		LHS_PRE = RHS;
		ok = klu_tsolve(Symbolic, Numeric, iFile.matA.Nsize, 1, &LHS_PRE.front(), &Common);
		if(!ok) {
			Errors::matrix_errors(MATRIX_SINGULAR, "");
		}

		/* Set the LHS values equal to the returning value provided by the KLU solution */
		lhsValues = LHS_PRE;
		for (int m = 0; m < lhsValues.size(); m++) {
			iFile.xVect[m][i] = lhsValues[m];
		}

		/* Guess next junction voltage */
		for (auto j : iFile.phaseJJ) {
			jj_phase thisJJ = iFile.phaseJJ.at(j.first);
			// Phi (Junction Phase Calculated for this timestep)
			if (thisJJ.posNRow == -1)
				thisJJ.pn1 = (-lhsValues.at(thisJJ.negNRow));
			else if (thisJJ.negNRow == -1)
				thisJJ.pn1 = (lhsValues.at(thisJJ.posNRow));
			else
				thisJJ.pn1 = (lhsValues.at(thisJJ.posNRow) - lhsValues.at(thisJJ.negNRow));
			// Vb (Junction Voltage Calculated for this timestep)
			thisJJ.vn1 = (lhsValues.at(thisJJ.voltNRow));
			thisJJ.dVn1 = (2 / iFile.tsim.prstep)*(thisJJ.vn1 - thisJJ.vn2) - thisJJ.dVn2;
			thisJJ.vn2 = thisJJ.vn1;
			thisJJ.dVn2 = thisJJ.dVn1;
			thisJJ.v0 = thisJJ.vn1 + iFile.tsim.prstep * thisJJ.dVn1;
			thisJJ.dPn1 = (2 / iFile.tsim.prstep)*(thisJJ.pn1 - thisJJ.pn2) - thisJJ.dPn2;
			thisJJ.phi0 = thisJJ.pn1 + hn_2_2e_hbar * (thisJJ.vn1 + thisJJ.v0);
			thisJJ.pn2 = thisJJ.pn1;
			thisJJ.dPn2 = thisJJ.dPn1;
			if (thisJJ.rType == 1) {
				if(fabs(thisJJ.v0) < thisJJ.lower) {
					thisJJ.It = 0.0;
					if(thisJJ.pPtr != -1) {
						if(iFile.matA.mElements[thisJJ.pPtr].value != thisJJ.subCond) {
							iFile.matA.mElements[thisJJ.pPtr].value = thisJJ.subCond;
							needsLU = true;
						}
					}
					if(thisJJ.nPtr != -1) {
						if(iFile.matA.mElements[thisJJ.nPtr].value != -thisJJ.subCond) {
							iFile.matA.mElements[thisJJ.nPtr].value = -thisJJ.subCond;
							needsLU = true;
						}
					}
				}
				else if(fabs(thisJJ.v0) < thisJJ.upper) {
					if(thisJJ.v0 < 0) thisJJ.It = -thisJJ.vG*((1/thisJJ.r0) - thisJJ.gLarge);
					else thisJJ.It = thisJJ.vG * ((1/thisJJ.r0) - thisJJ.gLarge);
					if(thisJJ.pPtr != -1) {
						if(iFile.matA.mElements[thisJJ.pPtr].value != thisJJ.transCond) {
							iFile.matA.mElements[thisJJ.pPtr].value = thisJJ.transCond;
							needsLU = true;
						}
					}
					if(thisJJ.nPtr != -1) {
						if(iFile.matA.mElements[thisJJ.nPtr].value != -thisJJ.transCond) {
							iFile.matA.mElements[thisJJ.nPtr].value = -thisJJ.transCond;
							needsLU = true;
						}
					}
				}
				else {
					if(thisJJ.v0 < 0) thisJJ.It = -((thisJJ.iC/thisJJ.iCFact) + ((1/thisJJ.r0) * thisJJ.lower) - ((1/thisJJ.rN) * thisJJ.upper));
					else thisJJ.It = ((thisJJ.iC/thisJJ.iCFact) + ((1/thisJJ.r0) * thisJJ.lower) - ((1/thisJJ.rN) * thisJJ.upper));
					if(thisJJ.pPtr != -1) {
						if(iFile.matA.mElements[thisJJ.pPtr].value != thisJJ.normalCond) {
							iFile.matA.mElements[thisJJ.pPtr].value = thisJJ.normalCond;
							needsLU = true;
						}
					}
					if(thisJJ.nPtr != -1) {
						if(iFile.matA.mElements[thisJJ.nPtr].value != -thisJJ.normalCond) {
							iFile.matA.mElements[thisJJ.nPtr].value = -thisJJ.normalCond;
							needsLU = true;
						}
					}
				}
			}
			// thisJJ.iS = -thisJJ.iC * sin(thisJJ.phi0) + ((2 * thisJJ.C) / iFile.tsim.prstep) * thisJJ.vn1 + thisJJ.C * thisJJ.dVn1 - thisJJ.It;
			thisJJ.iS = -((M_PI * thisJJ.Del) / (2 * EV * thisJJ.rNCalc)) * (sin(thisJJ.phi0)/sqrt(1 - thisJJ.D * (sin(thisJJ.phi0 / 2)*sin(thisJJ.phi0 / 2))))
								* tanh((thisJJ.Del)/(2*BOLTZMANN*thisJJ.T) * sqrt(1-thisJJ.D * (sin(thisJJ.phi0 / 2)*sin(thisJJ.phi0 / 2))))
								+ (((2 * thisJJ.C) / iFile.tsim.prstep)*thisJJ.vn1) + (thisJJ.C * thisJJ.dVn1) - thisJJ.It;
			iFile.phaseJJ.at(j.first) = thisJJ;
			/* Store the junction currents for printing */
			iFile.phaseJJ.at(j.first).jjCur.push_back(thisJJ.iS);
		}
		/* Calculate next Cap values */
		for (auto j : iFile.phaseCap) {
			cap_phase thisCap = iFile.phaseCap.at(j.first);
			thisCap.pn2 = thisCap.pn1;
			if (j.second.posNRow == -1)
				thisCap.pn1 = (-lhsValues.at(j.second.negNRow));
			else if (j.second.negNRow == -1)
				thisCap.pn1 = (lhsValues.at(j.second.posNRow));
			else
				thisCap.pn1 = (lhsValues.at(j.second.posNRow) - lhsValues.at(j.second.negNRow));
			thisCap.ICn1 = lhsValues.at(thisCap.curNRow);
			thisCap.dPn1 = (2/iFile.tsim.prstep) * (thisCap.pn1 - thisCap.pn2) - thisCap.dPn2;
			thisCap.dPn2 = thisCap.dPn1;
		}
		/* Calculate next TL values */
		for (auto j : iFile.txPhase) {
			tx_phase thisTL = iFile.txPhase.at(j.first);
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
			thisTL.dP1n1 = (2/iFile.tsim.prstep)*(thisTL.p1n1 - thisTL.p1n2) - thisTL.dP1n2;
			thisTL.p1n2 = thisTL.p1n1;
			thisTL.dP1n2 = thisTL.dP1n1;
			thisTL.dP2n1 = (2/iFile.tsim.prstep)*(thisTL.p2n1 - thisTL.p2n2) - thisTL.dP2n2;
			thisTL.p2n2 = thisTL.p2n1;
			thisTL.dP2n2 = thisTL.dP2n1;
			if(i >= thisTL.k) {
				if (thisTL.posNRow == -1) {
					thisTL.p1nk = -iFile.xVect[thisTL.negNRow][i-thisTL.k];
				}
				else if (thisTL.negNRow == -1) {
					thisTL.p1nk = iFile.xVect[thisTL.posNRow][i-thisTL.k];
				}
				else {
					thisTL.p1nk = iFile.xVect[thisTL.posNRow][i-thisTL.k] - iFile.xVect[thisTL.negNRow][i-thisTL.k];
				}
				if (thisTL.posN2Row == -1) {
					thisTL.p2nk = -iFile.xVect[thisTL.negN2Row][i-thisTL.k];
				}
				else if (thisTL.negN2Row == -1) {
					thisTL.p2nk = iFile.xVect[thisTL.posN2Row][i-thisTL.k];
				}
				else {
					thisTL.p2nk = iFile.xVect[thisTL.posN2Row][i-thisTL.k] - iFile.xVect[thisTL.negN2Row][i-thisTL.k]; 
				}
				thisTL.dP1nk = (2/iFile.tsim.prstep)*(thisTL.p1nk - thisTL.p1nk1) - thisTL.dP1nk1;
				thisTL.p1nk1 = thisTL.p1nk;
				thisTL.dP1nk1 = thisTL.dP1nk;
				thisTL.dP2nk = (2/iFile.tsim.prstep)*(thisTL.p2nk - thisTL.p2nk1) - thisTL.dP2nk1;
				thisTL.p2nk1 = thisTL.p2nk;
				thisTL.dP2nk1 = thisTL.dP2nk;
			}
			iFile.txPhase.at(j.first) = thisTL;
		}
		if(needsLU) {
				Matrix::csr_A_matrix(iFile);
				/* Do numeric factorization of matrix */
				Numeric = klu_factor(&iFile.matA.rowptr.front(), &iFile.matA.colind.front(), &iFile.matA.nzval.front(), Symbolic, &Common);
				needsLU = false;
		}
		/* Add the current time value to the time axis for plotting purposes */
		iFile.timeAxis.push_back(i*iFile.tsim.prstep);
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
}