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
				iFile.tsim.prstep = 1E-12;
				iFile.tsim.tstop = 1E-9;
				iFile.tsim.tstart = 0;
				iFile.tsim.maxtstep = 1E-12;
			}
			else {
				iFile.tsim.prstep = modifier(simtokens.at(1));
				if (simtokens.size() > 2) {
					iFile.tsim.tstop = modifier(simtokens.at(2));
					if (simtokens.size() > 3) {
						iFile.tsim.tstart = modifier(simtokens.at(3));
						if (simtokens.size() > 4) {
							iFile.tsim.maxtstep = modifier(simtokens.at(4));
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
Perform transient simulation
*/
/* Where to store the calculated values */
void transient_simulation(InputFile& iFile) {
	iFile.xVect.clear();
	iFile.timeAxis.clear();
	iFile.junctionCurrents.clear();
	/* Standard vector */
	std::vector<double> lhsValues(iFile.matA.Nsize, 0.0);
	int simSize = iFile.tsim.simsize();
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
	double hn_2_2e_hbar = (iFile.tsim.maxtstep / 2)*(2 * M_PI / PHI_ZERO);
	int ok, rowCounter;
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
		if (j.at(2) == 'B') {
			/* Identify the junction label */
			currentLabel = j.substr(2);
			columnIndexLabel = "C_P" + currentLabel;
			//columnIndex = index_of(columnNames, columnIndexLabel);
			simJunctions[j].label = currentLabel;
			/* Try to identify the column index of the positive node */
			simJunctions.at(j).vPositive = (int)iFile.matA.bMatrixConductanceMap.at(j).at(currentLabel + "-VP"); 
			/* Try to identify the column index of the negative node */
			simJunctions.at(j).vNegative = (int)iFile.matA.bMatrixConductanceMap.at(j).at(currentLabel + "-VN"); 
			/* Try to identify the column index of the phase node, panic if not found */
			try { simJunctions.at(j).bPhase = (int)iFile.matA.bMatrixConductanceMap.at(j).at(currentLabel + "-PHASE"); }
			catch (const std::out_of_range&) { simulation_errors(JJPHASE_NODE_NOT_FOUND, currentLabel); }
			/* Try to identify the junction capacitance, panic if not found */
			try { simJunctions.at(j).jjCap = iFile.matA.bMatrixConductanceMap.at(j).at(currentLabel + "-CAP"); }
			catch (const std::out_of_range&) { simulation_errors(JJCAP_NOT_FOUND, currentLabel); }
			/* Try to identify the junction critical current, panic if not found */
			try { simJunctions.at(j).jjIcrit = iFile.matA.bMatrixConductanceMap.at(j).at(currentLabel + "-ICRIT"); }
			catch (const std::out_of_range&) { simulation_errors(JJICRIT_NOT_FOUND, currentLabel); }
			simJunctions.at(j).jjVg = iFile.matA.bMatrixConductanceMap.at(j).at(currentLabel + "-VGAP");
			simJunctions.at(j).jjRtype = (int)iFile.matA.bMatrixConductanceMap.at(j).at(currentLabel + "-RTYPE");
			/* If the junction positive node is connected to ground */
			if (simJunctions.at(j).vPositive == -1) {
				simJunctions.at(j).VB = -lhsValues.at(simJunctions.at(j).vNegative);
				simJunctions.at(j).negativeNodeRow = iFile.matA.rowNames.at(simJunctions.at(j).vNegative);
			}
			/* If the junction negative node is connected to ground */
			else if (simJunctions.at(j).vNegative == -1) {
				simJunctions.at(j).VB = lhsValues.at(simJunctions.at(j).vPositive);
				simJunctions.at(j).positiveNodeRow = iFile.matA.rowNames.at(simJunctions.at(j).vPositive);
			}
			/* If both nodes are not connected to ground */
			else {
				simJunctions.at(j).VB = lhsValues.at(simJunctions.at(j).vPositive) - lhsValues.at(simJunctions.at(j).vNegative);
				simJunctions.at(j).positiveNodeRow = iFile.matA.rowNames.at(simJunctions.at(j).vPositive);
				simJunctions.at(j).negativeNodeRow = iFile.matA.rowNames.at(simJunctions.at(j).vNegative);
			}
			iFile.junctionCurrents[j].push_back(0);
		}
		else if (j.at(2) == 'L') {
			currentLabel = j.substr(2);
			inductanceVector[rowCounter] = iFile.matA.inductanceMap.at(j);
			iPNC[rowCounter] = iFile.matA.bMatrixConductanceMap.at(j).at(currentLabel + "-VP");
			iNNC[rowCounter] = iFile.matA.bMatrixConductanceMap.at(j).at(currentLabel + "-VN");
			iCNC[rowCounter] = iFile.matA.bMatrixConductanceMap.at(j).at(currentLabel + "-I");
		}
		else if (j.at(2) == 'N') {
			nodeConnectionVector[rowCounter] = iFile.matA.nodeConnections.at(j);
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
	for (int i = 0; i < simSize; i++) {
		std::cout << '\r';
		/* Start of initialization of the B matrix */
		RHS.clear();
		rowCounter = 0;
		/* Construct RHS matrix */
		for (auto j : iFile.matA.rowNames) {
			RHSvalue = 0.0;
			/* If this row item is identified as a node row then...*/
			if (j.at(2) == 'N') {
				tokens.clear();
				for (auto k : nodeConnectionVector.at(rowCounter)) {
					/* Add junction as calculated at the end of the current loop to the RHS */
					if (k.at(0) == 'B') {
						if(j == simJunctions.at("R_" + k).positiveNodeRow) RHSvalue += simJunctions.at("R_" + k).Is;
						else if (j == simJunctions.at("R_" + k).negativeNodeRow) RHSvalue -= simJunctions.at("R_" + k).Is;
					}
					/* Add the current value to the RHS in the correct row */
					else if (k.at(0) == 'I') {
						RHSvalue += iFile.matA.sources.at(k).at(i);
					}
				}
			}
			/* If this row item is identified as an inductor row */
			else if (j.at(2) == 'L') {
				/* Identify the inductor label */
				currentLabel = j.substr(2);
				/* Identify the relevant inductance of the inductor */
				inductance = inductanceVector.at(rowCounter);
				/* Identify the column index of the positive node */
				VP = iPNC.at(rowCounter);
				/* Identify the column index of the negative node */
				VN = iNNC.at(rowCounter);
				/* Try to identify the column index of the inductor current node */
				try { 
					CUR = iCNC.at(rowCounter);
					LCUR = lhsValues.at((int)CUR);
				}
				catch (const std::out_of_range&) { simulation_errors(INDUCTOR_CURRENT_NOT_FOUND, currentLabel); }
				/* If the inductor positive node is connected to ground */
				if (VP == -1.0) VB = -lhsValues.at((int)VN);
				/* If the inductor negative node is connected to ground */
				else if (VN == -1.0) VB = lhsValues.at((int)VP);
				/* If both nodes are not connected to ground */
				else VB = lhsValues.at((int)VP) - lhsValues.at((int)VN);
				/* R_L = (-2L/hn)IL - VL*/
				RHSvalue = (-2 * inductance / iFile.tsim.maxtstep)*LCUR - VB;
			}
			/* If this row item is identified as a junction row */
			else if (j.at(2) == 'B') {
				/* Identify the junction label */
				currentLabel = j.substr(2);
				/* R_B = Phi(n-1) + (hn/2)(2e/hbar)VB */
				RHSvalue = simJunctions.at(j).Phase_Prev + ((hn_2_2e_hbar)*simJunctions.at(j).VB);
			}
			/* If this row item is identified as a voltage source row */
			else if (j.at(2) == 'V') {
				/* Identify the voltage source label */
				currentLabel = j.substr(2);
				/* Assign the voltage source value at the current point in the time loop to the RHS value */
				RHSvalue = iFile.matA.sources.at(currentLabel).at(i);
			}
			else if (j.at(2) == 'T') {
                /* Identify the transmission line label */
                currentLabel = j.substr(2);
				char OneOrTwo = currentLabel.at(currentLabel.find('-') + 2);
				currentLabel.erase(currentLabel.find('-'), 3);
                imintd = i - (iFile.matA.xlines.at(currentLabel).TD/iFile.tsim.maxtstep);
				switch (OneOrTwo) {
				case '1':
					if ((imintd) > 0) {
						/* Assign the voltage source value at the time - TD to the RHS value */
						VP = iFile.matA.xlines.at(currentLabel).pNode2;
						VN = iFile.matA.xlines.at(currentLabel).nNode2;
						/* If the xline positive node is connected to ground */
						if (VP == -1.0) VB = -iFile.xVect.at((int)VN).at(imintd);
						/* If the xline negative node is connected to ground */
						else if (VN == -1.0) VB = iFile.xVect.at((int)VP).at(imintd);
						/* If both nodes are not connected to ground */
						else VB = iFile.xVect.at((int)VP).at(imintd) - iFile.xVect.at((int)VN).at(imintd);
						VN = iFile.matA.xlines.at(currentLabel).iNode2;
						z0voltage = (iFile.xVect.at((int)VP).at(imintd) - iFile.xVect.at((int)VN).at(imintd)); //xlines.at(currentLabel).Z0 * (x.at((int)VP).at(imintd) - x.at((int)VN).at(imintd));
						RHSvalue = VB + z0voltage;
					}
					else {
						RHSvalue = 0;
					}
					break;
				case '2':
					if ((imintd) > 0) {
						/* Assign the voltage source value at the time - TD to the RHS value */
						VP = iFile.matA.xlines.at(currentLabel).pNode1;
						VN = iFile.matA.xlines.at(currentLabel).nNode1;
						/* If the xline positive node is connected to ground */
						if (VP == -1.0) VB = -iFile.xVect.at((int)VN).at(imintd);
						/* If the xline negative node is connected to ground */
						else if (VN == -1.0) VB = iFile.xVect.at((int)VP).at(imintd);
						/* If both nodes are not connected to ground */
						else VB = iFile.xVect.at((int)VP).at(imintd) - iFile.xVect.at((int)VN).at(imintd);
						VN = iFile.matA.xlines.at(currentLabel).iNode1;
						z0voltage = (iFile.xVect.at((int)VP).at(imintd) - iFile.xVect.at((int)VN).at(imintd)); //xlines.at(currentLabel).Z0 * (x.at((int)VP).at(imintd) - x.at((int)VN).at(imintd));
						RHSvalue = VB + z0voltage;
					}
					else {
						RHSvalue = 0;
					}
					break;
				}
			}
			/* Add the RHS value as determined above to the correct spot in the RHS vector */
			//RHS.at(columnIndex) = RHSvalue;
			RHS.push_back(RHSvalue);
			rowCounter++;
		}
		/* End of the B matrix initialization */

		/* Solve Ax=b */
		LHS_PRE = RHS;
		ok = klu_tsolve(Symbolic, Numeric, iFile.matA.Nsize, 1, &LHS_PRE.front(), &Common);

		/* Set the LHS values equal to the returning value provided by the KLU solution */
		lhsValues = LHS_PRE;
		for (int m = 0; m < lhsValues.size(); m++) {
			iFile.xVect.at(m).at(i) = lhsValues.at(m);
		}

		/* Guess next junction voltage */
		for (auto j : simJunctions) {
			rcsj_sim_object thisJunction = simJunctions.at(j.first);
			if (j.second.vPositive == -1) thisJunction.VB = (-lhsValues.at(j.second.vNegative));
			else if (j.second.vNegative == -1) thisJunction.VB = (lhsValues.at(j.second.vPositive));
			else thisJunction.VB = (lhsValues.at(j.second.vPositive) - lhsValues.at(j.second.vNegative));
			if(thisJunction.jjRtype == 1) {
				if (thisJunction.VB >= thisJunction.jjVg && thisJunction.superconducting) {
					for(int k = 0; k < iFile.matA.mElements.size(); k++) {
						if(iFile.matA.mElements.at(k).label == thisJunction.label && iFile.matA.mElements.at(k).junctionEntry) {
							if(iFile.matA.mElements.at(k).junctionDirection == 'P')
								iFile.matA.mElements.at(k).value = ((2 * iFile.matA.mElements.at(k).tokens.at("CAP")) / iFile.tsim.maxtstep) + (1 / iFile.matA.mElements.at(k).tokens.at("RN"));
							else if (iFile.matA.mElements.at(k).junctionDirection == 'N')
								iFile.matA.mElements.at(k).value = -((2 * iFile.matA.mElements.at(k).tokens.at("CAP")) / iFile.tsim.maxtstep) + (1 / iFile.matA.mElements.at(k).tokens.at("RN"));
						}
					}
					thisJunction.superconducting = false;
					csr_A_matrix(iFile);
					/* Do numeric factorization of matrix */
					Numeric = klu_factor(&iFile.matA.rowptr.front(), &iFile.matA.colind.front(), &iFile.matA.nzval.front(), Symbolic, &Common);
				}
				else if (thisJunction.VB < thisJunction.jjVg && !thisJunction.superconducting) {
					for(int k = 0; k < iFile.matA.mElements.size(); k++) {
						if(iFile.matA.mElements.at(k).label == thisJunction.label && iFile.matA.mElements.at(k).junctionEntry) {
							if(iFile.matA.mElements.at(k).junctionDirection == 'P')
								iFile.matA.mElements.at(k).value = ((2 * iFile.matA.mElements.at(k).tokens.at("CAP")) / iFile.tsim.maxtstep) + (1 / iFile.matA.mElements.at(k).tokens.at("R0"));
							else if (iFile.matA.mElements.at(k).junctionDirection == 'N')
								iFile.matA.mElements.at(k).value = -((2 * iFile.matA.mElements.at(k).tokens.at("CAP")) / iFile.tsim.maxtstep) + (1 / iFile.matA.mElements.at(k).tokens.at("R0"));
						}
					}
					thisJunction.superconducting = true;
					csr_A_matrix(iFile);
					/* Do numeric factorization of matrix */
					Numeric = klu_factor(&iFile.matA.rowptr.front(), &iFile.matA.colind.front(), &iFile.matA.nzval.front(), Symbolic, &Common);
				}
			}
			thisJunction.Phase = lhsValues.at(j.second.bPhase);
			thisJunction.VB_dt = (2 / iFile.tsim.maxtstep)*(thisJunction.VB - thisJunction.VB_Prev) - thisJunction.VB_dt_Prev;
			thisJunction.VB_Guess = thisJunction.VB + iFile.tsim.maxtstep*thisJunction.VB_dt;
			thisJunction.Phase_Guess = thisJunction.Phase + (hn_2_2e_hbar)*(thisJunction.VB + thisJunction.VB_Guess);
			thisJunction.Is = -thisJunction.jjIcrit * sin(thisJunction.Phase_Guess) + (((2 * thisJunction.jjCap) / iFile.tsim.maxtstep)*thisJunction.VB) + (thisJunction.jjCap * thisJunction.VB_dt);
			thisJunction.VB_Prev = thisJunction.VB;
			thisJunction.VB_dt_Prev = thisJunction.VB_dt;
			thisJunction.Phase_Prev = thisJunction.Phase;
			simJunctions.at(j.first) = thisJunction;
			/* Store the junction currents for printing */
			iFile.junctionCurrents.at(j.first).push_back(thisJunction.Is);
		}
		/* Add the current time value to the time axis for plotting purposes */
		iFile.timeAxis.push_back(i*iFile.tsim.maxtstep);
		old_progress = progress;
		incremental_progress = incremental_progress + increments;
		progress = (int)(incremental_progress);
		if (progress > old_progress) {
			std::cout << std::setw(3) << std::right << std::fixed << std::setprecision(0) << progress << "%";
			pBar = "[";
			for (int p = 0; p <= (int)(progress_increments * i); p++) {
			  pBar.append("=");
			}
			std::cout << std::setw(31) << std::left << pBar << "]";
		}
	}
	std::cout << "\r" << std::setw(3) << std::right << std::fixed << std::setprecision(0) << 100 << "%" << std::setw(31) << std::left << pBar << "]\n";
}
