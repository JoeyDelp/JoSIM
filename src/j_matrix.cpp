// Copyright (c) 2018 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#include "j_matrix.hpp"

/*
  Systematically create A matrix
*/
void
matrix_A(InputFile& iFile)
{
	create_A_matrix(iFile);
	if (iFile.matA.rowNames.size() != iFile.matA.columnNames.size())
		matrix_errors(NON_SQUARE,
			std::to_string(iFile.matA.columnNames.size()) + "x" +
			std::to_string(iFile.matA.rowNames.size()));
	if (VERBOSE)
		std::cout << std::setw(35) << std::left
		<< "A matrix dimensions: " << iFile.matA.columnNames.size() << "\n\n";
	csr_A_matrix(iFile);
}
/*
  Identify each non zero matrix element
*/
void
create_A_matrix(InputFile& iFile)
{
	std::string cName, rName, cNameP, rNameP, cNameN, rNameN;
	std::vector<std::string> devicetokens, componentLabels;
	devicetokens.clear();
	componentLabels.clear();
	std::string label, nodeP, nodeN, subckt;
	std::unordered_map<std::string, int> rowMap, columnMap;
	rowMap.clear();
	columnMap.clear();
	int rowCounter, colCounter, lCounter;
	bool pGND, nGND;
	rowCounter = 0;
	colCounter = 0;
	lCounter = 0;
	/* Main circuit node identification*/
	for (auto i : iFile.maincircuitSegment) {
		element cElement;
		devicetokens = tokenize_space(i);
		double value = 0.0;
		/* Check if label exists, if not there is a bug in the program */
		try {
			label = devicetokens[0];
			if (std::find(componentLabels.begin(), componentLabels.end(), label) ==
				componentLabels.end())
				componentLabels.push_back(label);
			else {
				invalid_component_errors(DUPLICATE_LABEL, label);
			}
		}
		catch (const std::out_of_range&) {
			invalid_component_errors(MISSING_LABEL, i);
		}
		if (label.find_first_of("|") != std::string::npos) {
			subckt = iFile.subcircuitNameMap.at(label.substr(label.find_first_of("|") + 1));
		}
		/* Check if positive node exists, if not it's a bad device line definition
		 */
		try {
			nodeP = devicetokens[1];
		}
		catch (const std::out_of_range&) {
			invalid_component_errors(MISSING_PNODE, i);
		}
		/* Check if negative node exists, if not it's a bad device line definition
		 */
		try {
			nodeN = devicetokens[2];
		}
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
				if (iFile.parVal.find(devicetokens[3]) != iFile.parVal.end())
					value = iFile.parVal.at(devicetokens[3]);
				else if (iFile.subcircuitSegments[subckt].parVal.find(devicetokens[3]) != iFile.subcircuitSegments[subckt].parVal.end())
					value = iFile.subcircuitSegments[subckt].parVal.at(devicetokens[3]);
				else
					value = modifier(devicetokens[3]);
			}
			catch (const std::out_of_range&) {
				invalid_component_errors(RES_ERROR, i);
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
				/* Add the resistance value to the conductance map */
				iFile.matA.bMatrixConductanceMap[rNameP][label] = 0.0;
				iFile.matA.nodeConnections[rNameP].push_back(label);
				pGND = false;
			}
			else
				pGND = true;
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
				/* Add the resistance value to the conductance map */
				iFile.matA.bMatrixConductanceMap[rNameN][label] = 0.0;
				iFile.matA.nodeConnections[rNameN].push_back(label);
				nGND = false;
			}
			else
				nGND = true;
			/* Start of add elements to matrix section */
			/* If positive node is not grounded */
			if (!pGND) {
				/* Positive node row and column */
				e.label = label;
				e.columnIndex = columnMap[cNameP];
				e.rowIndex = rowMap[rNameP];
				e.value = 1 / value;
				iFile.matA.mElements.push_back(e);
				lCounter++;
				/* Add the column index of the positive node to the positive node row of
				 * the conductance map */
				 /* This will be used to identify the voltage later */
				iFile.matA.bMatrixConductanceMap[rNameP][label + "-VP"] = (double)e.columnIndex;
				/* If positive and negative node is not grounded */
				if (!nGND) {
					/* Positive node row and negative node column */
					e.label = label;
					e.columnIndex = columnMap[cNameN];
					e.rowIndex = rowMap[rNameP];
					e.value = -1 / value;
					iFile.matA.mElements.push_back(e);
					lCounter++;
					/* Add the column index of the negative node to the positive node row
					 * of the conductance map */
					 /* This will be used to identify the voltage later */
					iFile.matA.bMatrixConductanceMap[rNameP][label + "-VN"] = (double)e.columnIndex;
					/* Negative node row and positive node column */
					e.label = label;
					e.columnIndex = columnMap[cNameP];
					e.rowIndex = rowMap[rNameN];
					e.value = -1 / value;
					iFile.matA.mElements.push_back(e);
					lCounter++;
					/* Add the column index of the positive node to the negative node row
					 * of the conductance map */
					 /* This will be used to identify the voltage later */
					iFile.matA.bMatrixConductanceMap[rNameN][label + "-VP"] = (double)e.columnIndex;
				}
			}
			/* If negative node is not grounded */
			if (!nGND) {
				/* Negative node row and column */
				e.label = label;
				e.columnIndex = columnMap[cNameN];
				e.rowIndex = rowMap[rNameN];
				e.value = 1 / value;
				iFile.matA.mElements.push_back(e);
				lCounter++;
				/* Add the column index of the negative node to the negative node row of
				 * the conductance map */
				 /* This will be used to identify the voltage later */
				iFile.matA.bMatrixConductanceMap[rNameN][label + "-VN"] = (double)e.columnIndex;
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
			iFile.matA.elements.push_back(cElement);
		}
		/***************/
		/** CAPACITOR **/
		/***************/
		else if (i[0] == 'C') {
			/* Create a new matrix element for the resistor */
			matrix_element e;
			/* Check if value exists, if not it's a bad capacitor definition */
			try {
				if (iFile.parVal.find(devicetokens[3]) != iFile.parVal.end())
					value = iFile.parVal.at(devicetokens[3]);
				else if (iFile.subcircuitSegments[subckt].parVal.find(devicetokens[3]) != iFile.subcircuitSegments[subckt].parVal.end())
					value = iFile.subcircuitSegments[subckt].parVal.at(devicetokens[3]);
				else
					value = modifier(devicetokens[3]);
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
				/* Add the capacitance value to the conductance map */
				iFile.matA.bMatrixConductanceMap[rNameP][label] = value;
				iFile.matA.nodeConnections[rNameP].push_back(label);
				pGND = false;
			}
			else
				pGND = true;
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
				/* Add the capacitance value to the conductance map */
				iFile.matA.bMatrixConductanceMap[rNameN][label] = -value;
				iFile.matA.nodeConnections[rNameN].push_back(label);
				nGND = false;
			}
			else
				nGND = true;
			/* Start of add elements to matrix section */
			/* If positive node is not grounded */
			if (!pGND) {
				/* Positive node row and column */
				e.label = label;
				e.columnIndex = columnMap[cNameP];
				e.rowIndex = rowMap[rNameP];
				e.value = value / iFile.tsim.prstep;
				iFile.matA.mElements.push_back(e);
				lCounter++;
				/* Add the column index of the positive node to the positive node row in
				 * the conductance map */
				 /* This will be used to identify the voltage later */
				iFile.matA.bMatrixConductanceMap[rNameP][label + "-VP"] = (double)e.columnIndex;
				if (!nGND) {
					/* Positive node row and negative node column */
					e.label = label;
					e.columnIndex = columnMap[cNameN];
					e.rowIndex = rowMap[rNameP];
					e.value = -value / iFile.tsim.prstep;
					iFile.matA.mElements.push_back(e);
					lCounter++;
					/* Add the column index of the negative node to the positive node row
					 * in the conductance map */
					 /* This will be used to identify the voltage later */
					iFile.matA.bMatrixConductanceMap[rNameP][label + "-VN"] = (double)e.columnIndex;
					/* Negative node row and positive node column */
					e.label = label;
					e.columnIndex = columnMap[cNameP];
					e.rowIndex = rowMap[rNameN];
					e.value = -value / iFile.tsim.prstep;
					iFile.matA.mElements.push_back(e);
					lCounter++;
					/* Add the column index of the positive node to the negative node row
					 * in the conductance map */
					 /* This will be used to identify the voltage later */
					iFile.matA.bMatrixConductanceMap[rNameN][label + "-VP"] = (double)e.columnIndex;
				}
			}
			/* If negative node is not grounded */
			if (!nGND) {
				/* Negative node row and column */
				e.label = label;
				e.columnIndex = columnMap[cNameN];
				e.rowIndex = rowMap[rNameN];
				e.value = value / iFile.tsim.prstep;
				iFile.matA.mElements.push_back(e);
				lCounter++;
				/* Add the column index of the negative node to the negative node row in
				 * the conductance map */
				 /* This will be used to identify the voltage later */
				iFile.matA.bMatrixConductanceMap[rNameN][label + "-VN"] = (double)e.columnIndex;
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
			iFile.matA.elements.push_back(cElement);
		}
		/**************/
		/** INDUCTOR **/
		/**************/
		else if (i[0] == 'L') {
			/* Create a new matrix element for the resistor */
			matrix_element e;
			/* Check if value exists, if not it's a bad inductor definition */
			try {
				if (iFile.parVal.find(devicetokens[3]) != iFile.parVal.end())
					value = iFile.parVal.at(devicetokens[3]);
				else if (iFile.subcircuitSegments[subckt].parVal.find(devicetokens[3]) != iFile.subcircuitSegments[subckt].parVal.end())
					value = iFile.subcircuitSegments[subckt].parVal.at(devicetokens[3]);
				else
					value = modifier(devicetokens[3]);
			}
			catch (const std::out_of_range&) {
				invalid_component_errors(IND_ERROR, i);
			}
			cName = "C_I" + devicetokens[0];
			rName = "R_" + devicetokens[0];
			if (rowMap.count(rName) == 0) {
				rowMap[rName] = rowCounter;
				rowCounter++;
			}
			if (columnMap.count(cName) == 0) {
				columnMap[cName] = colCounter;
				colCounter++;
			}
			/* Add the inductor value to the conductance map */
			iFile.matA.inductanceMap[rName] = value;
			iFile.matA.bMatrixConductanceMap[rName][label] = value;
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
				/* Add the 0.0 to the conductance map */
				iFile.matA.bMatrixConductanceMap[rNameP][label] = 0.0;
				iFile.matA.nodeConnections[rNameP].push_back(label);
				pGND = false;
			}
			else
				pGND = true;
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
				/* Add the 0.0 to the conductance map */
				iFile.matA.bMatrixConductanceMap[rNameN][label] = 0.0;
				iFile.matA.nodeConnections[rNameN].push_back(label);
				nGND = false;
			}
			else
				nGND = true;
			/* Start of add elements to matrix section */
			iFile.matA.bMatrixConductanceMap[rName][label + "-VP"] = -1.0;
			iFile.matA.bMatrixConductanceMap[rName][label + "-VN"] = -1.0;
			/* If positive node is not grounded */
			if (!pGND) {
				/* Positive node row and column */
				lCounter++;
				/* Add the column index of the positive node to the positive node row of
				 * the conductance map */
				 /* This will be used to identify the voltage later */
				iFile.matA.bMatrixConductanceMap[rNameP][label + "-VP"] = (double)e.columnIndex;
				if (!nGND) {
					/* Positive node row and negative node column */
					lCounter++;
					/* Add the column index of the negative node to the positive node row
					 * of the conductance map */
					 /* This will be used to identify the voltage later */
					iFile.matA.bMatrixConductanceMap[rNameP][label + "-VN"] = (double)e.columnIndex;
					/* Negative node row and positive node column */
					lCounter++;
					/* Add the column index of the positive node to the negative node row
					 * of the conductance map */
					 /* This will be used to identify the voltage later */
					iFile.matA.bMatrixConductanceMap[rNameN][label + "-VP"] = (double)e.columnIndex;
				}
				/* Positive node row and inductor current node column */
				e.label = label;
				e.columnIndex = columnMap[cName];
				e.rowIndex = rowMap[rNameP];
				e.value = 1;
				iFile.matA.mElements.push_back(e);
				lCounter++;
				/* Inductor node row and positive node column */
				e.label = label;
				e.columnIndex = columnMap[cNameP];
				e.rowIndex = rowMap[rName];
				e.value = 1;
				iFile.matA.mElements.push_back(e);
				lCounter++;
				/* Add the column index of the positive node to the inductor node row of
				 * the conductance map */
				 /* This will be used to identify the voltage later */
				iFile.matA.bMatrixConductanceMap[rName][label + "-VP"] = (double)e.columnIndex;
			}
			if (!nGND) {
				/* Negative node row and column */
				lCounter++;
				/* Add the column index of the negative node to the negative node row of
				 * the conductance map */
				 /* This will be used to identify the voltage later */
				iFile.matA.bMatrixConductanceMap[rNameN][label + "-VN"] = (double)e.columnIndex;
				/* Negative node row and inductor current node column */
				e.label = label;
				e.columnIndex = columnMap[cName];
				e.rowIndex = rowMap[rNameN];
				e.value = -1;
				iFile.matA.mElements.push_back(e);
				lCounter++;
				/* Inductor node row and negative node column*/
				e.label = label;
				e.columnIndex = columnMap[cNameN];
				e.rowIndex = rowMap[rName];
				e.value = -1;
				iFile.matA.mElements.push_back(e);
				lCounter++;
				/* Add the column index of the negative node to the inductor node row of
				 * the conductance map */
				 /* This will be used to identify the voltage later */
				iFile.matA.bMatrixConductanceMap[rName][label + "-VN"] = (double)e.columnIndex;
			}
			/* Inductor node row and inductor current node column*/
			e.label = label;
			e.columnIndex = columnMap[cName];
			e.rowIndex = rowMap[rName];
			e.value = (-2 * value) / iFile.tsim.prstep;
			iFile.matA.branchRelations[label].index = lCounter;
			iFile.matA.branchRelations[label].current1RowIndex = rowMap.at(rName);
			iFile.matA.branchRelations[label].current1ColumnIndex = columnMap[cName];
			iFile.matA.branchRelations[label].inductance = value;
			iFile.matA.branchRelations[label].mutI1 = label;
			iFile.matA.mElements.push_back(e);
			lCounter++;
			/* Add the column index of the inductor current node to the inductor node
			 * row of the conductance map */
			 /* This will be used to identify the voltage later */
			iFile.matA.bMatrixConductanceMap[rName][label + "-I"] = (double)e.columnIndex;
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
			iFile.matA.elements.push_back(cElement);
		}
		/********************/
		/** VOLTAGE SOURCE **/
		/********************/
		else if (i[0] == 'V') {
			/* Create a new matrix element for the resistor */
			matrix_element e;
			/* Parse the function identified (if any)*/
			iFile.matA.sources[label] = function_parse(i, iFile);
			cName = "C_" + devicetokens[0];
			rName = "R_" + devicetokens[0];
			if (rowMap.count(rName) == 0) {
				rowMap[rName] = rowCounter;
				rowCounter++;
			}
			if (columnMap.count(cName) == 0) {
				columnMap[cName] = colCounter;
				colCounter++;
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
				iFile.matA.nodeConnections[rNameP].push_back(label);
				pGND = false;
			}
			else
				pGND = true;
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
				iFile.matA.nodeConnections[rNameN].push_back(label);
				nGND = false;
			}
			else
				nGND = true;
			/* Start of add elements to matrix section */
			/* If positive node is not grounded */
			if (!pGND) {
				/* Positive node row and column */
				lCounter++;
				if (!nGND) {
					/* Positive node row and negative node column */
					lCounter++;
					/* Negative node row and positive node column */
					lCounter++;
				}
				/* Positive node row and voltage node column */
				e.label = label;
				e.columnIndex = columnMap[cName];
				e.rowIndex = rowMap[rNameP];
				e.value = 1;
				iFile.matA.mElements.push_back(e);
				lCounter++;
				/* Voltage node row and positive node column */
				e.label = label;
				e.columnIndex = columnMap[cNameP];
				e.rowIndex = rowMap[rName];
				e.value = 1;
				iFile.matA.mElements.push_back(e);
				lCounter++;
			}
			/* If negative node is not grounded */
			if (!nGND) {
				/* Negative node row and column */
				lCounter++;
				/* Negative node row and voltage node column */
				e.label = label;
				e.columnIndex = columnMap[cName];
				e.rowIndex = rowMap[rNameN];
				e.value = -1;
				iFile.matA.mElements.push_back(e);
				lCounter++;
				/* Voltage node row and negative node column*/
				e.label = label;
				e.columnIndex = columnMap[cNameN];
				e.rowIndex = rowMap[rName];
				e.value = -1;
				iFile.matA.mElements.push_back(e);
				lCounter++;
			}
			/* Voltage node row and voltage node column*/
			lCounter++;
			/* Add the column index of the phase node to the junction node row of the
			 * conductance map */
			 /* This will be used to identify the voltage later */
			iFile.matA.bMatrixConductanceMap[rName][label] = (double)e.columnIndex;
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
			iFile.matA.elements.push_back(cElement);
		}
		/********************/
		/** CURRENT SOURCE **/
		/********************/
		else if (i[0] == 'I') {
			/* Parse the function identified (if any)*/
			iFile.matA.sources[label] = function_parse(i, iFile);
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
				iFile.matA.bMatrixConductanceMap[rNameP][label] = 1.0;
				iFile.matA.nodeConnections[rNameP].push_back(label);
				pGND = false;
			}
			else
				pGND = true;
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
				/* Add the 0.0 to the conductance map */
				iFile.matA.bMatrixConductanceMap[rNameN][label] = 1.0;
				iFile.matA.nodeConnections[rNameN].push_back(label);
				nGND = false;
			}
			else
				nGND = true;
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
			iFile.matA.elements.push_back(cElement);
		}
		/************************/
		/** JOSEPHSON JUNCTION **/
		/************************/
		else if (i[0] == 'B') {
			/* Create a new matrix element for the junction */
			matrix_element e;
			/* Identify the JJ parameters based on the model*/
			int jj_type = 0;
			std::unordered_map<std::string, double> jj_tokens;
			double jj_cap = 0.0;
			double jj_rn = 0.0;
			double jj_rzero = 0.0;
			double jj_icrit = 0.0;
			double jj_rtype = 0;
			double jj_vgap = 0.0;
			jj_comp(devicetokens, iFile, jj_type, jj_cap, jj_rn, jj_rzero, jj_icrit, jj_rtype, jj_vgap);
			cName = "C_P" + devicetokens[0];
			rName = "R_" + devicetokens[0];
			if (rowMap.count(rName) == 0) {
				rowMap[rName] = rowCounter;
				rowCounter++;
			}
			if (columnMap.count(cName) == 0) {
				columnMap[cName] = colCounter;
				colCounter++;
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
				iFile.matA.nodeConnections[rNameP].push_back(label);
				iFile.matA.bMatrixConductanceMap[rNameP][label + "-RTYPE"] = jj_rtype;
				iFile.matA.bMatrixConductanceMap[rNameP][label + "-CAP"] = jj_cap;
				iFile.matA.bMatrixConductanceMap[rNameP][label + "-ICRIT"] = jj_icrit;
				iFile.matA.bMatrixConductanceMap[rNameP][label + "-VGAP"] = jj_vgap;
				pGND = false;
			}
			else
				pGND = true;
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
				iFile.matA.nodeConnections[rNameN].push_back(label);
				iFile.matA.bMatrixConductanceMap[rNameP][label + "-RTYPE"] = jj_rtype;
				iFile.matA.bMatrixConductanceMap[rNameP][label + "-CAP"] = jj_cap;
				iFile.matA.bMatrixConductanceMap[rNameP][label + "-ICRIT"] = jj_icrit;
				iFile.matA.bMatrixConductanceMap[rNameP][label + "-VGAP"] = jj_vgap;
				nGND = false;
			}
			else
				nGND = true;
			/* Start of add elements to matrix section */
			iFile.matA.bMatrixConductanceMap[rName][label + "-VP"] = -1.0;
			iFile.matA.bMatrixConductanceMap[rName][label + "-VN"] = -1.0;
			/* If positive node is not grounded */
			if (!pGND) {
				/* Positive node row and column */
				e.label = label;
				e.columnIndex = columnMap[cNameP];
				e.rowIndex = rowMap[rNameP];
				e.junctionEntry = true;
				e.tokens["RN"] = jj_rn;
				e.tokens["CAP"] = jj_cap;
				e.tokens["R0"] = jj_rzero;
				e.junctionDirection = 'P';
				e.value = ((2 * jj_cap) / iFile.tsim.prstep) + (1 / jj_rzero);
				iFile.matA.mElements.push_back(e);
				iFile.matA.bMatrixConductanceMap[rName][label + "-MPTR_PP"] = iFile.matA.mElements.size() - 1;
				lCounter++;
				e.junctionEntry = false;
				/* Add the column index of the positive node to the positive node row of
				 * the conductance map */
				 /* This will be used to identify the voltage later */
				iFile.matA.bMatrixConductanceMap[rNameP][label + "-VP"] = (double)e.columnIndex;
				if (!nGND) {
					/* Positive node row and negative node column */
					e.label = label;
					e.columnIndex = columnMap[cNameN];
					e.rowIndex = rowMap[rNameP];
					e.junctionEntry = true;
					e.tokens["RN"] = jj_rn;
					e.tokens["CAP"] = jj_cap;
					e.tokens["R0"] = jj_rzero;
					e.junctionDirection = 'N';
					e.value = -(((2 * jj_cap) / iFile.tsim.prstep) + (1 / jj_rzero));
					iFile.matA.mElements.push_back(e);
					iFile.matA.bMatrixConductanceMap[rName][label + "-MPTR_PN"] = iFile.matA.mElements.size() - 1;
					lCounter++;
					e.junctionEntry = false;
					/* Add the column index of the negative node to the positive node row
					 * of the conductance map */
					 /* This will be used to identify the voltage later */
					iFile.matA.bMatrixConductanceMap[rNameP][label + "-VN"] = (double)e.columnIndex;
					/* Negative node row and positive node column */
					e.label = label;
					e.columnIndex = columnMap[cNameP];
					e.rowIndex = rowMap[rNameN];
					e.junctionEntry = true;
					e.tokens["RN"] = jj_rn;
					e.tokens["CAP"] = jj_cap;
					e.tokens["R0"] = jj_rzero;
					e.junctionDirection = 'N';
					e.value = -(((2 * jj_cap) / iFile.tsim.prstep) + (1 / jj_rzero));
					iFile.matA.mElements.push_back(e);
					iFile.matA.bMatrixConductanceMap[rName][label + "-MPTR_NP"] = iFile.matA.mElements.size() - 1;
					lCounter++;
					e.junctionEntry = false;
					/* Add the column index of the positive node to the negative node row
					 * of the conductance map */
					 /* This will be used to identify the voltage later */
					iFile.matA.bMatrixConductanceMap[rNameN][label + "-VP"] = (double)e.columnIndex;
				}
				/* Positive node row and phase node column */
				lCounter++;
				e.junctionEntry = false;
				/* Add the column index of the phase node to the positive node row of
				 * the conductance map */
				 /* This will be used to identify the voltage later */
				iFile.matA.bMatrixConductanceMap[rNameP][label + "-PHASE"] = (double)e.columnIndex;
				/* Junction node row and positive node column */
				e.label = label;
				e.columnIndex = columnMap[cNameP];
				e.rowIndex = rowMap[rName];
				e.value = (-iFile.tsim.prstep / 2) * ((2 * M_PI) / PHI_ZERO);
				iFile.matA.mElements.push_back(e);
				lCounter++;
				e.junctionEntry = false;
				/* Add the column index of the positive node to the junction node row of
				 * the conductance map */
				 /* This will be used to identify the voltage later */
				iFile.matA.bMatrixConductanceMap[rName][label + "-VP"] = (double)e.columnIndex;
			}
			/* If negative node is not grounded */
			if (!nGND) {
				/* Negative node row and column */
				e.label = label;
				e.columnIndex = columnMap[cNameN];
				e.rowIndex = rowMap[rNameN];
				e.junctionEntry = true;
				e.tokens["RN"] = jj_rn;
				e.tokens["CAP"] = jj_cap;
				e.tokens["R0"] = jj_rzero;
				e.junctionDirection = 'P';
				e.value = ((2 * jj_cap) / iFile.tsim.prstep) + (1 / jj_rzero);
				iFile.matA.mElements.push_back(e);
				iFile.matA.bMatrixConductanceMap[rName][label + "-MPTR_NN"] = iFile.matA.mElements.size() - 1;
				lCounter++;
				e.junctionEntry = false;
				/* Add the column index of the negative node to the negative node row of
				 * the conductance map */
				 /* This will be used to identify the voltage later */
				iFile.matA.bMatrixConductanceMap[rNameN][label + "-VN"] = (double)e.columnIndex;
				/* Negative node row and phase node column */
				lCounter++;
				/* Add the column index of the phase node to the negative node row of
				 * the conductance map */
				 /* This will be used to identify the voltage later */
				iFile.matA.bMatrixConductanceMap[rNameN][label + "-PHASE"] = (double)e.columnIndex;
				/* Junction node row and negative node column*/
				e.label = label;
				e.columnIndex = columnMap[cNameN];
				e.rowIndex = rowMap[rName];
				e.value = (iFile.tsim.prstep / 2) * ((2 * M_PI) / PHI_ZERO);
				iFile.matA.mElements.push_back(e);
				lCounter++;
				e.junctionEntry = false;
				/* Add the column index of the negative node to the junction node row of
				 * the conductance map */
				 /* This will be used to identify the voltage later */
				iFile.matA.bMatrixConductanceMap[rName][label + "-VN"] = (double)e.columnIndex;
			}
			/* Junction node row and phase node column*/
			e.label = label;
			e.columnIndex = columnMap[cName];
			e.rowIndex = rowMap[rName];
			e.value = 1;
			iFile.matA.mElements.push_back(e);
			lCounter++;
			e.junctionEntry = false;
			/* Add the column index of the phase node to the junction node row of the
			 * conductance map */
			 /* This will be used to identify the voltage later */
			iFile.matA.bMatrixConductanceMap[rName][label + "-PHASE"] = (double)e.columnIndex;
			iFile.matA.bMatrixConductanceMap[rName][label + "-RTYPE"] = jj_rtype;
			iFile.matA.bMatrixConductanceMap[rName][label + "-CAP"] = jj_cap;
			iFile.matA.bMatrixConductanceMap[rName][label + "-ICRIT"] = jj_icrit;
			iFile.matA.bMatrixConductanceMap[rName][label + "-VGAP"] = jj_vgap;
			iFile.matA.bMatrixConductanceMap[rName][label + "-R0"] = jj_rzero;
			iFile.matA.bMatrixConductanceMap[rName][label + "-RN"] = jj_rn;
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
			iFile.matA.elements.push_back(cElement);
		}
		/***********************/
		/** TRANSMISSION LINE **/
		/***********************/
		else if (i[0] == 'T') {
			std::string nodeP2, nodeN2, TN1, TN2, TV1, TV2, cNameP2, rNameP2, cNameN2,
				rNameN2, cNameNI1, rNameNI1, cNameNI2, rNameNI2, cName1, rName1, cName2,
				rName2;
			bool p2GND, n2GND;
			double TimeDelay = 0.0;
			/* Check if positive node 2 exists, if not it's a bad device line
			 * definition */
			try {
				nodeP2 = devicetokens[3];
			}
			catch (const std::out_of_range&) {
				invalid_component_errors(MISSING_PNODE, i);
			}
			/* Check if negative node 2 exists, if not it's a bad device line
			 * definition */
			try {
				nodeN2 = devicetokens[4];
			}
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
			for (size_t l = 5; l < devicetokens.size(); l++) {
				if (devicetokens[l].find("TD") != std::string::npos)
					TimeDelay = modifier((devicetokens[l]).substr(3));
				else if (devicetokens[l].find("Z0") != std::string::npos)
					value = modifier((devicetokens[l]).substr(3));
			}
			iFile.matA.xlines[label].Z0 = value;
			iFile.matA.xlines[label].label = label;
			iFile.matA.xlines[label].TD = TimeDelay;
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
				iFile.matA.bMatrixConductanceMap[rNameP][label] = 0.0;
				iFile.matA.nodeConnections[rNameP].push_back(label);
				pGND = false;
			}
			else
				pGND = true;
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
				iFile.matA.bMatrixConductanceMap[rNameN][label] = 0.0;
				iFile.matA.nodeConnections[rNameN].push_back(label);
				nGND = false;
			}
			else
				nGND = true;
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
				iFile.matA.bMatrixConductanceMap[rNameP2][label] = 0.0;
				iFile.matA.nodeConnections[rNameP2].push_back(label);
				p2GND = false;
			}
			else
				p2GND = true;
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
				iFile.matA.bMatrixConductanceMap[rNameN2][label] = 0.0;
				iFile.matA.nodeConnections[rNameN2].push_back(label);
				n2GND = false;
			}
			else
				n2GND = true;
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
			iFile.matA.bMatrixConductanceMap[rNameNI1][label] = 0.0;
			iFile.matA.nodeConnections[rNameNI1].push_back(label);
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
			iFile.matA.bMatrixConductanceMap[rNameNI2][label] = 0.0;
			iFile.matA.nodeConnections[rNameNI2].push_back(label);
			/* Start of add elements to matrix section */
			/* If positive node is not grounded */
			if (!pGND) {
				/* Positive node row and column */
				e.label = label;
				e.columnIndex = columnMap[cNameP];
				iFile.matA.xlines[label].pNode1 = columnMap[cNameP];
				e.rowIndex = rowMap[rNameP];
				e.value = 1 / value;
				iFile.matA.mElements.push_back(e);
				lCounter++;
				/* Add the column index of the positive node to the positive node row of
				 * the conductance map */
				 /* This will be used to identify the voltage later */
				iFile.matA.bMatrixConductanceMap[rNameP][label + "-VP"] = (double)e.columnIndex;
				/* If positive and intermediate node is not grounded */
				/* Positive node row and intermediate node column */
				e.columnIndex = columnMap[cNameNI1];
				iFile.matA.xlines[label].iNode1 = columnMap[cNameNI1];
				e.rowIndex = rowMap[rNameP];
				e.value = -1 / value;
				iFile.matA.mElements.push_back(e);
				lCounter++;
				/* Add the column index of the negative node to the positive node row of
				 * the conductance map */
				 /* This will be used to identify the voltage later */
				iFile.matA.bMatrixConductanceMap[rNameP][label + "-VN"] = (double)e.columnIndex;
				/* Intermediate node row and positive node column */
				e.columnIndex = columnMap[cNameP];
				e.rowIndex = rowMap[rNameNI1];
				e.value = -1 / value;
				iFile.matA.mElements.push_back(e);
				lCounter++;
				/* Add the column index of the positive node to the negative node row of
				 * the conductance map */
				 /* This will be used to identify the voltage later */
				iFile.matA.bMatrixConductanceMap[rNameNI1][label + "-VP"] = (double)e.columnIndex;
				/* Intermediate node row and column */
				e.columnIndex = columnMap[cNameNI1];
				e.rowIndex = rowMap[rNameNI1];
				e.value = 1 / value;
				iFile.matA.mElements.push_back(e);
				lCounter++;
				/* Add the column index of the positive node to the positive node row of
				 * the conductance map */
				 /* This will be used to identify the voltage later */
				iFile.matA.bMatrixConductanceMap[rNameNI1][label + "-VP"] = (double)e.columnIndex;
			}
			else {
				/* Intermediate node row and column */
				e.label = label;
				e.columnIndex = columnMap[cNameNI1];
				iFile.matA.xlines[label].iNode1 = columnMap[cNameNI1];
				e.rowIndex = rowMap[rNameNI1];
				e.value = 1 / value;
				iFile.matA.mElements.push_back(e);
				lCounter++;
				/* Add the column index of the positive node to the positive node row of
				 * the conductance map */
				 /* This will be used to identify the voltage later */
				iFile.matA.bMatrixConductanceMap[rNameNI1][label + "-VP"] = (double)e.columnIndex;
			}
			/* If second positive node is not grounded */
			if (!p2GND) {
				/* Positive node row and column */
				e.label = label;
				e.columnIndex = columnMap[cNameP2];
				iFile.matA.xlines[label].pNode2 = columnMap[cNameP2];
				e.rowIndex = rowMap[rNameP2];
				e.value = 1 / value;
				iFile.matA.mElements.push_back(e);
				lCounter++;
				/* Add the column index of the positive node to the positive node row of
				 * the conductance map */
				 /* This will be used to identify the voltage later */
				iFile.matA.bMatrixConductanceMap[rNameP2][label + "-VP"] = (double)e.columnIndex;
				/* If positive and intermediate node is not grounded */
				/* Positive node row and intermediate node column */
				e.label = label;
				e.columnIndex = columnMap[cNameNI2];
				iFile.matA.xlines[label].iNode2 = columnMap[cNameNI2];
				e.rowIndex = rowMap[rNameP2];
				e.value = -1 / value;
				iFile.matA.mElements.push_back(e);
				lCounter++;
				/* Add the column index of the negative node to the positive node row of
				 * the conductance map */
				 /* This will be used to identify the voltage later */
				iFile.matA.bMatrixConductanceMap[rNameP2][label + "-VN"] = (double)e.columnIndex;
				/* Intermediate node row and positive node column */
				e.label = label;
				e.columnIndex = columnMap[cNameP2];
				e.rowIndex = rowMap[rNameNI2];
				e.value = -1 / value;
				iFile.matA.mElements.push_back(e);
				lCounter++;
				/* Add the column index of the positive node to the negative node row of
				 * the conductance map */
				 /* This will be used to identify the voltage later */
				iFile.matA.bMatrixConductanceMap[rNameNI2][label + "-VP"] = (double)e.columnIndex;
				/* Intermediate node row and column */
				e.label = label;
				e.columnIndex = columnMap[cNameNI2];
				e.rowIndex = rowMap[rNameNI2];
				e.value = 1 / value;
				iFile.matA.mElements.push_back(e);
				lCounter++;
				/* Add the column index of the positive node to the positive node row of
				 * the conductance map */
				 /* This will be used to identify the voltage later */
				iFile.matA.bMatrixConductanceMap[rNameNI2][label + "-VP"] = (double)e.columnIndex;
			}
			else {
				/* Intermediate node row and column */
				e.label = label;
				e.columnIndex = columnMap[cNameNI2];
				iFile.matA.xlines[label].iNode2 = columnMap[cNameNI2];
				e.rowIndex = rowMap[rNameNI2];
				e.value = 1 / value;
				iFile.matA.mElements.push_back(e);
				lCounter++;
				/* Add the column index of the positive node to the positive node row of
				 * the conductance map */
				 /* This will be used to identify the voltage later */
				iFile.matA.bMatrixConductanceMap[rNameNI2][label + "-VP"] = (double)e.columnIndex;
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
			iFile.matA.mElements.push_back(e);
			lCounter++;
			/* Voltage node 1 row and intermediate node 1 column */
			e.label = label;
			e.columnIndex = columnMap[cNameNI1];
			e.rowIndex = rowMap[rName1];
			e.value = 1;
			iFile.matA.mElements.push_back(e);
			lCounter++;
			/* Add the column index of the phase node to the voltage node row of the
			 * conductance map */
			 /* This will be used to identify the voltage later */
			iFile.matA.bMatrixConductanceMap[rName1][label] = (double)e.columnIndex;
			/* Intermediate node 2 row and voltage node 2 column */
			e.label = label;
			e.columnIndex = columnMap[cName2];
			e.rowIndex = rowMap[rNameNI2];
			e.value = 1;
			iFile.matA.mElements.push_back(e);
			lCounter++;
			/* Voltage node 2 row and intermediate node 2 column */
			e.label = label;
			e.columnIndex = columnMap[cNameNI2];
			e.rowIndex = rowMap[rName2];
			e.value = 1;
			iFile.matA.mElements.push_back(e);
			lCounter++;
			/* Add the column index of the phase node to the voltage node row of the
			 * conductance map */
			 /* This will be used to identify the voltage later */
			iFile.matA.bMatrixConductanceMap[rName2][label] = (double)e.columnIndex;
			if (!nGND) {
				/* Negative node 1 row and voltage node 1 column */
				e.label = label;
				e.columnIndex = columnMap[cName1];
				e.rowIndex = rowMap[rNameN];
				e.value = -1;
				iFile.matA.mElements.push_back(e);
				lCounter++;
				/* Voltage node 1 row and negative node 1 column */
				e.label = label;
				e.columnIndex = columnMap[cNameN];
				iFile.matA.xlines[label].nNode1 = columnMap[cNameN];
				e.rowIndex = rowMap[rName1];
				e.value = 1;
				iFile.matA.mElements.push_back(e);
				lCounter++;
			}
			if (!n2GND) {
				/* Negative node 2 row and voltage node 2 column */
				e.label = label;
				e.columnIndex = columnMap[cName2];
				e.rowIndex = rowMap[rNameN2];
				e.value = 1;
				iFile.matA.mElements.push_back(e);
				lCounter++;
				/* Voltage node 2 row and intermediate node 2 column */
				e.label = label;
				e.columnIndex = columnMap[cNameN2];
				iFile.matA.xlines[label].iNode2 = columnMap[cNameN2];
				e.rowIndex = rowMap[rName2];
				e.value = 1;
				iFile.matA.mElements.push_back(e);
				lCounter++;
			}
		}
		/*********************/
		/** MUTUAL COUPLING **/
		/*********************/
		else if (i[0] == 'K') {
			iFile.mutualInductanceLines.push_back(i);
		}
	}
	double mutualL = 0.0, cf = 0.0;
	// Handle mutual inductances outside loop due to chicken egg situation
	for (auto i : iFile.mutualInductanceLines) {
		// Tokenize the line
		devicetokens = tokenize_space(i);
		// Check if label exists, if not there is a bug in the program
		try {
			label = devicetokens[0];
		}
		catch (const std::out_of_range&) {
			invalid_component_errors(MISSING_LABEL, i);
		}
		if (label.find_first_of("|") != std::string::npos) {
			subckt = iFile.subcircuitNameMap.at(label.substr(label.find_first_of("|") + 1));
		}
		// Check if coupling factor k exists, if not it's a bad mutual coupling definition
		try {
			if (iFile.parVal.find(devicetokens[3]) != iFile.parVal.end())
				cf = iFile.parVal.at(devicetokens[3]);
			else if (iFile.subcircuitSegments[subckt].parVal.find(devicetokens[3]) != iFile.subcircuitSegments[subckt].parVal.end())
				cf = iFile.subcircuitSegments[subckt].parVal.at(devicetokens[3]);
			else
				cf = modifier(devicetokens[3]);
		}
		catch (const std::out_of_range&) {
			invalid_component_errors(MUT_ERROR, i);
		}
		// Next identify the 2 coupling inductors
		std::string ind1, ind2;
		ind1 = devicetokens[1];
		ind2 = devicetokens[2];
		// Calculate the coupling factor M
		cf = cf * sqrt(iFile.matA.inductanceMap["R_" + ind1] * iFile.matA.inductanceMap["R_" + ind2]);
		mutualL = ((2 * cf) / iFile.tsim.prstep);
		iFile.matA.branchRelations.at(ind1).mutualInductance.push_back(mutualL);
		iFile.matA.branchRelations.at(ind2).mutualInductance.push_back(mutualL);
		iFile.matA.branchRelations.at(ind1).mutI2.push_back(ind2);
		iFile.matA.branchRelations.at(ind2).mutI2.push_back(ind1);
		iFile.matA.branchRelations.at(ind1).current2RowIndex.push_back(iFile.matA.branchRelations.at(ind2).current1RowIndex);
		iFile.matA.branchRelations.at(ind2).current2RowIndex.push_back(iFile.matA.branchRelations.at(ind1).current1RowIndex);
		iFile.matA.branchRelations.at(ind1).current2ColumnIndex.push_back(iFile.matA.branchRelations.at(ind2).current1ColumnIndex);
		iFile.matA.branchRelations.at(ind2).current2ColumnIndex.push_back(iFile.matA.branchRelations.at(ind1).current1ColumnIndex);
		// Inductor1
		matrix_element e;
		e.label = label;
		e.columnIndex = iFile.matA.branchRelations.at(ind1).current2ColumnIndex.back();
		e.rowIndex = iFile.matA.branchRelations.at(ind1).current1RowIndex;
		e.value = -mutualL;
		iFile.matA.mElements.push_back(e);
		// Inductor2
		e.label = label;
		e.columnIndex = iFile.matA.branchRelations.at(ind2).current2ColumnIndex.back();
		e.rowIndex = iFile.matA.branchRelations.at(ind2).current1RowIndex;
		e.value = -mutualL;
		iFile.matA.mElements.push_back(e);
	}
	std::map<int, std::string> rowMapFlip = flip_map(rowMap);
	std::map<int, std::string> columnMapFlip = flip_map(columnMap);
	std::transform(
		rowMapFlip.begin(),
		rowMapFlip.end(),
		back_inserter(iFile.matA.rowNames),
		[](std::pair<int, std::string> const& pair) { return pair.second; });
	std::transform(
		columnMapFlip.begin(),
		columnMapFlip.end(),
		back_inserter(iFile.matA.columnNames),
		[](std::pair<int, std::string> const& pair) { return pair.second; });
}
/*
 Create the A matrix in CSR format
 */
void csr_A_matrix(InputFile& iFile) {
	iFile.matA.Nsize = iFile.matA.rowNames.size();
	iFile.matA.Msize = iFile.matA.columnNames.size();
	std::vector<std::map<int,double>> aMat(iFile.matA.rowNames.size());
	iFile.matA.colind.clear();
	iFile.matA.nzval.clear();
	iFile.matA.rowptr.clear();
	iFile.matA.rowptr.push_back(0);
	for (auto i : iFile.matA.mElements){
		aMat[i.rowIndex][i.columnIndex] += i.value;
	}
	for(auto i : aMat) {
		for (auto j : i) {
			iFile.matA.nzval.push_back(j.second);
			iFile.matA.colind.push_back(j.first);
		}
		iFile.matA.rowptr.push_back(iFile.matA.rowptr.back() + i.size());
	}	
}