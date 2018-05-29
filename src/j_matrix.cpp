// Copyright (c) 2018 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#include "j_matrix.hpp"


std::unordered_map<std::string, std::unordered_map<std::string, double>> bMatrixConductanceMap;
std::unordered_map<std::string, double> inductanceMap;
std::unordered_map<std::string, model_xline> xlines;
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
    if (rowNames.size() != columnNames.size()) matrix_errors(NON_SQUARE,std::to_string(columnNames.size()) + "x" + std::to_string(rowNames.size()));
	if (VERBOSE) std::cout << std::setw(35) << std::left << "A matrix dimensions: " << columnNames.size() << "\n\n";

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
		catch (const std::out_of_range&) {
			invalid_component_errors(MISSING_LABEL, i);
		}
		/* Check if positive node exists, if not it's a bad device line definition */
		try { nodeP = devicetokens.at(1); }
		catch (const std::out_of_range&) {
			invalid_component_errors(MISSING_PNODE, i);
		}
		/* Check if negative node exists, if not it's a bad device line definition */
		try { nodeN = devicetokens.at(2); }
		catch (const std::out_of_range&) {
			invalid_component_errors(MISSING_NNODE, i);
		}
		/**************/
		/** RESISTOR **/
		/**************/
		if (i[0] == 'R') {
			/* Create a new matrix element for the resistor */
			matrix_element e;
			/* Check if value exists, if not it's a bad resistor definition */
			try {
				if(parVal.find(devicetokens.at(3)) != parVal.end()) value = parVal[devicetokens.at(3)];
				else value = modifier(devicetokens.at(3));
			}
			catch (const std::out_of_range&) {
				invalid_component_errors(RES_ERROR, i);
			}
			/* Check if positive node is connected to ground */
			if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
				cNameP = "C_NV" + nodeP;
				rNameP = "R_N" + nodeP;
				if(rowMap.count(rNameP) == 0) {
					rowMap[rNameP] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameP) == 0) {
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
				if (rowMap.count(rNameN) == 0) {
					rowMap[rNameN] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameN) == 0) {
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
			/* Check if value exists, if not it's a bad capacitor definition */
			try {
				if(parVal.find(devicetokens.at(3)) != parVal.end()) value = parVal[devicetokens.at(3)];
				else value = modifier(devicetokens.at(3));
			}
			catch (const std::out_of_range&) {
				invalid_component_errors(CAP_ERROR, i);
			}
			/* Check if positive node is connected to ground */
			if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
				cNameP = "C_NV" + nodeP;
				rNameP = "R_N" + nodeP;
				if (rowMap.count(rNameP) == 0) {
					rowMap[rNameP] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameP) == 0) {
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
				if (rowMap.count(rNameN) == 0) {
					rowMap[rNameN] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameN) == 0) {
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
			try {
				if(parVal.find(devicetokens.at(3)) != parVal.end()) value = parVal[devicetokens.at(3)];
				else value = modifier(devicetokens.at(3));
			}
			catch (const std::out_of_range&) {
				invalid_component_errors(IND_ERROR, i);
			}
			cName = "C_I" + devicetokens.at(0);
			rName = "R_" + devicetokens.at(0);
			if (rowMap.count(rName) == 0) {
				rowMap[rName] = rowCounter;
				rowCounter++;
			}
			if (columnMap.count(cName) == 0) {
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
				if (rowMap.count(rNameP) == 0) {
					rowMap[rNameP] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameP) == 0) {
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
				if (rowMap.count(rNameN) == 0) {
					rowMap[rNameN] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameN) == 0) {
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
			if (rowMap.count(rName) == 0) {
				rowMap[rName] = rowCounter;
				rowCounter++;
			}
			if (columnMap.count(cName) == 0) {
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
				if (rowMap.count(rNameP) == 0) {
					rowMap[rNameP] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameP) == 0) {
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
				if (rowMap.count(rNameN) == 0) {
					rowMap[rNameN] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameN) == 0) {
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
				if (rowMap.count(rNameP) == 0) {
					rowMap[rNameP] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameP) == 0) {
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
				if (rowMap.count(rNameN) == 0) {
					rowMap[rNameN] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameN) == 0) {
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
			if (rowMap.count(rName) == 0) {
				rowMap[rName] = rowCounter;
				rowCounter++;
			}
			if (columnMap.count(cName) == 0) {
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
				if (rowMap.count(rNameP) == 0) {
					rowMap[rNameP] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameP) == 0) {
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
				if (rowMap.count(rNameN) == 0) {
					rowMap[rNameN] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameN) == 0) {
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
		/***********************/
		/** TRANSMISSION LINE **/
		/***********************/
		else if (i[0] == 'T') {
			std::string nodeP2, nodeN2, TN1, TN2, TV1, TV2, cNameP2, rNameP2, cNameN2, rNameN2, cNameNI1, rNameNI1, cNameNI2, rNameNI2, cName1, rName1, cName2, rName2;
			bool p2GND, n2GND;
			double TimeDelay = 0.0;
			/* Check if positive node 2 exists, if not it's a bad device line definition */
			try { nodeP2 = devicetokens.at(3); }
			catch (const std::out_of_range&) {
				invalid_component_errors(MISSING_PNODE, i);
			}
			/* Check if negative node 2 exists, if not it's a bad device line definition */
			try { nodeN2 = devicetokens.at(4); }
			catch (const std::out_of_range&) {
				invalid_component_errors(MISSING_NNODE, i);
			}
			TN1 = "1" + label;
			TN2 = "2" + label;
			TV1 = label + "-V1";
			TV2 = label + "-V2";
			/* Create a new matrix element for the first Z0 */
			matrix_element e;
			if (devicetokens.size() < 7) {
				invalid_component_errors(TIME_ERROR, i);
			}
			/* Check if value exists, if not it's a bad Z0 definition */
			for (size_t l = 6; l < devicetokens.size(); l++) {
				if(devicetokens[l].find("TD") != std::string::npos) TimeDelay = modifier((devicetokens.at(l)).substr(3));
				else if (devicetokens[l].find("Z0") != std::string::npos) value = modifier((devicetokens.at(l)).substr(3));
			}
            xlines[label].Z0 = value;
            xlines[label].label = label;
            xlines[label].TD = TimeDelay;
            /* Check if positive node is connected to ground */
			if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
				cNameP = "C_NV" + nodeP;
				rNameP = "R_N" + nodeP;
				/* If row does not already exist, add to rows */
				if (rowMap.count(rNameP) == 0) {
					rowMap[rNameP] = rowCounter;
					rowCounter++;
				}
				/* If column does not already exist, add to columns */
				if (columnMap.count(cNameP) == 0) {
					columnMap[cNameP] = colCounter;
					colCounter++;
				}
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
				/* If row does not already exist, add to rows */
				if (rowMap.count(rNameN) == 0) {
					rowMap[rNameN] = rowCounter;
					rowCounter++;
				}
				/* If column does not already exist, add to columns */
				if (columnMap.count(cNameN) == 0) {
					columnMap[cNameN] = colCounter;
					colCounter++;
				}
				/* Add the resistance value to the conductance map */
				bMatrixConductanceMap[rNameN][label] = 0.0;
				nodeConnections[rNameN].push_back(label);
				nGND = false;
			}
			else nGND = true;
            /* Check if second positive node is connected to ground */
            if (nodeP2 != "0" && nodeP2.find("GND") == std::string::npos) {
                cNameP2 = "C_NV" + nodeP2;
                rNameP2 = "R_N" + nodeP2;
                /* If row does not already exist, add to rows */
                if (rowMap.count(rNameP2) == 0) {
                    rowMap[rNameP2] = rowCounter;
                    rowCounter++;
                }
                /* If column does not already exist, add to columns */
                if (columnMap.count(cNameP2) == 0) {
                    columnMap[cNameP2] = colCounter;
                    colCounter++;
                }
                /* Add the resistance value to the conductance map */
                bMatrixConductanceMap[rNameP2][label] = 0.0;
                nodeConnections[rNameP2].push_back(label);
                p2GND = false;
            }
            else p2GND = true;
            /* Check if second negative node is connected to ground */
            if (nodeN2 != "0" && nodeN2.find("GND") == std::string::npos) {
                cNameN2 = "C_NV" + nodeN2;
                rNameN2 = "R_N" + nodeN2;
                /* If row does not already exist, add to rows */
                if (rowMap.count(rNameN2) == 0) {
                    rowMap[rNameN2] = rowCounter;
                    rowCounter++;
                }
                /* If column does not already exist, add to columns */
                if (columnMap.count(cNameN2) == 0) {
                    columnMap[cNameN2] = colCounter;
                    colCounter++;
                }
                /* Add the resistance value to the conductance map */
                bMatrixConductanceMap[rNameN2][label] = 0.0;
                nodeConnections[rNameN2].push_back(label);
                n2GND = false;
            }
            else n2GND = true;
            /* Add the intermediate node 1 */
            cNameNI1 = "C_NV" + TN1;
            rNameNI1 = "R_N" + TN1;
            /* If row does not already exist, add to rows */
            if (rowMap.count(rNameNI1) == 0) {
                rowMap[rNameNI1] = rowCounter;
                rowCounter++;
            }
            /* If column does not already exist, add to columns */
            if (columnMap.count(cNameNI1) == 0) {
                columnMap[cNameNI1] = colCounter;
                colCounter++;
            }
            /* Add the resistance value to the conductance map */
            bMatrixConductanceMap[rNameNI1][label] = 0.0;
            nodeConnections[rNameNI1].push_back(label);
            /* Add the intermediate node 2 */
            cNameNI2 = "C_NV" + TN2;
            rNameNI2 = "R_N" + TN2;
            /* If row does not already exist, add to rows */
            if (rowMap.count(rNameNI2) == 0) {
                rowMap[rNameNI2] = rowCounter;
                rowCounter++;
            }
            /* If column does not already exist, add to columns */
            if (columnMap.count(cNameNI2) == 0) {
                columnMap[cNameNI2] = colCounter;
                colCounter++;
            }
            /* Add the resistance value to the conductance map */
            bMatrixConductanceMap[rNameNI2][label] = 0.0;
            nodeConnections[rNameNI2].push_back(label);
            /* Start of add elements to matrix section */
            /* If positive node is not grounded */
            if (!pGND) {
                /* Positive node row and column */
                e.label = label;
                e.columnIndex = columnMap[cNameP];
                xlines[label].pNode1 = columnMap[cNameP];
                e.rowIndex = rowMap[rNameP];
                e.value = 1 / value;
                mElements.push_back(e);
                /* Add the column index of the positive node to the positive node row of the conductance map */
                /* This will be used to identify the voltage later */
                bMatrixConductanceMap[rNameP][label + "-VP"] = (double)e.columnIndex;
                /* If positive and intermediate node is not grounded */
                /* Positive node row and intermediate node column */
                e.columnIndex = columnMap[cNameNI1];
                xlines[label].iNode1 = columnMap[cNameNI1];
                e.rowIndex = rowMap[rNameP];
                e.value = -1 / value;
                mElements.push_back(e);
                /* Add the column index of the negative node to the positive node row of the conductance map */
                /* This will be used to identify the voltage later */
                bMatrixConductanceMap[rNameP][label + "-VN"] = (double)e.columnIndex;
                /* Intermediate node row and positive node column */
                e.columnIndex = columnMap[cNameP];
                e.rowIndex = rowMap[rNameNI1];
                e.value = -1 / value;
                mElements.push_back(e);
                /* Add the column index of the positive node to the negative node row of the conductance map */
                /* This will be used to identify the voltage later */
                bMatrixConductanceMap[rNameNI1][label + "-VP"] = (double)e.columnIndex;
                /* Intermediate node row and column */
                e.columnIndex = columnMap[cNameNI1];
                e.rowIndex = rowMap[rNameNI1];
                e.value = 1 / value;
                mElements.push_back(e);
                /* Add the column index of the positive node to the positive node row of the conductance map */
                /* This will be used to identify the voltage later */
                bMatrixConductanceMap[rNameNI1][label + "-VP"] = (double)e.columnIndex;
            }
            else {
                /* Intermediate node row and column */
                e.label = label;
                e.columnIndex = columnMap[cNameNI1];
                xlines[label].iNode1 = columnMap[cNameNI1];
                e.rowIndex = rowMap[rNameNI1];
                e.value = 1 / value;
                mElements.push_back(e);
                /* Add the column index of the positive node to the positive node row of the conductance map */
                /* This will be used to identify the voltage later */
                bMatrixConductanceMap[rNameNI1][label + "-VP"] = (double)e.columnIndex;
            }
            /* If second positive node is not grounded */
            if (!p2GND) {
                /* Positive node row and column */
                e.label = label;
                e.columnIndex = columnMap[cNameP2];
                xlines[label].pNode2 = columnMap[cNameP2];
                e.rowIndex = rowMap[rNameP2];
                e.value = 1 / value;
                mElements.push_back(e);
                /* Add the column index of the positive node to the positive node row of the conductance map */
                /* This will be used to identify the voltage later */
                bMatrixConductanceMap[rNameP2][label + "-VP"] = (double)e.columnIndex;
                /* If positive and intermediate node is not grounded */
                /* Positive node row and intermediate node column */
                e.label = label;
                e.columnIndex = columnMap[cNameNI2];
                xlines[label].iNode2 = columnMap[cNameNI2];
                e.rowIndex = rowMap[rNameP2];
                e.value = -1 / value;
                mElements.push_back(e);
                /* Add the column index of the negative node to the positive node row of the conductance map */
                /* This will be used to identify the voltage later */
                bMatrixConductanceMap[rNameP2][label + "-VN"] = (double)e.columnIndex;
                /* Intermediate node row and positive node column */
                e.label = label;
                e.columnIndex = columnMap[cNameP2];
                e.rowIndex = rowMap[rNameNI2];
                e.value = -1 / value;
                mElements.push_back(e);
                /* Add the column index of the positive node to the negative node row of the conductance map */
                /* This will be used to identify the voltage later */
                bMatrixConductanceMap[rNameNI2][label + "-VP"] = (double)e.columnIndex;
                /* Intermediate node row and column */
                e.label = label;
                e.columnIndex = columnMap[cNameNI2];
                e.rowIndex = rowMap[rNameNI2];
                e.value = 1 / value;
                mElements.push_back(e);
                /* Add the column index of the positive node to the positive node row of the conductance map */
                /* This will be used to identify the voltage later */
                bMatrixConductanceMap[rNameNI2][label + "-VP"] = (double)e.columnIndex;
            }
            else {
                /* Intermediate node row and column */
                e.label = label;
                e.columnIndex = columnMap[cNameNI2];
                xlines[label].iNode2 = columnMap[cNameNI2];
                e.rowIndex = rowMap[rNameNI2];
                e.value = 1 / value;
                mElements.push_back(e);
                /* Add the column index of the positive node to the positive node row of the conductance map */
                /* This will be used to identify the voltage later */
                bMatrixConductanceMap[rNameNI2][label + "-VP"] = (double)e.columnIndex;
            }
            cName1 = "C_" + TV1;
            rName1 = "R_" + TV1;
            if (rowMap.count(rName1) == 0) {
                rowMap[rName1] = rowCounter;
                rowCounter++;
            }
            if (columnMap.count(cName1) == 0) {
                columnMap[cName1] = colCounter;
                colCounter++;
            }
            cName2 = "C_" + TV2;
            rName2 = "R_" + TV2;
            if (rowMap.count(rName2) == 0) {
                rowMap[rName2] = rowCounter;
                rowCounter++;
            }
            if (columnMap.count(cName2) == 0) {
                columnMap[cName2] = colCounter;
                colCounter++;
            }
            /* Intermediate node 1 row and voltage node 1 column */
            e.label = label;
            e.columnIndex = columnMap[cName1];
            e.rowIndex = rowMap[rNameNI1];
            e.value = 1;
            mElements.push_back(e);
            /* Voltage node 1 row and intermediate node 1 column */
            e.label = label;
            e.columnIndex = columnMap[cNameNI1];
            e.rowIndex = rowMap[rName1];
            e.value = 1;
            mElements.push_back(e);
            /* Add the column index of the phase node to the voltage node row of the conductance map */
            /* This will be used to identify the voltage later */
            bMatrixConductanceMap[rName1][label] = (double)e.columnIndex;
            /* Intermediate node 2 row and voltage node 2 column */
            e.label = label;
            e.columnIndex = columnMap[cName2];
            e.rowIndex = rowMap[rNameNI2];
            e.value = 1;
            mElements.push_back(e);
            /* Voltage node 2 row and intermediate node 2 column */
            e.label = label;
            e.columnIndex = columnMap[cNameNI2];
            e.rowIndex = rowMap[rName2];
            e.value = 1;
            mElements.push_back(e);
            /* Add the column index of the phase node to the voltage node row of the conductance map */
            /* This will be used to identify the voltage later */
            bMatrixConductanceMap[rName2][label] = (double)e.columnIndex;
            if(!nGND) {
                /* Negative node 1 row and voltage node 1 column */
                e.label = label;
                e.columnIndex = columnMap[cName1];
                e.rowIndex = rowMap[rNameN];
                e.value = -1;
                mElements.push_back(e);
                /* Voltage node 1 row and negative node 1 column */
                e.label = label;
                e.columnIndex = columnMap[cNameN];
                xlines[label].nNode1 = columnMap[cNameN];
                e.rowIndex = rowMap[rName1];
                e.value = 1;
                mElements.push_back(e);
            }
            if(!n2GND) {
                /* Negative node 2 row and voltage node 2 column */
                e.label = label;
                e.columnIndex = columnMap[cName2];
                e.rowIndex = rowMap[rNameN2];
                e.value = 1;
                mElements.push_back(e);
                /* Voltage node 2 row and intermediate node 2 column */
                e.label = label;
                e.columnIndex = columnMap[cNameN2];
                xlines[label].iNode2 = columnMap[cNameN2];
                e.rowIndex = rowMap[rName2];
                e.value = 1;
                mElements.push_back(e);
            }
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
	for (const auto &i : columnNames) std::cout << std::setw(10) << std::left << i;
	std::cout << "\n";
	size_t counter = 0;
	for (auto i : A_matrix) {
		std::cout << std::setw(10) << std::left << rowNames.at(counter) + ":";
		for (auto j : i) {
			std::cout << std::setw(10) << std::left << j;
		}
		std::cout << "\n";
		counter++;
	}
}
