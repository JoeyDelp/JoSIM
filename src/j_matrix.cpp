// Copyright (c) 2018 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#include "j_matrix.hpp"

/*
  Systematically create A matrix
*/
void
matrix_A(InputFile& iFile)
{
	if(analType == VANAL) create_A_matrix_volt(iFile);
	else if(analType == PANAL) create_A_matrix_phase(iFile);
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
  Identify each non zero matrix element in voltage form
*/
void
create_A_matrix_volt(InputFile& iFile)
{
	std::string cName, rName, cNameP, rNameP, cNameN, rNameN;
	std::vector<std::string> devicetokens, componentLabels;
	devicetokens.clear();
	componentLabels.clear();
	std::string label, nodeP, nodeN, subckt;
	std::unordered_map<std::string, int> rowMap, columnMap;
	rowMap.clear();
	columnMap.clear();
	int rowCounter, colCounter;
	bool pGND, nGND;
	rowCounter = 0;
	colCounter = 0;
	/* Main circuit node identification */
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
			iFile.matA.impedanceMap[rName] = value;
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
				/* Add the column index of the positive node to the positive node row of
				 * the conductance map */
				 /* This will be used to identify the voltage later */
				iFile.matA.bMatrixConductanceMap[rNameP][label + "-VP"] = (double)e.columnIndex;
				if (!nGND) {
					/* Add the column index of the negative node to the positive node row
					 * of the conductance map */
					 /* This will be used to identify the voltage later */
					iFile.matA.bMatrixConductanceMap[rNameP][label + "-VN"] = (double)e.columnIndex;
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
				/* Inductor node row and positive node column */
				e.label = label;
				e.columnIndex = columnMap[cNameP];
				e.rowIndex = rowMap[rName];
				e.value = 1;
				iFile.matA.mElements.push_back(e);
				/* Add the column index of the positive node to the inductor node row of
				 * the conductance map */
				 /* This will be used to identify the voltage later */
				iFile.matA.bMatrixConductanceMap[rName][label + "-VP"] = (double)e.columnIndex;
			}
			if (!nGND) {
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
				/* Inductor node row and negative node column*/
				e.label = label;
				e.columnIndex = columnMap[cNameN];
				e.rowIndex = rowMap[rName];
				e.value = -1;
				iFile.matA.mElements.push_back(e);
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
			iFile.matA.branchRelations[label].current1RowIndex = rowMap.at(rName);
			iFile.matA.branchRelations[label].current1ColumnIndex = columnMap[cName];
			iFile.matA.branchRelations[label].inductance = value;
			iFile.matA.branchRelations[label].mutI1 = label;
			iFile.matA.mElements.push_back(e);
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
				/* Positive node row and voltage node column */
				e.label = label;
				e.columnIndex = columnMap[cName];
				e.rowIndex = rowMap[rNameP];
				e.value = 1;
				iFile.matA.mElements.push_back(e);
				/* Voltage node row and positive node column */
				e.label = label;
				e.columnIndex = columnMap[cNameP];
				e.rowIndex = rowMap[rName];
				e.value = 1;
				iFile.matA.mElements.push_back(e);
			}
			/* If negative node is not grounded */
			if (!nGND) {
				/* Negative node row and voltage node column */
				e.label = label;
				e.columnIndex = columnMap[cName];
				e.rowIndex = rowMap[rNameN];
				e.value = -1;
				iFile.matA.mElements.push_back(e);
				/* Voltage node row and negative node column*/
				e.label = label;
				e.columnIndex = columnMap[cNameN];
				e.rowIndex = rowMap[rName];
				e.value = -1;
				iFile.matA.mElements.push_back(e);
			}
			/* Voltage node row and voltage node column*/
			;
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
			double jj_cap, jj_rn, jj_rzero, jj_icrit, jj_rtype, jj_vgap, jj_icfact, jj_delv;
			jj_cap = jj_rn = jj_rzero = jj_icrit = jj_rtype = jj_vgap = jj_icfact = jj_delv = 0.0;
			jj_comp(devicetokens, iFile, jj_type, jj_cap, jj_rn, jj_rzero, jj_icrit, jj_rtype, jj_vgap, jj_icfact, jj_delv);
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
				iFile.matA.bMatrixConductanceMap[rNameP][label + "-ICFACT"] = jj_icfact;
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
				iFile.matA.bMatrixConductanceMap[rNameP][label + "-ICFACT"] = jj_icfact;
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
					e.junctionEntry = false;
					/* Add the column index of the positive node to the negative node row
					 * of the conductance map */
					 /* This will be used to identify the voltage later */
					iFile.matA.bMatrixConductanceMap[rNameN][label + "-VP"] = (double)e.columnIndex;
				}
				/* Positive node row and phase node column */
				;
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
				e.junctionEntry = false;
				/* Add the column index of the negative node to the negative node row of
				 * the conductance map */
				 /* This will be used to identify the voltage later */
				iFile.matA.bMatrixConductanceMap[rNameN][label + "-VN"] = (double)e.columnIndex;
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
			iFile.matA.bMatrixConductanceMap[rName][label + "-ICFACT"] = jj_icfact;
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
				/* Add the column index of the negative node to the positive node row of
				 * the conductance map */
				 /* This will be used to identify the voltage later */
				iFile.matA.bMatrixConductanceMap[rNameP][label + "-VN"] = (double)e.columnIndex;
				/* Intermediate node row and positive node column */
				e.columnIndex = columnMap[cNameP];
				e.rowIndex = rowMap[rNameNI1];
				e.value = -1 / value;
				iFile.matA.mElements.push_back(e);
				/* Add the column index of the positive node to the negative node row of
				 * the conductance map */
				 /* This will be used to identify the voltage later */
				iFile.matA.bMatrixConductanceMap[rNameNI1][label + "-VP"] = (double)e.columnIndex;
				/* Intermediate node row and column */
				e.columnIndex = columnMap[cNameNI1];
				e.rowIndex = rowMap[rNameNI1];
				e.value = 1 / value;
				iFile.matA.mElements.push_back(e);
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
			/* Voltage node 1 row and intermediate node 1 column */
			e.label = label;
			e.columnIndex = columnMap[cNameNI1];
			e.rowIndex = rowMap[rName1];
			e.value = 1;
			iFile.matA.mElements.push_back(e);
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
			/* Voltage node 2 row and intermediate node 2 column */
			e.label = label;
			e.columnIndex = columnMap[cNameNI2];
			e.rowIndex = rowMap[rName2];
			e.value = 1;
			iFile.matA.mElements.push_back(e);
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
				/* Voltage node 1 row and negative node 1 column */
				e.label = label;
				e.columnIndex = columnMap[cNameN];
				iFile.matA.xlines[label].nNode1 = columnMap[cNameN];
				e.rowIndex = rowMap[rName1];
				e.value = 1;
				iFile.matA.mElements.push_back(e);
			}
			if (!n2GND) {
				/* Negative node 2 row and voltage node 2 column */
				e.label = label;
				e.columnIndex = columnMap[cName2];
				e.rowIndex = rowMap[rNameN2];
				e.value = 1;
				iFile.matA.mElements.push_back(e);
				/* Voltage node 2 row and intermediate node 2 column */
				e.label = label;
				e.columnIndex = columnMap[cNameN2];
				iFile.matA.xlines[label].iNode2 = columnMap[cNameN2];
				e.rowIndex = rowMap[rName2];
				e.value = 1;
				iFile.matA.mElements.push_back(e);
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
		cf = cf * sqrt(iFile.matA.impedanceMap["R_" + ind1] * iFile.matA.impedanceMap["R_" + ind2]);
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
  Identify each non zero matrix element in phase form
*/
void
create_A_matrix_phase(InputFile& iFile)
{
	std::string cName, rName, cNameP, rNameP, cNameN, rNameN;
	std::vector<std::string> devicetokens, componentLabels;
	devicetokens.clear();
	componentLabels.clear();
	std::string label, nodeP, nodeN, subckt;
	std::unordered_map<std::string, int> rowMap, columnMap;
	rowMap.clear();
	columnMap.clear();
	int rowCounter, colCounter;
	bool pGND, nGND;
	rowCounter = 0;
	colCounter = 0;
	/* Main circuit node identification */
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
		/********************/
		/** PHASE RESISTOR **/
		/********************/
		if (i[0] == 'R') {
			std::string R = devicetokens[0];
			/* Create a new matrix element for the resistor */
			matrix_element e;
			/* Check if value exists, if not it's a bad resistor definition */
			try {
				if (iFile.parVal.find(devicetokens[3]) != iFile.parVal.end())
					iFile.pRes[R].R = iFile.parVal.at(devicetokens[3]);
				else if (iFile.subcircuitSegments[subckt].parVal.find(devicetokens[3]) != iFile.subcircuitSegments[subckt].parVal.end())
					iFile.pRes[R].R = iFile.subcircuitSegments[subckt].parVal.at(devicetokens[3]);
				else
					iFile.pRes[R].R = modifier(devicetokens[3]);
			}
			catch (const std::out_of_range&) {
				invalid_component_errors(RES_ERROR, i);
			}
			cName = "C_I" + devicetokens[0];
			rName = "R_" + devicetokens[0];
			iFile.pRes[R].bIResistorC = cName;
			iFile.pRes[R].bIResistorR = rName;
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
				cNameP = "C_NP" + nodeP;
				rNameP = "R_N" + nodeP;
				iFile.pRes[R].pPositiveC = cNameP;
				iFile.pRes[R].pPositiveR = rNameP;
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
				cNameN = "C_NP" + nodeN;
				rNameN = "R_N" + nodeN;
				iFile.pRes[R].pNegativeC = cNameN;
				iFile.pRes[R].pNegativeR = rNameN;
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
				/* Positive node row and resistor current column */
				e.label = label;
				e.columnIndex = columnMap[cName];
				e.rowIndex = rowMap[rNameP];
				iFile.pRes[R].pPR = e.rowIndex;
				e.value = 1;
				iFile.matA.mElements.push_back(e);
				/* Resistor current row and positive node column */
				e.label = label;
				e.columnIndex = columnMap[cNameP];
				iFile.pRes[R].pPC = e.columnIndex;
				e.rowIndex = rowMap[rName];
				e.value = 1;
				iFile.matA.mElements.push_back(e);
			}
			if (!nGND) {
				/* Negative node row and resistor current column */
				e.label = label;
				e.columnIndex = columnMap[cName];
				e.rowIndex = rowMap[rNameN];
				iFile.pRes[R].pNR = e.rowIndex;
				e.value = -1;
				iFile.matA.mElements.push_back(e);
				/* Resistor current row and negative node column*/
				e.label = label;
				e.columnIndex = columnMap[cNameN];
				iFile.pRes[R].pNC = e.columnIndex;
				e.rowIndex = rowMap[rName];
				e.value = -1;
				iFile.matA.mElements.push_back(e);
			}
			/* Resistor current row and resistor current column*/
			e.label = label;
			e.columnIndex = columnMap[cName];
			iFile.pRes[R].bIC = e.columnIndex;
			e.rowIndex = rowMap[rName];
			iFile.pRes[R].bIR = e.rowIndex;
			e.value = -(M_PI * iFile.pRes[R].R * iFile.tsim.prstep) / PHI_ZERO;
			iFile.pRes[R].Rmptr = iFile.matA.mElements.size();
			iFile.matA.mElements.push_back(e);
			/* End of add elements to matrix section */
			/* Element identification for use later when plotting values*/
			cElement.label = label;
			cElement.value = iFile.pRes[R].R;
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
		/*********************/
		/** PHASE CAPACITOR **/
		/*********************/
		else if (i[0] == 'C') {
			std::string C = devicetokens[0];
			/* Create a new matrix element for the capacitor */
			matrix_element e;
			/* Check if value exists, if not it's a bad capacitor definition */
			try {
				if (iFile.parVal.find(devicetokens[3]) != iFile.parVal.end())
					iFile.pCap[C].C = iFile.parVal.at(devicetokens[3]);
				else if (iFile.subcircuitSegments[subckt].parVal.find(devicetokens[3]) != iFile.subcircuitSegments[subckt].parVal.end())
					iFile.pCap[C].C = iFile.subcircuitSegments[subckt].parVal.at(devicetokens[3]);
				else
					iFile.pCap[C].C = modifier(devicetokens[3]);
			}
			catch (const std::out_of_range&) {
				invalid_component_errors(CAP_ERROR, i);
			}
			cName = "C_I" + devicetokens[0];
			rName = "R_" + devicetokens[0];
			iFile.pCap[C].bICapacitorC = cName;
			iFile.pCap[C].bICapacitorR = rName;
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
				cNameP = "C_NP" + nodeP;
				rNameP = "R_N" + nodeP;
				iFile.pCap[C].pPositiveC = cNameP;
				iFile.pCap[C].pPositiveR = rNameP;
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
				cNameN = "C_NP" + nodeN;
				rNameN = "R_N" + nodeN;
				iFile.pCap[C].pNegativeC = cNameN;
				iFile.pCap[C].pNegativeR = rNameN;
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
			/* If positive node is not grounded */
			if (!pGND) {
				/* Positive node row and capacitor current column */
				e.label = label;
				e.columnIndex = columnMap[cName];
				e.rowIndex = rowMap[rNameP];
				iFile.pCap[C].pPR = e.rowIndex;
				e.value = 1;
				iFile.matA.mElements.push_back(e);
				/* Capacitor current row and positive node column */
				e.label = label;
				e.columnIndex = columnMap[cNameP];
				iFile.pCap[C].pPC = e.columnIndex;
				e.rowIndex = rowMap[rName];
				e.value = 1;
				iFile.matA.mElements.push_back(e);
			}
			/* If negative node is not grounded */
			if (!nGND) {
				/* Negative node row and capacitor current column */
				e.label = label;
				e.columnIndex = columnMap[cName];
				e.rowIndex = rowMap[rNameN];
				iFile.pCap[C].pNR = e.rowIndex;
				e.value = -1;
				iFile.matA.mElements.push_back(e);
				/* Capacitor current row and negative node column*/
				e.label = label;
				e.columnIndex = columnMap[cNameN];
				iFile.pCap[C].pNC = e.columnIndex;
				e.rowIndex = rowMap[rName];
				e.value = -1;
				iFile.matA.mElements.push_back(e);
			}
			/* Capacitor current row and capacitor current column*/
			e.label = label;
			e.columnIndex = columnMap[cName];
			iFile.pCap[C].bIC = e.columnIndex;
			e.rowIndex = rowMap[rName];
			iFile.pCap[C].bIR = e.rowIndex;
			e.value = (-2 * M_PI * iFile.tsim.prstep * iFile.tsim.prstep) / (PHI_ZERO * 4 * value);
			iFile.pCap[C].Cmptr = iFile.matA.mElements.size();
			iFile.matA.mElements.push_back(e);
			/* End of add elements to matrix section */
			/* Element identification for use later when plotting values*/
			cElement.label = label;
			cElement.value = iFile.pCap[C].C;
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
		/** PHASE INDUCTOR **/
		/********************/
		else if (i[0] == 'L') {
			std::string L = devicetokens[0];
			/* Create a new matrix element for the inductor */
			matrix_element e;
			/* Check if value exists, if not it's a bad inductor definition */
			try {
				if (iFile.parVal.find(devicetokens[3]) != iFile.parVal.end())
					iFile.pInd[L].L = iFile.parVal.at(devicetokens[3]);
				else if (iFile.subcircuitSegments[subckt].parVal.find(devicetokens[3]) != iFile.subcircuitSegments[subckt].parVal.end())
					iFile.pInd[L].L = iFile.subcircuitSegments[subckt].parVal.at(devicetokens[3]);
				else
					iFile.pInd[L].L = modifier(devicetokens[3]);
			}
			catch (const std::out_of_range&) {
				invalid_component_errors(IND_ERROR, i);
			}
			cName = "C_I" + devicetokens[0];
			rName = "R_" + devicetokens[0];
			iFile.pInd[L].bIInductorC = cName;
			iFile.pInd[L].bIInductorR = rName;
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
				cNameP = "C_NP" + nodeP;
				rNameP = "R_N" + nodeP;
				iFile.pInd[L].pPositiveC = cNameP;
				iFile.pInd[L].pPositiveR = rNameP;
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
				cNameN = "C_NP" + nodeN;
				rNameN = "R_N" + nodeN;
				iFile.pInd[L].pNegativeC = cNameN;
				iFile.pInd[L].pNegativeR = rNameN;
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
				/* Positive node row and inductor current column */
				e.label = label;
				e.columnIndex = columnMap[cName];
				e.rowIndex = rowMap[rNameP];
				iFile.pInd[L].pPR = e.rowIndex;
				e.value = 1;
				iFile.matA.mElements.push_back(e);
				/* Inductor current row and positive node column */
				e.label = label;
				e.columnIndex = columnMap[cNameP];
				iFile.pInd[L].pPC = e.columnIndex;
				e.rowIndex = rowMap[rName];
				iFile.pInd[L].bIR = e.rowIndex;
				e.value = 1;
				iFile.matA.mElements.push_back(e);
			}
			if (!nGND) {
				/* Negative node row and inductor current column */
				e.label = label;
				e.columnIndex = columnMap[cName];
				e.rowIndex = rowMap[rNameN];
				iFile.pInd[L].pNR = e.rowIndex;
				e.value = -1;
				iFile.matA.mElements.push_back(e);
				/* Inductor current row and negative node column*/
				e.label = label;
				e.columnIndex = columnMap[cNameN];
				iFile.pInd[L].pNC = e.columnIndex;
				e.rowIndex = rowMap[rName];
				e.value = -1;
				iFile.matA.mElements.push_back(e);
			}
			/* Inductor node row and inductor current node column*/
			e.label = label;
			e.columnIndex = columnMap[cName];
			iFile.pInd[L].bIC = e.columnIndex;
			e.rowIndex = rowMap[rName];
			iFile.pInd[L].bIR = e.rowIndex;
			e.value = -(iFile.pInd[L].L * 2 * M_PI) / PHI_ZERO;
			iFile.pInd[L].Lmptr = iFile.matA.mElements.size();
			iFile.matA.mElements.push_back(e);
			/* End of add elements to matrix section */
			/* Element identification for use later when plotting values */
			cElement.label = label;
			cElement.value = iFile.pInd[L].L;
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
		/**************************/
		/** PHASE VOLTAGE SOURCE **/
		/**************************/
		else if (i[0] == 'V') {
			std::string VS = devicetokens[0];
			/* Create a new matrix element for the voltage source */
			matrix_element e;
			/* Parse the function identified (if any)*/
			iFile.matA.sources[label] = function_parse(i, iFile);
			cName = "C_" + devicetokens[0];
			rName = "R_" + devicetokens[0];
			iFile.pVS[VS].bIVoltC = cName;
			iFile.pVS[VS].bIVoltR = rName;
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
				cNameP = "C_NP" + nodeP;
				rNameP = "R_N" + nodeP;
				iFile.pVS[VS].pPositiveC = cNameP;
				iFile.pVS[VS].pPositiveR = rNameP;
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
				cNameN = "C_NP" + nodeN;
				rNameN = "R_N" + nodeN;
				iFile.pVS[VS].pNegativeC = cNameN;
				iFile.pVS[VS].pNegativeR = rNameN;
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
				/* Positive node row and voltage current column */
				e.label = label;
				e.columnIndex = columnMap[cName];
				iFile.pVS[VS].bIC = e.columnIndex;
				e.rowIndex = rowMap[rNameP];
				iFile.pVS[VS].pPR = e.rowIndex;
				e.value = 1;
				iFile.matA.mElements.push_back(e);
				/* Voltage current row and positive node column */
				e.label = label;
				e.columnIndex = columnMap[cNameP];
				iFile.pVS[VS].pPC = e.columnIndex;
				e.rowIndex = rowMap[rName];
				iFile.pVS[VS].bIR = e.rowIndex;
				e.value = 1;
				iFile.matA.mElements.push_back(e);
			}
			/* If negative node is not grounded */
			if (!nGND) {
				/* Negative node row and voltage current column */
				e.label = label;
				e.columnIndex = columnMap[cName];
				iFile.pVS[VS].bIC = e.columnIndex;
				e.rowIndex = rowMap[rNameN];
				iFile.pVS[VS].pNR = e.rowIndex;
				e.value = -1;
				iFile.matA.mElements.push_back(e);
				/* Voltage current row and negative node column*/
				e.label = label;
				e.columnIndex = columnMap[cNameN];
				iFile.pVS[VS].pNC = e.columnIndex;
				e.rowIndex = rowMap[rName];
				iFile.pVS[VS].bIR = e.rowIndex;
				e.value = -1;
				iFile.matA.mElements.push_back(e);
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
		/********************/
		/** CURRENT SOURCE **/
		/********************/
		else if (i[0] == 'I') {
			/* Parse the function identified (if any)*/
			iFile.matA.sources[label] = function_parse(i, iFile);
			/* Check if positive node is connected to ground */
			if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
				cNameP = "C_NP" + nodeP;
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
				cNameN = "C_NP" + nodeN;
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
		/******************************/
		/** PHASE JOSEPHSON JUNCTION **/
		/******************************/
		else if (i[0] == 'B') {
			std::string cVolt, rVolt, jj;
			jj = devicetokens[0];
			/* Create a new matrix element for the junction */
			matrix_element e;
			/* Identify the JJ parameters based on the model*/
			int jj_type = 0;
			std::unordered_map<std::string, double> jj_tokens;
			double jj_rtype = 0.0;
			jj_comp(devicetokens, iFile, iFile.pJJ[jj].Rtype,
					 iFile.pJJ[jj].C, iFile.pJJ[jj].RN, iFile.pJJ[jj].R0,
					 iFile.pJJ[jj].Ic, jj_rtype, iFile.pJJ[jj].Vg, iFile.pJJ[jj].IcFact, iFile.pJJ[jj].DelV);
			iFile.pJJ[jj].Rtype = jj_rtype;
			cVolt = "C_V" + devicetokens[0] + "-VJ";
			rVolt = "R_" + devicetokens[0] + "-VJ";
			iFile.pJJ[jj].vColumn = cVolt;
			iFile.pJJ[jj].vRow = rVolt;
			if (rowMap.count(rVolt) == 0) {
				rowMap[rVolt] = rowCounter;
				rowCounter++;
			}
			if (columnMap.count(cVolt) == 0) {
				columnMap[cVolt] = colCounter;
				colCounter++;
			}
			/* Check if positive node is connected to ground */
			if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
				cNameP = "C_NP" + nodeP;
				rNameP = "R_N" + nodeP;
				iFile.pJJ[jj].pPositiveC = cNameP;
				iFile.pJJ[jj].pPositiveR = rNameP;
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
				cNameN = "C_NP" + nodeN;
				rNameN = "R_N" + nodeN;
				iFile.pJJ[jj].pNegativeC = cNameN;
				iFile.pJJ[jj].pNegativeR = rNameN;
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
				/* Positive node row and Voltage column */
				e.label = label;
				e.columnIndex = columnMap[cVolt];
				e.rowIndex = rowMap[rNameP];
				iFile.pJJ[jj].pPR = e.rowIndex;;
				e.value = 1 / iFile.pJJ[jj].R0 + ((2*iFile.pJJ[jj].C) / iFile.tsim.prstep);
				iFile.pJJ[jj].RmptrP = iFile.matA.mElements.size();
				iFile.matA.mElements.push_back(e);
				e.junctionEntry = false;
				/* Volt row and positive node column */
				e.label = label;
				e.columnIndex = columnMap[cNameP];
				iFile.pJJ[jj].pPC = e.columnIndex;
				e.rowIndex = rowMap[rVolt];
				e.value = 1;
				iFile.matA.mElements.push_back(e);
				e.junctionEntry = false;
			}
			/* If negative node is not grounded */
			if (!nGND) {
				/* Negative node row and Voltage column */
				e.label = label;
				e.columnIndex = columnMap[cVolt];
				e.rowIndex = rowMap[rNameN];
				iFile.pJJ[jj].pNR = e.rowIndex;				
				e.value = -1 / iFile.pJJ[jj].R0 - ((2*iFile.pJJ[jj].C) / iFile.tsim.prstep);
				iFile.pJJ[jj].RmptrN = iFile.matA.mElements.size();
				iFile.matA.mElements.push_back(e);
				e.junctionEntry = false;
				/* Volt row and negative node column */
				e.label = label;
				e.columnIndex = columnMap[cNameN];
				iFile.pJJ[jj].pNC = e.columnIndex;
				e.rowIndex = rowMap[rVolt];	
				e.value = -1;
				iFile.matA.mElements.push_back(e);
				e.junctionEntry = false;
			}
			/* Voltage row and column */
			e.label = label;
			e.columnIndex = columnMap[cVolt];
			iFile.pJJ[jj].vC = e.columnIndex;
			e.rowIndex = rowMap[rVolt];
			iFile.pJJ[jj].vR = e.rowIndex;
			e.value = -(iFile.tsim.prstep / 2) * ((2 * M_PI) / PHI_ZERO);
			iFile.matA.mElements.push_back(e);
			e.junctionEntry = false;
			/* Junction parameters */
			iFile.pJJ[jj].gLarge = iFile.pJJ[jj].Ic / (iFile.pJJ[jj].IcFact * iFile.pJJ[jj].DelV);
			iFile.pJJ[jj].lower = iFile.pJJ[jj].Vg - 0.5*iFile.pJJ[jj].DelV;
			iFile.pJJ[jj].upper = iFile.pJJ[jj].Vg + 0.5 * iFile.pJJ[jj].DelV;
			iFile.pJJ[jj].subCond = 1 / iFile.pJJ[jj].R0 + ((2*iFile.pJJ[jj].C) / iFile.tsim.prstep);
			iFile.pJJ[jj].transCond = iFile.pJJ[jj].gLarge + ((2*iFile.pJJ[jj].C) / iFile.tsim.prstep);
			iFile.pJJ[jj].normalCond = 1 / iFile.pJJ[jj].RN + ((2*iFile.pJJ[jj].C) / iFile.tsim.prstep);
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
		/*****************************/
		/** PHASE TRANSMISSION LINE **/
		/*****************************/
		else if (i[0] == 'T') {
			std::string nodeP2, nodeN2, cNameP2, rNameP2, cNameN2,
				rNameN2, cName1, rName1, cName2,
				rName2, tl;
			bool p2GND, n2GND;
			/* Check if positive node 2 exists, if not it's a bad device line
			 * definition */
			tl = devicetokens[0];
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
			/* Create a new matrix element for the first Z0 */
			matrix_element e;
			if (devicetokens.size() < 7) {
				invalid_component_errors(TIME_ERROR, i);
			}
			/* Check if value exists, if not it's a bad Z0 definition */
			for (size_t l = 5; l < devicetokens.size(); l++) {
				if (devicetokens[l].find("TD") != std::string::npos)
					iFile.pTL[tl].TD = modifier((devicetokens[l]).substr(3));
				else if (devicetokens[l].find("Z0") != std::string::npos)
					iFile.pTL[tl].Z0 = modifier((devicetokens[l]).substr(3));
			}
			iFile.pTL[tl].k = iFile.pTL[tl].TD / iFile.tsim.prstep;
			/* Create a branch relation for the TL current I1 */
			cName1 = "C_I1" + label;
			rName1 = "R_" + label + "-I1";
			iFile.pTL[tl].bCurrentC1 = cName1;
			iFile.pTL[tl].bCurrentR1 = rName1;
			if (rowMap.count(rName1) == 0) {
				rowMap[rName1] = rowCounter;
				rowCounter++;
			}
			if (columnMap.count(cName1) == 0) {
				columnMap[cName1] = colCounter;
				colCounter++;
			}
			/* Create a branch relation for the TL current I2 */
			cName2 = "C_I2" + label;
			rName2 = "R_" + label + "-I2";
			iFile.pTL[tl].bCurrentC2 = cName2;
			iFile.pTL[tl].bCurrentR2 = rName2;
			if (rowMap.count(rName2) == 0) {
				rowMap[rName2] = rowCounter;
				rowCounter++;
			}
			if (columnMap.count(cName2) == 0) {
				columnMap[cName2] = colCounter;
				colCounter++;
			}
			/* Check if 1st positive node is connected to ground */
			if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
				cNameP = "C_NP" + nodeP;
				rNameP = "R_N" + nodeP;
				iFile.pTL[tl].pPositiveC1 = cNameP;
				iFile.pTL[tl].pPositiveR1 = rNameP;
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
				iFile.matA.nodeConnections[rNameP].push_back(label);
				pGND = false;
			}
			else
				pGND = true;
			/* Check if 1st negative node is connected to ground */
			if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
				cNameN = "C_NP" + nodeN;
				rNameN = "R_N" + nodeN;
				iFile.pTL[tl].pNegativeC1 = cNameN;
				iFile.pTL[tl].pNegativeR1 = rNameN;
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
				iFile.matA.nodeConnections[rNameN].push_back(label);
				nGND = false;
			}
			else
				nGND = true;
			/* Check if 2nd positive node is connected to ground */
			if (nodeP2 != "0" && nodeP2.find("GND") == std::string::npos) {
				cNameP2 = "C_NP" + nodeP2;
				rNameP2 = "R_N" + nodeP2;
				iFile.pTL[tl].pPositiveC2 = cNameP2;
				iFile.pTL[tl].pPositiveR2 = rNameP2;
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
				iFile.matA.nodeConnections[rNameP2].push_back(label);
				p2GND = false;
			}
			else
				p2GND = true;
			/* Check if 2nd negative node is connected to ground */
			if (nodeN2 != "0" && nodeN2.find("GND") == std::string::npos) {
				cNameN2 = "C_NP" + nodeN2;
				rNameN2 = "R_N" + nodeN2;
				iFile.pTL[tl].pNegativeC2 = cNameN2;
				iFile.pTL[tl].pNegativeR2 = rNameN2;
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
				iFile.matA.nodeConnections[rNameN2].push_back(label);
				n2GND = false;
			}
			else
				n2GND = true;
			/* Start of add elements to matrix section */
			/* If 1st positive node is not grounded */
			if (!pGND) {
				/* TL current 1 row and positive node column */
				e.columnIndex = columnMap[cNameP];
				iFile.pTL[tl].pPC1 = e.columnIndex;
				e.rowIndex = rowMap[rName1];
				e.value = 1;
				iFile.matA.mElements.push_back(e);
				/* Positive node row and TL current 1 column */
				e.columnIndex = columnMap[cName1];
				e.rowIndex = rowMap[rNameP];
				iFile.pTL[tl].pPR1 = e.rowIndex;
				e.value = 1;
				iFile.matA.mElements.push_back(e);
				if(!nGND) {
					/* TL current 1 row and negative node column */
					e.columnIndex = columnMap[cNameN];
					iFile.pTL[tl].pNC1 = e.columnIndex;
					e.rowIndex = rowMap[rName1];
					e.value = -1;
					iFile.matA.mElements.push_back(e);
					/* Negative node row and TL current 1 column */
					e.columnIndex = columnMap[cName1];
					e.rowIndex = rowMap[rNameN];
					iFile.pTL[tl].pNR1 = e.rowIndex;
					e.value = -1;
					iFile.matA.mElements.push_back(e);
				}
			}
			/* If second positive node is not grounded */
			if (!p2GND) {
				/* TL current 2 row and positive node column */
				e.columnIndex = columnMap[cNameP2];
				iFile.pTL[tl].pPC2 = e.columnIndex;
				e.rowIndex = rowMap[rName2];
				e.value = 1;
				iFile.matA.mElements.push_back(e);
				/* Positive node row and TL current 2 column */
				e.columnIndex = columnMap[cName2];
				e.rowIndex = rowMap[rNameP2];
				iFile.pTL[tl].pPR2 = e.rowIndex;
				e.value = 1;
				iFile.matA.mElements.push_back(e);
				if(!n2GND) {
					/* TL current 2 row and negative node column */
					e.columnIndex = columnMap[cNameN2];
					iFile.pTL[tl].pNC2 = e.columnIndex;
					e.rowIndex = rowMap[rName2];
					e.value = -1;
					iFile.matA.mElements.push_back(e);
					/* Negative node row and TL current 2 column */
					e.columnIndex = columnMap[cName2];
					e.rowIndex = rowMap[rNameN2];
					iFile.pTL[tl].pNR2 = e.rowIndex;
					e.value = -1;
					iFile.matA.mElements.push_back(e);
				}
			}
			/* TL current 1 row and column */
			e.label = label;
			e.columnIndex = columnMap[cName1];
			iFile.pTL[tl].bIC1 = e.columnIndex;
			e.rowIndex = rowMap[rName1];
			iFile.pTL[tl].bIR1 = e.rowIndex;
			e.value = -(M_PI * iFile.tsim.prstep * iFile.pTL[tl].Z0) / (PHI_ZERO);
			iFile.matA.mElements.push_back(e);
			/* TL current 2 row and column */
			e.label = label;
			e.columnIndex = columnMap[cName2];
			iFile.pTL[tl].bIC2 = e.columnIndex;
			e.rowIndex = rowMap[rName2];
			iFile.pTL[tl].bIR2 = e.rowIndex;			
			e.value = -(M_PI * iFile.tsim.prstep * iFile.pTL[tl].Z0) / (PHI_ZERO);
			iFile.matA.mElements.push_back(e);
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
		mutualL = cf * sqrt(iFile.pInd[ind1].L * iFile.pInd[ind2].L);
		// Inductor1
		matrix_element e;
		e.label = label;
		e.columnIndex = iFile.pInd[ind2].bIC;
		e.rowIndex = iFile.pInd[ind1].bIR;
		e.value = -(mutualL * 2 * M_PI) / PHI_ZERO;
		iFile.pInd[ind1].Mmptr = iFile.matA.mElements.size();
		iFile.matA.mElements.push_back(e);
		// Inductor2
		e.label = label;
		e.columnIndex = iFile.pInd[ind1].bIC;
		e.rowIndex = iFile.pInd[ind2].bIR;
		e.value = -(mutualL * 2 * M_PI) / PHI_ZERO;
		iFile.pInd[ind2].Mmptr = iFile.matA.mElements.size();
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