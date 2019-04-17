// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#include "JoSIM/j_matrix.h"

using namespace JoSIM;

void
Matrix::create_matrix(Input &iObj) {
	if(iObj.argAnal == AnalysisType::Voltage) Matrix::create_A_volt(iObj);
	else if (iObj.argAnal == AnalysisType::Phase) Matrix::create_A_phase(iObj);
	Matrix::create_CSR();
}

void
Matrix::create_A_volt(Input &iObj) {
	std::string cName, rName, cNameP, rNameP, cNameN, rNameN;
	std::vector<std::string> devicetokens, componentLabels;
	devicetokens.clear();
	componentLabels.clear();
	std::string label, nodeP, nodeN, subckt = "";
	std::unordered_map<std::string, int> rowMap, columnMap;
	rowMap.clear();
	columnMap.clear();
	int rowCounter, colCounter, expStart, expEnd;
	bool pGND, nGND;
	rowCounter = colCounter = 0;
	for (auto i : iObj.expNetlist) {
		expStart = expEnd = -1;
		devicetokens = Misc::tokenize_space(i.first);
		for (int t = 0; t < devicetokens.size(); t++) {
			if(devicetokens[t].find('{') != std::string::npos) {
				expStart = t;
			}
			if(devicetokens[t].find('}') != std::string::npos) {
				expEnd = t;
			}
		}
		if(expStart == -1 && expEnd != -1) Errors::invalid_component_errors(INVALID_EXPR, i.first);
		else if(expStart != -1 && expEnd == -1) Errors::invalid_component_errors(INVALID_EXPR, i.first);
		if (expStart != -1 && expStart == expEnd) {
			devicetokens[expStart] = devicetokens[expStart].substr(devicetokens[expStart].find('{') + 1, devicetokens[expStart].size() - 1);
			devicetokens[expStart] = devicetokens[expStart].substr(0, devicetokens[expStart].find('}'));
			devicetokens[expStart] = Misc::precise_to_string(Parser::parse_param(devicetokens[expStart], iObj.parameters.parsedParams, i.second), 25);
		}
		else if (expStart != -1 && expEnd != -1) {
			int d = expStart + 1;
			while (expStart != expEnd) {
				devicetokens[expStart] += devicetokens[d];
				devicetokens.erase(devicetokens.begin()+d);
				expEnd--;
			}
			devicetokens[expStart] = devicetokens[expStart].substr(devicetokens[expStart].find('{') + 1, devicetokens[expStart].size() - 1);
			devicetokens[expStart] = devicetokens[expStart].substr(0, devicetokens[expStart].find('}'));
			devicetokens[expStart] = Misc::precise_to_string(Parser::parse_param(devicetokens[expStart], iObj.parameters.parsedParams, i.second), 25);
		}
		double value = 0.0;
		try {
			label = devicetokens[0];
			if (std::find(componentLabels.begin(), componentLabels.end(), label) ==
				componentLabels.end()) {
					if(label.find_first_of("_*!@#$\\/%^&*()") != std::string::npos) {
						std::cerr << "W: The use of special characters in label names is not advised." << std::endl;
						std::cerr << "W: This might produce unexpected results." << std::endl;
						std::cerr << "W: Continuing operation." << std::endl;
					}
					componentLabels.push_back(label);
				}
			else {
				Errors::invalid_component_errors(DUPLICATE_LABEL, label);
			}
		}
		catch (std::exception &e) {
			Errors::invalid_component_errors(MISSING_LABEL, i.first);
		}
		try {
			nodeP = devicetokens[1];
		}
		catch (std::exception &e) {
			Errors::invalid_component_errors(MISSING_PNODE, i.first);
		}
		try {
			nodeN = devicetokens[2];
		}
		catch (std::exception &e) {
			Errors::invalid_component_errors(MISSING_NNODE, i.first);
		}
		/**************/
		/** RESISTOR **/
		/**************/
		if (i.first[0] == 'R') {
			const std::string& R = label;
			matrix_element e;
			try {
        auto parameter_name = ParameterName(devicetokens[3], i.second);
				if (iObj.parameters.parsedParams.count(parameter_name) != 0)
					value = iObj.parameters.parsedParams.at(parameter_name);
				else
					value = Misc::modifier(devicetokens[3]);
			}
			catch (std::exception &e) {
				Errors::invalid_component_errors(RES_ERROR, i.first);
			}
			components.voltRes[R].label = R;
			components.voltRes[R].value = value;
			if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
				cNameP = "C_NV" + nodeP;
				rNameP = "R_N" + nodeP;
				components.voltRes[R].posNodeR = rNameP;
				components.voltRes[R].posNodeC = cNameP;
				if (rowMap.count(rNameP) == 0) {
					rowMap[rNameP] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameP) == 0) {
					columnMap[cNameP] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameP].push_back(label);
				pGND = false;
			}
			else
				pGND = true;
			if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
				cNameN = "C_NV" + nodeN;
				rNameN = "R_N" + nodeN;
				components.voltRes[R].negNodeR = rNameN;
				components.voltRes[R].negNodeC = cNameN;
				if (rowMap.count(rNameN) == 0) {
					rowMap[rNameN] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameN) == 0) {
					columnMap[cNameN] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameN].push_back(label);
				nGND = false;
			}
			else
				nGND = true;
			if (!pGND) {
				e.label = label;
				e.colIndex = columnMap.at(cNameP);
				e.rowIndex = rowMap.at(rNameP);
				components.voltRes[R].posNRow = rowMap.at(rNameP);
				components.voltRes[R].posNCol = columnMap.at(cNameP);
				e.value = 1 / value;
				components.voltRes[R].ppPtr = mElements.size();
				mElements.push_back(e);
				if (!nGND) {
					e.label = label;
					e.colIndex = columnMap.at(cNameN);
					e.rowIndex = rowMap.at(rNameP);
					components.voltRes[R].posNRow = rowMap.at(rNameP);
					components.voltRes[R].negNCol = columnMap.at(cNameN);
					e.value = -1 / value;
					components.voltRes[R].pnPtr = mElements.size();
					mElements.push_back(e);
					e.label = label;
					e.colIndex = columnMap.at(cNameP);
					e.rowIndex = rowMap.at(rNameN);
					components.voltRes[R].negNRow = rowMap.at(rNameN);
					components.voltRes[R].posNCol = columnMap.at(cNameP);
					e.value = -1 / value;
					components.voltRes[R].npPtr = mElements.size();
					mElements.push_back(e);
					e.label = label;
					e.colIndex = columnMap.at(cNameN);
					e.rowIndex = rowMap.at(rNameN);
					components.voltRes[R].negNRow = rowMap.at(rNameN);
					components.voltRes[R].negNCol = columnMap.at(cNameN);
					e.value = 1 / value;
					components.voltRes[R].nnPtr = mElements.size();
					mElements.push_back(e);
				}
			}
			else if (!nGND) {
				e.label = label;
				e.colIndex = columnMap.at(cNameN);
				e.rowIndex = rowMap.at(rNameN);
				components.voltRes[R].negNRow = rowMap.at(rNameN);
				components.voltRes[R].negNCol = columnMap.at(cNameN);
				e.value = 1 / value;
				components.voltRes[R].nnPtr = mElements.size();
				mElements.push_back(e);
			}
		}
		/***************/
		/** CAPACITOR **/
		/***************/
		else if (i.first[0] == 'C') {
			const std::string& C = label;
			matrix_element e;
			try {
        auto parameter_name = ParameterName(devicetokens[3], i.second);
				if (iObj.parameters.parsedParams.count(parameter_name) != 0)
					value = iObj.parameters.parsedParams.at(parameter_name);
				else
					value = Misc::modifier(devicetokens[3]);
			}
			catch (std::exception &e) {
				Errors::invalid_component_errors(CAP_ERROR, i.first);
			}
			components.voltCap[C].label = devicetokens[0];
			components.voltCap[C].value = value;
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
			components.voltCap[C].curNodeC = cName;
			components.voltCap[C].curNodeR = rName;
			if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
				cNameP = "C_NV" + nodeP;
				rNameP = "R_N" + nodeP;
				components.voltCap[C].posNodeC = cNameP;
				components.voltCap[C].posNodeR = rNameP;
				if (rowMap.count(rNameP) == 0) {
					rowMap[rNameP] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameP) == 0) {
					columnMap[cNameP] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameP].push_back(label);
				pGND = false;
			}
			else
				pGND = true;
			if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
				cNameN = "C_NV" + nodeN;
				rNameN = "R_N" + nodeN;
				components.voltCap[C].negNodeC = cNameN;
				components.voltCap[C].negNodeR = rNameN;
				if (rowMap.count(rNameN) == 0) {
					rowMap[rNameN] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameN) == 0) {
					columnMap[cNameN] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameN].push_back(label);
				nGND = false;
			}
			else
				nGND = true;
			if (!pGND) {
				e.label = label;
				e.colIndex = columnMap.at(cNameP);
				e.rowIndex = rowMap.at(rName);
				components.voltCap[C].posNCol = e.colIndex;
				components.voltCap[C].curNRow = e.rowIndex;
				e.value = 1;
				mElements.push_back(e);
				e.label = label;
				e.colIndex = columnMap.at(cName);
				e.rowIndex = rowMap.at(rNameP);
				components.voltCap[C].curNCol = e.colIndex;
				components.voltCap[C].posNRow = e.rowIndex;
				e.value = 1;
				mElements.push_back(e);
				if (!nGND) {
					e.label = label;
					e.colIndex = columnMap.at(cNameN);
					e.rowIndex = rowMap.at(rName);
					components.voltCap[C].negNCol = e.colIndex;
					components.voltCap[C].curNRow = e.rowIndex;
					e.value = -1;
					mElements.push_back(e);
					e.label = label;
					e.colIndex = columnMap.at(cName);
					e.rowIndex = rowMap.at(rNameN);
					components.voltCap[C].curNCol = e.colIndex;
					components.voltCap[C].negNRow = e.rowIndex;
					e.value = -1;
					mElements.push_back(e);
				}
			}
			else if (!nGND) {
				e.label = label;
				e.colIndex = columnMap.at(cNameN);
				e.rowIndex = rowMap.at(rName);
				components.voltCap[C].negNCol = e.colIndex;
				components.voltCap[C].curNRow = e.rowIndex;
				e.value = -1;
				mElements.push_back(e);
				e.label = label;
				e.colIndex = columnMap.at(cName);
				e.rowIndex = rowMap.at(rNameN);
				components.voltCap[C].curNCol = e.colIndex;
				components.voltCap[C].negNRow = e.rowIndex;
				e.value = -1;
				mElements.push_back(e);
			}
			e.label = label;
			e.colIndex = columnMap.at(cName);
			e.rowIndex = rowMap.at(rName);
			components.voltCap[C].curNCol = e.colIndex;
			components.voltCap[C].curNRow = e.rowIndex;
			e.value = iObj.transSim.prstep / (2 * value);
			components.voltCap[C].capPtr = mElements.size();
			mElements.push_back(e);
		}
		/**************/
		/** INDUCTOR **/
		/**************/
		else if (i.first[0] == 'L') {
			const std::string& L = label;
			matrix_element e;
			try {
        auto parameter_name = ParameterName(devicetokens[3], i.second);
				if (iObj.parameters.parsedParams.count(parameter_name) != 0)
					value = iObj.parameters.parsedParams.at(parameter_name);
				else
					value = Misc::modifier(devicetokens[3]);
			}
			catch (std::exception &e) {
				Errors::invalid_component_errors(IND_ERROR, i.first);
			}
			components.voltInd[L].label = label;
			components.voltInd[L].value = value;
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
			components.voltInd[L].curNodeC = cName;
			components.voltInd[L].curNodeR = rName;
			if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
				cNameP = "C_NV" + nodeP;
				rNameP = "R_N" + nodeP;
				components.voltInd[L].posNodeC = cNameP;
				components.voltInd[L].posNodeR = rNameP;
				if (rowMap.count(rNameP) == 0) {
					rowMap[rNameP] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameP) == 0) {
					columnMap[cNameP] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameP].push_back(label);
				pGND = false;
			}
			else
				pGND = true;
			if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
				cNameN = "C_NV" + nodeN;
				rNameN = "R_N" + nodeN;
				components.voltInd[L].negNodeC = cNameN;
				components.voltInd[L].negNodeR = rNameN;
				if (rowMap.count(rNameN) == 0) {
					rowMap[rNameN] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameN) == 0) {
					columnMap[cNameN] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameN].push_back(label);
				nGND = false;
			}
			else
				nGND = true;
			if (!pGND) {
				e.label = label;
				e.colIndex = columnMap.at(cName);
				e.rowIndex = rowMap.at(rNameP);
				components.voltInd[L].curNCol = e.colIndex;
				components.voltInd[L].posNRow = e.rowIndex;
				e.value = 1;
				mElements.push_back(e);
				e.label = label;
				e.colIndex = columnMap.at(cNameP);
				e.rowIndex = rowMap.at(rName);
				components.voltInd[L].posNCol = e.colIndex;
				components.voltInd[L].curNRow = e.rowIndex;
				e.value = 1;
				mElements.push_back(e);
				if (!nGND) {
					e.label = label;
					e.colIndex = columnMap.at(cName);
					e.rowIndex = rowMap.at(rNameN);
					components.voltInd[L].curNCol = e.colIndex;
					components.voltInd[L].negNRow = e.rowIndex;
					e.value = -1;
					mElements.push_back(e);
					e.label = label;
					e.colIndex = columnMap.at(cNameN);
					e.rowIndex = rowMap.at(rName);
					components.voltInd[L].negNCol = e.colIndex;
					components.voltInd[L].curNRow = e.rowIndex;
					e.value = -1;
					mElements.push_back(e);
				}
			}
			else if (!nGND) {
				e.label = label;
				e.colIndex = columnMap.at(cName);
				e.rowIndex = rowMap.at(rNameN);
				components.voltInd[L].curNCol = e.colIndex;
				components.voltInd[L].negNRow = e.rowIndex;
				e.value = -1;
				mElements.push_back(e);
				e.label = label;
				e.colIndex = columnMap.at(cNameN);
				e.rowIndex = rowMap.at(rName);
				components.voltInd[L].negNCol = e.colIndex;
				components.voltInd[L].curNRow = e.rowIndex;
				e.value = -1;
				mElements.push_back(e);
			}
			e.label = label;
			e.colIndex = columnMap.at(cName);
			e.rowIndex = rowMap.at(rName);
			components.voltInd[L].curNCol = e.colIndex;
			components.voltInd[L].curNRow = e.rowIndex;
			e.value = (-2 * value) / iObj.transSim.prstep;
			components.voltInd[L].indPtr = mElements.size();
			mElements.push_back(e);
		}
		/********************/
		/** VOLTAGE SOURCE **/
		/********************/
		else if (i.first[0] == 'V') {
			const std::string& V = label;
			matrix_element e;
			sources[label] = Misc::parse_function(i.first, iObj, i.second);
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
			components.voltVs[V].curNodeC = cName;
			components.voltVs[V].curNodeR = rName;
			if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
				cNameP = "C_NV" + nodeP;
				rNameP = "R_N" + nodeP;
				components.voltVs[V].posNodeC = cNameP;
				components.voltVs[V].posNodeR = rNameP;
				if (rowMap.count(rNameP) == 0) {
					rowMap[rNameP] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameP) == 0) {
					columnMap[cNameP] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameP].push_back(label);
				pGND = false;
			}
			else
				pGND = true;
			if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
				cNameN = "C_NV" + nodeN;
				rNameN = "R_N" + nodeN;
				components.voltVs[V].negNodeC = cNameN;
				components.voltVs[V].negNodeR = rNameN;
				if (rowMap.count(rNameN) == 0) {
					rowMap[rNameN] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameN) == 0) {
					columnMap[cNameN] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameN].push_back(label);
				nGND = false;
			}
			else
				nGND = true;
			if (!pGND) {
				e.label = label;
				e.colIndex = columnMap.at(cName);
				e.rowIndex = rowMap.at(rNameP);
				components.voltVs[V].curNCol = e.colIndex;
				components.voltVs[V].posNRow = e.rowIndex;
				e.value = 1;
				mElements.push_back(e);
				e.label = label;
				e.colIndex = columnMap.at(cNameP);
				e.rowIndex = rowMap.at(rName);
				components.voltVs[V].posNCol = e.colIndex;
				components.voltVs[V].curNRow = e.rowIndex;
				e.value = 1;
				mElements.push_back(e);
				if (!nGND) {
					e.label = label;
					e.colIndex = columnMap.at(cName);
					e.rowIndex = rowMap.at(rNameN);
					components.voltVs[V].curNCol = e.colIndex;
					components.voltVs[V].negNRow = e.rowIndex;
					e.value = -1;
					mElements.push_back(e);
					e.label = label;
					e.colIndex = columnMap.at(cNameN);
					e.rowIndex = rowMap.at(rName);
					components.voltVs[V].negNCol = e.colIndex;
					components.voltVs[V].curNRow = e.rowIndex;
					e.value = -1;
					mElements.push_back(e);
				}
			}
			else if (!nGND) {
				e.label = label;
				e.colIndex = columnMap.at(cName);
				e.rowIndex = rowMap.at(rNameN);
				components.voltVs[V].curNCol = e.colIndex;
				components.voltVs[V].negNRow = e.rowIndex;
				e.value = -1;
				mElements.push_back(e);
				e.label = label;
				e.colIndex = columnMap.at(cNameN);
				e.rowIndex = rowMap.at(rName);
				components.voltVs[V].negNCol = e.colIndex;
				components.voltVs[V].curNRow = e.rowIndex;
				e.value = -1;
				mElements.push_back(e);
			}
		}
		/********************/
		/** CURRENT SOURCE **/
		/********************/
		else if (i.first[0] == 'I') {
			sources[label] = Misc::parse_function(i.first, iObj, i.second);
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
				nodeConnections[rNameP].push_back(label);
				pGND = false;
			}
			else
				pGND = true;
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
				nodeConnections[rNameN].push_back(label);
				nGND = false;
			}
			else
				nGND = true;
		}
		/************************/
		/** JOSEPHSON JUNCTION **/
		/************************/
		else if (i.first[0] == 'B') {
			std::string jj = label;
			matrix_element e;
			std::string modelstring = "", area = "";
			for (int t = devicetokens.size() - 1; t > 2; t--) {
				if (devicetokens[t].find('=') == std::string::npos) {
					if (iObj.netlist.models.count(std::make_pair(devicetokens[t], i.second)) != 0) {
						modelstring = iObj.netlist.models.at(std::make_pair(devicetokens[t], i.second));
						break;
					}
					else if (iObj.netlist.models.count(std::make_pair(devicetokens[t], "")) != 0) {
						modelstring = iObj.netlist.models.at(std::make_pair(devicetokens[t], ""));
						break;
					}
					else { Errors::invalid_component_errors(MODEL_NOT_DEFINED,
														  devicetokens[t]);
							break;
					}
				}
				if (devicetokens[t].find("AREA") != std::string::npos) {
					area = devicetokens[t].substr(
						devicetokens[t].find_first_of('=') + 1,
						devicetokens[t].size() - 1);
				}
			}
			if(area == "" && iObj.argVerb) Errors::invalid_component_errors(MODEL_AREA_NOT_GIVEN, label);
			components.voltJJ[jj].label = jj;
			components.jj_model(modelstring, area, jj, iObj, i.second);
			components.voltJJ[jj].label = label;
			cName = "C_P" + devicetokens[0];
			rName = "R_" + devicetokens[0];
			components.voltJJ[jj].phaseNodeC = cName;
			components.voltJJ[jj].phaseNodeR = rName;
			if (rowMap.count(rName) == 0) {
				rowMap[rName] = rowCounter;
				rowCounter++;
			}
			if (columnMap.count(cName) == 0) {
				columnMap[cName] = colCounter;
				colCounter++;
			}
			if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
				cNameP = "C_NV" + nodeP;
				rNameP = "R_N" + nodeP;
				components.voltJJ[jj].posNodeC = cNameP;
				components.voltJJ[jj].posNodeR = rNameP;
				if (rowMap.count(rNameP) == 0) {
					rowMap[rNameP] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameP) == 0) {
					columnMap[cNameP] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameP].push_back(label);
				pGND = false;
			}
			else
				pGND = true;
			if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
				cNameN = "C_NV" + nodeN;
				rNameN = "R_N" + nodeN;
				components.voltJJ[jj].negNodeC = cNameN;
				components.voltJJ[jj].negNodeR = rNameN;
				if (rowMap.count(rNameN) == 0) {
					rowMap[rNameN] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameN) == 0) {
					columnMap[cNameN] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameN].push_back(label);
				nGND = false;
			}
			else
				nGND = true;
			if (!pGND) {
				e.label = label;
				e.colIndex = columnMap.at(cNameP);
				e.rowIndex = rowMap.at(rNameP);
				components.voltJJ[jj].posNCol = e.colIndex;
				components.voltJJ[jj].posNRow = e.rowIndex;
				e.value = ((2 * components.voltJJ.at(jj).C) / iObj.transSim.prstep) + (1 / components.voltJJ.at(jj).r0);
				components.voltJJ[jj].ppPtr = mElements.size();
				mElements.push_back(e);
				e.label = label;
				e.colIndex = columnMap.at(cNameP);
				e.rowIndex = rowMap.at(rName);
				components.voltJJ[jj].posNCol = e.colIndex;
				components.voltJJ[jj].phaseNRow = e.rowIndex;
				e.value = (-iObj.transSim.prstep / 2) * ((2 * M_PI) / PHI_ZERO);
				mElements.push_back(e);
				if (!nGND) {
					e.label = label;
					e.colIndex = columnMap.at(cNameN);
					e.rowIndex = rowMap.at(rNameP);
					components.voltJJ[jj].negNCol = e.colIndex;
					components.voltJJ[jj].posNRow = e.rowIndex;
					e.value = -(((2 * components.voltJJ.at(jj).C) / iObj.transSim.prstep) + (1 / components.voltJJ.at(jj).r0));
					components.voltJJ[jj].npPtr = mElements.size();
					mElements.push_back(e);
					e.label = label;
					e.colIndex = columnMap.at(cNameP);
					e.rowIndex = rowMap.at(rNameN);
					components.voltJJ[jj].posNCol = e.colIndex;
					components.voltJJ[jj].negNRow = e.rowIndex;
					e.value = -(((2 * components.voltJJ.at(jj).C) / iObj.transSim.prstep) + (1 / components.voltJJ.at(jj).r0));
					components.voltJJ[jj].pnPtr = mElements.size();
					mElements.push_back(e);
					e.label = label;
					e.colIndex = columnMap.at(cNameN);
					e.rowIndex = rowMap.at(rNameN);
					components.voltJJ[jj].negNCol = e.colIndex;
					components.voltJJ[jj].negNRow = e.rowIndex;
					e.value = ((2 * components.voltJJ.at(jj).C) / iObj.transSim.prstep) + (1 / components.voltJJ.at(jj).r0);
					components.voltJJ[jj].nnPtr = mElements.size();
					mElements.push_back(e);
					e.label = label;
					e.colIndex = columnMap.at(cNameN);
					e.rowIndex = rowMap.at(rName);
					components.voltJJ[jj].negNCol = e.colIndex;
					components.voltJJ[jj].phaseNRow = e.rowIndex;
					e.value = (iObj.transSim.prstep / 2) * ((2 * M_PI) / PHI_ZERO);
					mElements.push_back(e);
				}
			}
			else if (!nGND) {
				e.label = label;
				e.colIndex = columnMap.at(cNameN);
				e.rowIndex = rowMap.at(rNameN);
				components.voltJJ[jj].negNCol = e.colIndex;
				components.voltJJ[jj].negNRow = e.rowIndex;
				e.value = ((2 * components.voltJJ.at(jj).C) / iObj.transSim.prstep) + (1 / components.voltJJ.at(jj).r0);
				components.voltJJ[jj].nnPtr = mElements.size();
				mElements.push_back(e);
				e.label = label;
				e.colIndex = columnMap.at(cNameN);
				e.rowIndex = rowMap.at(rName);
				components.voltJJ[jj].negNCol = e.colIndex;
				components.voltJJ[jj].phaseNRow = e.rowIndex;
				e.value = (iObj.transSim.prstep / 2) * ((2 * M_PI) / PHI_ZERO);
				mElements.push_back(e);
			}
			e.label = label;
			e.colIndex = columnMap.at(cName);
			e.rowIndex = rowMap.at(rName);
			components.voltJJ[jj].phaseNCol = e.colIndex;
			components.voltJJ[jj].phaseNRow = e.rowIndex;
			e.value = 1;
			mElements.push_back(e);
			components.voltJJ[jj].gLarge = components.voltJJ.at(jj).iC / (components.voltJJ.at(jj).iCFact * components.voltJJ.at(jj).delV);
			components.voltJJ[jj].lowerB = components.voltJJ.at(jj).vG - 0.5*components.voltJJ.at(jj).delV;
			components.voltJJ[jj].upperB = components.voltJJ.at(jj).vG + 0.5 * components.voltJJ.at(jj).delV;
			components.voltJJ[jj].subCond = 1 / components.voltJJ.at(jj).r0 + ((2*components.voltJJ.at(jj).C) / iObj.transSim.prstep);
			components.voltJJ[jj].transCond = components.voltJJ.at(jj).gLarge + ((2*components.voltJJ.at(jj).C) / iObj.transSim.prstep);
			components.voltJJ[jj].normalCond = 1 / components.voltJJ.at(jj).rN + ((2*components.voltJJ.at(jj).C) / iObj.transSim.prstep);
			components.voltJJ[jj].Del0 = 1.76 * BOLTZMANN * components.voltJJ.at(jj).tC;
			components.voltJJ[jj].Del = components.voltJJ.at(jj).Del0 * sqrt(cos((M_PI/2) *
				(components.voltJJ.at(jj).T/components.voltJJ.at(jj).tC) * (components.voltJJ.at(jj).T/components.voltJJ.at(jj).tC)));
			components.voltJJ[jj].rNCalc = ((M_PI * components.voltJJ.at(jj).Del) / (2 * EV * components.voltJJ.at(jj).iC)) *
				tanh(components.voltJJ.at(jj).Del / (2 * BOLTZMANN * components.voltJJ.at(jj).T));
			components.voltJJ[jj].iS = -components.voltJJ.at(jj).iC * sin(components.voltJJ.at(jj).phi0);
		}
		/***********************/
		/** TRANSMISSION LINE **/
		/***********************/
		else if (i.first[0] == 'T') {
			std::string label, cName2, rName2, cNameP2, rNameP2, cNameN2,
				rNameN2, nodeP2, nodeN2;
      const std::string &Tx = label;
			bool pGND2, nGND2;
			matrix_element e;
			double z0 = 10, tD = 0.0;
			for (int t = 5; t < devicetokens.size(); t++) {
				if(devicetokens[t].find("TD") != std::string::npos)
					tD = Parser::parse_param(
						devicetokens[t].substr(devicetokens[t].find("TD=") + 3,
						devicetokens[t].size() - 1), iObj.parameters.parsedParams, i.second);
				else if(devicetokens[t].find("Z0") != std::string::npos)
					z0 = Parser::parse_param(
						devicetokens[t].substr(devicetokens[t].find("Z0=") + 3,
						devicetokens[t].size() - 1), iObj.parameters.parsedParams, i.second);
				else if(devicetokens[t].find("LOSSLESS") != std::string::npos) {}
				else Errors::invalid_component_errors(INVALID_TX_DEFINED, i.first);
			}
			components.txLine[Tx].label = label;
			components.txLine[Tx].k = tD / iObj.transSim.prstep;
			components.txLine[Tx].value = z0;
			cName = "C_I1" + devicetokens[0];
			rName = "R_" + devicetokens[0] + "-I1";
			cName2 = "C_I2" + devicetokens[0];
			rName2 = "R_" + devicetokens[0] + "-I2";
			if (rowMap.count(rName) == 0) {
				rowMap[rName] = rowCounter;
				rowCounter++;
			}
			if (columnMap.count(cName) == 0) {
				columnMap[cName] = colCounter;
				colCounter++;
			}
			if (rowMap.count(rName2) == 0) {
				rowMap[rName2] = rowCounter;
				rowCounter++;
			}
			if (columnMap.count(cName2) == 0) {
				columnMap[cName2] = colCounter;
				colCounter++;
			}
			components.txLine[Tx].curNode1C = cName;
			components.txLine[Tx].curNode1R = rName;
			components.txLine[Tx].curNode2C = cName2;
			components.txLine[Tx].curNode2R = rName2;
			if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
				cNameP = "C_NV" + nodeP;
				rNameP = "R_N" + nodeP;
				components.txLine[Tx].posNodeC = cNameP;
				components.txLine[Tx].posNodeR = rNameP;
				if (rowMap.count(rNameP) == 0) {
					rowMap[rNameP] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameP) == 0) {
					columnMap[cNameP] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameP].push_back(label);
				pGND = false;
			}
			else
				pGND = true;
			if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
				cNameN = "C_NV" + nodeN;
				rNameN = "R_N" + nodeN;
				components.txLine[Tx].negNodeC = cNameN;
				components.txLine[Tx].negNodeR = rNameN;
				if (rowMap.count(rNameN) == 0) {
					rowMap[rNameN] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameN) == 0) {
					columnMap[cNameN] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameN].push_back(label);
				nGND = false;
			}
			else
				nGND = true;
			try {
				nodeP2 = devicetokens[3];
			}
			catch (std::exception &e) {
				Errors::invalid_component_errors(MISSING_PNODE, i.first);
			}
			/* Check if negative node exists, if not it's a bad device line definition
			*/
			try {
				nodeN2 = devicetokens[4];
			}
			catch (std::exception &e) {
				Errors::invalid_component_errors(MISSING_NNODE, i.first);
			}
			if (nodeP2 != "0" && nodeP2.find("GND") == std::string::npos) {
				cNameP2 = "C_NV" + nodeP2;
				rNameP2 = "R_N" + nodeP2;
				components.txLine[Tx].posNode2C = cNameP2;
				components.txLine[Tx].posNode2R = rNameP2;
				if (rowMap.count(rNameP2) == 0) {
					rowMap[rNameP2] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameP2) == 0) {
					columnMap[cNameP2] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameP2].push_back(label);
				pGND2 = false;
			}
			else
				pGND2 = true;
			if (nodeN2 != "0" && nodeN2.find("GND") == std::string::npos) {
				cNameN2 = "C_NV" + nodeN2;
				rNameN2 = "R_N" + nodeN2;
				components.txLine[Tx].negNode2C = cNameN2;
				components.txLine[Tx].negNode2R = rNameN2;
				if (rowMap.count(rNameN2) == 0) {
					rowMap[rNameN2] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameN2) == 0) {
					columnMap[cNameN2] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameN2].push_back(label);
				nGND2 = false;
			}
			else
				nGND2 = true;
			if (!pGND) {
				e.label = label;
				e.colIndex = columnMap.at(cName);
				e.rowIndex = rowMap.at(rNameP);
				if(std::find(relXInd.begin(), relXInd.end(), e.rowIndex) == relXInd.end()) relXInd.push_back(e.rowIndex);
				components.txLine[Tx].curN1Col = e.colIndex;
				components.txLine[Tx].posNRow = e.rowIndex;
				e.value = 1;
				mElements.push_back(e);
				e.label = label;
				e.colIndex = columnMap.at(cNameP);
				e.rowIndex = rowMap.at(rName);
				if(std::find(relXInd.begin(), relXInd.end(), e.rowIndex) == relXInd.end()) relXInd.push_back(e.rowIndex);
				components.txLine[Tx].posNCol = e.colIndex;
				components.txLine[Tx].curN1Row = e.rowIndex;
				e.value = 1;
				mElements.push_back(e);
				if (!nGND) {
					e.label = label;
					e.colIndex = columnMap.at(cName);
					e.rowIndex = rowMap.at(rNameN);
					if(std::find(relXInd.begin(), relXInd.end(), e.rowIndex) == relXInd.end()) relXInd.push_back(e.rowIndex);
					components.txLine[Tx].curN1Col = e.colIndex;
					components.txLine[Tx].negNRow = e.rowIndex;
					e.value = -1;
					mElements.push_back(e);
					e.label = label;
					e.colIndex = columnMap.at(cNameN);
					e.rowIndex = rowMap.at(rName);
					if(std::find(relXInd.begin(), relXInd.end(), e.rowIndex) == relXInd.end()) relXInd.push_back(e.rowIndex);
					components.txLine[Tx].negNCol = e.colIndex;
					components.txLine[Tx].curN1Row = e.rowIndex;
					e.value = -1;
					mElements.push_back(e);
				}
			}
			else if (!nGND) {
				e.label = label;
				e.colIndex = columnMap.at(cName);
				e.rowIndex = rowMap.at(rNameN);
				if(std::find(relXInd.begin(), relXInd.end(), e.rowIndex) == relXInd.end()) relXInd.push_back(e.rowIndex);
				components.txLine[Tx].curN1Col = e.colIndex;
				components.txLine[Tx].negNRow = e.rowIndex;
				e.value = -1;
				mElements.push_back(e);
				e.label = label;
				e.colIndex = columnMap.at(cNameN);
				e.rowIndex = rowMap.at(rName);
				if(std::find(relXInd.begin(), relXInd.end(), e.rowIndex) == relXInd.end()) relXInd.push_back(e.rowIndex);
				components.txLine[Tx].negNCol = e.colIndex;
				components.txLine[Tx].curN1Row = e.rowIndex;
				e.value = -1;
				mElements.push_back(e);
			}
			e.label = label;
			e.colIndex = columnMap.at(cName);
			e.rowIndex = rowMap.at(rName);
			if(std::find(relXInd.begin(), relXInd.end(), e.rowIndex) == relXInd.end()) relXInd.push_back(e.rowIndex);
			components.txLine[Tx].curN1Col = e.colIndex;
			components.txLine[Tx].curN1Row = e.rowIndex;
			e.value = -z0;
			mElements.push_back(e);
			if (!pGND2) {
				e.label = label;
				e.colIndex = columnMap.at(cName2);
				e.rowIndex = rowMap.at(rNameP2);
				if(std::find(relXInd.begin(), relXInd.end(), e.rowIndex) == relXInd.end()) relXInd.push_back(e.rowIndex);
				components.txLine[Tx].curN2Col = e.colIndex;
				components.txLine[Tx].posN2Row = e.rowIndex;
				e.value = 1;
				mElements.push_back(e);
				e.label = label;
				e.colIndex = columnMap.at(cNameP2);
				e.rowIndex = rowMap.at(rName2);
				if(std::find(relXInd.begin(), relXInd.end(), e.rowIndex) == relXInd.end()) relXInd.push_back(e.rowIndex);
				components.txLine[Tx].posN2Col = e.colIndex;
				components.txLine[Tx].curN2Row = e.rowIndex;
				e.value = 1;
				mElements.push_back(e);
				if (!nGND2) {
					e.label = label;
					e.colIndex = columnMap.at(cName2);
					e.rowIndex = rowMap.at(rNameN2);
					if(std::find(relXInd.begin(), relXInd.end(), e.rowIndex) == relXInd.end()) relXInd.push_back(e.rowIndex);
					components.txLine[Tx].curN2Col = e.colIndex;
					components.txLine[Tx].negN2Row = e.rowIndex;
					e.value = -1;
					mElements.push_back(e);
					e.label = label;
					e.colIndex = columnMap.at(cNameN2);
					e.rowIndex = rowMap.at(rName2);
					if(std::find(relXInd.begin(), relXInd.end(), e.rowIndex) == relXInd.end()) relXInd.push_back(e.rowIndex);
					components.txLine[Tx].negN2Col = e.colIndex;
					components.txLine[Tx].curN2Row = e.rowIndex;
					e.value = -1;
					mElements.push_back(e);
				}
			}
			else if (!nGND2) {
				e.label = label;
				e.colIndex = columnMap.at(cName2);
				e.rowIndex = rowMap.at(rNameN2);
				if(std::find(relXInd.begin(), relXInd.end(), e.rowIndex) == relXInd.end()) relXInd.push_back(e.rowIndex);
				components.txLine[Tx].curN2Col = e.colIndex;
				components.txLine[Tx].negN2Row = e.rowIndex;
				e.value = -1;
				mElements.push_back(e);
				e.label = label;
				e.colIndex = columnMap.at(cNameN2);
				e.rowIndex = rowMap.at(rName2);
				if(std::find(relXInd.begin(), relXInd.end(), e.rowIndex) == relXInd.end()) relXInd.push_back(e.rowIndex);
				components.txLine[Tx].negN2Col = e.colIndex;
				components.txLine[Tx].curN2Row = e.rowIndex;
				e.value = -1;
				mElements.push_back(e);
			}
			e.label = label;
			e.colIndex = columnMap.at(cName2);
			e.rowIndex = rowMap.at(rName2);
			if(std::find(relXInd.begin(), relXInd.end(), e.rowIndex) == relXInd.end()) relXInd.push_back(e.rowIndex);
			components.txLine[Tx].curN2Col = e.colIndex;
			components.txLine[Tx].curN2Row = e.rowIndex;
			e.value = -z0;
			mElements.push_back(e);
		}
		/******************/
		/** PHASE SOURCE **/
		/******************/
		else if (i.first[0] == 'P') {
			std::cerr << "E: Phase source not allowed in voltage analysis." << std::endl;
			std::cerr << "E: Infringing line: " << i.first << std::endl;
			std::cerr << "E: Simulation will now terminate." << std::endl;
			std::cerr << std::endl;
			exit(-1);
		}
		/*********************/
		/** MUTUAL COUPLING **/
		/*********************/
		else if (i.first[0] == 'K') {
			components.mutualInductanceLines.push_back(i);
		}
	}
	double mutualL = 0.0, cf = 0.0;
	for (const auto &i : components.mutualInductanceLines) {
		devicetokens = Misc::tokenize_space(i.first);
		try {
			label = devicetokens[0];
		}
		catch (std::exception &e) {
			Errors::invalid_component_errors(MISSING_LABEL, i.first);
		}
		try {
      auto parameter_name = ParameterName(devicetokens[3], i.second);
			if (iObj.parameters.parsedParams.count(parameter_name) != 0)
					cf = iObj.parameters.parsedParams.at(parameter_name);
			else
				cf = Misc::modifier(devicetokens[3]);
		}
		catch (std::exception &e) {
			Errors::invalid_component_errors(MUT_ERROR, i.first);
		}
		std::string ind1, ind2;
		ind1 = devicetokens[1];
		ind2 = devicetokens[2];
		if(components.voltInd.count(ind1) == 0) Errors::invalid_component_errors(MISSING_INDUCTOR, ind1);
		if(components.voltInd.count(ind2) == 0) Errors::invalid_component_errors(MISSING_INDUCTOR, ind2);
		cf = cf * sqrt(components.voltInd.at(ind1).value * components.voltInd.at(ind2).value);
		mutualL = ((2 * cf) / iObj.transSim.prstep);
		components.voltInd.at(ind1).mut[ind2] = mutualL;
		components.voltInd.at(ind2).mut[ind1] = mutualL;
		matrix_element e;
		e.label = label;
		e.colIndex = components.voltInd.at(ind1).curNCol;
		e.rowIndex = components.voltInd.at(ind2).curNRow;
		e.value = -mutualL;
		components.voltInd.at(ind1).mutPtr[ind2] = mElements.size();
		mElements.push_back(e);
		e.label = label;
		e.colIndex = components.voltInd.at(ind2).curNCol;
		e.rowIndex = components.voltInd.at(ind1).curNRow;
		e.value = -mutualL;
		components.voltInd.at(ind2).mutPtr[ind1] = mElements.size();
		mElements.push_back(e);
	}
	std::map<int, std::string> rowMapFlip = Misc::flip_map(rowMap);
	std::map<int, std::string> columnMapFlip = Misc::flip_map(columnMap);
	std::transform(
		rowMapFlip.begin(),
		rowMapFlip.end(),
		back_inserter(rowNames),
		[](std::pair<int, std::string> const& pair) { return pair.second; });
	std::transform(
		columnMapFlip.begin(),
		columnMapFlip.end(),
		back_inserter(columnNames),
		[](std::pair<int, std::string> const& pair) { return pair.second; });
}

void
Matrix::create_A_phase(Input &iObj) {
	std::string cName, rName, cNameP, rNameP, cNameN, rNameN;
	std::vector<std::string> devicetokens, componentLabels;
	devicetokens.clear();
	componentLabels.clear();
	std::string label, nodeP, nodeN, subckt;
	std::unordered_map<std::string, int> rowMap, columnMap;
	rowMap.clear();
	columnMap.clear();
	int rowCounter, colCounter, expStart, expEnd;
	bool pGND, nGND;
	rowCounter = colCounter = 0;
	/* Main circuit node identification */
	for (auto i : iObj.expNetlist) {
		expStart = expEnd = -1;
		devicetokens = Misc::tokenize_space(i.first);
				for (int t = 0; t < devicetokens.size(); t++) {
			if(devicetokens[t].find('{') != std::string::npos) {
				expStart = t;
			}
			if(devicetokens[t].find('}') != std::string::npos) {
				expEnd = t;
			}
		}
		if(expStart == -1 && expEnd != -1) Errors::invalid_component_errors(INVALID_EXPR, i.first);
		else if(expStart != -1 && expEnd == -1) Errors::invalid_component_errors(INVALID_EXPR, i.first);
		if (expStart != -1 && expStart == expEnd) {
			devicetokens[expStart] = devicetokens[expStart].substr(devicetokens[expStart].find('{') + 1, devicetokens[expStart].size() - 1);
			devicetokens[expStart] = devicetokens[expStart].substr(0, devicetokens[expStart].find('}'));
			devicetokens[expStart] = Misc::precise_to_string(Parser::parse_param(devicetokens[expStart], iObj.parameters.parsedParams, i.second), 25);
		}
		else if (expStart != -1 && expEnd != -1) {
			int d = expStart + 1;
			while (expStart != expEnd) {
				devicetokens[expStart] += devicetokens[d];
				devicetokens.erase(devicetokens.begin()+d);
				expEnd--;
			}
			devicetokens[expStart] = devicetokens[expStart].substr(devicetokens[expStart].find('{') + 1, devicetokens[expStart].size() - 1);
			devicetokens[expStart] = devicetokens[expStart].substr(0, devicetokens[expStart].find('}'));
			devicetokens[expStart] = Misc::precise_to_string(Parser::parse_param(devicetokens[expStart], iObj.parameters.parsedParams, i.second), 25);
		}
		double value = 0.0;
		/* Check if label exists, if not there is a bug in the program */
		try {
			label = devicetokens[0];
			if (std::find(componentLabels.begin(), componentLabels.end(), label) ==
				componentLabels.end()){
					if(label.find_first_of("_*!@#$\\/%^&*()") != std::string::npos) {
						std::cerr << "W: The use of special characters in label names is not advised." << std::endl;
						std::cerr << "W: This might produce unexpected results." << std::endl;
						std::cerr << "W: Continuing operation." << std::endl;
					}
					componentLabels.push_back(label);
				}
			else {
				Errors::invalid_component_errors(DUPLICATE_LABEL, label);
			}
		}
		catch (std::exception &e) {
			Errors::invalid_component_errors(MISSING_LABEL, i.first);
		}
		try {
			nodeP = devicetokens[1];
		}
		catch (std::exception &e) {
			Errors::invalid_component_errors(MISSING_PNODE, i.first);
		}
		/* Check if negative node exists, if not it's a bad device line definition
		 */
		try {
			nodeN = devicetokens[2];
		}
		catch (std::exception &e) {
			Errors::invalid_component_errors(MISSING_NNODE, i.first);
		}
		/********************/
		/** PHASE RESISTOR **/
		/********************/
		if (i.first[0] == 'R') {
			std::string R = devicetokens[0];
			matrix_element e;
			try {
        auto parameter_name = ParameterName(devicetokens[3], i.second);
				if (iObj.parameters.parsedParams.count(parameter_name) != 0)
					value = iObj.parameters.parsedParams.at(parameter_name);
				else
					value = Misc::modifier(devicetokens[3]);
			}
			catch (std::exception &e) {
				Errors::invalid_component_errors(RES_ERROR, i.first);
			}
			cName = "C_I" + devicetokens[0];
			rName = "R_" + devicetokens[0];
			components.phaseRes[R].curNodeC = cName;
			components.phaseRes[R].curNodeR = rName;
			components.phaseRes[R].value = value;
			components.phaseRes[R].label = R;
			if (rowMap.count(rName) == 0) {
				rowMap[rName] = rowCounter;
				rowCounter++;
			}
			if (columnMap.count(cName) == 0) {
				columnMap[cName] = colCounter;
				colCounter++;
			}
			if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
				cNameP = "C_NP" + nodeP;
				rNameP = "R_N" + nodeP;
				components.phaseRes[R].posNodeC = cNameP;
				components.phaseRes[R].posNodeR = rNameP;
				if (rowMap.count(rNameP) == 0) {
					rowMap[rNameP] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameP) == 0) {
					columnMap[cNameP] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameP].push_back(label);
				pGND = false;
			}
			else
				pGND = true;
			if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
				cNameN = "C_NP" + nodeN;
				rNameN = "R_N" + nodeN;
				components.phaseRes[R].negNodeC = cNameN;
				components.phaseRes[R].negNodeR = rNameN;
				if (rowMap.count(rNameN) == 0) {
					rowMap[rNameN] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameN) == 0) {
					columnMap[cNameN] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameN].push_back(label);
				nGND = false;
			}
			else
				nGND = true;
			if (!pGND) {
				e.label = label;
				e.colIndex = columnMap.at(cName);
				e.rowIndex = rowMap.at(rNameP);
				components.phaseRes[R].posNRow = e.rowIndex;
				e.value = 1;
				mElements.push_back(e);
				e.label = label;
				e.colIndex = columnMap.at(cNameP);
				e.rowIndex = rowMap.at(rName);
				components.phaseRes[R].posNCol = e.colIndex;
				e.value = 1;
				mElements.push_back(e);
			}
			if (!nGND) {
				e.label = label;
				e.colIndex = columnMap.at(cName);
				e.rowIndex = rowMap.at(rNameN);
				components.phaseRes[R].negNRow = e.rowIndex;
				e.value = -1;
				mElements.push_back(e);
				e.label = label;
				e.colIndex = columnMap.at(cNameN);
				e.rowIndex = rowMap.at(rName);
				components.phaseRes[R].negNCol = e.colIndex;
				e.value = -1;
				mElements.push_back(e);
			}
			e.label = label;
			e.colIndex = columnMap.at(cName);
			e.rowIndex = rowMap.at(rName);
			components.phaseRes[R].curNCol = e.colIndex;
			components.phaseRes[R].curNRow = e.rowIndex;
			e.value = -(M_PI * components.phaseRes[R].value * iObj.transSim.prstep) / PHI_ZERO;
			components.phaseRes[R].resPtr = mElements.size();
			mElements.push_back(e);
		}
		/*********************/
		/** PHASE CAPACITOR **/
		/*********************/
		else if (i.first[0] == 'C') {
			std::string C = devicetokens[0];
			matrix_element e;
			try {
        auto parameter_name = ParameterName(devicetokens[3], i.second);
				if (iObj.parameters.parsedParams.count(parameter_name) != 0)
					value = iObj.parameters.parsedParams.at(parameter_name);
				else
					value = Misc::modifier(devicetokens[3]);
			}
			catch (std::exception &e) {
				Errors::invalid_component_errors(CAP_ERROR, i.first);
			}
			cName = "C_I" + devicetokens[0];
			rName = "R_" + devicetokens[0];
			components.phaseCap[C].curNodeC = cName;
			components.phaseCap[C].curNodeR = rName;
			components.phaseCap[C].value = value;
			components.phaseCap[C].label = C;
			if (rowMap.count(rName) == 0) {
				rowMap[rName] = rowCounter;
				rowCounter++;
			}
			if (columnMap.count(cName) == 0) {
				columnMap[cName] = colCounter;
				colCounter++;
			}
			if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
				cNameP = "C_NP" + nodeP;
				rNameP = "R_N" + nodeP;
				components.phaseCap[C].posNodeC = cNameP;
				components.phaseCap[C].posNodeR = rNameP;
				if (rowMap.count(rNameP) == 0) {
					rowMap[rNameP] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameP) == 0) {
					columnMap[cNameP] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameP].push_back(label);
				pGND = false;
			}
			else
				pGND = true;
			if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
				cNameN = "C_NP" + nodeN;
				rNameN = "R_N" + nodeN;
				components.phaseCap[C].negNodeC = cNameN;
				components.phaseCap[C].negNodeR = rNameN;
				if (rowMap.count(rNameN) == 0) {
					rowMap[rNameN] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameN) == 0) {
					columnMap[cNameN] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameN].push_back(label);
				nGND = false;
			}
			else
				nGND = true;
			if (!pGND) {
				e.label = label;
				e.colIndex = columnMap.at(cName);
				e.rowIndex = rowMap.at(rNameP);
				components.phaseCap[C].posNRow = e.rowIndex;
				e.value = 1;
				mElements.push_back(e);
				e.label = label;
				e.colIndex = columnMap.at(cNameP);
				e.rowIndex = rowMap.at(rName);
				components.phaseCap[C].posNCol = e.colIndex;
				e.value = 1;
				mElements.push_back(e);
			}
			if (!nGND) {
				e.label = label;
				e.colIndex = columnMap.at(cName);
				e.rowIndex = rowMap.at(rNameN);
				components.phaseCap[C].negNRow = e.rowIndex;
				e.value = -1;
				mElements.push_back(e);
				e.label = label;
				e.colIndex = columnMap.at(cNameN);
				e.rowIndex = rowMap.at(rName);
				components.phaseCap[C].negNCol = e.colIndex;
				e.value = -1;
				mElements.push_back(e);
			}
			e.label = label;
			e.colIndex = columnMap.at(cName);
			e.rowIndex = rowMap.at(rName);
			components.phaseCap[C].curNCol = e.colIndex;
			components.phaseCap[C].curNRow = e.rowIndex;
			e.value = (-2 * M_PI * iObj.transSim.prstep * iObj.transSim.prstep) / (PHI_ZERO * 4 * components.phaseCap[C].value);
			components.phaseCap[C].capPtr = mElements.size();
			mElements.push_back(e);
		}
		/********************/
		/** PHASE INDUCTOR **/
		/********************/
		else if (i.first[0] == 'L') {
			std::string L = devicetokens[0];
			matrix_element e;
			try {
        auto parameter_name = ParameterName(devicetokens[3], i.second);
				if (iObj.parameters.parsedParams.count(parameter_name) != 0)
					value = iObj.parameters.parsedParams.at(parameter_name);
				else
					value = Misc::modifier(devicetokens[3]);
			}
			catch (std::exception &e) {
				Errors::invalid_component_errors(IND_ERROR, i.first);
			}
			cName = "C_I" + devicetokens[0];
			rName = "R_" + devicetokens[0];
			components.phaseInd[L].curNodeC = cName;
			components.phaseInd[L].curNodeR = rName;
			components.phaseInd[L].value = value;
			components.phaseInd[L].label = L;
			if (rowMap.count(rName) == 0) {
				rowMap[rName] = rowCounter;
				rowCounter++;
			}
			if (columnMap.count(cName) == 0) {
				columnMap[cName] = colCounter;
				colCounter++;
			}
			if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
				cNameP = "C_NP" + nodeP;
				rNameP = "R_N" + nodeP;
				components.phaseInd[L].posNodeC = cNameP;
				components.phaseInd[L].posNodeR = rNameP;
				if (rowMap.count(rNameP) == 0) {
					rowMap[rNameP] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameP) == 0) {
					columnMap[cNameP] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameP].push_back(label);
				pGND = false;
			}
			else
				pGND = true;
			if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
				cNameN = "C_NP" + nodeN;
				rNameN = "R_N" + nodeN;
				components.phaseInd[L].negNodeC = cNameN;
				components.phaseInd[L].negNodeR = rNameN;
				if (rowMap.count(rNameN) == 0) {
					rowMap[rNameN] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameN) == 0) {
					columnMap[cNameN] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameN].push_back(label);
				nGND = false;
			}
			else
				nGND = true;
			if (!pGND) {
				e.label = label;
				e.colIndex = columnMap.at(cName);
				e.rowIndex = rowMap.at(rNameP);
				components.phaseInd[L].posNRow = e.rowIndex;
				e.value = 1;
				mElements.push_back(e);
				e.label = label;
				e.colIndex = columnMap.at(cNameP);
				e.rowIndex = rowMap.at(rName);
				components.phaseInd[L].posNCol = e.colIndex;
				e.value = 1;
				mElements.push_back(e);
			}
			if (!nGND) {
				e.label = label;
				e.colIndex = columnMap.at(cName);
				e.rowIndex = rowMap.at(rNameN);
				components.phaseInd[L].negNRow = e.rowIndex;
				e.value = -1;
				mElements.push_back(e);
				e.label = label;
				e.colIndex = columnMap.at(cNameN);
				e.rowIndex = rowMap.at(rName);
				components.phaseInd[L].negNCol = e.colIndex;
				e.value = -1;
				mElements.push_back(e);
			}
			e.label = label;
			e.colIndex = columnMap.at(cName);
			e.rowIndex = rowMap.at(rName);
			components.phaseInd[L].curNCol = e.colIndex;
			components.phaseInd[L].curNRow = e.rowIndex;
			e.value = -(components.phaseInd[L].value * 2 * M_PI) / PHI_ZERO;
			components.phaseInd[L].indPtr = mElements.size();
			mElements.push_back(e);
		}
		/**************************/
		/** PHASE VOLTAGE SOURCE **/
		/**************************/
		else if (i.first[0] == 'V') {
			std::string VS = devicetokens[0];
			matrix_element e;
			sources[label] = Misc::parse_function(i.first, iObj, i.second);
			cName = "C_" + devicetokens[0];
			rName = "R_" + devicetokens[0];
			components.phaseVs[VS].curNodeC = cName;
			components.phaseVs[VS].curNodeR = rName;
			components.phaseVs[VS].label = VS;
			if (rowMap.count(rName) == 0) {
				rowMap[rName] = rowCounter;
				rowCounter++;
			}
			if (columnMap.count(cName) == 0) {
				columnMap[cName] = colCounter;
				colCounter++;
			}
			if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
				cNameP = "C_NP" + nodeP;
				rNameP = "R_N" + nodeP;
				components.phaseVs[VS].posNodeC = cNameP;
				components.phaseVs[VS].posNodeR = rNameP;
				if (rowMap.count(rNameP) == 0) {
					rowMap[rNameP] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameP) == 0) {
					columnMap[cNameP] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameP].push_back(label);
				pGND = false;
			}
			else
				pGND = true;
			if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
				cNameN = "C_NP" + nodeN;
				rNameN = "R_N" + nodeN;
				components.phaseVs[VS].negNodeC = cNameN;
				components.phaseVs[VS].negNodeR = rNameN;
				if (rowMap.count(rNameN) == 0) {
					rowMap[rNameN] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameN) == 0) {
					columnMap[cNameN] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameN].push_back(label);
				nGND = false;
			}
			else
				nGND = true;
			if (!pGND) {
				e.label = label;
				e.colIndex = columnMap.at(cName);
				e.rowIndex = rowMap.at(rNameP);
				components.phaseVs[VS].curNCol = e.colIndex;
				components.phaseVs[VS].posNRow = e.rowIndex;
				e.value = 1;
				mElements.push_back(e);
				e.label = label;
				e.colIndex = columnMap.at(cNameP);
				e.rowIndex = rowMap.at(rName);
				components.phaseVs[VS].posNCol = e.colIndex;
				components.phaseVs[VS].curNRow = e.rowIndex;
				e.value = 1;
				mElements.push_back(e);
			}
			if (!nGND) {
				e.label = label;
				e.colIndex = columnMap.at(cName);
				e.rowIndex = rowMap.at(rNameN);
				components.phaseVs[VS].curNCol = e.colIndex;
				components.phaseVs[VS].negNRow = e.rowIndex;
				e.value = -1;
				mElements.push_back(e);
				e.label = label;
				e.colIndex = columnMap.at(cNameN);
				e.rowIndex = rowMap.at(rName);
				components.phaseVs[VS].negNCol = e.colIndex;
				components.phaseVs[VS].curNRow = e.rowIndex;
				e.value = -1;
				mElements.push_back(e);
			}
		}
		/********************/
		/** CURRENT SOURCE **/
		/********************/
		else if (i.first[0] == 'I') {
			sources[label] = Misc::parse_function(i.first, iObj, i.second);
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
				nodeConnections[rNameP].push_back(label);
				pGND = false;
			}
			else
				pGND = true;
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
				nodeConnections[rNameN].push_back(label);
				nGND = false;
			}
			else
				nGND = true;
		}
		/******************************/
		/** PHASE JOSEPHSON JUNCTION **/
		/******************************/
		else if (i.first[0] == 'B') {
			std::string cVolt, rVolt, jj;
			jj = devicetokens[0];
			matrix_element e;
			std::string modelstring = "", area = "";
			for (int t = devicetokens.size() - 1; t > 2; t--) {
				if (devicetokens[t].find('=') == std::string::npos) {
					if (iObj.netlist.models.count(std::make_pair(devicetokens[t], i.second)) != 0) {
						modelstring = iObj.netlist.models.at(std::make_pair(devicetokens[t], i.second));
						break;
					}
					else if (iObj.netlist.models.count(std::make_pair(devicetokens[t], "")) != 0) {
						modelstring = iObj.netlist.models.at(std::make_pair(devicetokens[t], ""));
						break;
					}
					else {
						Errors::invalid_component_errors(MODEL_NOT_DEFINED,
														  devicetokens[t]);
						break;
					}
				}
				if (devicetokens[t].find("AREA") != std::string::npos) {
					area = devicetokens[t].substr(
						devicetokens[t].find_first_of('=') + 1,
						devicetokens[t].size() - 1);
				}
			}
			if(area == "") Errors::invalid_component_errors(MODEL_AREA_NOT_GIVEN, label);
			components.phaseJJ[jj].label = jj;
			components.jj_model_phase(modelstring, area, jj, iObj, subckt);
			cVolt = "C_V" + devicetokens[0];
			rVolt = "R_" + devicetokens[0];
			components.phaseJJ[jj].voltNodeC = cVolt;
			components.phaseJJ[jj].voltNodeR = rVolt;
			if (rowMap.count(rVolt) == 0) {
				rowMap[rVolt] = rowCounter;
				rowCounter++;
			}
			if (columnMap.count(cVolt) == 0) {
				columnMap[cVolt] = colCounter;
				colCounter++;
			}
			if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
				cNameP = "C_NP" + nodeP;
				rNameP = "R_N" + nodeP;
				components.phaseJJ[jj].posNodeC = cNameP;
				components.phaseJJ[jj].posNodeR = rNameP;
				if (rowMap.count(rNameP) == 0) {
					rowMap[rNameP] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameP) == 0) {
					columnMap[cNameP] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameP].push_back(label);
				pGND = false;
			}
			else
				pGND = true;
			if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
				cNameN = "C_NP" + nodeN;
				rNameN = "R_N" + nodeN;
				components.phaseJJ[jj].negNodeC = cNameN;
				components.phaseJJ[jj].negNodeR = rNameN;
				if (rowMap.count(rNameN) == 0) {
					rowMap[rNameN] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameN) == 0) {
					columnMap[cNameN] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameN].push_back(label);
				nGND = false;
			}
			else
				nGND = true;
			if (!pGND) {
				e.label = label;
				e.colIndex = columnMap.at(cVolt);
				e.rowIndex = rowMap.at(rNameP);
				components.phaseJJ[jj].posNRow = e.rowIndex;;
				e.value = 1 / components.phaseJJ[jj].r0 + ((2*components.phaseJJ[jj].C) / iObj.transSim.prstep);
				components.phaseJJ[jj].pPtr = mElements.size();
				mElements.push_back(e);
				e.label = label;
				e.colIndex = columnMap.at(cNameP);
				e.rowIndex = rowMap.at(rVolt);
				components.phaseJJ[jj].posNCol = e.colIndex;
				e.value = 1;
				mElements.push_back(e);
			}
			if (!nGND) {
				e.label = label;
				e.colIndex = columnMap.at(cVolt);
				e.rowIndex = rowMap.at(rNameN);
				components.phaseJJ[jj].negNRow = e.rowIndex;
				e.value = -1 / components.phaseJJ[jj].r0 - ((2*components.phaseJJ[jj].C) / iObj.transSim.prstep);
				components.phaseJJ[jj].nPtr = mElements.size();
				mElements.push_back(e);
				e.label = label;
				e.colIndex = columnMap.at(cNameN);
				e.rowIndex = rowMap.at(rVolt);
				components.phaseJJ[jj].negNCol = e.colIndex;
				e.value = -1;
				mElements.push_back(e);
			}
			e.label = label;
			e.colIndex = columnMap.at(cVolt);
			e.rowIndex = rowMap.at(rVolt);
			components.phaseJJ[jj].voltNCol = e.colIndex;
			components.phaseJJ[jj].voltNRow = e.rowIndex;
			e.value = -(iObj.transSim.prstep / 2) * ((2 * M_PI) / PHI_ZERO);
			mElements.push_back(e);
			components.phaseJJ[jj].gLarge = components.phaseJJ[jj].iC / (components.phaseJJ[jj].iCFact * components.phaseJJ[jj].delV);
			components.phaseJJ[jj].lower = components.phaseJJ[jj].vG - 0.5*components.phaseJJ[jj].delV;
			components.phaseJJ[jj].upper = components.phaseJJ[jj].vG + 0.5 * components.phaseJJ[jj].delV;
			components.phaseJJ[jj].subCond = 1 / components.phaseJJ[jj].r0 + ((2*components.phaseJJ[jj].C) / iObj.transSim.prstep);
			components.phaseJJ[jj].transCond = components.phaseJJ[jj].gLarge + ((2*components.phaseJJ[jj].C) / iObj.transSim.prstep);
			components.phaseJJ[jj].normalCond = 1 / components.phaseJJ[jj].rN + ((2*components.phaseJJ[jj].C) / iObj.transSim.prstep);
			components.phaseJJ[jj].Del0 = 1.76 * BOLTZMANN * components.phaseJJ[jj].tC;
			components.phaseJJ[jj].Del = components.phaseJJ[jj].Del0 * sqrt(cos((M_PI/2) * (components.phaseJJ[jj].T/components.phaseJJ[jj].tC) * (components.phaseJJ[jj].T/components.phaseJJ[jj].tC)));
			components.phaseJJ[jj].rNCalc = ((M_PI * components.phaseJJ[jj].Del) / (2 * EV * components.phaseJJ[jj].iC)) * tanh(components.phaseJJ[jj].Del / (2 * BOLTZMANN * components.phaseJJ[jj].T));
			components.phaseJJ[jj].iS = -components.phaseJJ[jj].iC * sin(components.phaseJJ[jj].phi0);
		}
		/*****************************/
		/** PHASE TRANSMISSION LINE **/
		/*****************************/
		else if (i.first[0] == 'T') {
			std::string nodeP2, nodeN2, cNameP2, rNameP2, cNameN2,
				rNameN2, cName1, rName1, cName2,
				rName2, tl;
			bool p2GND, n2GND;
			tl = devicetokens[0];
			try {
				nodeP2 = devicetokens[3];
			}
			catch (std::exception &e) {
				Errors::invalid_component_errors(MISSING_PNODE, i.first);
			}
			try {
				nodeN2 = devicetokens[4];
			}
			catch (std::exception &e) {
				Errors::invalid_component_errors(MISSING_NNODE, i.first);
			}
			matrix_element e;
			if (devicetokens.size() < 7) {
				Errors::invalid_component_errors(TIME_ERROR, i.first);
			}
			for (size_t l = 5; l < devicetokens.size(); l++) {
				if (devicetokens[l].find("TD") != std::string::npos)
					components.txPhase[tl].tD = Misc::modifier((devicetokens[l]).substr(3));
				else if (devicetokens[l].find("Z0") != std::string::npos)
					components.txPhase[tl].value = Misc::modifier((devicetokens[l]).substr(3));
			}
			components.txPhase[tl].k = components.txPhase[tl].tD / iObj.transSim.prstep;
			cName1 = "C_I1" + label;
			rName1 = "R_" + label + "-I1";
			components.txPhase[tl].curNode1C = cName1;
			components.txPhase[tl].curNode1R = rName1;
			if (rowMap.count(rName1) == 0) {
				rowMap[rName1] = rowCounter;
				rowCounter++;
			}
			if (columnMap.count(cName1) == 0) {
				columnMap[cName1] = colCounter;
				colCounter++;
			}
			cName2 = "C_I2" + label;
			rName2 = "R_" + label + "-I2";
			components.txPhase[tl].curNode2C = cName2;
			components.txPhase[tl].curNode2R = rName2;
			if (rowMap.count(rName2) == 0) {
				rowMap[rName2] = rowCounter;
				rowCounter++;
			}
			if (columnMap.count(cName2) == 0) {
				columnMap[cName2] = colCounter;
				colCounter++;
			}
			if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
				cNameP = "C_NP" + nodeP;
				rNameP = "R_N" + nodeP;
				components.txPhase[tl].posNodeC = cNameP;
				components.txPhase[tl].posNodeR = rNameP;
				if (rowMap.count(rNameP) == 0) {
					rowMap[rNameP] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameP) == 0) {
					columnMap[cNameP] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameP].push_back(label);
				pGND = false;
			}
			else
				pGND = true;
			if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
				cNameN = "C_NP" + nodeN;
				rNameN = "R_N" + nodeN;
				components.txPhase[tl].negNodeC = cNameN;
				components.txPhase[tl].negNodeR = rNameN;
				if (rowMap.count(rNameN) == 0) {
					rowMap[rNameN] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameN) == 0) {
					columnMap[cNameN] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameN].push_back(label);
				nGND = false;
			}
			else
				nGND = true;
			if (nodeP2 != "0" && nodeP2.find("GND") == std::string::npos) {
				cNameP2 = "C_NP" + nodeP2;
				rNameP2 = "R_N" + nodeP2;
				components.txPhase[tl].posNode2C = cNameP2;
				components.txPhase[tl].posNode2R = rNameP2;
				if (rowMap.count(rNameP2) == 0) {
					rowMap[rNameP2] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameP2) == 0) {
					columnMap[cNameP2] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameP2].push_back(label);
				p2GND = false;
			}
			else
				p2GND = true;
			if (nodeN2 != "0" && nodeN2.find("GND") == std::string::npos) {
				cNameN2 = "C_NP" + nodeN2;
				rNameN2 = "R_N" + nodeN2;
				components.txPhase[tl].negNode2C = cNameN2;
				components.txPhase[tl].negNode2R = rNameN2;
				if (rowMap.count(rNameN2) == 0) {
					rowMap[rNameN2] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameN2) == 0) {
					columnMap[cNameN2] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameN2].push_back(label);
				n2GND = false;
			}
			else
				n2GND = true;
			if (!pGND) {
				e.colIndex = columnMap.at(cNameP);
				e.rowIndex = rowMap.at(rName1);
				if(std::find(relXInd.begin(), relXInd.end(), e.rowIndex) == relXInd.end()) relXInd.push_back(e.rowIndex);
				components.txPhase[tl].posNCol = e.colIndex;
				e.value = 1;
				mElements.push_back(e);
				e.colIndex = columnMap.at(cName1);
				e.rowIndex = rowMap.at(rNameP);
				if(std::find(relXInd.begin(), relXInd.end(), e.rowIndex) == relXInd.end()) relXInd.push_back(e.rowIndex);
				components.txPhase[tl].posNRow = e.rowIndex;
				e.value = 1;
				mElements.push_back(e);
			}
			if(!nGND) {
				e.colIndex = columnMap.at(cNameN);
				e.rowIndex = rowMap.at(rName1);
				if(std::find(relXInd.begin(), relXInd.end(), e.rowIndex) == relXInd.end()) relXInd.push_back(e.rowIndex);
				components.txPhase[tl].negNCol = e.colIndex;
				e.value = -1;
				mElements.push_back(e);
				e.colIndex = columnMap.at(cName1);
				e.rowIndex = rowMap.at(rNameN);
				if(std::find(relXInd.begin(), relXInd.end(), e.rowIndex) == relXInd.end()) relXInd.push_back(e.rowIndex);
				components.txPhase[tl].negNRow = e.rowIndex;
				e.value = -1;
				mElements.push_back(e);
			}
			if (!p2GND) {
				e.colIndex = columnMap.at(cNameP2);
				e.rowIndex = rowMap.at(rName2);
				if(std::find(relXInd.begin(), relXInd.end(), e.rowIndex) == relXInd.end()) relXInd.push_back(e.rowIndex);
				components.txPhase[tl].posN2Col = e.colIndex;
				e.value = 1;
				mElements.push_back(e);
				e.colIndex = columnMap.at(cName2);
				e.rowIndex = rowMap.at(rNameP2);
				if(std::find(relXInd.begin(), relXInd.end(), e.rowIndex) == relXInd.end()) relXInd.push_back(e.rowIndex);
				components.txPhase[tl].posN2Row = e.rowIndex;
				e.value = 1;
				mElements.push_back(e);
			}
			if(!n2GND) {
				e.colIndex = columnMap.at(cNameN2);
				components.txPhase[tl].negN2Col = e.colIndex;
				e.rowIndex = rowMap.at(rName2);
				if(std::find(relXInd.begin(), relXInd.end(), e.rowIndex) == relXInd.end()) relXInd.push_back(e.rowIndex);
				e.value = -1;
				mElements.push_back(e);
				e.colIndex = columnMap.at(cName2);
				e.rowIndex = rowMap.at(rNameN2);
				if(std::find(relXInd.begin(), relXInd.end(), e.rowIndex) == relXInd.end()) relXInd.push_back(e.rowIndex);
				components.txPhase[tl].negN2Row = e.rowIndex;
				e.value = -1;
				mElements.push_back(e);
			}
			e.label = label;
			e.colIndex = columnMap.at(cName1);
			e.rowIndex = rowMap.at(rName1);
			if(std::find(relXInd.begin(), relXInd.end(), e.rowIndex) == relXInd.end()) relXInd.push_back(e.rowIndex);
			components.txPhase[tl].curN1Col = e.colIndex;
			components.txPhase[tl].curN1Row = e.rowIndex;
			e.value = -(M_PI * iObj.transSim.prstep * components.txPhase[tl].value) / (PHI_ZERO);
			mElements.push_back(e);
			e.label = label;
			e.colIndex = columnMap.at(cName2);
			e.rowIndex = rowMap.at(rName2);
			if(std::find(relXInd.begin(), relXInd.end(), e.rowIndex) == relXInd.end()) relXInd.push_back(e.rowIndex);
			components.txPhase[tl].curN2Col = e.colIndex;
			components.txPhase[tl].curN2Row = e.rowIndex;
			e.value = -(M_PI * iObj.transSim.prstep * components.txPhase[tl].value) / (PHI_ZERO);
			mElements.push_back(e);
		}
		/******************/
		/** PHASE SOURCE **/
		/******************/
		else if (i.first[0] == 'P') {
			std::string PS = devicetokens[0];
			matrix_element e;
			sources[label] = Misc::parse_function(i.first, iObj, i.second);
			cName = "C_" + devicetokens[0];
			rName = "R_" + devicetokens[0];
			components.phasePs[PS].curNodeC = cName;
			components.phasePs[PS].curNodeR = rName;
			components.phasePs[PS].label = PS;
			if (rowMap.count(rName) == 0) {
				rowMap[rName] = rowCounter;
				rowCounter++;
			}
			if (columnMap.count(cName) == 0) {
				columnMap[cName] = colCounter;
				colCounter++;
			}
			if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
				cNameP = "C_NP" + nodeP;
				rNameP = "R_N" + nodeP;
				components.phasePs[PS].posNodeC = cNameP;
				components.phasePs[PS].posNodeR = rNameP;
				if (rowMap.count(rNameP) == 0) {
					rowMap[rNameP] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameP) == 0) {
					columnMap[cNameP] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameP].push_back(label);
				pGND = false;
			}
			else
				pGND = true;
			if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
				cNameN = "C_NP" + nodeN;
				rNameN = "R_N" + nodeN;
				components.phasePs[PS].negNodeC = cNameN;
				components.phasePs[PS].negNodeR = rNameN;
				if (rowMap.count(rNameN) == 0) {
					rowMap[rNameN] = rowCounter;
					rowCounter++;
				}
				if (columnMap.count(cNameN) == 0) {
					columnMap[cNameN] = colCounter;
					colCounter++;
				}
				nodeConnections[rNameN].push_back(label);
				nGND = false;
			}
			else
				nGND = true;
			if (!pGND) {
				e.label = label;
				e.colIndex = columnMap.at(cName);
				e.rowIndex = rowMap.at(rNameP);
				components.phasePs[PS].curNCol = e.colIndex;
				components.phasePs[PS].posNRow = e.rowIndex;
				e.value = 1;
				mElements.push_back(e);
				e.label = label;
				e.colIndex = columnMap.at(cNameP);
				e.rowIndex = rowMap.at(rName);
				components.phasePs[PS].posNCol = e.colIndex;
				components.phasePs[PS].curNRow = e.rowIndex;
				e.value = 1;
				mElements.push_back(e);
			}
			if (!nGND) {
				e.label = label;
				e.colIndex = columnMap.at(cName);
				e.rowIndex = rowMap.at(rNameN);
				components.phasePs[PS].curNCol = e.colIndex;
				components.phasePs[PS].negNRow = e.rowIndex;
				e.value = -1;
				mElements.push_back(e);
				e.label = label;
				e.colIndex = columnMap.at(cNameN);
				e.rowIndex = rowMap.at(rName);
				components.phasePs[PS].negNCol = e.colIndex;
				components.phasePs[PS].curNRow = e.rowIndex;
				e.value = -1;
				mElements.push_back(e);
			}
		}
		/*********************/
		/** MUTUAL COUPLING **/
		/*********************/
		else if (i.first[0] == 'K') {
			components.mutualInductanceLines.push_back(i);
		}
	}
	double mutualL = 0.0, cf = 0.0;
	for (const auto &i : components.mutualInductanceLines) {
		devicetokens = Misc::tokenize_space(i.first);
		try {
			label = devicetokens[0];
		}
		catch (std::exception &e) {
			Errors::invalid_component_errors(MISSING_LABEL, i.first);
		}
		try {
      ParameterName parameter_name = ParameterName(devicetokens[3], i.second);
			if (iObj.parameters.parsedParams.count(parameter_name) != 0)
				cf = iObj.parameters.parsedParams.at(parameter_name);
			else
				cf = Misc::modifier(devicetokens[3]);
		}
		catch (std::exception &e) {
			Errors::invalid_component_errors(MUT_ERROR, i.first);
		}
		std::string ind1, ind2;
		ind1 = devicetokens[1];
		ind2 = devicetokens[2];
		mutualL = cf * sqrt(components.phaseInd[ind1].value * components.phaseInd[ind2].value);
		matrix_element e;
		e.label = label;
		e.colIndex = components.phaseInd[ind2].curNCol;
		e.rowIndex = components.phaseInd[ind1].curNRow;
		e.value = -(mutualL * 2 * M_PI) / PHI_ZERO;
		components.phaseInd[ind1].mutPtr = mElements.size();
		mElements.push_back(e);
		e.label = label;
		e.colIndex = components.phaseInd[ind1].curNCol;
		e.rowIndex = components.phaseInd[ind2].curNRow;
		e.value = -(mutualL * 2 * M_PI) / PHI_ZERO;
		components.phaseInd[ind2].mutPtr = mElements.size();
		mElements.push_back(e);
	}
	std::map<int, std::string> rowMapFlip = Misc::flip_map(rowMap);
	std::map<int, std::string> columnMapFlip = Misc::flip_map(columnMap);
	std::transform(
		rowMapFlip.begin(),
		rowMapFlip.end(),
		back_inserter(rowNames),
		[](std::pair<int, std::string> const& pair) { return pair.second; });
	std::transform(
		columnMapFlip.begin(),
		columnMapFlip.end(),
		back_inserter(columnNames),
		[](std::pair<int, std::string> const& pair) { return pair.second; });
}

void
Matrix::create_CSR() {
	Nsize = rowNames.size();
	Msize = columnNames.size();
	std::vector<std::map<int,double>> aMat(rowNames.size());
	colind.clear();
	nzval.clear();
	rowptr.clear();
	rowptr.push_back(0);
	for (const auto& i : mElements){
		aMat[i.rowIndex][i.colIndex] += i.value;
	}
	for(const auto& i : aMat) {
		for (auto j : i) {
			nzval.push_back(j.second);
			colind.push_back(j.first);
		}
		rowptr.push_back(rowptr.back() + i.size());
	}
}

void
Matrix::find_relevant_x(Input &iObj) {
	std::vector<std::string> tokens, tokens2;
	std::string label, label2;
	int index1, index2;
	for (const auto& i : iObj.controls) {
		if(i.find("PRINT") != std::string::npos) {
			if(i.at(0) == '.') iObj.relevantX.push_back(i.substr(1));
			else iObj.relevantX.push_back(i);
		} else if(i.find("PLOT") != std::string::npos) {
			if(i.at(0) == '.') iObj.relevantX.push_back(i.substr(1));
			else iObj.relevantX.push_back(i);
		} else if(i.find("SAVE") != std::string::npos) {
			if(i.at(0) == '.') iObj.relevantX.push_back(i.substr(1));
				else iObj.relevantX.push_back(i);
		}
	}
	std::sort(iObj.relevantX.begin(), iObj.relevantX.end());
	auto last = std::unique(iObj.relevantX.begin(), iObj.relevantX.end());
	iObj.relevantX.erase(last, iObj.relevantX.end());
	for (const auto& i : iObj.relevantX) {
		if(i.find("PRINT") != std::string::npos) {
			tokens = Misc::tokenize_space(i);
			if(tokens.at(1) == "DEVI") {
				label = tokens.at(2);
				if(label.find('_') != std::string::npos) {
					tokens = Misc::tokenize_delimeter(label, "_");
					label = tokens.back();
					for(int j = 0; j < tokens.size() - 1; j++) label += "|" + tokens.at(j);
				}
				else if(label.find('.') != std::string::npos) {
					std::replace(label.begin(), label.end(), '.', '|');
				}
				switch(label[0]) {
					case 'R':
						if(components.voltRes.count(label) != 0) {
							if(components.voltRes.at(label).posNCol == -1) {
								if(std::find(relXInd.begin(), relXInd.end(), components.voltRes.at(label).negNCol) == relXInd.end()){
									relXInd.push_back(components.voltRes.at(label).negNCol);}
							} else if(components.voltRes.at(label).negNCol == -1) {
								if(std::find(relXInd.begin(), relXInd.end(), components.voltRes.at(label).posNCol) == relXInd.end()){
									relXInd.push_back(components.voltRes.at(label).posNCol);}
							} else {
								if(std::find(relXInd.begin(), relXInd.end(), components.voltRes.at(label).posNCol) == relXInd.end()){
									relXInd.push_back(components.voltRes.at(label).posNCol);}
								if(std::find(relXInd.begin(), relXInd.end(), components.voltRes.at(label).negNCol) == relXInd.end()){
									relXInd.push_back(components.voltRes.at(label).negNCol);}
							}
						} else if(components.phaseRes.count(label) != 0)
							if(std::find(relXInd.begin(), relXInd.end(), components.phaseRes.at(label).curNCol) == relXInd.end()){
								relXInd.push_back(components.phaseRes.at(label).curNCol);}
						break;
					case 'L':
						if(components.voltInd.count(label) != 0) {
							if(std::find(relXInd.begin(), relXInd.end(), components.voltInd.at(label).curNCol) == relXInd.end()){
								relXInd.push_back(components.voltInd.at(label).curNCol);}
						}
						else if(components.phaseInd.count(label) != 0) {
							if(std::find(relXInd.begin(), relXInd.end(), components.phaseInd.at(label).curNCol) == relXInd.end()){
								relXInd.push_back(components.phaseInd.at(label).curNCol);}
						}
						break;
					case 'C':
						if(components.voltCap.count(label) != 0) {
							if(std::find(relXInd.begin(), relXInd.end(), components.voltCap.at(label).curNCol) == relXInd.end()){
								relXInd.push_back(components.voltCap.at(label).curNCol);}
						}
						else if(components.phaseCap.count(label) != 0) {
							if(std::find(relXInd.begin(), relXInd.end(), components.phaseCap.at(label).curNCol) == relXInd.end()){
								relXInd.push_back(components.phaseCap.at(label).curNCol);}
						}
						break;
				}
			} else if(tokens.at(1) == "DEVV") {
				label = tokens.at(2);
				if(label.find('_') != std::string::npos) {
					tokens = Misc::tokenize_delimeter(label, "_");
					label = tokens.back();
					for(int j = 0; j < tokens.size() - 1; j++) label += "|" + tokens.at(j);
				} else if(label.find('.') != std::string::npos)
					std::replace(label.begin(), label.end(), '.', '|');
				switch(label[0]) {
					case 'R':
						if(components.voltRes.count(label) != 0) {
							if(components.voltRes.at(label).posNCol == -1) {
								if(std::find(relXInd.begin(), relXInd.end(), components.voltRes.at(label).negNCol) == relXInd.end()){
									relXInd.push_back(components.voltRes.at(label).negNCol);}
							}
							else if(components.voltRes.at(label).negNCol == -1) {
								if(std::find(relXInd.begin(), relXInd.end(), components.voltRes.at(label).posNCol) == relXInd.end()){
									relXInd.push_back(components.voltRes.at(label).posNCol);}
							}
							else {
								if(std::find(relXInd.begin(), relXInd.end(), components.voltRes.at(label).posNCol) == relXInd.end()){
									relXInd.push_back(components.voltRes.at(label).posNCol);}
								if(std::find(relXInd.begin(), relXInd.end(), components.voltRes.at(label).negNCol) == relXInd.end()){
									relXInd.push_back(components.voltRes.at(label).negNCol);}
							}
						}
						else if(components.phaseRes.count(label) != 0) {
							if(components.phaseRes.at(label).posNCol == -1) {
								if(std::find(relXInd.begin(), relXInd.end(), components.phaseRes.at(label).negNCol) == relXInd.end()){
									relXInd.push_back(components.phaseRes.at(label).negNCol);}
							}
							else if(components.phaseRes.at(label).negNCol == -1) {
								if(std::find(relXInd.begin(), relXInd.end(), components.phaseRes.at(label).posNCol) == relXInd.end()){
									relXInd.push_back(components.phaseRes.at(label).posNCol);}
							}
							else {
								if(std::find(relXInd.begin(), relXInd.end(), components.phaseRes.at(label).posNCol) == relXInd.end()){
									relXInd.push_back(components.phaseRes.at(label).posNCol);}
								if(std::find(relXInd.begin(), relXInd.end(), components.phaseRes.at(label).negNCol) == relXInd.end()){
									relXInd.push_back(components.phaseRes.at(label).negNCol);}
							}
						}
						break;
					case 'L':
						if(components.voltInd.count(label) != 0) {
							if(components.voltInd.at(label).posNCol == -1) { 
								if(std::find(relXInd.begin(), relXInd.end(), components.voltInd.at(label).negNCol) == relXInd.end()){
									relXInd.push_back(components.voltInd.at(label).negNCol);}
							}
							else if(components.voltInd.at(label).negNCol == -1) {
								if(std::find(relXInd.begin(), relXInd.end(), components.voltInd.at(label).posNCol) == relXInd.end()){
									relXInd.push_back(components.voltInd.at(label).posNCol);}
							}
							else {
								if(std::find(relXInd.begin(), relXInd.end(), components.voltInd.at(label).posNCol) == relXInd.end()){
									relXInd.push_back(components.voltInd.at(label).posNCol);}
								if(std::find(relXInd.begin(), relXInd.end(), components.voltInd.at(label).negNCol) == relXInd.end()){
									relXInd.push_back(components.voltInd.at(label).negNCol);}
							}
						}
						else if(components.phaseInd.count(label) != 0) {
							if(components.phaseInd.at(label).posNCol == -1) { 
								if(std::find(relXInd.begin(), relXInd.end(), components.phaseInd.at(label).negNCol) == relXInd.end()){
									relXInd.push_back(components.phaseInd.at(label).negNCol);}
							}
							else if(components.phaseInd.at(label).negNCol == -1) {
								if(std::find(relXInd.begin(), relXInd.end(), components.phaseInd.at(label).posNCol) == relXInd.end()){
									relXInd.push_back(components.phaseInd.at(label).posNCol);}
							}
							else {
								if(std::find(relXInd.begin(), relXInd.end(), components.phaseInd.at(label).posNCol) == relXInd.end()){
									relXInd.push_back(components.phaseInd.at(label).posNCol);}
								if(std::find(relXInd.begin(), relXInd.end(), components.phaseInd.at(label).negNCol) == relXInd.end()){
									relXInd.push_back(components.phaseInd.at(label).negNCol);}
							}
						}
						break;
					case 'C':
						if(components.voltCap.count(label) != 0) {
							if(components.voltCap.at(label).posNCol == -1) { 
								if(std::find(relXInd.begin(), relXInd.end(), components.voltCap.at(label).negNCol) == relXInd.end()){
									relXInd.push_back(components.voltCap.at(label).negNCol);}
							}
							else if(components.voltCap.at(label).negNCol == -1) {
								if(std::find(relXInd.begin(), relXInd.end(), components.voltCap.at(label).posNCol) == relXInd.end()){
									relXInd.push_back(components.voltCap.at(label).posNCol);}
							}
							else {
								if(std::find(relXInd.begin(), relXInd.end(), components.voltCap.at(label).posNCol) == relXInd.end()){
									relXInd.push_back(components.voltCap.at(label).posNCol);}
								if(std::find(relXInd.begin(), relXInd.end(), components.voltCap.at(label).negNCol) == relXInd.end()){
									relXInd.push_back(components.voltCap.at(label).negNCol);}
							}
						}
						else if(components.phaseCap.count(label) != 0) {
							if(components.phaseCap.at(label).posNCol == -1) {
								if(std::find(relXInd.begin(), relXInd.end(), components.phaseCap.at(label).negNCol) == relXInd.end()){
									relXInd.push_back(components.phaseCap.at(label).negNCol);}
							}
							else if(components.phaseCap.at(label).negNCol == -1) {
								if(std::find(relXInd.begin(), relXInd.end(), components.phaseCap.at(label).posNCol) == relXInd.end()){
									relXInd.push_back(components.phaseCap.at(label).posNCol);}
							}
							else {
								if(std::find(relXInd.begin(), relXInd.end(), components.phaseCap.at(label).posNCol) == relXInd.end()){
									relXInd.push_back(components.phaseCap.at(label).posNCol);}
								if(std::find(relXInd.begin(), relXInd.end(), components.phaseCap.at(label).negNCol) == relXInd.end()){
									relXInd.push_back(components.phaseCap.at(label).negNCol);}
							}
						}
						break;
					case 'B':
						if(components.voltJJ.count(label) != 0) {
							if(components.voltJJ.at(label).posNCol == -1) { 
								if(std::find(relXInd.begin(), relXInd.end(), components.voltJJ.at(label).negNCol) == relXInd.end()){
									relXInd.push_back(components.voltJJ.at(label).negNCol);}
							}
							else if(components.voltJJ.at(label).negNCol == -1) {
								if(std::find(relXInd.begin(), relXInd.end(), components.voltJJ.at(label).posNCol) == relXInd.end()){
									relXInd.push_back(components.voltJJ.at(label).posNCol);}
							}
							else {
								if(std::find(relXInd.begin(), relXInd.end(), components.voltJJ.at(label).posNCol) == relXInd.end()){
									relXInd.push_back(components.voltJJ.at(label).posNCol);}
								if(std::find(relXInd.begin(), relXInd.end(), components.voltJJ.at(label).negNCol) == relXInd.end()){
									relXInd.push_back(components.voltJJ.at(label).negNCol);}
							}
						}
						else if(components.phaseJJ.count(label) != 0) {
							if(components.phaseJJ.at(label).posNCol == -1) {
								if(std::find(relXInd.begin(), relXInd.end(), components.phaseJJ.at(label).negNCol) == relXInd.end()){
									relXInd.push_back(components.phaseJJ.at(label).negNCol);}
							}
							else if(components.phaseJJ.at(label).negNCol == -1) {
								if(std::find(relXInd.begin(), relXInd.end(), components.phaseJJ.at(label).posNCol) == relXInd.end()){
									relXInd.push_back(components.phaseJJ.at(label).posNCol);}
							}
							else {
								if(std::find(relXInd.begin(), relXInd.end(), components.phaseJJ.at(label).posNCol) == relXInd.end()){
									relXInd.push_back(components.phaseJJ.at(label).posNCol);}
								if(std::find(relXInd.begin(), relXInd.end(), components.phaseJJ.at(label).negNCol) == relXInd.end()){
									relXInd.push_back(components.phaseJJ.at(label).negNCol);}
							}
						}
						break;
				}
			} else if(tokens.at(1) == "NODEV") {
				if (tokens.size() == 3) {
					label = tokens.at(2);
					if(label.find('_') != std::string::npos) {
						tokens = Misc::tokenize_delimeter(label, "_");
						label = tokens.back();
						for(int j = 0; j < tokens.size() - 1; j++) label += "|" + tokens.at(j);
					} else if(label.find('.') != std::string::npos) {
						std::replace(label.begin(), label.end(), '.', '|');
					}
					if(std::find(columnNames.begin(), columnNames.end(), "C_NV" + label) != columnNames.end()) {
						index1 = std::distance(columnNames.begin(), std::find(columnNames.begin(), columnNames.end(), "C_NV" + label));
						if(std::find(relXInd.begin(), relXInd.end(), index1) == relXInd.end()){
							relXInd.push_back(index1);}
					} else if(std::find(columnNames.begin(), columnNames.end(), "C_NP" + label) != columnNames.end()) {
						index1 = std::distance(columnNames.begin(), std::find(columnNames.begin(), columnNames.end(), "C_NP" + label));
						if(std::find(relXInd.begin(), relXInd.end(), index1) == relXInd.end()){
							relXInd.push_back(index1);}
					}
				} else if (tokens.size() == 4) {
					label = tokens.at(2);
					label2 = tokens.at(3);
					if(label.find('_') != std::string::npos) {
						tokens = Misc::tokenize_delimeter(label, "_");
						label = tokens.back();
						for(int j = 0; j < tokens.size() - 1; j++) label += "|" + tokens.at(j);
					} else if(label.find('.') != std::string::npos) {
						std::replace(label.begin(), label.end(), '.', '|');
					}
					if(label2.find('_') != std::string::npos) {
						tokens = Misc::tokenize_delimeter(label2, "_");
						label2 = tokens.back();
						for(int j = 0; j < tokens.size() - 1; j++) label2 = label + "|" + tokens.at(j);
					} else if(label2.find('.') != std::string::npos) {
						std::replace(label2.begin(), label2.end(), '.', '|');
					}
					if(label == "0" || label == "GND") {
						if(std::find(columnNames.begin(), columnNames.end(), "C_NV" + label2) != columnNames.end()) {
							index1 = std::distance(columnNames.begin(), std::find(columnNames.begin(), columnNames.end(), "C_NV" + label2));
							if(std::find(relXInd.begin(), relXInd.end(), index1) == relXInd.end()){
								relXInd.push_back(index1);}
						} else if(std::find(columnNames.begin(), columnNames.end(), "C_NP" + label2) != columnNames.end()) {
							index1 = std::distance(columnNames.begin(), std::find(columnNames.begin(), columnNames.end(), "C_NP" + label2));
							if(std::find(relXInd.begin(), relXInd.end(), index1) == relXInd.end()){
								relXInd.push_back(index1);}
						}
					} else if (label2 == "0" || label2 == "GND") {
						if(std::find(columnNames.begin(), columnNames.end(), "C_NV" + label) != columnNames.end()) {
							index1 = std::distance(columnNames.begin(), std::find(columnNames.begin(), columnNames.end(), "C_NV" + label));
							if(std::find(relXInd.begin(), relXInd.end(), index1) == relXInd.end()){
								relXInd.push_back(index1);}
						} else if(std::find(columnNames.begin(), columnNames.end(), "C_NP" + label) != columnNames.end()) {
							index1 = std::distance(columnNames.begin(), std::find(columnNames.begin(), columnNames.end(), "C_NP" + label));
							if(std::find(relXInd.begin(), relXInd.end(), index1) == relXInd.end()){
								relXInd.push_back(index1);}
						}
					} else {
						if(std::find(columnNames.begin(), columnNames.end(), "C_NV" + label) != columnNames.end()) {
							index1 = std::distance(columnNames.begin(), std::find(columnNames.begin(), columnNames.end(), "C_NV" + label));
							index2 = std::distance(columnNames.begin(), std::find(columnNames.begin(), columnNames.end(), "C_NV" + label2));
							if(std::find(relXInd.begin(), relXInd.end(), index1) == relXInd.end()){
								relXInd.push_back(index1);}
							if(std::find(relXInd.begin(), relXInd.end(), index2) == relXInd.end()){
								relXInd.push_back(index2);}
						} else if(std::find(columnNames.begin(), columnNames.end(), "C_NP" + label) != columnNames.end()) {
							index1 = std::distance(columnNames.begin(), std::find(columnNames.begin(), columnNames.end(), "C_NP" + label));
							index2 = std::distance(columnNames.begin(), std::find(columnNames.begin(), columnNames.end(), "C_NP" + label2));
							if(std::find(relXInd.begin(), relXInd.end(), index1) == relXInd.end()){
								relXInd.push_back(index1);}
							if(std::find(relXInd.begin(), relXInd.end(), index2) == relXInd.end()){
								relXInd.push_back(index2);}
						}
					}
				}
			} else if(tokens.at(1) == "NODEP") {
				if (tokens.size() == 3) {
					label = tokens.at(2);
					if(label.find('_') != std::string::npos) {
						tokens = Misc::tokenize_delimeter(label, "_");
						label = tokens.back();
						for(int j = 0; j < tokens.size() - 1; j++) label += "|" + tokens.at(j);
					} else if(label.find('.') != std::string::npos) {
						std::replace(label.begin(), label.end(), '.', '|');
					}
					if(std::find(columnNames.begin(), columnNames.end(), "C_NP" + label) != columnNames.end()) {
						index1 = std::distance(columnNames.begin(), std::find(columnNames.begin(), columnNames.end(), "C_NP" + label));
						if(std::find(relXInd.begin(), relXInd.end(), index1) == relXInd.end()){
							relXInd.push_back(index1);}
					} else if(std::find(columnNames.begin(), columnNames.end(), "C_NV" + label) != columnNames.end()) {
						index1 = std::distance(columnNames.begin(), std::find(columnNames.begin(), columnNames.end(), "C_NV" + label));
						if(std::find(relXInd.begin(), relXInd.end(), index1) == relXInd.end()){
							relXInd.push_back(index1);}
					}
				} else if (tokens.size() == 4) {
					label = tokens.at(2);
					label2 = tokens.at(3);
					if(label.find('_') != std::string::npos) {
						tokens = Misc::tokenize_delimeter(label, "_");
						label = tokens.back();
						for(int j = 0; j < tokens.size() - 1; j++) label += "|" + tokens.at(j);
					} else if(label.find('.') != std::string::npos) {
						std::replace(label.begin(), label.end(), '.', '|');
					}
					if(label2.find('_') != std::string::npos) {
						tokens = Misc::tokenize_delimeter(label2, "_");
						label2 = tokens.back();
						for(int j = 0; j < tokens.size() - 1; j++) label2 = label + "|" + tokens.at(j);
					} else if(label2.find('.') != std::string::npos) {
						std::replace(label2.begin(), label2.end(), '.', '|');
					}
					if(label == "0" || label == "GND") {
						if(std::find(columnNames.begin(), columnNames.end(), "C_NP" + label2) != columnNames.end()) {
							index1 = std::distance(columnNames.begin(), std::find(columnNames.begin(), columnNames.end(), "C_NP" + label2));
							if(std::find(relXInd.begin(), relXInd.end(), index1) == relXInd.end()){
								relXInd.push_back(index1);}
						} else if(std::find(columnNames.begin(), columnNames.end(), "C_NV" + label2) != columnNames.end()) {
							index1 = std::distance(columnNames.begin(), std::find(columnNames.begin(), columnNames.end(), "C_NV" + label2));
							if(std::find(relXInd.begin(), relXInd.end(), index1) == relXInd.end()){
								relXInd.push_back(index1);}
						}
					} else if (label2 == "0" || label2 == "GND") {
					 	if(std::find(columnNames.begin(), columnNames.end(), "C_NP" + label) != columnNames.end()) {
							index1 = std::distance(columnNames.begin(), std::find(columnNames.begin(), columnNames.end(), "C_NP" + label));
							if(std::find(relXInd.begin(), relXInd.end(), index1) == relXInd.end()){
								relXInd.push_back(index1);}
						} else if(std::find(columnNames.begin(), columnNames.end(), "C_NV" + label) != columnNames.end()) {
							index1 = std::distance(columnNames.begin(), std::find(columnNames.begin(), columnNames.end(), "C_NV" + label));
							if(std::find(relXInd.begin(), relXInd.end(), index1) == relXInd.end()){
								relXInd.push_back(index1);}
						}
					} else {
						if(std::find(columnNames.begin(), columnNames.end(), "C_NV" + label) != columnNames.end()) {
							index1 = std::distance(columnNames.begin(), std::find(columnNames.begin(), columnNames.end(), "C_NV" + label));
							index2 = std::distance(columnNames.begin(), std::find(columnNames.begin(), columnNames.end(), "C_NV" + label2));
							if(std::find(relXInd.begin(), relXInd.end(), index1) == relXInd.end()){
								relXInd.push_back(index1);}
							if(std::find(relXInd.begin(), relXInd.end(), index2) == relXInd.end()){
								relXInd.push_back(index2);}
						} else if(std::find(columnNames.begin(), columnNames.end(), "C_NP" + label) != columnNames.end()) {
							index1 = std::distance(columnNames.begin(), std::find(columnNames.begin(), columnNames.end(), "C_NP" + label));
							index2 = std::distance(columnNames.begin(), std::find(columnNames.begin(), columnNames.end(), "C_NP" + label2));
							if(std::find(relXInd.begin(), relXInd.end(), index1) == relXInd.end()){
								relXInd.push_back(index1);}
							if(std::find(relXInd.begin(), relXInd.end(), index2) == relXInd.end()){
								relXInd.push_back(index2);}
						}
					}
				}
			} else if(tokens.at(1) == "PHASE") {
				label = tokens.at(2);
				if(label.find('_') != std::string::npos) {
					tokens = Misc::tokenize_delimeter(label, "_");
					label = tokens.back();
					for(int j = 0; j < tokens.size() - 1; j++) label += "|" + tokens.at(j);
				} else if(label.find('.') != std::string::npos) {
					std::replace(label.begin(), label.end(), '.', '|');
				}
				switch(label[0]) {
					case 'R':
						if(components.phaseRes.count(label) != 0) {
							if(components.phaseRes.at(label).posNCol == -1) {
								if(std::find(relXInd.begin(), relXInd.end(), components.phaseRes.at(label).negNCol) == relXInd.end()){
									relXInd.push_back(components.phaseRes.at(label).negNCol);}
							}
							else if(components.phaseRes.at(label).negNCol == -1) {
								if(std::find(relXInd.begin(), relXInd.end(), components.phaseRes.at(label).posNCol) == relXInd.end()){
									relXInd.push_back(components.phaseRes.at(label).posNCol);}
							}
							else {
								if(std::find(relXInd.begin(), relXInd.end(), components.phaseRes.at(label).posNCol) == relXInd.end()){
									relXInd.push_back(components.phaseRes.at(label).posNCol);}
								if(std::find(relXInd.begin(), relXInd.end(), components.phaseRes.at(label).negNCol) == relXInd.end()){
									relXInd.push_back(components.phaseRes.at(label).negNCol);}
							}
						}
						break;
					case 'L':
						if(components.phaseInd.count(label) != 0) {
							if(components.phaseInd.at(label).posNCol == -1){
								if(std::find(relXInd.begin(), relXInd.end(), components.phaseInd.at(label).negNCol) == relXInd.end()){
									relXInd.push_back(components.phaseInd.at(label).negNCol);}
							}
							else if(components.phaseInd.at(label).negNCol == -1){
								if(std::find(relXInd.begin(), relXInd.end(), components.phaseInd.at(label).posNCol) == relXInd.end()){
									relXInd.push_back(components.phaseInd.at(label).posNCol);}
							}
							else {
								if(std::find(relXInd.begin(), relXInd.end(), components.phaseInd.at(label).posNCol) == relXInd.end()){
									relXInd.push_back(components.phaseInd.at(label).posNCol);}
								if(std::find(relXInd.begin(), relXInd.end(), components.phaseInd.at(label).negNCol) == relXInd.end()){
									relXInd.push_back(components.phaseInd.at(label).negNCol);}
							}
						}
						break;
					case 'C':
						if(components.phaseCap.count(label) != 0) {
							if(components.phaseCap.at(label).posNCol == -1){
								if(std::find(relXInd.begin(), relXInd.end(), components.phaseCap.at(label).negNCol) == relXInd.end()){
									relXInd.push_back(components.phaseCap.at(label).negNCol);}
							}
							else if(components.phaseCap.at(label).negNCol == -1){
								if(std::find(relXInd.begin(), relXInd.end(), components.phaseCap.at(label).posNCol) == relXInd.end()){
									relXInd.push_back(components.phaseCap.at(label).posNCol);}
							}
							else {
								if(std::find(relXInd.begin(), relXInd.end(), components.phaseCap.at(label).posNCol) == relXInd.end()){
									relXInd.push_back(components.phaseCap.at(label).posNCol);}
								if(std::find(relXInd.begin(), relXInd.end(), components.phaseCap.at(label).negNCol) == relXInd.end()){
									relXInd.push_back(components.phaseCap.at(label).negNCol);}
							}
						}
						break;
					case 'B':
						if(components.voltJJ.count(label) != 0) {
							if(std::find(relXInd.begin(), relXInd.end(), components.voltJJ.at(label).phaseNCol) == relXInd.end()){
								relXInd.push_back(components.voltJJ.at(label).phaseNCol);}
						}
						else if(components.phaseJJ.count(label) != 0) {
							if(components.phaseJJ.at(label).posNCol == -1) {
								if(std::find(relXInd.begin(), relXInd.end(), components.phaseJJ.at(label).negNCol) == relXInd.end()){
									relXInd.push_back(components.phaseJJ.at(label).negNCol);}
							}
							else if(components.phaseJJ.at(label).negNCol == -1) {
								if(std::find(relXInd.begin(), relXInd.end(), components.phaseJJ.at(label).posNCol) == relXInd.end()){
									relXInd.push_back(components.phaseJJ.at(label).posNCol);}
							}
							else {
								if(std::find(relXInd.begin(), relXInd.end(), components.phaseJJ.at(label).posNCol) == relXInd.end()){
									relXInd.push_back(components.phaseJJ.at(label).posNCol);}
								if(std::find(relXInd.begin(), relXInd.end(), components.phaseJJ.at(label).negNCol) == relXInd.end()){
									relXInd.push_back(components.phaseJJ.at(label).negNCol);}
							}
						}
						break;
				}
			}
		} else if(i.find("PLOT") != std::string::npos) {
			tokens = Misc::tokenize_space(i);
			for (int j = 1; j < tokens.size(); j++) {
				if (tokens.at(j)[0] == 'V') {
					tokens2 = Misc::tokenize_delimeter(tokens.at(j), "V() ,");
					if(tokens2.size() == 1) {
						label = tokens2.at(0);
						if(label.find('_') != std::string::npos) {
							tokens2 = Misc::tokenize_delimeter(label, "_");
							label = tokens2.back();
							for(int k = 0; k < tokens2.size() - 1; k++) label += "|" + tokens2.at(k);
						} else if(label.find('.') != std::string::npos) {
							std::replace(label.begin(), label.end(), '.', '|');
						}
						switch(label[0]) {
							case 'R':
								if(components.voltRes.count(label) != 0) {
									if(components.voltRes.at(label).posNCol == -1) {
										if(std::find(relXInd.begin(), relXInd.end(), components.voltRes.at(label).negNCol) == relXInd.end()){
											relXInd.push_back(components.voltRes.at(label).negNCol);}
									}
									else if(components.voltRes.at(label).negNCol == -1) {
										if(std::find(relXInd.begin(), relXInd.end(), components.voltRes.at(label).posNCol) == relXInd.end()){
											relXInd.push_back(components.voltRes.at(label).posNCol);}
									}
									else {
										if(std::find(relXInd.begin(), relXInd.end(), components.voltRes.at(label).posNCol) == relXInd.end()){
											relXInd.push_back(components.voltRes.at(label).posNCol);}
										if(std::find(relXInd.begin(), relXInd.end(), components.voltRes.at(label).negNCol) == relXInd.end()){
											relXInd.push_back(components.voltRes.at(label).negNCol);}
									}
								} else if(components.phaseRes.count(label) != 0) {
									if(components.phaseRes.at(label).posNCol == -1) {
										if(std::find(relXInd.begin(), relXInd.end(), components.phaseRes.at(label).negNCol) == relXInd.end()){
											relXInd.push_back(components.phaseRes.at(label).negNCol);}
									}
									else if(components.phaseRes.at(label).negNCol == -1) {
										if(std::find(relXInd.begin(), relXInd.end(), components.phaseRes.at(label).posNCol) == relXInd.end()){
											relXInd.push_back(components.phaseRes.at(label).posNCol);}
									}
									else {
										if(std::find(relXInd.begin(), relXInd.end(), components.phaseRes.at(label).posNCol) == relXInd.end()){
											relXInd.push_back(components.phaseRes.at(label).posNCol);}
										if(std::find(relXInd.begin(), relXInd.end(), components.phaseRes.at(label).negNCol) == relXInd.end()){
											relXInd.push_back(components.phaseRes.at(label).negNCol);}
									}
								}
								break;
							case 'L':
								if(components.voltInd.count(label) != 0) {
									if(components.voltInd.at(label).posNCol == -1) {
										if(std::find(relXInd.begin(), relXInd.end(), components.voltInd.at(label).negNCol) == relXInd.end()){
											relXInd.push_back(components.voltInd.at(label).negNCol);}
									}
									else if(components.voltInd.at(label).negNCol == -1) {
										if(std::find(relXInd.begin(), relXInd.end(), components.voltInd.at(label).posNCol) == relXInd.end()){
											relXInd.push_back(components.voltInd.at(label).posNCol);}
									}
									else {
										if(std::find(relXInd.begin(), relXInd.end(), components.voltInd.at(label).posNCol) == relXInd.end()){
											relXInd.push_back(components.voltInd.at(label).posNCol);}
										if(std::find(relXInd.begin(), relXInd.end(), components.voltInd.at(label).negNCol) == relXInd.end()){
											relXInd.push_back(components.voltInd.at(label).negNCol);}
									}
								} else if(components.phaseInd.count(label) != 0) {
									if(components.phaseInd.at(label).posNCol == -1) { 
										if(std::find(relXInd.begin(), relXInd.end(), components.phaseInd.at(label).negNCol) == relXInd.end()){
											relXInd.push_back(components.phaseInd.at(label).negNCol);}
									}
									else if(components.phaseInd.at(label).negNCol == -1) {
										if(std::find(relXInd.begin(), relXInd.end(), components.phaseInd.at(label).posNCol) == relXInd.end()){
											relXInd.push_back(components.phaseInd.at(label).posNCol);}
									}
									else {
										if(std::find(relXInd.begin(), relXInd.end(), components.phaseInd.at(label).posNCol) == relXInd.end()){
											relXInd.push_back(components.phaseInd.at(label).posNCol);}
										if(std::find(relXInd.begin(), relXInd.end(), components.phaseInd.at(label).negNCol) == relXInd.end()){
											relXInd.push_back(components.phaseInd.at(label).negNCol);}
									}
								}
								break;
							case 'C':
								if(components.voltCap.count(label) != 0) {
									if(components.voltCap.at(label).posNCol == -1) {
										if(std::find(relXInd.begin(), relXInd.end(), components.voltCap.at(label).negNCol) == relXInd.end()){
											relXInd.push_back(components.voltCap.at(label).negNCol);}
									}
									else if(components.voltCap.at(label).negNCol == -1) {
										if(std::find(relXInd.begin(), relXInd.end(), components.voltCap.at(label).posNCol) == relXInd.end()){
											relXInd.push_back(components.voltCap.at(label).posNCol);}
									}
									else {
										if(std::find(relXInd.begin(), relXInd.end(), components.voltCap.at(label).posNCol) == relXInd.end()){
											relXInd.push_back(components.voltCap.at(label).posNCol);}
										if(std::find(relXInd.begin(), relXInd.end(), components.voltCap.at(label).negNCol) == relXInd.end()){
											relXInd.push_back(components.voltCap.at(label).negNCol);}
									}
								} else if(components.phaseCap.count(label) != 0) {
									if(components.phaseCap.at(label).posNCol == -1) {
										if(std::find(relXInd.begin(), relXInd.end(), components.phaseCap.at(label).negNCol) == relXInd.end()){
											relXInd.push_back(components.phaseCap.at(label).negNCol);}
									}
									else if(components.phaseCap.at(label).negNCol == -1) {
										if(std::find(relXInd.begin(), relXInd.end(), components.phaseCap.at(label).posNCol) == relXInd.end()){
											relXInd.push_back(components.phaseCap.at(label).posNCol);}
									}
									else {
										if(std::find(relXInd.begin(), relXInd.end(), components.phaseCap.at(label).posNCol) == relXInd.end()){
											relXInd.push_back(components.phaseCap.at(label).posNCol);}
										if(std::find(relXInd.begin(), relXInd.end(), components.phaseCap.at(label).negNCol) == relXInd.end()){
											relXInd.push_back(components.phaseCap.at(label).negNCol);}
									}
								}
								break;
							case 'B':
								if(components.voltJJ.count(label) != 0) {
									if(components.voltJJ.at(label).posNCol == -1) {
										if(std::find(relXInd.begin(), relXInd.end(), components.voltJJ.at(label).negNCol) == relXInd.end()){
											relXInd.push_back(components.voltJJ.at(label).negNCol);}
									}
									else if(components.voltJJ.at(label).negNCol == -1) {
										if(std::find(relXInd.begin(), relXInd.end(), components.voltJJ.at(label).posNCol) == relXInd.end()){
											relXInd.push_back(components.voltJJ.at(label).posNCol);}
									}
									else {
										if(std::find(relXInd.begin(), relXInd.end(), components.voltJJ.at(label).posNCol) == relXInd.end()){
											relXInd.push_back(components.voltJJ.at(label).posNCol);}
										if(std::find(relXInd.begin(), relXInd.end(), components.voltJJ.at(label).negNCol) == relXInd.end()){
											relXInd.push_back(components.voltJJ.at(label).negNCol);}
									}
								} else if(components.phaseJJ.count(label) != 0) {
									if(components.phaseJJ.at(label).posNCol == -1) {
										if(std::find(relXInd.begin(), relXInd.end(), components.phaseJJ.at(label).negNCol) == relXInd.end()){
											relXInd.push_back(components.phaseJJ.at(label).negNCol);}
									}
									else if(components.phaseJJ.at(label).negNCol == -1) {
										if(std::find(relXInd.begin(), relXInd.end(), components.phaseJJ.at(label).posNCol) == relXInd.end()){
											relXInd.push_back(components.phaseJJ.at(label).posNCol);}
									}
									else {
										if(std::find(relXInd.begin(), relXInd.end(), components.phaseJJ.at(label).posNCol) == relXInd.end()){
											relXInd.push_back(components.phaseJJ.at(label).posNCol);}
										if(std::find(relXInd.begin(), relXInd.end(), components.phaseJJ.at(label).negNCol) == relXInd.end()){
											relXInd.push_back(components.phaseJJ.at(label).negNCol);}
									}
								}
								break;
							default:
								if(std::find(columnNames.begin(), columnNames.end(), "C_NV" + label) != columnNames.end()) {
									index1 = std::distance(columnNames.begin(), std::find(columnNames.begin(), columnNames.end(), "C_NV" + label));
									if(std::find(relXInd.begin(), relXInd.end(), index1) == relXInd.end()){
										relXInd.push_back(index1);}
								} else if(std::find(columnNames.begin(), columnNames.end(), "C_NP" + label) != columnNames.end()) {
									index1 = std::distance(columnNames.begin(), std::find(columnNames.begin(), columnNames.end(), "C_NP" + label));
									if(std::find(relXInd.begin(), relXInd.end(), index1) == relXInd.end()){
										relXInd.push_back(index1);}
								}
								break;
						}
					} else {
						label = tokens2.at(0);
						label2 = tokens2.at(1);
						if(label.find('_') != std::string::npos) {
							tokens2 = Misc::tokenize_delimeter(label, "_");
							label = tokens2.back();
							for(int k = 0; k < tokens2.size() - 1; k++) label += "|" + tokens2.at(k);
						} else if(label.find('.') != std::string::npos) {
							std::replace(label.begin(), label.end(), '.', '|');
						}
						if(label2.find('_') != std::string::npos) {
							tokens2 = Misc::tokenize_delimeter(label2, "_");
							label2 = tokens2.back();
							for(int k = 0; k < tokens2.size() - 1; k++) label2 = label + "|" + tokens2.at(k);
						} else if(label2.find('.') != std::string::npos) {
							std::replace(label2.begin(), label2.end(), '.', '|');
						}
						if(label == "0" || label == "GND") {
							if(std::find(columnNames.begin(), columnNames.end(), "C_NV" + label2) != columnNames.end()) {
								index1 = std::distance(columnNames.begin(), std::find(columnNames.begin(), columnNames.end(), "C_NV" + label2));
								if(std::find(relXInd.begin(), relXInd.end(), index1) == relXInd.end()){
									relXInd.push_back(index1);}
							} else if(std::find(columnNames.begin(), columnNames.end(), "C_NP" + label2) != columnNames.end()) {
								index1 = std::distance(columnNames.begin(), std::find(columnNames.begin(), columnNames.end(), "C_NP" + label2));
								if(std::find(relXInd.begin(), relXInd.end(), index1) == relXInd.end()){
									relXInd.push_back(index1);}
							}
						} else if (label2 == "0" || label2 == "GND") {
							if(std::find(columnNames.begin(), columnNames.end(), "C_NV" + label) != columnNames.end()) {
								index1 = std::distance(columnNames.begin(), std::find(columnNames.begin(), columnNames.end(), "C_NV" + label));
								if(std::find(relXInd.begin(), relXInd.end(), index1) == relXInd.end()){
									relXInd.push_back(index1);}
							} else if(std::find(columnNames.begin(), columnNames.end(), "C_NP" + label) != columnNames.end()) {
								index1 = std::distance(columnNames.begin(), std::find(columnNames.begin(), columnNames.end(), "C_NP" + label));
								if(std::find(relXInd.begin(), relXInd.end(), index1) == relXInd.end()){
									relXInd.push_back(index1);}
							}
						} else {
							if(std::find(columnNames.begin(), columnNames.end(), "C_NV" + label) != columnNames.end()) {
								index1 = std::distance(columnNames.begin(), std::find(columnNames.begin(), columnNames.end(), "C_NV" + label));
								index2 = std::distance(columnNames.begin(), std::find(columnNames.begin(), columnNames.end(), "C_NV" + label2));
								if(std::find(relXInd.begin(), relXInd.end(), index1) == relXInd.end()){
									relXInd.push_back(index1);}
								if(std::find(relXInd.begin(), relXInd.end(), index2) == relXInd.end()){
									relXInd.push_back(index2);}
							} else if(std::find(columnNames.begin(), columnNames.end(), "C_NP" + label) != columnNames.end()) {
								index1 = std::distance(columnNames.begin(), std::find(columnNames.begin(), columnNames.end(), "C_NP" + label));
								index2 = std::distance(columnNames.begin(), std::find(columnNames.begin(), columnNames.end(), "C_NP" + label2));
								if(std::find(relXInd.begin(), relXInd.end(), index1) == relXInd.end()){
									relXInd.push_back(index1);}
								if(std::find(relXInd.begin(), relXInd.end(), index2) == relXInd.end()){
									relXInd.push_back(index2);}
							}
						}
					}
				} else if (tokens.at(j)[0] == 'C') {
					tokens2 = Misc::tokenize_delimeter(tokens.at(j), "C() ,");
					if(tokens2.size() == 1) {
						label = tokens2.at(0);
						if(label.find('_') != std::string::npos) {
							tokens2 = Misc::tokenize_delimeter(label, "_");
							label = tokens2.back();
							for(int k = 0; k < tokens2.size() - 1; k++) label += "|" + tokens2.at(k);
						} else if(label.find('.') != std::string::npos) {
							std::replace(label.begin(), label.end(), '.', '|');
						}
						switch(label[0]) {
							case 'R':
								if(components.voltRes.count(label) != 0) {
									if(components.voltRes.at(label).posNCol == -1) {
										if(std::find(relXInd.begin(), relXInd.end(), components.voltRes.at(label).negNCol) == relXInd.end()){
											relXInd.push_back(components.voltRes.at(label).negNCol);}
									}
									else if(components.voltRes.at(label).negNCol == -1) {
										if(std::find(relXInd.begin(), relXInd.end(), components.voltRes.at(label).posNCol) == relXInd.end()){
											relXInd.push_back(components.voltRes.at(label).posNCol);}
									}
									else {
										if(std::find(relXInd.begin(), relXInd.end(), components.voltRes.at(label).posNCol) == relXInd.end()){
											relXInd.push_back(components.voltRes.at(label).posNCol);}
										if(std::find(relXInd.begin(), relXInd.end(), components.voltRes.at(label).negNCol) == relXInd.end()){
											relXInd.push_back(components.voltRes.at(label).negNCol);}
									}
								} else if(components.phaseRes.count(label) != 0)
									if(std::find(relXInd.begin(), relXInd.end(), components.phaseRes.at(label).curNCol) == relXInd.end()){
										relXInd.push_back(components.phaseRes.at(label).curNCol);}
								break;
							case 'L':
								if(components.voltInd.count(label) != 0) {
									if(std::find(relXInd.begin(), relXInd.end(), components.voltInd.at(label).curNCol) == relXInd.end()){
										relXInd.push_back(components.voltInd.at(label).curNCol);}
								}
								else if(components.phaseInd.count(label) != 0) {
									if(std::find(relXInd.begin(), relXInd.end(), components.phaseInd.at(label).curNCol) == relXInd.end()){
										relXInd.push_back(components.phaseInd.at(label).curNCol);}
								}
								break;
							case 'C':
								if(components.voltCap.count(label) != 0) {
									if(std::find(relXInd.begin(), relXInd.end(), components.voltCap.at(label).curNCol) == relXInd.end()){
										relXInd.push_back(components.voltCap.at(label).curNCol);}
								}
								else if(components.phaseCap.count(label) != 0) {
									if(std::find(relXInd.begin(), relXInd.end(), components.phaseCap.at(label).curNCol) == relXInd.end()){
										relXInd.push_back(components.phaseCap.at(label).curNCol);}
								}
								break;
						}
					}
				} else if (tokens.at(j).find("#BRANCH") != std::string::npos) {
					tokens2 = Misc::tokenize_delimeter(tokens.at(j), " #");
					label = tokens2.at(0);
					if(label.find('_') != std::string::npos) {
						tokens2 = Misc::tokenize_delimeter(label, "_");
						label = tokens2.back();
						for(int k = 0; k < tokens2.size() - 1; k++) label += "|" + tokens2.at(k);
					} else if(label.find('.') != std::string::npos) {
						std::replace(label.begin(), label.end(), '.', '|');
					}
					switch(label[0]) {
						case 'R':
							if(components.voltRes.count(label) != 0) {
								if(components.voltRes.at(label).posNCol == -1) {
									if(std::find(relXInd.begin(), relXInd.end(), components.voltRes.at(label).negNCol) == relXInd.end()){
										relXInd.push_back(components.voltRes.at(label).negNCol);}
								}
								else if(components.voltRes.at(label).negNCol == -1) {
									if(std::find(relXInd.begin(), relXInd.end(), components.voltRes.at(label).posNCol) == relXInd.end()){
										relXInd.push_back(components.voltRes.at(label).posNCol);}
								}
								else {
									if(std::find(relXInd.begin(), relXInd.end(), components.voltRes.at(label).posNCol) == relXInd.end()){
										relXInd.push_back(components.voltRes.at(label).posNCol);}
									if(std::find(relXInd.begin(), relXInd.end(), components.voltRes.at(label).negNCol) == relXInd.end()){
										relXInd.push_back(components.voltRes.at(label).negNCol);}
								}
							} else if(components.phaseRes.count(label) != 0) {
								if(std::find(relXInd.begin(), relXInd.end(), components.phaseRes.at(label).curNCol) == relXInd.end()){
									relXInd.push_back(components.phaseRes.at(label).curNCol);}
							}
							break;
						case 'L':
							if(components.voltInd.count(label) != 0) {
								if(std::find(relXInd.begin(), relXInd.end(), components.voltInd.at(label).curNCol) == relXInd.end()){
									relXInd.push_back(components.voltInd.at(label).curNCol);}
							}
							else if(components.phaseInd.count(label) != 0) {
								if(std::find(relXInd.begin(), relXInd.end(), components.phaseInd.at(label).curNCol) == relXInd.end()){
									relXInd.push_back(components.phaseInd.at(label).curNCol);}
							}
							break;
						case 'C':
							if(components.voltCap.count(label) != 0) {
								if(std::find(relXInd.begin(), relXInd.end(), components.voltCap.at(label).curNCol) == relXInd.end()){
									relXInd.push_back(components.voltCap.at(label).curNCol);}
							}
							else if(components.phaseCap.count(label) != 0) {
								if(std::find(relXInd.begin(), relXInd.end(), components.phaseCap.at(label).curNCol) == relXInd.end()){
									relXInd.push_back(components.phaseCap.at(label).curNCol);}
							}
							break;
					}
				} else if (tokens.at(j)[0] == 'P') {
					tokens2 = Misc::tokenize_delimeter(tokens.at(j), "P() ,");
					if(tokens2.size() == 1) {
						label = tokens2.at(0);
						if(label.find('_') != std::string::npos) {
							tokens2 = Misc::tokenize_delimeter(label, "_");
							label = tokens2.back();
							for(int k = 0; k < tokens2.size() - 1; k++) label += "|" + tokens2.at(k);
						} else if(label.find('.') != std::string::npos) {
							std::replace(label.begin(), label.end(), '.', '|');
						}
						switch(label[0]) {
							case 'R':
								if(components.phaseRes.count(label) != 0) {
									if(components.phaseRes.at(label).posNCol == -1) {
										if(std::find(relXInd.begin(), relXInd.end(), components.phaseRes.at(label).negNCol) == relXInd.end()){
											relXInd.push_back(components.phaseRes.at(label).negNCol);}
									}
									else if(components.phaseRes.at(label).negNCol == -1) {
										if(std::find(relXInd.begin(), relXInd.end(), components.phaseRes.at(label).posNCol) == relXInd.end()){
											relXInd.push_back(components.phaseRes.at(label).posNCol);}
									}
									else {
										if(std::find(relXInd.begin(), relXInd.end(), components.phaseRes.at(label).posNCol) == relXInd.end()){
											relXInd.push_back(components.phaseRes.at(label).posNCol);}
										if(std::find(relXInd.begin(), relXInd.end(), components.phaseRes.at(label).negNCol) == relXInd.end()){
											relXInd.push_back(components.phaseRes.at(label).negNCol);}
									}
								}
								break;
							case 'L':
								if(components.phaseInd.count(label) != 0) {
									if(components.phaseInd.at(label).posNCol == -1) {
										if(std::find(relXInd.begin(), relXInd.end(), components.phaseInd.at(label).negNCol) == relXInd.end()){
											relXInd.push_back(components.phaseInd.at(label).negNCol);}
									}
									else if(components.phaseInd.at(label).negNCol == -1) {
										if(std::find(relXInd.begin(), relXInd.end(), components.phaseInd.at(label).posNCol) == relXInd.end()){
											relXInd.push_back(components.phaseInd.at(label).posNCol);}
									}
									else {
										if(std::find(relXInd.begin(), relXInd.end(), components.phaseInd.at(label).posNCol) == relXInd.end()){
											relXInd.push_back(components.phaseInd.at(label).posNCol);}
										if(std::find(relXInd.begin(), relXInd.end(), components.phaseInd.at(label).negNCol) == relXInd.end()){
											relXInd.push_back(components.phaseInd.at(label).negNCol);}
									}
								}
								break;
							case 'C':
								if(components.phaseCap.count(label) != 0) {
									if(components.phaseCap.at(label).posNCol == -1) {
										if(std::find(relXInd.begin(), relXInd.end(), components.phaseCap.at(label).negNCol) == relXInd.end()){
											relXInd.push_back(components.phaseCap.at(label).negNCol);}
									}
									else if(components.phaseCap.at(label).negNCol == -1) {
										if(std::find(relXInd.begin(), relXInd.end(), components.phaseCap.at(label).posNCol) == relXInd.end()){
											relXInd.push_back(components.phaseCap.at(label).posNCol);}
									}
									else {
										if(std::find(relXInd.begin(), relXInd.end(), components.phaseCap.at(label).posNCol) == relXInd.end()){
											relXInd.push_back(components.phaseCap.at(label).posNCol);}
										if(std::find(relXInd.begin(), relXInd.end(), components.phaseCap.at(label).negNCol) == relXInd.end()){
											relXInd.push_back(components.phaseCap.at(label).negNCol);}
									}
								}
								break;
							case 'B':
								if(components.voltJJ.count(label) != 0) {
									if(std::find(relXInd.begin(), relXInd.end(), components.voltJJ.at(label).phaseNCol) == relXInd.end()){
										relXInd.push_back(components.voltJJ.at(label).phaseNCol);}
								}
								else if(components.phaseJJ.count(label) != 0) {
									if(components.phaseJJ.at(label).posNCol == -1) {
										if(std::find(relXInd.begin(), relXInd.end(), components.phaseJJ.at(label).negNCol) == relXInd.end()){
											relXInd.push_back(components.phaseJJ.at(label).negNCol);}
									}
									else if(components.phaseJJ.at(label).negNCol == -1) {
										if(std::find(relXInd.begin(), relXInd.end(), components.phaseJJ.at(label).posNCol) == relXInd.end()){
											relXInd.push_back(components.phaseJJ.at(label).posNCol);}
									}
									else {
										if(std::find(relXInd.begin(), relXInd.end(), components.phaseJJ.at(label).posNCol) == relXInd.end()){
											relXInd.push_back(components.phaseJJ.at(label).posNCol);}
										if(std::find(relXInd.begin(), relXInd.end(), components.phaseJJ.at(label).negNCol) == relXInd.end()){
											relXInd.push_back(components.phaseJJ.at(label).negNCol);}
									}
								}
								break;
							default:
								if(std::find(columnNames.begin(), columnNames.end(), "C_NP" + label) != columnNames.end()) {
									index1 = std::distance(columnNames.begin(), std::find(columnNames.begin(), columnNames.end(), "C_NP" + label));
									if(std::find(relXInd.begin(), relXInd.end(), index1) == relXInd.end()){
										relXInd.push_back(index1);}
								} else if(std::find(columnNames.begin(), columnNames.end(), "C_NV" + label) != columnNames.end()) {
									index1 = std::distance(columnNames.begin(), std::find(columnNames.begin(), columnNames.end(), "C_NV" + label));
									if(std::find(relXInd.begin(), relXInd.end(), index1) == relXInd.end()){
										relXInd.push_back(index1);}
								}
								break;
						}
					} else {
						label = tokens2.at(0);
						label2 = tokens2.at(1);
						if(label.find('_') != std::string::npos) {
							tokens2 = Misc::tokenize_delimeter(label, "_");
							label = tokens2.back();
							for(int k = 0; k < tokens2.size() - 1; k++) label += "|" + tokens2.at(k);
						} else if(label.find('.') != std::string::npos) {
							std::replace(label.begin(), label.end(), '.', '|');
						}
						if(label2.find('_') != std::string::npos) {
							tokens2 = Misc::tokenize_delimeter(label2, "_");
							label2 = tokens2.back();
							for(int k = 0; k < tokens2.size() - 1; k++) label2 = label + "|" + tokens2.at(k);
						} else if(label2.find('.') != std::string::npos) {
							std::replace(label2.begin(), label2.end(), '.', '|');
						}
						if(label == "0" || label == "GND") {
							if(std::find(columnNames.begin(), columnNames.end(), "C_NP" + label2) != columnNames.end()) {
								index1 = std::distance(columnNames.begin(), std::find(columnNames.begin(), columnNames.end(), "C_NP" + label2));
								if(std::find(relXInd.begin(), relXInd.end(), index1) == relXInd.end()){
									relXInd.push_back(index1);}
							} else if(std::find(columnNames.begin(), columnNames.end(), "C_NV" + label2) != columnNames.end()) {
								index1 = std::distance(columnNames.begin(), std::find(columnNames.begin(), columnNames.end(), "C_NV" + label2));
								if(std::find(relXInd.begin(), relXInd.end(), index1) == relXInd.end()){
									relXInd.push_back(index1);}
							}
						} else if (label2 == "0" || label2 == "GND") {
							if(std::find(columnNames.begin(), columnNames.end(), "C_NP" + label) != columnNames.end()) {
								index1 = std::distance(columnNames.begin(), std::find(columnNames.begin(), columnNames.end(), "C_NP" + label));
								if(std::find(relXInd.begin(), relXInd.end(), index1) == relXInd.end()){
									relXInd.push_back(index1);}
							} else if(std::find(columnNames.begin(), columnNames.end(), "C_NV" + label) != columnNames.end()) {
								index1 = std::distance(columnNames.begin(), std::find(columnNames.begin(), columnNames.end(), "C_NV" + label));
								if(std::find(relXInd.begin(), relXInd.end(), index1) == relXInd.end()){
									relXInd.push_back(index1);}
							}
						} else {
							if(std::find(columnNames.begin(), columnNames.end(), "C_NV" + label) != columnNames.end()) {
								index1 = std::distance(columnNames.begin(), std::find(columnNames.begin(), columnNames.end(), "C_NV" + label));
								index2 = std::distance(columnNames.begin(), std::find(columnNames.begin(), columnNames.end(), "C_NV" + label2));
								if(std::find(relXInd.begin(), relXInd.end(), index1) == relXInd.end()){
									relXInd.push_back(index1);}
								if(std::find(relXInd.begin(), relXInd.end(), index2) == relXInd.end()){
									relXInd.push_back(index2);}
							} else if(std::find(columnNames.begin(), columnNames.end(), "C_NP" + label) != columnNames.end()) {
								index1 = std::distance(columnNames.begin(), std::find(columnNames.begin(), columnNames.end(), "C_NP" + label));
								index2 = std::distance(columnNames.begin(), std::find(columnNames.begin(), columnNames.end(), "C_NP" + label2));
								if(std::find(relXInd.begin(), relXInd.end(), index1) == relXInd.end()){
									relXInd.push_back(index1);}
								if(std::find(relXInd.begin(), relXInd.end(), index2) == relXInd.end()){
									relXInd.push_back(index2);}
							}
						}
					}
				}
			}
		}
	}
}
