#include "include/j_matrix.hpp"

std::unordered_map<std::string, std::unordered_map<std::string, double>> bMatrixConductanceMap;
std::unordered_map<std::string, double> inductanceMap;
std::unordered_map<std::string, std::vector<std::string>> nodeConnections;
std::vector<matrix_element> mElements;
std::vector<element> elements;
std::vector<std::string> rowNames, columnNames;
std::vector<double> nzval;
std::vector<int> colind;
std::vector<int> rowptr;
std::unordered_map<std::string, std::vector<double>> sources;
int Nsize, Msize;
/*
  Systematically create A matrix
*/
void matrix_A(InputFile & iFile) {
	create_A_matrix(iFile);
	if (rowNames.size() != columnNames.size()) matrix_errors(NON_SQUARE, columnNames.size() + "x" + rowNames.size());
	if (VERBOSE) std::cout << std::setw(35) << std::left << "A matrix dimentions: " << columnNames.size() << "\n\n";

}
/*
  Create the A matrix in Compressed Row Storage (CRS) format
*/
void create_A_matrix(InputFile& iFile) {
	std::string cName, rName, cNameP, rNameP, cNameN, rNameN;
	std::vector<std::string> devicetokens, componentLabels;
	std::string label, nodeP, nodeN;
	std::unordered_map<std::string, int> rowMap, columnMap;
	int rowCounter, colCounter;
	std::unordered_map<std::string, int>::const_iterator got;
	bool pGND, nGND;
	rowCounter = 0;
	colCounter = 0;
	/* Main circuit node identification*/
	for (auto i : iFile.maincircuitSegment) {
		element cElement;
		devicetokens = tokenize_space(i);
		double value = 0.0;
		/* Check if label exists, if not there is a bug in the program */
		try {
			label = devicetokens.at(0); 
			if (std::find(componentLabels.begin(), componentLabels.end(), label) == componentLabels.end()) componentLabels.push_back(label);
			else {
				invalid_component_errors(DUPLICATE_LABEL, label);
			}
		}
		catch (const std::out_of_range) {
			invalid_component_errors(MISSING_LABEL, i);
		}
		/* Check if positive node exists, if not it's a bad device line definition */
		try { nodeP = devicetokens.at(1); }
		catch (const std::out_of_range) {
			invalid_component_errors(MISSING_PNODE, i);
		}
		/* Check if negative node exists, if not it's a bad device line definition */
		try { nodeN = devicetokens.at(2); }
		catch (const std::out_of_range) {
			invalid_component_errors(MISSING_NNODE, i);
		}
		/**************/
		/** RESISTOR **/
		/**************/
		if (i[0] == 'R') {
			/* Create a new matrix element for the resistor */
			matrix_element e;
			/* Check if value exists, if not it's a bad resistor definition */
			try { value = modifier(devicetokens.at(3)); }
			catch (const std::out_of_range) {
				invalid_component_errors(RES_ERROR, i);
			}
			/* Check if positive node is connected to ground */
			if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
				cNameP = "C_NV" + nodeP;
				rNameP = "R_N" + nodeP;
				if ((got = rowMap.find(rNameP)) == rowMap.end()) {
					rowMap[rNameP] = rowCounter;
					rowCounter++;
				}
				if ((got = columnMap.find(cNameP)) == columnMap.end()) {
					columnMap[cNameP] = colCounter;
					colCounter++;
				}
				/* If row does not already exist, add to rows */
				//unique_push(rowNames, rNameP);
				/* If column does not already exist, add to columns */
				//unique_push(columnNames, cNameP);
				/* Add the resistance value to the conductance map */
				bMatrixConductanceMap[rNameP][label] = 0.0;
				nodeConnections[rNameP].push_back(label);
				pGND = false;
			}
			else pGND = true;
			/* Check if negative node is connected to ground */
			if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
				cNameN = "C_NV" + nodeN;
				rNameN = "R_N" + nodeN;
				if ((got = rowMap.find(rNameN)) == rowMap.end()) {
					rowMap[rNameN] = rowCounter;
					rowCounter++;
				}
				if ((got = columnMap.find(cNameN)) == columnMap.end()) {
					columnMap[cNameN] = colCounter;
					colCounter++;
				}
				/* If row does not already exist, add to rows */
				//unique_push(rowNames, rNameN);
				/* If column does not already exist, add to columns */
				//unique_push(columnNames, cNameN);
				/* Add the resistance value to the conductance map */
				bMatrixConductanceMap[rNameN][label] = 0.0;
				nodeConnections[rNameN].push_back(label);
				nGND = false;
			}
			else nGND = true;
			/* Start of add elements to matrix section */
			/* If positive node is not grounded */
			if (!pGND) {
				/* Positive node row and column */
				e.label = label;
				e.columnIndex = columnMap[cNameP];
				e.rowIndex = rowMap[rNameP];
				e.value = 1 / value;
				mElements.push_back(e);
				/* Add the column index of the positive node to the positive node row of the conductance map */
				/* This will be used to identify the voltage later */
				bMatrixConductanceMap[rNameP][label + "-VP"] = (double)e.columnIndex;
				/* If positive and negative node is not grounded */
				if (!nGND) {
					/* Positive node row and negative node column */
					e.label = label;
					e.columnIndex = columnMap[cNameN];
					e.rowIndex = rowMap[rNameP];
					e.value = -1 / value;
					mElements.push_back(e);
					/* Add the column index of the negative node to the positive node row of the conductance map */
					/* This will be used to identify the voltage later */
					bMatrixConductanceMap[rNameP][label + "-VN"] = (double)e.columnIndex;
					/* Negative node row and positive node column */
					e.label = label;
					e.columnIndex = columnMap[cNameP];
					e.rowIndex = rowMap[rNameN];
					e.value = -1 / value;
					mElements.push_back(e);
					/* Add the column index of the positive node to the negative node row of the conductance map */
					/* This will be used to identify the voltage later */
					bMatrixConductanceMap[rNameN][label + "-VP"] = (double)e.columnIndex;
				}
			}
			/* If negative node is not grounded */
			if (!nGND) {
				/* Negative node row and column */
				e.label = label;
				e.columnIndex = columnMap[cNameN];
				e.rowIndex = rowMap[rNameN];
				e.value = 1 / value;
				mElements.push_back(e);
				/* Add the column index of the negative node to the negative node row of the conductance map */
				/* This will be used to identify the voltage later */
				bMatrixConductanceMap[rNameN][label + "-VN"] = (double)e.columnIndex;
			}
			/* End of add elements to matrix section */
			/* Element identification for use later when plotting values*/
			cElement.label = label;
			cElement.value = value;
			if (!pGND) {
				if (!nGND) {
					cElement.VPindex = columnMap[cNameP];
					cElement.VNindex = columnMap[cNameN];
				}
				else {
					cElement.VPindex = columnMap[cNameP];
				}
			}
			else {
				if (!nGND) {
					cElement.VNindex = columnMap[cNameN];
				}
			}
			elements.push_back(cElement);
		}
		/***************/
		/** CAPACITOR **/
		/***************/
		else if (i[0] == 'C') {
			/* Create a new matrix element for the resistor */
			matrix_element e;
			/* Check if value exists, if not it's a bad capactitor definition */
			try { value = modifier(devicetokens.at(3)); }
			catch (const std::out_of_range) {
				invalid_component_errors(CAP_ERROR, i);
			}
			/* Check if positive node is connected to ground */
			if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
				cNameP = "C_NV" + nodeP;
				rNameP = "R_N" + nodeP;
				if ((got = rowMap.find(rNameP)) == rowMap.end()) {
					rowMap[rNameP] = rowCounter;
					rowCounter++;
				}
				if ((got = columnMap.find(cNameP)) == columnMap.end()) {
					columnMap[cNameP] = colCounter;
					colCounter++;
				}
				/* If row does not already exist, add to rows */
				//unique_push(rowNames, rNameP);
				/* If column does not already exist, add to columns */
				//unique_push(columnNames, cNameP);
				/* Add the capacitance value to the conductance map */
				bMatrixConductanceMap[rNameP][label] = value;
				nodeConnections[rNameP].push_back(label);
				pGND = false;
			}
			else pGND = true;
			/* Check if negative node is connected to ground */
			if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
				cNameN = "C_NV" + nodeN;
				rNameN = "R_N" + nodeN;
				if ((got = rowMap.find(rNameN)) == rowMap.end()) {
					rowMap[rNameN] = rowCounter;
					rowCounter++;
				}
				if ((got = columnMap.find(cNameN)) == columnMap.end()) {
					columnMap[cNameN] = colCounter;
					colCounter++;
				}
				/* If row does not already exist, add to rows */
				//unique_push(rowNames, rNameN);
				/* If column does not already exist, add to columns */
				//unique_push(columnNames, cNameN);
				/* Add the capacitance value to the conductance map */
				bMatrixConductanceMap[rNameN][label] = -value;
				nodeConnections[rNameN].push_back(label);
				nGND = false;
			}
			else nGND = true;
			/* Start of add elements to matrix section */
			/* If positive node is not grounded */
			if (!pGND) {
				/* Positive node row and column */
				e.label = label;
				e.columnIndex = columnMap[cNameP];
				e.rowIndex = rowMap[rNameP];
				e.value = value / tsim.maxtstep;
				mElements.push_back(e);
				/* Add the column index of the positive node to the positive node row in the conductance map */
				/* This will be used to identify the voltage later */
				bMatrixConductanceMap[rNameP][label + "-VP"] = (double)e.columnIndex;
				if (!nGND) {
					/* Positive node row and negative node column */
					e.label = label;
					e.columnIndex = columnMap[cNameN];
					e.rowIndex = rowMap[rNameP];
					e.value = -value / tsim.maxtstep;
					mElements.push_back(e);
					/* Add the column index of the negative node to the positive node row in the conductance map */
					/* This will be used to identify the voltage later */
					bMatrixConductanceMap[rNameP][label + "-VN"] = (double)e.columnIndex;
					/* Negative node row and positive node column */
					e.label = label;
					e.columnIndex = columnMap[cNameP];
					e.rowIndex = rowMap[rNameN];
					e.value = -value / tsim.maxtstep;
					mElements.push_back(e);
					/* Add the column index of the positive node to the negative node row in the conductance map */
					/* This will be used to identify the voltage later */
					bMatrixConductanceMap[rNameN][label + "-VP"] = (double)e.columnIndex;
				}
			}
			/* If negative node is not grounded */
			if (!nGND) {
				/* Negative node row and column */
				e.label = label;
				e.columnIndex = columnMap[cNameN];
				e.rowIndex = rowMap[rNameN];
				e.value = value / tsim.maxtstep;
				mElements.push_back(e);
				/* Add the column index of the negative node to the negative node row in the conductance map */
				/* This will be used to identify the voltage later */
				bMatrixConductanceMap[rNameN][label + "-VN"] = (double)e.columnIndex;
			}
			/* End of add elements to matrix section */
			/* Element identification for use later when plotting values*/
			cElement.label = label;
			cElement.value = value;
			if (!pGND) {
				if (!nGND) {
					cElement.VPindex = columnMap[cNameP];
					cElement.VNindex = columnMap[cNameN];
				}
				else {
					cElement.VPindex = columnMap[cNameP];
				}
			}
			else {
				if (!nGND) {
					cElement.VNindex = columnMap[cNameN];
				}
			}
			elements.push_back(cElement);
		}
		/**************/
		/** INDUCTOR **/
		/**************/
		else if (i[0] == 'L') {
			/* Create a new matrix element for the resistor */
			matrix_element e;
			/* Check if value exists, if not it's a bad inductor definition */
			try { value = modifier(devicetokens.at(3)); }
			catch (const std::out_of_range) {
				invalid_component_errors(IND_ERROR, i);
			}
			cName = "C_I" + devicetokens.at(0);
			rName = "R_" + devicetokens.at(0);
			if ((got = rowMap.find(rName)) == rowMap.end()) {
				rowMap[rName] = rowCounter;
				rowCounter++;
			}
			if ((got = columnMap.find(cName)) == columnMap.end()) {
				columnMap[cName] = colCounter;
				colCounter++;
			}
			/* Add the inductor as a row to the rows */
			//unique_push(rowNames, rName);
			/* Add the inductor current column to the columns */
			//unique_push(columnNames, cName);
			/* Add the inductor value to the conductance map */
			inductanceMap[rName] = value;
			bMatrixConductanceMap[rName][label] = value;
			/* Check if positive node is connected to ground */
			if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
				cNameP = "C_NV" + nodeP;
				rNameP = "R_N" + nodeP;
				if ((got = rowMap.find(rNameP)) == rowMap.end()) {
					rowMap[rNameP] = rowCounter;
					rowCounter++;
				}
				if ((got = columnMap.find(cNameP)) == columnMap.end()) {
					columnMap[cNameP] = colCounter;
					colCounter++;
				}
				/* If row does not already exist, add to rows */
				//unique_push(rowNames, rNameP);
				/* If column does not already exist, add to columns */
				//unique_push(columnNames, cNameP);
				/* Add the 0.0 to the conductance map */
				bMatrixConductanceMap[rNameP][label] = 0.0;
				nodeConnections[rNameP].push_back(label);
				pGND = false;
			}
			else pGND = true;
			/* Check if negative node is connected to ground */
			if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
				cNameN = "C_NV" + nodeN;
				rNameN = "R_N" + nodeN;
				if ((got = rowMap.find(rNameN)) == rowMap.end()) {
					rowMap[rNameN] = rowCounter;
					rowCounter++;
				}
				if ((got = columnMap.find(cNameN)) == columnMap.end()) {
					columnMap[cNameN] = colCounter;
					colCounter++;
				}
				/* If row does not already exist, add to rows */
				//unique_push(rowNames, rNameN);
				/* If column does not already exist, add to columns */
				//unique_push(columnNames, cNameN);
				/* Add the 0.0 to the conductance map */
				bMatrixConductanceMap[rNameN][label] = 0.0;
				nodeConnections[rNameN].push_back(label);
				nGND = false;
			}
			else nGND = true;
			/* Start of add elements to matrix section */
			bMatrixConductanceMap[rName][label + "-VP"] = -1.0;
			bMatrixConductanceMap[rName][label + "-VN"] = -1.0;
			/* If positive node is not grounded */
			if (!pGND) {
				/* Positive node row and column */
				e.label = label;
				e.columnIndex = columnMap[cNameP];
				e.rowIndex = rowMap[rNameP];
				e.value = 0;
				mElements.push_back(e);
				/* Add the column index of the positive node to the positive node row of the conductance map */
				/* This will be used to identify the voltage later */
				bMatrixConductanceMap[rNameP][label + "-VP"] = (double)e.columnIndex;
				if (!nGND) {
					/* Positive node row and negative node column */
					e.label = label;
					e.columnIndex = columnMap[cNameN];
					e.rowIndex = rowMap[rNameP];
					e.value = 0;
					mElements.push_back(e);
					/* Add the column index of the negative node to the positive node row of the conductance map */
					/* This will be used to identify the voltage later */
					bMatrixConductanceMap[rNameP][label + "-VN"] = (double)e.columnIndex;
					/* Negative node row and positive node column */
					e.label = label;
					e.columnIndex = columnMap[cNameP];
					e.rowIndex = rowMap[rNameN];
					e.value = 0;
					mElements.push_back(e);
					/* Add the column index of the positive node to the negative node row of the conductance map */
					/* This will be used to identify the voltage later */
					bMatrixConductanceMap[rNameN][label + "-VP"] = (double)e.columnIndex;
				}
				/* Positive node row and inductor current node column */
				e.label = label;
				e.columnIndex = columnMap[cName];
				e.rowIndex = rowMap[rNameP];
				e.value = 1;
				mElements.push_back(e);
				/* Inductor node row and positive node column */
				e.label = label;
				e.columnIndex = columnMap[cNameP];
				e.rowIndex = rowMap[rName];
				e.value = 1;
				mElements.push_back(e);
				/* Add the column index of the positive node to the inductor node row of the conductance map */
				/* This will be used to identify the voltage later */
				bMatrixConductanceMap[rName][label + "-VP"] = (double)e.columnIndex;
			}
			if (!nGND) {
				/* Negative node row and column */
				e.label = label;
				e.columnIndex = columnMap[cNameN];
				e.rowIndex = rowMap[rNameN];
				e.value = 0;
				mElements.push_back(e);
				/* Add the column index of the negative node to the negative node row of the conductance map */
				/* This will be used to identify the voltage later */
				bMatrixConductanceMap[rNameN][label + "-VN"] = (double)e.columnIndex;
				/* Negative node row and inductor current node column */
				e.label = label;
				e.columnIndex = columnMap[cName];
				e.rowIndex = rowMap[rNameN];
				e.value = -1;
				mElements.push_back(e);
				/* Inductor node row and negative node column*/
				e.label = label;
				e.columnIndex = columnMap[cNameN];
				e.rowIndex = rowMap[rName];
				e.value = -1;
				mElements.push_back(e);
				/* Add the column index of the negative node to the inductor node row of the conductance map */
				/* This will be used to identify the voltage later */
				bMatrixConductanceMap[rName][label + "-VN"] = (double)e.columnIndex;
			}
			/* Inductor node row and inductor current node column*/
			e.label = label;
			e.columnIndex = columnMap[cName];
			e.rowIndex = rowMap[rName];
			e.value = (-2 * value) / tsim.maxtstep;
			mElements.push_back(e);
			/* Add the column index of the inductor current node to the inductor node row of the conductance map */
			/* This will be used to identify the voltage later */
			bMatrixConductanceMap[rName][label + "-I"] = (double)e.columnIndex;
			/* End of add elements to matrix section */
			/* Element identification for use later when plotting values*/
			cElement.label = label;
			cElement.value = value;
			cElement.CURindex = columnMap[cName];
			if (!pGND) {
				if (!nGND) {
					cElement.VPindex = columnMap[cNameP];
					cElement.VNindex = columnMap[cNameN];
				}
				else {
					cElement.VPindex = columnMap[cNameP];
				}
			}
			else {
				if (!nGND) {
					cElement.VNindex = columnMap[cNameN];
				}
			}
			elements.push_back(cElement);
		}
		/********************/
		/** VOLTAGE SOURCE **/
		/********************/
		else if (i[0] == 'V') {
			/* Create a new matrix element for the resistor */
			matrix_element e;
			/* Parse the function identified (if any)*/
			sources[label] = function_parse(i);
			cName = "C_" + devicetokens.at(0);
			rName = "R_" + devicetokens.at(0);
			if ((got = rowMap.find(rName)) == rowMap.end()) {
				rowMap[rName] = rowCounter;
				rowCounter++;
			}
			if ((got = columnMap.find(cName)) == columnMap.end()) {
				columnMap[cName] = colCounter;
				colCounter++;
			}
			/* Add the voltage source as a row to the rows */
			//unique_push(rowNames, rName);
			/* Add the voltage source as a column to the columns */
			//unique_push(columnNames, cName);
			/* Check if positive node is connected to ground */
			if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
				cNameP = "C_NV" + nodeP;
				rNameP = "R_N" + nodeP;
				if ((got = rowMap.find(rNameP)) == rowMap.end()) {
					rowMap[rNameP] = rowCounter;
					rowCounter++;
				}
				if ((got = columnMap.find(cNameP)) == columnMap.end()) {
					columnMap[cNameP] = colCounter;
					colCounter++;
				}
				/* If row does not already exist, add to rows */
				//unique_push(rowNames, rNameP);
				/* If column does not already exist, add to column */
				//unique_push(columnNames, cNameP);
				nodeConnections[rNameP].push_back(label);
				pGND = false;
			}
			else pGND = true;
			/* Check if negative node is connected to ground */
			if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
				cNameN = "C_NV" + nodeN;
				rNameN = "R_N" + nodeN;
				if ((got = rowMap.find(rNameN)) == rowMap.end()) {
					rowMap[rNameN] = rowCounter;
					rowCounter++;
				}
				if ((got = columnMap.find(cNameN)) == columnMap.end()) {
					columnMap[cNameN] = colCounter;
					colCounter++;
				}
				/* If row does not already exist, add to rows */
				//unique_push(rowNames, rNameN);
				/* If column does not already exist, add to column */
				//unique_push(columnNames, cNameN);
				nodeConnections[rNameN].push_back(label);
				nGND = false;
			}
			else nGND = true;
			/* Start of add elements to matrix section */
			/* If positive node is not grounded */
			if (!pGND) {
				/* Positive node row and column */
				e.label = label;
				e.columnIndex = columnMap[cNameP];
				e.rowIndex = rowMap[rNameP];
				e.value = 0;
				mElements.push_back(e);
				if (!nGND) {
					/* Positive node row and negative node column */
					e.label = label;
					e.columnIndex = columnMap[cNameN];
					e.rowIndex = rowMap[rNameP];
					e.value = 0;
					mElements.push_back(e);
					/* Negative node row and positive node column */
					e.label = label;
					e.columnIndex = columnMap[cNameN];
					e.rowIndex = rowMap[rNameP];
					e.value = 0;
					mElements.push_back(e);
				}
				/* Positive node row and voltage node column */
				e.label = label;
				e.columnIndex = columnMap[cName];
				e.rowIndex = rowMap[rNameP];
				e.value = 1;
				mElements.push_back(e);
				/* Voltage node row and positive node column */
				e.label = label;
				e.columnIndex = columnMap[cNameP];
				e.rowIndex = rowMap[rName];
				e.value = 1;
				mElements.push_back(e);
			}
			/* If negative node is not grounded */
			if (!nGND) {
				/* Negative node row and column */
				e.label = label;
				e.columnIndex = columnMap[cNameN];
				e.rowIndex = rowMap[rNameN];
				e.value = 0;
				mElements.push_back(e);
				/* Negative node row and voltage node column */
				e.label = label;
				e.columnIndex = columnMap[cName];
				e.rowIndex = rowMap[rNameN];
				e.value = -1;
				mElements.push_back(e);
				/* Voltage node row and negative node column*/
				e.label = label;
				e.columnIndex = columnMap[cNameN];
				e.rowIndex = rowMap[rName];
				e.value = -1;
				mElements.push_back(e);
			}
			/* Voltage node row and voltage node column*/
			e.label = label;
			e.columnIndex = columnMap[cName];
			e.rowIndex = rowMap[rName];
			e.value = 0;
			mElements.push_back(e);
			/* Add the column index of the phase node to the junction node row of the conductance map */
			/* This will be used to identify the voltage later */
			bMatrixConductanceMap[rName][label] = (double)e.columnIndex;
			/* End of add elements to matrix section */
			/* Element identification for use later when plotting values*/
			cElement.label = label;
			cElement.value = value;
			if (!pGND) {
				if (!nGND) {
					cElement.VPindex = columnMap[cNameP];
					cElement.VNindex = columnMap[cNameN];
				}
				else {
					cElement.VPindex = columnMap[cNameP];
				}
			}
			else {
				if (!nGND) {
					cElement.VNindex = columnMap[cNameN];
				}
			}
			elements.push_back(cElement);
		}
		/********************/
		/** CURRENT SOURCE **/
		/********************/
		else if (i[0] == 'I') {
			/* Parse the function identified (if any)*/
			sources[label] = function_parse(i);
			/* Check if positive node is connected to ground */
			if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
				cNameP = "C_NV" + nodeP;
				rNameP = "R_N" + nodeP;
				if ((got = rowMap.find(rNameP)) == rowMap.end()) {
					rowMap[rNameP] = rowCounter;
					rowCounter++;
				}
				if ((got = columnMap.find(cNameP)) == columnMap.end()) {
					columnMap[cNameP] = colCounter;
					colCounter++;
				}
				/* If row does not already exist, add to rows */
				//unique_push(rowNames, rNameP);
				/* If column does not already exist, add to columns */
				//unique_push(columnNames, cNameP);
				bMatrixConductanceMap[rNameP][label] = 1.0;
				nodeConnections[rNameP].push_back(label);
				pGND = false;
			}
			else pGND = true;
			/* Check if negative node is connected to ground */
			if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
				cNameN = "C_NV" + nodeN;
				rNameN = "R_N" + nodeN;
				if ((got = rowMap.find(rNameN)) == rowMap.end()) {
					rowMap[rNameN] = rowCounter;
					rowCounter++;
				}
				if ((got = columnMap.find(cNameN)) == columnMap.end()) {
					columnMap[cNameN] = colCounter;
					colCounter++;
				}
				/* If row does not already exist, add to rows */
				//unique_push(rowNames, rNameN);
				/* If column does not already exist, add to columns */
				//unique_push(columnNames, cNameN);
				/* Add the 0.0 to the conductance map */
				bMatrixConductanceMap[rNameN][label] = 1.0;
				nodeConnections[rNameN].push_back(label);
				nGND = false;
			}
			else nGND = true;
			/* Element identification for use later when plotting values*/
			cElement.label = label;
			cElement.value = value;
			if (!pGND) {
				if (!nGND) {
					cElement.VPindex = columnMap[cNameP];
					cElement.VNindex = columnMap[cNameN];
				}
				else {
					cElement.VPindex = columnMap[cNameP];
				}
			}
			else {
				if (!nGND) {
					cElement.VNindex = columnMap[cNameN];
				}
			}
			elements.push_back(cElement);
		}
		/************************/
		/** JOSEPHSON JUNCTION **/
		/************************/
		else if (i[0] == 'B') {
			/* Create a new matrix element for the resistor */
			matrix_element e;
			/* Identify the JJ parameters based on the model*/
			double jj_cap, jj_rn, jj_rzero, jj_icrit;
			jj_comp(devicetokens, jj_cap, jj_rn, jj_rzero, jj_icrit);
			cName = "C_P" + devicetokens.at(0);
			rName = "R_" + devicetokens.at(0);
			if ((got = rowMap.find(rName)) == rowMap.end()) {
				rowMap[rName] = rowCounter;
				rowCounter++;
			}
			if ((got = columnMap.find(cName)) == columnMap.end()) {
				columnMap[cName] = colCounter;
				colCounter++;
			}
			/* Add the junction as a row to the rows */
			//unique_push(rowNames, rName);
			/* Add the junction phase as a column to the columns */
			//unique_push(columnNames, cName);
			/* Check if positive node is connected to ground */
			if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
				cNameP = "C_NV" + nodeP;
				rNameP = "R_N" + nodeP;
				if ((got = rowMap.find(rNameP)) == rowMap.end()) {
					rowMap[rNameP] = rowCounter;
					rowCounter++;
				}
				if ((got = columnMap.find(cNameP)) == columnMap.end()) {
					columnMap[cNameP] = colCounter;
					colCounter++;
				}
				/* If row does not already exist, add to rows */
				//unique_push(rowNames, rNameP);
				/* If column does not already exist, add to column */
				//unique_push(columnNames, cNameP);
				nodeConnections[rNameP].push_back(label);
				bMatrixConductanceMap[rNameP][label + "-CAP"] = jj_cap;
				bMatrixConductanceMap[rNameP][label + "-ICRIT"] = jj_icrit;
				pGND = false;
			}
			else pGND = true;
			/* Check if negative node is connected to ground */
			if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
				cNameN = "C_NV" + nodeN;
				rNameN = "R_N" + nodeN;
				if ((got = rowMap.find(rNameN)) == rowMap.end()) {
					rowMap[rNameN] = rowCounter;
					rowCounter++;
				}
				if ((got = columnMap.find(cNameN)) == columnMap.end()) {
					columnMap[cNameN] = colCounter;
					colCounter++;
				}
				/* If row does not already exist, add to rows */
				//unique_push(rowNames, rNameN);
				/* If column does not already exist, add to column */
				//unique_push(columnNames, cNameN);
				nodeConnections[rNameN].push_back(label);
				bMatrixConductanceMap[rNameP][label + "-CAP"] = jj_cap;
				bMatrixConductanceMap[rNameP][label + "-ICRIT"] = jj_icrit;
				nGND = false;
			}
			else nGND = true;
			/* Start of add elements to matrix section */
			bMatrixConductanceMap[rName][label + "-VP"] = -1.0;
			bMatrixConductanceMap[rName][label + "-VN"] = -1.0;
			/* If positive node is not grounded */
			if (!pGND) {
				/* Positive node row and column */
				e.label = label;
				e.columnIndex = columnMap[cNameP];
				e.rowIndex = rowMap[rNameP];
				e.value = ((2 * jj_cap) / tsim.maxtstep) + (1 / jj_rzero);
				mElements.push_back(e);
				/* Add the column index of the positive node to the positive node row of the conductance map */
				/* This will be used to identify the voltage later */
				bMatrixConductanceMap[rNameP][label + "-VP"] = (double)e.columnIndex;
				if (!nGND) {
					/* Positive node row and negative node column */
					e.label = label;
					e.columnIndex = columnMap[cNameN];
					e.rowIndex = rowMap[rNameP];
					e.value = -(((2 * jj_cap) / tsim.maxtstep) + (1 / jj_rzero));
					mElements.push_back(e);
					/* Add the column index of the negative node to the positive node row of the conductance map */
					/* This will be used to identify the voltage later */
					bMatrixConductanceMap[rNameP][label + "-VN"] = (double)e.columnIndex;
					/* Negative node row and positive node column */
					e.label = label;
					e.columnIndex = columnMap[cNameP];
					e.rowIndex = rowMap[rNameN];
					e.value = -(((2 * jj_cap) / tsim.maxtstep) + (1 / jj_rzero));
					mElements.push_back(e);
					/* Add the column index of the positive node to the negative node row of the conductance map */
					/* This will be used to identify the voltage later */
					bMatrixConductanceMap[rNameN][label + "-VP"] = (double)e.columnIndex;
				}
				/* Positive node row and phase node column */
				e.label = label;
				e.columnIndex = columnMap[cName];
				e.rowIndex = rowMap[rNameP];
				e.value = 0;
				mElements.push_back(e);
				/* Add the column index of the phase node to the positive node row of the conductance map */
				/* This will be used to identify the voltage later */
				bMatrixConductanceMap[rNameP][label + "-PHASE"] = (double)e.columnIndex;
				/* Junction node row and positive node column */
				e.label = label;
				e.columnIndex = columnMap[cNameP];
				e.rowIndex = rowMap[rName];
				e.value = (-tsim.maxtstep/2) * ((2 * M_PI)/PHI_ZERO);
				mElements.push_back(e);
				/* Add the column index of the positive node to the junction node row of the conductance map */
				/* This will be used to identify the voltage later */
				bMatrixConductanceMap[rName][label + "-VP"] = (double)e.columnIndex;
			}
			/* If negative node is not grounded */
			if (!nGND) {
				/* Negative node row and column */
				e.label = label;
				e.columnIndex = columnMap[cNameN];
				e.rowIndex = rowMap[rNameN];
				e.value = ((2 * jj_cap) / tsim.maxtstep) + (1 / jj_rzero);
				mElements.push_back(e);
				/* Add the column index of the negative node to the negative node row of the conductance map */
				/* This will be used to identify the voltage later */
				bMatrixConductanceMap[rNameN][label + "-VN"] = (double)e.columnIndex;
				/* Negative node row and phase node column */
				e.label = label;
				e.columnIndex = columnMap[cName];
				e.rowIndex = rowMap[rNameN];
				e.value = 0;
				mElements.push_back(e);
				/* Add the column index of the phase node to the negative node row of the conductance map */
				/* This will be used to identify the voltage later */
				bMatrixConductanceMap[rNameN][label + "-PHASE"] = (double)e.columnIndex;
				/* Junction node row and negative node column*/
				e.label = label;
				e.columnIndex = columnMap[cNameN];
				e.rowIndex = rowMap[rName];
				e.value = (tsim.maxtstep / 2) * ((2 * M_PI) / PHI_ZERO);
				mElements.push_back(e);
				/* Add the column index of the negative node to the junction node row of the conductance map */
				/* This will be used to identify the voltage later */
				bMatrixConductanceMap[rName][label + "-VN"] = (double)e.columnIndex;
			}
			/* Junction node row and phase node column*/
			e.label = label;
			e.columnIndex = columnMap[cName];
			e.rowIndex = rowMap[rName];
			e.value = 1;
			mElements.push_back(e);
			/* Add the column index of the phase node to the junction node row of the conductance map */
			/* This will be used to identify the voltage later */
			bMatrixConductanceMap[rName][label + "-PHASE"] = (double)e.columnIndex;
			bMatrixConductanceMap[rName][label + "-CAP"] = jj_cap;
			bMatrixConductanceMap[rName][label + "-ICRIT"] = jj_icrit;
			/* Element identification for use later when plotting values*/
			cElement.label = label;
			cElement.value = value;
			if (!pGND) {
				if (!nGND) {
					cElement.VPindex = columnMap[cNameP];
					cElement.VNindex = columnMap[cNameN];
				}
				else {
					cElement.VPindex = columnMap[cNameP];
				}
			}
			else {
				if (!nGND) {
					cElement.VNindex = columnMap[cNameN];
				}
			}
			elements.push_back(cElement);
		}
		/* End of add elements to matrix section */
	}
	std::map<int, std::string> rowMapFlip = flip_map(rowMap);
	std::map<int, std::string> columnMapFlip = flip_map(columnMap);
	std::transform(rowMapFlip.begin(), rowMapFlip.end(), back_inserter(rowNames), [](std::pair<int, std::string> const & pair)
	{
		return pair.second;
	});
	std::transform(columnMapFlip.begin(), columnMapFlip.end(), back_inserter(columnNames), [](std::pair<int, std::string> const & pair)
	{
		return pair.second;
	});
	/* Now that conductance A matrix has been identified we can convert to CSR format */
	/* Optionally display matrix contents in verbose mode */
	if (VERBOSE) {
		print_A_matrix();
	}
	/* Now convert matrix into CSR format so that it can be solved using general math libraries */
	/* First create A_matrix in matrix form so the looping is easier to accomplish */
	std::vector<std::vector<double> > A_matrix(rowNames.size(), std::vector<double>(columnNames.size()));
	for (auto i : mElements) {
		A_matrix[i.rowIndex][i.columnIndex] += i.value;
	}
	Nsize = A_matrix.size();
	Msize = A_matrix[0].size();
	int columnCounter, elementCounter = 0;
	rowptr.push_back(0);
	for (auto i : A_matrix) {
		columnCounter = 0;
		for (auto j : i) {
			if (j != 0.0) {
				/* Row-major order*/
				nzval.push_back(j);
				/* Element column */
				colind.push_back(columnCounter);
				elementCounter++;
			}
			columnCounter++;
		}
		rowptr.push_back(elementCounter);
	}
}
/*
  Print A matrix
*/
void print_A_matrix() {
	std::vector<std::vector<double> > A_matrix(rowNames.size(), std::vector<double>(columnNames.size()));
	for (auto i : mElements) {
		A_matrix[i.rowIndex][i.columnIndex] += i.value;
	}
	std::cout << "A matrix: \n";
	std::cout << std::setw(10) << std::left << "";
	for (auto i : columnNames) std::cout << std::setw(10) << std::left << i;
	std::cout << "\n";
	int counter = 0;
	for (auto i : A_matrix) {
		std::cout << std::setw(10) << std::left << rowNames.at(counter) + ":";
		for (auto j : i) {
			std::cout << std::setw(10) << std::left << j;
		}
		std::cout << "\n";
		counter++;
	}
}