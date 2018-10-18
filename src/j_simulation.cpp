// Copyright (c) 2018 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#include "j_simulation.hpp"

void identify_simulation(InputFile& iFile) {
	std::vector<std::string> simtokens;
	// Identify a line if it is a simulation control
	for (const auto &i : iFile.controlPart) {
		// If transient analysis...
		if (i.find("TRAN") != std::string::npos) {
			// Set simulation type to transient
			iFile.simulationType = TRANSIENT;
			// Tokenize the string
			simtokens = tokenize_delimeter(i, " ,");
			if (simtokens.size() < 2) {
				control_errors(TRANS_ERROR, "Too few parameters: " + i);
				iFile.tsim.maxtstep = 1E-12;
				iFile.tsim.tstop = 1E-9;
				iFile.tsim.tstart = 0;
			}
			else {
				iFile.tsim.prstep = modifier(simtokens[1]);
				if (simtokens.size() > 2) {
					iFile.tsim.tstop = modifier(simtokens[2]);
					if (simtokens.size() > 3) {
						iFile.tsim.tstart = modifier(simtokens[3]);
						if (simtokens.size() > 4) {
							iFile.tsim.maxtstep = modifier(simtokens[4]);
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
		control_errors(NO_SIM, "");
	}
}

/*
	Perform transient voltage simulation
*/
void transient_voltage_simulation(InputFile& iFile) {
	iFile.xVect.clear();
	iFile.timeAxis.clear();
	iFile.junctionCurrents.clear();
	/* Standard vector */
	std::vector<double> lhsValues(iFile.matA.Nsize, 0.0);
	int simSize = iFile.tsim.simsize();
	/* Where to store the calculated values */
	for (int m = 0; m < iFile.matA.Nsize; m++) {
		iFile.xVect.emplace_back(std::vector<double>(simSize, 0.0));
	}
	/* Perform time loop */
	std::vector<double> RHS(iFile.matA.columnNames.size(), 0.0), LHS_PRE, inductanceVector(iFile.matA.rowNames.size()), iPNC(iFile.matA.rowNames.size()), iNNC(iFile.matA.rowNames.size()), iCNC(iFile.matA.rowNames.size());
	std::vector<std::vector<std::string>> nodeConnectionVector(iFile.matA.rowNames.size());
	/* Variables to be used by the RHS matrix construction routine */
	std::string currentLabel, columnIndexLabel;
	//std::map<std::string, double>& currentConductance = NULL;
	std::vector<std::string> tokens;
	std::unordered_map<std::string, rcsj_sim_object> simJunctions;
	double VP, VN, CUR, LCUR, VB, RHSvalue, inductance, z0voltage;
	double hn_2_2e_hbar = (iFile.tsim.prstep / 2)*(2 * M_PI / PHI_ZERO);
	int ok, rowCounter;
	bool needsLU = false;
	klu_symbolic * Symbolic;
	klu_common Common;
	klu_numeric * Numeric;
	/* Set the default settings for KLU */
	ok = klu_defaults(&Common);
	/* Do symbolic analysis of matrix */
	Symbolic = klu_analyze(iFile.matA.Nsize, &iFile.matA.rowptr.front(), &iFile.matA.colind.front(), &Common);
	/* Do numeric factorization of matrix */
	Numeric = klu_factor(&iFile.matA.rowptr.front(), &iFile.matA.colind.front(), &iFile.matA.nzval.front(), Symbolic, &Common);
	/* Set up the junctions */
	rowCounter = 0;
	for (auto j : iFile.matA.rowNames) {
		if (j[2] == 'B') {
			/* Identify the junction label */
			currentLabel = j.substr(2);
			columnIndexLabel = "C_P" + currentLabel;
			//columnIndex = index_of(columnNames, columnIndexLabel);
			simJunctions[j].label = currentLabel;
			/* Try to identify the column index of the positive node */
			simJunctions[j].vPositive = (int)iFile.matA.bMatrixConductanceMap[j].at(currentLabel + "-VP");
			/* Try to identify the column index of the negative node */
			simJunctions[j].vNegative = (int)iFile.matA.bMatrixConductanceMap[j].at(currentLabel + "-VN");
			/* Try to identify the column index of the phase node, panic if not found */
			try { simJunctions[j].bPhase = (int)iFile.matA.bMatrixConductanceMap[j].at(currentLabel + "-PHASE"); }
			catch (const std::out_of_range&) { simulation_errors(JJPHASE_NODE_NOT_FOUND, currentLabel); }
			/* Try to identify the junction capacitance, panic if not found */
			try { simJunctions[j].jjCap = iFile.matA.bMatrixConductanceMap[j].at(currentLabel + "-CAP"); }
			catch (const std::out_of_range&) { simulation_errors(JJCAP_NOT_FOUND, currentLabel); }
			/* Try to identify the junction critical current, panic if not found */
			try { simJunctions[j].jjIcrit = iFile.matA.bMatrixConductanceMap[j].at(currentLabel + "-ICRIT"); }
			catch (const std::out_of_range&) { simulation_errors(JJICRIT_NOT_FOUND, currentLabel); }
			simJunctions[j].jjVg = iFile.matA.bMatrixConductanceMap[j].at(currentLabel + "-VGAP");
			simJunctions[j].jjRtype = (int)iFile.matA.bMatrixConductanceMap[j].at(currentLabel + "-RTYPE");
			simJunctions[j].jjRn = iFile.matA.bMatrixConductanceMap[j].at(currentLabel + "-RN");
			simJunctions[j].jjRzero = iFile.matA.bMatrixConductanceMap[j].at(currentLabel + "-R0");
			simJunctions[j].jjIcfact = iFile.matA.bMatrixConductanceMap[j].at(currentLabel + "-ICFACT");
			simJunctions[j].gLarge = simJunctions[j].jjIcrit / (simJunctions[j].jjIcfact * simJunctions[j].delV);
			if(iFile.matA.bMatrixConductanceMap[j].count(currentLabel + "-MPTR_PP") != 0) simJunctions[j].mptrPP = iFile.matA.bMatrixConductanceMap[j].at(currentLabel + "-MPTR_PP");
			if(iFile.matA.bMatrixConductanceMap[j].count(currentLabel + "-MPTR_PN") != 0) simJunctions[j].mptrPN = iFile.matA.bMatrixConductanceMap[j].at(currentLabel + "-MPTR_PN");
			if(iFile.matA.bMatrixConductanceMap[j].count(currentLabel + "-MPTR_NP") != 0) simJunctions[j].mptrNP = iFile.matA.bMatrixConductanceMap[j].at(currentLabel + "-MPTR_NP");
			if(iFile.matA.bMatrixConductanceMap[j].count(currentLabel + "-MPTR_NN") != 0) simJunctions[j].mptrNN = iFile.matA.bMatrixConductanceMap[j].at(currentLabel + "-MPTR_NN");
			simJunctions[j].lower = simJunctions[j].jjVg - 0.5 * simJunctions[j].delV;
			simJunctions[j].upper = simJunctions[j].jjVg + 0.5 * simJunctions[j].delV;
			simJunctions[j].subCond = (2 * simJunctions[j].jjCap)/iFile.tsim.prstep + 1/simJunctions[j].jjRzero;
			simJunctions[j].transCond = (2 * simJunctions[j].jjCap)/iFile.tsim.prstep + simJunctions[j].gLarge;
			simJunctions[j].normalCond = (2 * simJunctions[j].jjCap)/iFile.tsim.prstep + 1/simJunctions[j].jjRn;
			/* If the junction positive node is connected to ground */
			if (simJunctions[j].vPositive == -1) {
				simJunctions[j].VB = -lhsValues.at(simJunctions[j].vNegative);
				simJunctions[j].negativeNodeRow = iFile.matA.rowNames.at(simJunctions[j].vNegative);
			}
			/* If the junction negative node is connected to ground */
			else if (simJunctions[j].vNegative == -1) {
				simJunctions[j].VB = lhsValues.at(simJunctions[j].vPositive);
				simJunctions[j].positiveNodeRow = iFile.matA.rowNames.at(simJunctions[j].vPositive);
			}
			/* If both nodes are not connected to ground */
			else {
				simJunctions[j].VB = lhsValues.at(simJunctions[j].vPositive) - lhsValues.at(simJunctions[j].vNegative);
				simJunctions[j].positiveNodeRow = iFile.matA.rowNames.at(simJunctions[j].vPositive);
				simJunctions[j].negativeNodeRow = iFile.matA.rowNames.at(simJunctions[j].vNegative);
			}
			iFile.junctionCurrents[j].clear();
		}
		else if (j[2] == 'L') {
			currentLabel = j.substr(2);
			inductanceVector[rowCounter] = iFile.matA.impedanceMap[j];
			iPNC[rowCounter] = iFile.matA.bMatrixConductanceMap[j].at(currentLabel + "-VP");
			iNNC[rowCounter] = iFile.matA.bMatrixConductanceMap[j].at(currentLabel + "-VN");
			iCNC[rowCounter] = iFile.matA.bMatrixConductanceMap[j].at(currentLabel + "-I");
		}
		else if (j[2] == 'N') {
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
	/* Now the loop */
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
						if (j == simJunctions.at("R_" + k).positiveNodeRow) RHSvalue += simJunctions.at("R_" + k).Is;
						else if (j == simJunctions.at("R_" + k).negativeNodeRow) RHSvalue -= simJunctions.at("R_" + k).Is;
					}
					/* Add the current value to the RHS in the correct row */
					else if (k[0] == 'I') {
						RHSvalue += iFile.matA.sources[k][i];
					}
				}
			}
			/* If this row item is identified as an inductor row */
			else if (j[2] == 'L') {
				/* Identify the inductor label */
				currentLabel = j.substr(2);
				/* Identify the relevant inductance of the inductor */
				inductance = inductanceVector[rowCounter];
				/* Identify the column index of the positive node */
				VP = iPNC[rowCounter];
				/* Identify the column index of the negative node */
				VN = iNNC[rowCounter];
				/* Try to identify the column index of the inductor current node */
				try {
					CUR = iCNC[rowCounter];
					LCUR = lhsValues.at((int)CUR);
				}
				catch (const std::out_of_range&) { simulation_errors(INDUCTOR_CURRENT_NOT_FOUND, currentLabel); }
				/* If the inductor positive node is connected to ground */
				if (VP == -1.0) VB = -lhsValues.at((int)VN);
				/* If the inductor negative node is connected to ground */
				else if (VN == -1.0) VB = lhsValues.at((int)VP);
				/* If both nodes are not connected to ground */
				else VB = lhsValues.at((int)VP) - lhsValues.at((int)VN);
				if (!iFile.matA.branchRelations[currentLabel].mutI2.empty()) {
					RHSvalue = (-2 * inductance / iFile.tsim.prstep)*LCUR - VB;
					for (int c = 0; c < iFile.matA.branchRelations[currentLabel].mutI2.size(); c++) {
						CUR2 = lhsValues.at(iFile.matA.branchRelations[currentLabel].current2RowIndex[c]);
						mutualL = iFile.matA.branchRelations[currentLabel].mutualInductance[c];
						RHSvalue -= mutualL * CUR2;
					}
				}
				else {
					/* R_L = (-2L/hn)IL - VL*/
					RHSvalue = (-2 * inductance / iFile.tsim.prstep)*LCUR - VB;
				}
			}
			/* If this row item is identified as a junction row */
			else if (j[2] == 'B') {
				/* Identify the junction label */
				currentLabel = j.substr(2);
				/* R_B = Phi(n-1) + (hn/2)(2e/hbar)VB */
				RHSvalue = simJunctions[j].Phase + ((hn_2_2e_hbar)*simJunctions[j].VB);
			}
			/* If this row item is identified as a voltage source row */
			else if (j[2] == 'V') {
				/* Identify the voltage source label */
				currentLabel = j.substr(2);
				/* Assign the voltage source value at the current point in the time loop to the RHS value */
				RHSvalue = iFile.matA.sources[currentLabel][i];
			}
			else if (j[2] == 'T') {
				/* Identify the transmission line label */
				currentLabel = j.substr(2);
				char OneOrTwo = currentLabel.at(currentLabel.find('-') + 2);
				currentLabel.erase(currentLabel.find('-'), 3);
				imintd = i - (iFile.matA.xlines[currentLabel].TD / iFile.tsim.prstep);
				switch (OneOrTwo) {
				case '1':
					if ((imintd) > 0) {
						/* Assign the voltage source value at the time - TD to the RHS value */
						VP = iFile.matA.xlines[currentLabel].pNode2;
						VN = iFile.matA.xlines[currentLabel].nNode2;
						/* If the xline positive node is connected to ground */
						if (VP == -1.0) VB = -iFile.xVect.at((int)VN)[imintd];
						/* If the xline negative node is connected to ground */
						else if (VN == -1.0) VB = iFile.xVect.at((int)VP)[imintd];
						/* If both nodes are not connected to ground */
						else VB = iFile.xVect.at((int)VP)[imintd] - iFile.xVect.at((int)VN)[imintd];
						VN = iFile.matA.xlines[currentLabel].iNode2;
						z0voltage = (iFile.xVect.at((int)VP)[imintd] - iFile.xVect.at((int)VN)[imintd]); //xlines[currentLabel].Z0 * (x.at((int)VP)[imintd] - x.at((int)VN)[imintd]);
						RHSvalue = VB + z0voltage;
					}
					else {
						RHSvalue = 0;
					}
					break;
				case '2':
					if ((imintd) > 0) {
						/* Assign the voltage source value at the time - TD to the RHS value */
						VP = iFile.matA.xlines[currentLabel].pNode1;
						VN = iFile.matA.xlines[currentLabel].nNode1;
						/* If the xline positive node is connected to ground */
						if (VP == -1.0) VB = -iFile.xVect.at((int)VN)[imintd];
						/* If the xline negative node is connected to ground */
						else if (VN == -1.0) VB = iFile.xVect.at((int)VP)[imintd];
						/* If both nodes are not connected to ground */
						else VB = iFile.xVect.at((int)VP)[imintd] - iFile.xVect.at((int)VN)[imintd];
						VN = iFile.matA.xlines[currentLabel].iNode1;
						z0voltage = (iFile.xVect.at((int)VP)[imintd] - iFile.xVect.at((int)VN)[imintd]); //xlines[currentLabel].Z0 * (x.at((int)VP)[imintd] - x.at((int)VN)[imintd]);
						RHSvalue = VB + z0voltage;
					}
					else {
						RHSvalue = 0;
					}
					break;
				}
			}
			/* Add the RHS value as determined above to the correct spot in the RHS vector */
			//RHS[columnIndex] = RHSvalue;
			RHS.push_back(RHSvalue);
			rowCounter++;
		}
		/* End of the B matrix initialization */

		/* Solve Ax=b */
		LHS_PRE = RHS;
		ok = klu_tsolve(Symbolic, Numeric, iFile.matA.Nsize, 1, &LHS_PRE.front(), &Common);
		if(!ok) {
			matrix_errors(MATRIX_SINGULAR, "");
		}

		/* Set the LHS values equal to the returning value provided by the KLU solution */
		lhsValues = LHS_PRE;
		for (int m = 0; m < lhsValues.size(); m++) {
			iFile.xVect[m][i] = lhsValues[m];
		}

		/* Guess next junction voltage */
		for (auto j : simJunctions) {
			rcsj_sim_object thisJunction = simJunctions.at(j.first);
			if (j.second.vPositive == -1) thisJunction.VB = (-lhsValues.at(j.second.vNegative));
			else if (j.second.vNegative == -1) thisJunction.VB = (lhsValues.at(j.second.vPositive));
			else thisJunction.VB = (lhsValues.at(j.second.vPositive) - lhsValues.at(j.second.vNegative));
			thisJunction.VB_dt = (2 / iFile.tsim.prstep)*(thisJunction.VB - thisJunction.VB_Prev) - thisJunction.VB_dt_Prev;
			thisJunction.VB_Guess = thisJunction.VB + iFile.tsim.prstep*thisJunction.VB_dt;
			if (thisJunction.jjRtype == 1) {
				if(fabs(thisJunction.VB_Guess) < thisJunction.lower) {
					thisJunction.transitionCurrent = 0.0;
					if(thisJunction.mptrPP != -1) {
						if(iFile.matA.mElements[thisJunction.mptrPP].value != thisJunction.subCond) {
							iFile.matA.mElements[thisJunction.mptrPP].value = thisJunction.subCond;
							needsLU = true;
						}
					}
					if(thisJunction.mptrNN != -1) {
						if(iFile.matA.mElements[thisJunction.mptrNN].value != thisJunction.subCond) {
							iFile.matA.mElements[thisJunction.mptrNN].value = thisJunction.subCond;
							needsLU = true;
						}
					}
					if(thisJunction.mptrPN != -1) {
						if(iFile.matA.mElements[thisJunction.mptrPN].value != -thisJunction.subCond) {
							iFile.matA.mElements[thisJunction.mptrPN].value = -thisJunction.subCond;
							needsLU = true;
						}
					}
					if(thisJunction.mptrNP != -1) {
						if(iFile.matA.mElements[thisJunction.mptrNP].value != -thisJunction.subCond) {
							iFile.matA.mElements[thisJunction.mptrNP].value = -thisJunction.subCond;
							needsLU = true;
						}
					}
				}
				else if(fabs(thisJunction.VB_Guess) < thisJunction.upper) {
					if(thisJunction.VB_Guess < 0) thisJunction.transitionCurrent = -thisJunction.lower*((1/thisJunction.jjRzero) - thisJunction.gLarge);
					else thisJunction.transitionCurrent = thisJunction.lower*((1/thisJunction.jjRzero) - thisJunction.gLarge);
					if(thisJunction.mptrPP != -1) {
						if(iFile.matA.mElements[thisJunction.mptrPP].value != thisJunction.transCond) {
							iFile.matA.mElements[thisJunction.mptrPP].value = thisJunction.transCond;
							needsLU = true;
						}
					}
					if(thisJunction.mptrNN != -1) {
						if(iFile.matA.mElements[thisJunction.mptrNN].value != thisJunction.transCond) {
							iFile.matA.mElements[thisJunction.mptrNN].value = thisJunction.transCond;
							needsLU = true;
						}
					}
					if(thisJunction.mptrPN != -1) {
						if(iFile.matA.mElements[thisJunction.mptrPN].value != thisJunction.transCond) {
							iFile.matA.mElements[thisJunction.mptrPN].value = -thisJunction.transCond;
							needsLU = true;
						}
					}
					if(thisJunction.mptrNP != -1) {
						if(iFile.matA.mElements[thisJunction.mptrNP].value != thisJunction.transCond) {
							iFile.matA.mElements[thisJunction.mptrNP].value = -thisJunction.transCond;
							needsLU = true;
						}
					}
				}
				else {
					if(thisJunction.VB_Guess < 0) thisJunction.transitionCurrent = -(thisJunction.jjIcrit/thisJunction.jjIcfact + thisJunction.jjVg * (1/thisJunction.jjRzero) - thisJunction.lower * (1/thisJunction.jjRn));
					else thisJunction.transitionCurrent = (thisJunction.jjIcrit/thisJunction.jjIcfact + thisJunction.jjVg * (1/thisJunction.jjRzero) - thisJunction.lower * (1/thisJunction.jjRn));
					if(thisJunction.mptrPP != -1) {
						if(iFile.matA.mElements[thisJunction.mptrPP].value != thisJunction.normalCond) {
							iFile.matA.mElements[thisJunction.mptrPP].value = thisJunction.normalCond;
							needsLU = true;
						}
					}
					if(thisJunction.mptrNN != -1) {
						if(iFile.matA.mElements[thisJunction.mptrNN].value != thisJunction.normalCond) {
							iFile.matA.mElements[thisJunction.mptrNN].value = thisJunction.normalCond;
							needsLU = true;
						}
					}
					if(thisJunction.mptrPN != -1) {
						if(iFile.matA.mElements[thisJunction.mptrPN].value != thisJunction.normalCond) {
							iFile.matA.mElements[thisJunction.mptrPN].value = -thisJunction.normalCond;
							needsLU = true;
						}
					}
					if(thisJunction.mptrNP != -1) {
						if(iFile.matA.mElements[thisJunction.mptrNP].value != thisJunction.normalCond) {
							iFile.matA.mElements[thisJunction.mptrNP].value = -thisJunction.normalCond;
							needsLU = true;
						}
					}
				}
			}
			thisJunction.Phase = lhsValues.at(j.second.bPhase);
			thisJunction.Phase_Guess = thisJunction.Phase + (hn_2_2e_hbar)*(thisJunction.VB + thisJunction.VB_Guess);
			thisJunction.Is = -thisJunction.jjIcrit * sin(thisJunction.Phase_Guess) + (((2 * thisJunction.jjCap) / iFile.tsim.prstep)*thisJunction.VB) + (thisJunction.jjCap * thisJunction.VB_dt) - thisJunction.transitionCurrent;
			thisJunction.VB_Prev = thisJunction.VB;
			thisJunction.VB_dt_Prev = thisJunction.VB_dt;
			thisJunction.Phase_Prev = thisJunction.Phase;
			simJunctions.at(j.first) = thisJunction;
			/* Store the junction currents for printing */
			iFile.junctionCurrents.at(j.first).push_back(thisJunction.Is);
		}
		if(needsLU) {
				csr_A_matrix(iFile);
				/* Do numeric factorization of matrix */
				Numeric = klu_factor(&iFile.matA.rowptr.front(), &iFile.matA.colind.front(), &iFile.matA.nzval.front(), Symbolic, &Common);
				needsLU  = false;
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

/*
	Perform transient phase simulation
*/
void transient_phase_simulation(InputFile& iFile) {
	iFile.xVect.clear();
	iFile.timeAxis.clear();
	iFile.junctionCurrents.clear();
	/* Standard vector */
	std::vector<double> lhsValues(iFile.matA.Nsize, 0.0);
	int simSize = iFile.tsim.simsize();
	/* Where to store the calculated values */
	for (int m = 0; m < iFile.matA.Nsize; m++) {
		iFile.xVect.emplace_back(std::vector<double>(simSize, 0.0));
	}
	/* Perform time loop */
	std::vector<double> RHS(iFile.matA.columnNames.size(), 0.0), LHS_PRE, impedanceVector(iFile.matA.rowNames.size()), iPNC(iFile.matA.rowNames.size()), iNNC(iFile.matA.rowNames.size()), iCNC(iFile.matA.rowNames.size());
	std::vector<std::vector<std::string>> nodeConnectionVector(iFile.matA.rowNames.size());
	/* Variables to be used by the RHS matrix construction routine */
	std::string currentLabel, columnIndexLabel;
	std::vector<std::string> tokens;
	double PP, PN, CUR, RCUR, Pn, RHSvalue, impedance, z0voltage;
	double hn_2_2e_hbar = (iFile.tsim.prstep / 2)*(2 * M_PI / PHI_ZERO);
	int ok, rowCounter;
	bool needsLU = false;
	klu_symbolic * Symbolic;
	klu_common Common;
	klu_numeric * Numeric;
	/* Set the default settings for KLU */
	ok = klu_defaults(&Common);
	/* Do symbolic analysis of matrix */
	Symbolic = klu_analyze(iFile.matA.Nsize, &iFile.matA.rowptr.front(), &iFile.matA.colind.front(), &Common);
	/* Do numeric factorization of matrix */
	Numeric = klu_factor(&iFile.matA.rowptr.front(), &iFile.matA.colind.front(), &iFile.matA.nzval.front(), Symbolic, &Common);
	/* Set up the junctions */
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
						if (j == iFile.pJJ.at(k).pPositiveR)
							RHSvalue += iFile.pJJ.at(k).Is;
						else
							RHSvalue -= iFile.pJJ.at(k).Is;
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
				if (iFile.pRes[currentLabel].pPR == -1.0)
					iFile.pRes[currentLabel].Pn1 = -lhsValues.at(iFile.pRes[currentLabel].pNR);
				else if (iFile.pRes[currentLabel].pNR == -1.0)
					iFile.pRes[currentLabel].Pn1 = lhsValues.at(iFile.pRes[currentLabel].pPR);
				else
					iFile.pRes[currentLabel].Pn1 = lhsValues.at(iFile.pRes[currentLabel].pPR) - lhsValues.at(iFile.pRes[currentLabel].pNR);
				iFile.pRes[currentLabel].IRn1 = lhsValues.at(iFile.pRes[currentLabel].bIR);
				/* R_L = (2pi Rhn / 2Phi0)IR + PR*/
				RHSvalue += ((M_PI * iFile.pRes[currentLabel].R * iFile.tsim.prstep) / PHI_ZERO) * iFile.pRes[currentLabel].IRn1 + iFile.pRes[currentLabel].Pn1;
			}
			/* If this row item is identified as a junction row */
			else if (j[2] == 'B') {
				currentLabel = j.substr(2);
				currentLabel = currentLabel.substr(0, currentLabel.size() - 3);
				if(j.substr(j.size() - 3) == "-VJ") {
					RHSvalue = iFile.pJJ.at(currentLabel).Pn1 + hn_2_2e_hbar * iFile.pJJ.at(currentLabel).Vn1;
				}
			}
			/* If this row item is identified as a junction row */
			else if (j[2] == 'C') {
				currentLabel = j.substr(2);
				RHSvalue = -((2*M_PI*iFile.tsim.prstep*iFile.tsim.prstep)/(4 * PHI_ZERO * iFile.pCap[currentLabel].C)) * iFile.pCap[currentLabel].ICn1 - iFile.pCap[currentLabel].Pn1 - (iFile.tsim.prstep * iFile.pCap[currentLabel].dPn1);
			}
			/* If this row item is identified as a voltage source row */
			else if (j[2] == 'V') {
				currentLabel = j.substr(2);
				if (iFile.pVS[currentLabel].pPR == -1.0)
					iFile.pVS[currentLabel].Pn1 = -lhsValues.at(iFile.pVS[currentLabel].pNR);
				else if (iFile.pVS[currentLabel].pNR == -1.0)
					iFile.pVS[currentLabel].Pn1 = lhsValues.at(iFile.pVS[currentLabel].pPR);
				else
					iFile.pVS[currentLabel].Pn1 = lhsValues.at(iFile.pVS[currentLabel].pPR) - lhsValues.at(iFile.pVS[currentLabel].pNR);
				if (i >= 1)
					RHSvalue = iFile.pVS[currentLabel].Pn1 + ((iFile.tsim.prstep * M_PI) / PHI_ZERO) * (iFile.matA.sources[currentLabel][i] + iFile.matA.sources[currentLabel][i-1]);
				else if (i == 0)
					RHSvalue = iFile.pVS[currentLabel].Pn1 + ((iFile.tsim.prstep  * M_PI) / PHI_ZERO) * iFile.matA.sources[currentLabel][i];
			}
			else if (j[2] == 'T') {
				currentLabel = j.substr(2);
				currentLabel = currentLabel.substr(0, currentLabel.size() - 3);
				if(j.substr(j.size() - 3) == "-I1") {
					RHSvalue = ((iFile.tsim.prstep * M_PI * iFile.pTL[currentLabel].Z0)/(PHI_ZERO)) * iFile.pTL[currentLabel].I2nk + iFile.pTL[currentLabel].P1n1 + (iFile.tsim.prstep / 2) * (iFile.pTL[currentLabel].dP1n1 + iFile.pTL[currentLabel].dP2nk);
				}
				else if(j.substr(j.size() - 3) == "-I2") {
					RHSvalue = ((iFile.tsim.prstep * M_PI * iFile.pTL[currentLabel].Z0)/(PHI_ZERO)) * iFile.pTL[currentLabel].I1nk + iFile.pTL[currentLabel].P2n1 + (iFile.tsim.prstep / 2) * (iFile.pTL[currentLabel].dP2n1 + iFile.pTL[currentLabel].dP1nk);
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
			matrix_errors(MATRIX_SINGULAR, "");
		}

		/* Set the LHS values equal to the returning value provided by the KLU solution */
		lhsValues = LHS_PRE;
		for (int m = 0; m < lhsValues.size(); m++) {
			iFile.xVect[m][i] = lhsValues[m];
		}

		/* Guess next junction voltage */
		for (auto j : iFile.pJJ) {
			phase_JJ thisJJ = iFile.pJJ.at(j.first);
			// Phi (Junction Phase Calculated for this timestep)
			if (thisJJ.pPC == -1)
				thisJJ.Pn1 = (-lhsValues.at(thisJJ.pNC));
			else if (thisJJ.pNC == -1)
				thisJJ.Pn1 = (lhsValues.at(thisJJ.pPC));
			else
				thisJJ.Pn1 = (lhsValues.at(thisJJ.pPC) - lhsValues.at(thisJJ.pNC));
			// Vb (Junction Voltage Calculated for this timestep)
			thisJJ.Vn1 = (lhsValues.at(thisJJ.vC));
			thisJJ.dVn1 = (2 / iFile.tsim.prstep)*(thisJJ.Vn1 - thisJJ.Vn2) - thisJJ.dVn2;
			thisJJ.Vn2 = thisJJ.Vn1;
			thisJJ.dVn2 = thisJJ.dVn1;
			thisJJ.V0 = thisJJ.Vn1 + iFile.tsim.prstep * thisJJ.dVn1;
			thisJJ.dPn1 = (2 / iFile.tsim.prstep)*(thisJJ.Pn1 - thisJJ.Pn2) - thisJJ.dPn2;
			thisJJ.Phi0 = thisJJ.Pn1 + hn_2_2e_hbar * (thisJJ.Vn1 + thisJJ.V0);
			thisJJ.Pn2 = thisJJ.Pn1;
			thisJJ.dPn2 = thisJJ.dPn1;
			if (thisJJ.Rtype == 1) {
				if(fabs(thisJJ.V0) < thisJJ.lower) {
					thisJJ.It = 0.0;
					if(thisJJ.RmptrP != -1) {
						if(iFile.matA.mElements[thisJJ.RmptrP].value != thisJJ.subCond) {
							iFile.matA.mElements[thisJJ.RmptrP].value = thisJJ.subCond;
							needsLU = true;
						}
					}
					if(thisJJ.RmptrN != -1) {
						if(iFile.matA.mElements[thisJJ.RmptrN].value != -thisJJ.subCond) {
							iFile.matA.mElements[thisJJ.RmptrN].value = -thisJJ.subCond;
							needsLU = true;
						}
					}
				}
				else if(fabs(thisJJ.V0) < thisJJ.upper) {
					if(thisJJ.V0 < 0) thisJJ.It = -thisJJ.Vg*((1/thisJJ.R0) - thisJJ.gLarge);
					else thisJJ.It = thisJJ.Vg * ((1/thisJJ.R0) - thisJJ.gLarge);
					if(thisJJ.RmptrP != -1) {
						if(iFile.matA.mElements[thisJJ.RmptrP].value != thisJJ.transCond) {
							iFile.matA.mElements[thisJJ.RmptrP].value = thisJJ.transCond;
							needsLU = true;
						}
					}
					if(thisJJ.RmptrN != -1) {
						if(iFile.matA.mElements[thisJJ.RmptrN].value != -thisJJ.transCond) {
							iFile.matA.mElements[thisJJ.RmptrN].value = -thisJJ.transCond;
							needsLU = true;
						}
					}
				}
				else {
					if(thisJJ.V0 < 0) thisJJ.It = -((thisJJ.Ic/thisJJ.IcFact) + ((1/thisJJ.R0) * thisJJ.lower) - ((1/thisJJ.RN) * thisJJ.upper));
					else thisJJ.It = ((thisJJ.Ic/thisJJ.IcFact) + ((1/thisJJ.R0) * thisJJ.lower) - ((1/thisJJ.RN) * thisJJ.upper));
					if(thisJJ.RmptrP != -1) {
						if(iFile.matA.mElements[thisJJ.RmptrP].value != thisJJ.normalCond) {
							iFile.matA.mElements[thisJJ.RmptrP].value = thisJJ.normalCond;
							needsLU = true;
						}
					}
					if(thisJJ.RmptrN != -1) {
						if(iFile.matA.mElements[thisJJ.RmptrN].value != -thisJJ.normalCond) {
							iFile.matA.mElements[thisJJ.RmptrN].value = -thisJJ.normalCond;
							needsLU = true;
						}
					}
				}
			}
			thisJJ.Is = -thisJJ.Ic * sin(thisJJ.Phi0) + ((2 * thisJJ.C) / iFile.tsim.prstep) * thisJJ.Vn1 + thisJJ.C * thisJJ.dVn1 - thisJJ.It;
			iFile.pJJ.at(j.first) = thisJJ;
			/* Store the junction currents for printing */
			iFile.pJJ.at(j.first).junctionCurrent.push_back(thisJJ.Is);
		}
		/* Calculate next Cap values */
		for (auto j : iFile.pCap) {
			phase_Capacitor thisCap = iFile.pCap.at(j.first);
			thisCap.Pn2 = thisCap.Pn1;
			if (j.second.pPR == -1)
				thisCap.Pn1 = (-lhsValues.at(j.second.pNR));
			else if (j.second.pNR == -1)
				thisCap.Pn1 = (lhsValues.at(j.second.pPR));
			else
				thisCap.Pn1 = (lhsValues.at(j.second.pPR) - lhsValues.at(j.second.pNR));
			thisCap.ICn1 = lhsValues.at(thisCap.bIR);
			thisCap.dPn1 = (2/iFile.tsim.prstep) * (thisCap.Pn1 - thisCap.Pn2) - thisCap.dPn2;
			thisCap.dPn2 = thisCap.dPn1;
		}
		/* Calculate next TL values */
		for (auto j : iFile.pTL) {
			phase_TL thisTL = iFile.pTL.at(j.first);
			if (j.second.pPR1 == -1) {
				thisTL.P1n1 = (-lhsValues.at(j.second.pNR1));
			}
			else if (j.second.pNR1 == -1) {
				thisTL.P1n1 = (lhsValues.at(j.second.pPR1));
			}
			else {
				thisTL.P1n1 = (lhsValues.at(j.second.pPR1) - lhsValues.at(j.second.pNR1));
			}
			if (j.second.pPR2 == -1) {
				thisTL.P2n1 = (-lhsValues.at(j.second.pNR2));
			}
			else if (j.second.pNR2 == -1) {
				thisTL.P2n1 = (lhsValues.at(j.second.pPR2));
			}
			else {
				thisTL.P2n1 = (lhsValues.at(j.second.pPR2) - lhsValues.at(j.second.pNR2));
			}
			thisTL.dP1n1 = (2/iFile.tsim.prstep)*(thisTL.P1n1 - thisTL.P1n2) - thisTL.dP1n2;
			thisTL.P1n2 = thisTL.P1n1;
			thisTL.dP1n2 = thisTL.dP1n1;
			thisTL.dP2n1 = (2/iFile.tsim.prstep)*(thisTL.P2n1 - thisTL.P2n2) - thisTL.dP2n2;
			thisTL.P2n2 = thisTL.P2n1;
			thisTL.dP2n2 = thisTL.dP2n1;
			if(i >= thisTL.k) {
				thisTL.I2nk = iFile.xVect[thisTL.bIR2][i-thisTL.k];
				thisTL.I1nk = iFile.xVect[thisTL.bIR1][i-thisTL.k];
				if (thisTL.pPR1 == -1) {
					thisTL.P1nk = -iFile.xVect[thisTL.pNR1][i-thisTL.k];
				}
				else if (thisTL.pNR1 == -1) {
					thisTL.P1nk = iFile.xVect[thisTL.pPR1][i-thisTL.k];
				}
				else {
					thisTL.P1nk = iFile.xVect[thisTL.pPR1][i-thisTL.k] - iFile.xVect[thisTL.pNR1][i-thisTL.k];
				}
				if (thisTL.pPR2 == -1) {
					thisTL.P2nk = -iFile.xVect[thisTL.pNR2][i-thisTL.k];
				}
				else if (thisTL.pNR2 == -1) {
					thisTL.P2nk = iFile.xVect[thisTL.pPR2][i-thisTL.k];
				}
				else {
					thisTL.P2nk = iFile.xVect[thisTL.pPR2][i-thisTL.k] - iFile.xVect[thisTL.pNR2][i-thisTL.k]; 
				}
				thisTL.dP1nk = (2/iFile.tsim.prstep)*(thisTL.P1nk - thisTL.P1nk1) - thisTL.dP1nk1;
				thisTL.P1nk1 = thisTL.P1nk;
				thisTL.dP1nk1 = thisTL.dP1nk;
				thisTL.dP2nk = (2/iFile.tsim.prstep)*(thisTL.P2nk - thisTL.P2nk1) - thisTL.dP2nk1;
				thisTL.P2nk1 = thisTL.P2nk;
				thisTL.dP2nk1 = thisTL.dP2nk;
			}
			iFile.pTL.at(j.first) = thisTL;
		}
		if(needsLU) {
				csr_A_matrix(iFile);
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