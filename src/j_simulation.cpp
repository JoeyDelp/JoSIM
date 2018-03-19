#include "include/j_simulation.hpp"

trans_sim tsim;

void identify_simulation(InputFile& iFile) {
	std::string simline;
	for (auto i : iFile.controlPart) {
		if (i.find(".TRAN") != std::string::npos) {
			simline = i;
			iFile.simulationType = TRANSIENT;
			break;
		}
		if (i.find(".DC") != std::string::npos) {
			simline = i;
			iFile.simulationType = DC;
			break;
		}
		if (i.find(".AC") != std::string::npos) {
			simline = i;
			iFile.simulationType = AC;
		}
		if (i.find(".PHASE") != std::string::npos) {
			simline = i;
			iFile.simulationType = PHASE;
		}
	}

	std::vector<std::string> simtokens;
	switch (iFile.simulationType) {
	case TRANSIENT:
		simtokens = tokenize_delimeter(simline, " ,");
		if (simtokens.size() < 3) {
			control_errors(TRANS_ERROR, "Too few parameters");
		}
		else if (simtokens.size() >= 3) {
			tsim.prstep = modifier(simtokens[1]);
			tsim.tstop = modifier(simtokens[2]);
			if (simtokens.size() == 4) {
				tsim.tstart = modifier(simtokens[3]);
			}
			if (simtokens.size() == 5) {
				tsim.maxtstep = modifier(simtokens[4]);
			}
		}
		break;
	case DC:
	case AC:
	case PHASE:
	case NONE:
		control_errors(NO_SIM, "");
	}
}
/*
Perform transient simulation
*/
void transient_simulation() {
	/* Perform time loop */
	std::vector<double> RHS;
	/* Where to store the calulated values */
	/* Mapped */
	std::map<std::string, double> lhsMappedValues;
	/* Standard vector */
	std::vector<double> lhsValues;
	/* All stored mapped values*/
	std::vector<std::map<std::string, double>> lhs;
	/* Initialize x matrix */
	for (auto i : columnNames) {
		lhsMappedValues[i] = 0.0;
		lhsValues.push_back(0.0);
	}
	/* Variables to be used by the RHS matrix construction routine */
	std::string currentLabel;
	std::map<std::string, double> currentConductance;
	std::vector<std::string> tokens;
	std::map<std::string, rcsj_sim_object> simJunctions;
	double VP, VN, CUR, PH, LCUR, VB, Phase, VB_dt, VB_guess, Phase_guess, Is, jjIcrit, jjCap, RHSvalue, inductance;
	double hn_2_2e_hbar = (tsim.maxtstep / 2)*(2 * M_PI / PHI_ZERO);
	int counter, ok, ldim, nrhs;
	klu_symbolic * Symbolic;
	klu_common Common;
	klu_numeric * Numeric;
	/* Set the default settings for KLU */
	ok = klu_defaults(&Common);
	/* Do symbolic analysis of matrix */
	Symbolic = klu_analyze(Nsize, &rowptr.front(), &colind.front(), &Common);
	/* Do numeric factorization of matrix */
	Numeric = klu_factor(&rowptr.front(), &colind.front(), &nzval.front(), Symbolic, &Common);
	/***************/
	/** TIME LOOP **/
	/***************/
	for (int i = 0; i < tsim.simsize(); i++) {
		/* Start of initialization of the B matrix */
		/* Construct RHS matrix */
		RHS.clear();
		for (auto j : rowNames) {
			RHSvalue = 0.0;
			try { currentConductance = bMatrixConductanceMap.at(j); }
			catch (std::out_of_range) { }
			/* If this row item is identified as a node row then...*/
			if (j.find("_N") != std::string::npos) {
				tokens.clear();
				for (auto k : currentConductance) {
					/* If this row item contains a junction add it to the list of row items */
					if (k.first[0] == 'B') {
						if (k.first.find("-") != std::string::npos) currentLabel = substring_before(k.first, "-");
						else currentLabel = k.first;
						unique_push(tokens, currentLabel);
					}
					/* If this row item contains a current source add it to the list of row items */
					else if (k.first[0] == 'I') {
						currentLabel = k.first;
						unique_push(tokens, currentLabel);
					}
					/* If this row item contains a resistor add it to the list of row items */
					else if (k.first[0] == 'R') {
						if (k.first.find("-") != std::string::npos) currentLabel = substring_before(k.first, "-");
						else currentLabel = k.first;
						unique_push(tokens, currentLabel);
					}
					/* If this row item contains a capacitor add it to the list of row items */
					else if (k.first[0] == 'C') {
						if (k.first.find("-") != std::string::npos) currentLabel = substring_before(k.first, "-");
						else currentLabel = k.first;
						unique_push(tokens, currentLabel);
					}
					/* If this row item contains a inductor add it to the list of row items */
					else if (k.first[0] == 'L') {
						if (k.first.find("-") != std::string::npos) currentLabel = substring_before(k.first, "-");
						else currentLabel = k.first;
						unique_push(tokens, currentLabel);
					}
				}
				for (auto k : tokens) {
					/* Add junction as calculated at the end of the current loop to the RHS */
					if (k[0] == 'B') RHSvalue += simJunctions["R_" + k].Is;
					/* Add the current value to the RHS in the correct row */
					else if (k[0] == 'I') RHSvalue += sources[k][i];
				}
			}
			/* If this row item is identified as an inductor row */
			else if (j.find("_L") != std::string::npos) {
				/* Identify the inductor label */
				currentLabel = substring_after(j, "R_");
				/* Identify the relevant inductance of the inductor */
				inductance = currentConductance[currentLabel];
				/* Try to identify the column index of the positive node */
				try { VP = currentConductance.at(currentLabel + "-VP"); }
				catch (std::out_of_range) { VP = -1.0; }
				/* Try to identifiy the column index of the negative node */
				try { VN = currentConductance.at(currentLabel + "-VN"); }
				catch (std::out_of_range) { VN = -1.0; }
				/* Try to identifiy the column index of the inductor current node */
				try { 
					CUR = currentConductance.at(currentLabel + "-I");
					LCUR = lhsValues[(int)CUR];
				}
				catch (std::out_of_range) { simulation_errors(INDUCTOR_CURRENT_NOT_FOUND, currentLabel); }
				/* If the inductor positive node is connected to ground */
				if (VP == -1.0) VB = -lhsValues[(int)VN]; //RHSvalue = (-2*inductance/tsim.maxtstep)*lhsValues[(int)CUR] - ( -lhsValues[(int)VN]);
				/* If the inductor negative node is connected to ground */
				else if (VN == -1.0) VB = lhsValues[(int)VP]; //RHSvalue = (-2 * inductance / tsim.maxtstep)*lhsValues[(int)CUR] - (lhsValues[(int)VP]);
				/* If both nodes are not connected to ground */
				else VB = lhsValues[(int)VP] - lhsValues[(int)VN];
				/* R_L = (-2L/hn)IL - VL*/
				RHSvalue = (-2 * inductance / tsim.maxtstep)*LCUR - VB;
			}
			/* If this row item is identified as a junction row */
			else if (j.find("_B") != std::string::npos) {
				/* If this is the first loop iteration create the junction object*/
				if (i == 0) {
					/* Identify the junction label */
					currentLabel = substring_after(j, "R_");
					simJunctions[j].label = currentLabel;
					/* Try to identify the column index of the positive node */
					try { simJunctions[j].vPositive = (int)currentConductance.at(currentLabel + "-VP"); }
					catch (std::out_of_range) { }
					/* Try to identifiy the column index of the negative node */
					try { simJunctions[j].vNegative = (int)currentConductance.at(currentLabel + "-VN"); }
					catch (std::out_of_range) { }
					/* Try to identify the column index of the phase node, panick if not found */
					try { simJunctions[j].bPhase = (int)currentConductance.at(currentLabel + "-PHASE"); }
					catch (std::out_of_range) { simulation_errors(JJPHASE_NODE_NOT_FOUND, currentLabel); }
					/* Try to identify the junction capacitance, panick if not found */
					try { jjCap = bMatrixConductanceMap.at(j).at(currentLabel + "-CAP"); }
					catch (std::out_of_range) { simulation_errors(JJCAP_NOT_FOUND, currentLabel); }
					/* Try to identify the junction critical current, panick if not found */
					try { jjIcrit = bMatrixConductanceMap.at(j).at(currentLabel + "-ICRIT"); }
					catch (std::out_of_range) { simulation_errors(JJICRIT_NOT_FOUND, currentLabel); }
					/* If the junction positive node is connected to ground */
					if (simJunctions[j].vPositive == -1.0) simJunctions[j].VB = -lhsValues[simJunctions[j].vNegative];
					/* If the junction negativie node is connected to ground */
					else if (simJunctions[j].vNegative == -1.0) simJunctions[j].VB = lhsValues[simJunctions[j].vPositive];
					/* If both nodes are not connected to ground */
					else simJunctions[j].VB = lhsValues[simJunctions[j].vPositive - simJunctions[j].vNegative];
				}
				/* For every other iteration of the loop*/
				else {
					/* If the junction positive node is connected to ground */
					if (simJunctions[j].vPositive == -1.0) simJunctions[j].VB = -lhsValues[simJunctions[j].vNegative];
					/* If the junction negativie node is connected to ground */
					else if (simJunctions[j].vNegative == -1.0) simJunctions[j].VB = lhsValues[simJunctions[j].vPositive];
					/* If both nodes are not connected to ground */
					else simJunctions[j].VB = lhsValues[simJunctions[j].vPositive - simJunctions[j].vNegative];
				}
				/* R_B = Phi(n-1) + (hn/2)(2e/hbar)VB */
				RHSvalue = simJunctions[j].Phase_Prev + ((hn_2_2e_hbar)*simJunctions[j].VB);
			}
			/* If this row item is identified as a voltage source row */
			else if (j.find("_V") != std::string::npos) {
				/* Identify the voltage source label */
				currentLabel = substring_after(j, "R_");
				/* Assign the voltage source value at the current point in the time loop to the RHS value */
				RHSvalue = sources[currentLabel][i];
			}
			/* Add the RHS value as determined above to the correct spot in the RHS vector */
			RHS.push_back(RHSvalue);
		}
		/* End of the B matrix initialization */

		/* Solve Ax=b */
		ok = klu_solve(Symbolic, Numeric, Nsize, 1, &RHS.front(), &Common);

		/* Set the LHS values equal to the returning value provided by the KLU solution */
		lhsValues = RHS;
		counter = 0;
		/* Mapped LHS values for reference when printing values requested by the user */
		lhs.push_back(lhsMappedValues);
		for (auto m : lhsMappedValues) {
			lhs.at(i).at(m.first) = RHS[counter];
			counter++;
		}

		/* Guess next junction voltage */
		for (auto j : simJunctions) {
			if (j.second.vPositive == -1.0) j.second.VB = (-lhsValues[j.second.vNegative]);
			else if (j.second.vNegative == -1.0) j.second.VB = (lhsValues[j.second.vPositive]);
			else j.second.VB = (lhsValues[j.second.vPositive] - lhsValues[j.second.vNegative]);
			j.second.Phase = lhsValues[j.second.bPhase];
			j.second.VB_dt = (2 / tsim.maxtstep)*(j.second.VB - j.second.VB_Prev) - j.second.VB_dt_Prev;
			j.second.VB_Guess = j.second.VB + tsim.maxtstep*j.second.VB_dt;
			j.second.Phase_Guess = j.second.Phase + (hn_2_2e_hbar)*(j.second.VB + j.second.VB_Guess);
			j.second.Is = -j.second.jjIcrit * sin(j.second.Phase_Guess) + (((2 * j.second.jjCap) / tsim.maxtstep)*j.second.VB) + (j.second.jjCap * j.second.VB_dt);
			j.second.VB_Prev = j.second.VB;
			j.second.VB_dt_Prev = j.second.VB_dt;
			j.second.Phase_Prev = j.second.Phase;
		}
	}
}