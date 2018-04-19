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
	std::map<std::string, std::map<std::string, double>> initialConditionsMap;
	for (auto i : rowNames) {
		if (i.find("_B") != std::string::npos) {
			initialConditionsMap[i]["V_PREV"] = 0.0;
			initialConditionsMap[i]["V_dt_PREV"] = 0.0;
			initialConditionsMap[i]["P_PREV"] = 0.0;
			initialConditionsMap[i]["Is"] = 0.0;
		}
	}
	std::vector<double> RHS;
	/* Where to store the calulated values */
	std::map<std::string, double> LHSvalues;
	std::map<std::string, std::vector<double>> LHS;
	for (auto i : columnNames) {
		/* Initial values of X (LHS) */
		LHSvalues[i] = 0.0;
	}
	double RHSvalue, inductance;
	std::string currentLabel;
	std::map<std::string, std::string> currentNode;
	std::map<std::string, double> currentConductance;
	std::vector<std::string> tokens;
	for (int i = 0; i < tsim.simsize(); i++) {
		/* Construct RHS matrix */
		RHS.clear();
		for (auto j : rowNames) {
			RHSvalue = 0.0;
			try { currentNode = bMatrixNodeMap.at(j); }
			catch (std::out_of_range) { }
			try { currentConductance = bMatrixConductanceMap.at(j); }
			catch (std::out_of_range) { }
			if (j.find("_N") != std::string::npos) {
				for (auto k : currentConductance) {
					if (k.first[0] == 'B') {
						currentLabel = substring_before(k.first, "_");
						RHSvalue += initialConditionsMap["R_" + currentLabel]["Is"];
					}
					if (k.first[0] == 'I') {
						RHSvalue += k.second * sources[k.first][i];
					}
				}
				RHS.push_back(RHSvalue);
			}
			else if (j.find("_L") != std::string::npos) {
				currentLabel = substring_after(j, "R_");
				inductance = currentConductance[currentLabel];
				tokens = tokenize_delimeter(currentNode[currentLabel + "-V"], "-");
				if (tokens[0] == "GND") RHSvalue = (2*inductance/tsim.maxtstep)*LHSvalues[currentNode[currentLabel + "-I"]] - ( -LHSvalues[tokens[1]]);
				else if (tokens[1] == "GND") RHSvalue = (2 * inductance / tsim.maxtstep)*LHSvalues[currentNode[currentLabel + "-I"]] - (LHSvalues[tokens[0]]);
				else RHSvalue = (2 * inductance / tsim.maxtstep)*LHSvalues[currentNode[currentLabel + "-I"]] - (LHSvalues[tokens[0]] - LHSvalues[tokens[1]]);
				RHS.push_back(RHSvalue);
			}
			else if (j.find("_B") != std::string::npos) {
				currentLabel = substring_after(j, "R_");
				tokens = tokenize_delimeter(currentNode[currentLabel + "-V"], "-");
				if (tokens[0] == "GND") RHSvalue = initialConditionsMap[currentNode[currentLabel + "-PHASE"]]["P_PREV"] + (((tsim.maxtstep / 2)*(2 * M_PI / PHI_ZERO))*(-LHSvalues[tokens[1]]));
				else if (tokens[1] == "GND") RHSvalue = initialConditionsMap[currentNode[currentLabel + "-PHASE"]]["P_PREV"] + (((tsim.maxtstep / 2)*(2 * M_PI / PHI_ZERO))*(LHSvalues[tokens[0]]));
				else RHSvalue = initialConditionsMap[currentNode[currentLabel + "-PHASE"]]["P_PREV"] + (((tsim.maxtstep / 2)*(2 * M_PI / PHI_ZERO))*(LHSvalues[tokens[0]] - LHSvalues[tokens[1]]));
				RHS.push_back(RHSvalue);
			}
			else if (j.find("_V") != std::string::npos) {
				currentLabel = substring_after(j, "R_");
				RHSvalue = sources[currentLabel][i];
				RHS.push_back(RHSvalue);
			}
		}
		/* Solve Ax=b */
		klu_symbolic * Symbolic;
		klu_common Common;
		klu_numeric * Numeric;
		int ok, ldim, nrhs;
		ok = klu_defaults(&Common);
		Symbolic = klu_analyze(Nsize, &rowptr.front(), &colind.front(), &Common);
		Numeric = klu_factor(&rowptr.front(), &colind.front(), &nzval.front(), Symbolic, &Common);
		ok = klu_solve(Symbolic, Numeric, Nsize, 1, &RHS.front(), &Common);

		/* Guess next junction voltage */
		for (auto j : bMatrixNodeMap) {
			if (j.first.find("_B") != std::string::npos) {
				currentLabel = substring_after(j.first, "R_");
				tokens = tokenize_delimeter(bMatrixNodeMap[j.first][currentLabel + "-V"], "-");
				double VB, Phase;
				if (tokens[0] == "GND") VB = (-LHSvalues[tokens[1]]);
				else if (tokens[1] == "GND") VB = (LHSvalues[tokens[0]]);
				else VB = (LHSvalues[tokens[0]] - LHSvalues[tokens[1]]);
				Phase = LHSvalues[j.second[currentLabel + "-PHASE"]];
				double VB_dt = (2 / tsim.maxtstep)*(VB - initialConditionsMap["R_" + currentLabel]["V_PREV"]) - initialConditionsMap["R_" + currentLabel]["V_dt_PREV"];
				double VB_guess = VB + tsim.maxtstep*VB_dt;
				double Phase_guess = Phase + ((tsim.maxtstep / 2)*(2 * M_PI / PHI_ZERO))*(VB + VB_guess);
				double Is = -bMatrixConductanceMap[j.first][currentLabel + "-ICRIT"] * sin(Phase_guess) + (((2 * bMatrixConductanceMap[j.first][currentLabel + "-CAP"]) / tsim.maxtstep)*VB) + (bMatrixConductanceMap[j.first][currentLabel + "-CAP"] * VB_dt);

				initialConditionsMap["R_" + currentLabel]["V_PREV"] = VB;
				initialConditionsMap["R_" + currentLabel]["V_dt_PREV"] = VB_dt;
				initialConditionsMap["R_" + currentLabel]["P_PREV"] = Phase;
				initialConditionsMap["R_" + currentLabel]["Is"] = Is;
			}
		}
	}
}