#include "include/simulation.hpp"

trans_sim tsim;

void identify_simulation(InputFile& iFile) {
	std::string simline;
	for (auto i : iFile.controlPart) {
		if (i.find(".TRAN") != std::string::npos) {
			simline = i;
			iFile.simulationType = TRANS;
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
	case TRANS:
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
	double RHSvalue;
	std::string currentLabel;
	std::map<std::string, std::string> currentNode;
	std::map<std::string, double> currentConductance;
	for (int i = 0; i < tsim.simsize(); i++) {
		/* Construct RHS matrix */
		for (auto j : rowNames) {
			currentNode = bMatrixNodeMap[j];
			currentConductance = bMatrixConductanceMap[j];
			if (j.find("_N") != std::string::npos) {

			}
			else if (j.find("_L") != std::string::npos) {
				
			}
			else if (j.find("_B") != std::string::npos) {
				
			}
			else if (j.find("_V") != std::string::npos) {
				
			}
		}
	}
}