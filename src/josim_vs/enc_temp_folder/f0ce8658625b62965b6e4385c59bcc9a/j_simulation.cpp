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
	/* Set up initial conditions for junctions */
	std::map<std::string, double> initialConditionsMap;
	for (auto i : rowNames) {
		if (i.find("_B") != std::string::npos) {
			initialConditionsMap[i + "-V_PREV"] = 0.0;
			initialConditionsMap[i + "-V_dt_PREV"] = 0.0;
			initialConditionsMap[i + "-P_PREV"] = 0.0;
			initialConditionsMap[i + "-Is"] = 0.0;
		}
	}
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
	double RHSvalue, inductance;
	std::string currentLabel;
	std::map<std::string, double> currentConductance;
	std::vector<std::string> tokens;
	double VP, VN, CUR, PH, VB, Phase, VB_dt, VB_guess, Phase_guess, Is, jjIcrit, jjCap;
	double hn_2_2e_hbar = (tsim.maxtstep / 2)*(2 * M_PI / PHI_ZERO);
	int counter;
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
			if (j.find("_N") != std::string::npos) {
				tokens.clear();
				for (auto k : currentConductance) {
					if (k.first[0] == 'B') {
						if (k.first.find("-") != std::string::npos) currentLabel = substring_before(k.first, "-");
						else currentLabel = k.first;
						unique_push(tokens, currentLabel);
						//RHSvalue += initialConditionsMap["R_" + currentLabel]["Is"];
					}
					else if (k.first[0] == 'I') {
						currentLabel = k.first;
						unique_push(tokens, currentLabel);
						//RHSvalue += k.second * sources[k.first][i];
					}
					else if (k.first[0] == 'R') {
						if (k.first.find("-") != std::string::npos) currentLabel = substring_before(k.first, "-");
						else currentLabel = k.first;
						unique_push(tokens, currentLabel);
					}
					else if (k.first[0] == 'C') {
						if (k.first.find("-") != std::string::npos) currentLabel = substring_before(k.first, "-");
						else currentLabel = k.first;
						unique_push(tokens, currentLabel);
					}
					else if (k.first[0] == 'L') {
						if (k.first.find("-") != std::string::npos) currentLabel = substring_before(k.first, "-");
						else currentLabel = k.first;
						unique_push(tokens, currentLabel);
					}
				}
				for (auto k : tokens) {
					if (k[0] == 'B') RHSvalue += initialConditionsMap["R_" + k + "-Is"];
					else if (k[0] == 'I') RHSvalue += sources[k][i];
				}
			}
			else if (j.find("_L") != std::string::npos) {
				currentLabel = substring_after(j, "R_");
				inductance = currentConductance[currentLabel];
				try { VP = currentConductance.at(currentLabel + "-VP"); }
				catch (std::out_of_range) { VP = -1.0; }
				try { VN = currentConductance.at(currentLabel + "-VN"); }
				catch (std::out_of_range) { VN = -1.0; }
				try { CUR = currentConductance.at(currentLabel + "-I"); }
				catch (std::out_of_range) { CUR = -1.0; }
				if (VP == -1.0) RHSvalue = (-2*inductance/tsim.maxtstep)*lhsValues[(int)CUR] - ( -lhsValues[(int)VN]);
				else if (VN == -1.0) RHSvalue = (-2 * inductance / tsim.maxtstep)*lhsValues[(int)CUR] - (lhsValues[(int)VP]);
				else RHSvalue = (-2 * inductance / tsim.maxtstep)*lhsValues[(int)CUR] - (lhsValues[(int)VP] - lhsValues[(int)VN]);
			}
			else if (j.find("_B") != std::string::npos) {
				currentLabel = substring_after(j, "R_");
				try { VP = currentConductance.at(currentLabel + "-VP"); }
				catch (std::out_of_range) { VP = -1.0;  }
				try { VN = currentConductance.at(currentLabel + "-VN"); }
				catch (std::out_of_range) { VN = -1.0; }
				if (VP == -1.0) RHSvalue = initialConditionsMap["R_" + currentLabel + "-P_PREV"] + ((hn_2_2e_hbar)*(-lhsValues[(int)VN]));
				else if (VN == -1.0) RHSvalue = initialConditionsMap["R_" + currentLabel + "-P_PREV"] + ((hn_2_2e_hbar)*(lhsValues[(int)VP]));
				else RHSvalue = initialConditionsMap["R_" + currentLabel + "-P_PREV"] + ((hn_2_2e_hbar)*(lhsValues[(int)VP] - lhsValues[(int)VN]));
			}
			else if (j.find("_V") != std::string::npos) {
				currentLabel = substring_after(j, "R_");
				RHSvalue = sources[currentLabel][i];
			}
			RHS.push_back(RHSvalue);
		}
		/* End of the B matrix initialization */
		/* Solve Ax=b */
		klu_symbolic * Symbolic;
		klu_common Common;
		klu_numeric * Numeric;
		int ok, ldim, nrhs;
		ok = klu_defaults(&Common);
		Symbolic = klu_analyze(Nsize, &rowptr.front(), &colind.front(), &Common);
		Numeric = klu_factor(&rowptr.front(), &colind.front(), &nzval.front(), Symbolic, &Common);
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
		for (auto j : rowNames) {
			if (j.find("_B") != std::string::npos) {
				currentLabel = substring_after(j, "R_");
				try { VP = bMatrixConductanceMap.at(j).at(currentLabel + "-VP"); }
				catch (std::out_of_range) { VP = -1.0; }
				try { VN = bMatrixConductanceMap.at(j).at(currentLabel + "-VN"); }
				catch (std::out_of_range) { VN = -1.0; }
				try { PH = bMatrixConductanceMap.at(j).at(currentLabel + "-PHASE"); }
				catch (std::out_of_range) { PH = -1.0; }
				if (VP == -1.0) VB = (-lhsValues[(int)VN]);
				else if (VN == -1.0) VB = (lhsValues[(int)VP]);
				else VB = (lhsValues[(int)VP] - lhsValues[(int)VN]);
				Phase = lhsValues[(int)PH];
				VB_dt = (2 / tsim.maxtstep)*(VB - initialConditionsMap["R_" + currentLabel + "-V_PREV"]) - initialConditionsMap["R_" + currentLabel + "-V_dt_PREV"];
				VB_guess = VB + tsim.maxtstep*VB_dt;
				Phase_guess = Phase + (hn_2_2e_hbar)*(VB + VB_guess);
				try { jjCap = bMatrixConductanceMap.at(j).at(currentLabel + "-CAP"); }
				catch (std::out_of_range) { simulation_errors(JJCAP_NOT_FOUND, currentLabel); }
				try { jjIcrit = bMatrixConductanceMap.at(j).at(currentLabel + "-ICRIT"); }
				catch (std::out_of_range) { simulation_errors(JJICRIT_NOT_FOUND, currentLabel); }
				Is = -jjIcrit * sin(Phase_guess) + (((2 * jjCap) / tsim.maxtstep)*VB) + (jjCap * VB_dt);
				initialConditionsMap["R_" + currentLabel + "-V_PREV"] = VB;
				initialConditionsMap["R_" + currentLabel + "-V_dt_PREV"] = VB_dt;
				initialConditionsMap["R_" + currentLabel + "-P_PREV"] = Phase;
				initialConditionsMap["R_" + currentLabel + "-Is"] = Is;
			}
		}
	}
}