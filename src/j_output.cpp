// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#include "j_output.h"

void
Output::relevant_traces(Input &iObj, Matrix &mObj, Simulation &sObj) {
	timesteps = &sObj.results.timeAxis;
	std::vector<std::string> tokens, tokens2;
	std::string label, label2;
	int index1, index2;
	for (const auto& i : iObj.controls) {
		if(i.find("PRINT") != std::string::npos) {
			Trace thisTrace;
			tokens = Misc::tokenize_space(i);
			if(tokens.at(1) == "DEVI") {
				if(tokens.size() > 4) {
					std::cout << "W: Print request for device current has too many arguments. " << std::endl;
					std::cout << "W: Line: " << i << std::endl;
					std::cout << "W: Ignoring the extra argument." << std::endl;
					std::cout << std::endl;
				}
				label = tokens.at(2);
				if(label.find('_') != std::string::npos) {
					tokens = Misc::tokenize_delimeter(label, "_");
					label = tokens.back();
					for(int j = 0; j < tokens.size() - 1; j++) label += "|" + tokens.at(j);
				}
				else if(label.find('.') != std::string::npos) {
					std::replace(label.begin(), label.end(), '.', '|');
				}
				thisTrace.name = label;
				switch(label[0]) {
					case 'R':
						thisTrace.type = 'C';
						if(mObj.components.voltRes.count(label) != 0) {
							if(mObj.components.voltRes.at(label).posNCol == -1) {
								std::transform(
									sObj.results.xVect.at(mObj.components.voltRes.at(label).negNCol).begin(),
									sObj.results.xVect.at(mObj.components.voltRes.at(label).negNCol).end(),
									std::back_inserter(thisTrace.calcData),
									std::bind(std::multiplies<double>(),
										std::placeholders::_1,
										(1 / mObj.components.voltRes.at(label).value)));
							}
							else if(mObj.components.voltRes.at(label).negNCol == -1) {
								std::transform(
									sObj.results.xVect.at(mObj.components.voltRes.at(label).posNCol).begin(),
									sObj.results.xVect.at(mObj.components.voltRes.at(label).posNCol).end(),
									std::back_inserter(thisTrace.calcData),
									std::bind(std::multiplies<double>(),
										std::placeholders::_1,
										(1 / mObj.components.voltRes.at(label).value)));
							}
							else {
								std::transform(
									sObj.results.xVect.at(mObj.components.voltRes.at(label).posNCol).begin(),
									sObj.results.xVect.at(mObj.components.voltRes.at(label).posNCol).end(),
									sObj.results.xVect.at(mObj.components.voltRes.at(label).negNCol).begin(),
									std::back_inserter(thisTrace.calcData),
									std::minus<double>());
								std::transform(
									thisTrace.calcData.begin(),
									thisTrace.calcData.end(),
									thisTrace.calcData.begin(),
									std::bind(std::multiplies<double>(),
										std::placeholders::_1,
										(1 / mObj.components.voltRes.at(label).value)));
							}
							thisTrace.pointer = false;
							traces.push_back(thisTrace);
						}
						else if(mObj.components.phaseRes.count(label) != 0) {
							thisTrace.traceData = &sObj.results.xVect.at(mObj.components.phaseRes.at(label).curNCol);
							traces.push_back(thisTrace);
						}
						else {
							std::cout << "W: Unknown device " << label << std::endl;
							std::cout << "W: Cannot print current for this device." << std::endl;
							std::cout << "W: Ignoring this print request." << std::endl;
							std::cout << std::endl;
						}
						break;
					case 'L':
						thisTrace.type = 'C';
						if(mObj.components.voltInd.count(label) != 0) {
							thisTrace.traceData = &sObj.results.xVect.at(mObj.components.voltInd.at(label).curNCol);
							traces.push_back(thisTrace);
						}
						else if(mObj.components.phaseInd.count(label) != 0) {
							thisTrace.traceData = &sObj.results.xVect.at(mObj.components.phaseInd.at(label).curNCol);
							traces.push_back(thisTrace);
						}
						else {
							std::cout << "W: Unknown device " << label << std::endl;
							std::cout << "W: Cannot print current for this device." << std::endl;
							std::cout << "W: Ignoring this print request." << std::endl;
							std::cout << std::endl;
						}
						break;
					case 'C':
						thisTrace.type = 'C';
						if(mObj.components.voltCap.count(label) != 0) {
							thisTrace.traceData = &sObj.results.xVect.at(mObj.components.voltCap.at(label).curNCol);
							traces.push_back(thisTrace);
						}
						else if(mObj.components.phaseCap.count(label) != 0) {
							thisTrace.traceData = &sObj.results.xVect.at(mObj.components.phaseCap.at(label).curNCol);
							traces.push_back(thisTrace);
						}
						else {
							std::cout << "W: Unknown device " << label << std::endl;
							std::cout << "W: Cannot print current for this device." << std::endl;
							std::cout << "W: Ignoring this print request." << std::endl;
							std::cout << std::endl;
						}
						break;
					case 'B':
						thisTrace.type = 'C';
						if(mObj.components.voltJJ.count(label) != 0) {
							thisTrace.traceData = &mObj.components.voltJJ.at(label).jjCur;
							traces.push_back(thisTrace);
						}
						else if(mObj.components.phaseJJ.count(label) != 0) {
							thisTrace.traceData = &mObj.components.phaseJJ.at(label).jjCur;
							traces.push_back(thisTrace);
						}
						else {
							std::cout << "W: Unknown device " << label << std::endl;
							std::cout << "W: Cannot print current for this device." << std::endl;
							std::cout << "W: Ignoring this print request." << std::endl;
							std::cout << std::endl;
						}
						break;
					case 'V':
						std::cout << "W: Requesting current through a voltage source." << std::endl;
						std::cout << "W: Line: " << i << std::endl;
						std::cout << "W: This is invalid and the request will be ignored." << std::endl;
						std::cout << std::endl;
						break;
					case 'I':
						thisTrace.type = 'C';
						if(mObj.sources.count(label) != 0) {
							thisTrace.traceData = &mObj.sources.at(label);
							traces.push_back(thisTrace);
						}
						else {
							std::cout << "W: Unknown device " << label << std::endl;
							std::cout << "W: Cannot print current for this device." << std::endl;
							std::cout << "W: Ignoring this print request." << std::endl;
							std::cout << std::endl;
						}
						break;
				}
			}
			else if(tokens.at(1) == "DEVV") {
				if(tokens.size() > 4) {
					std::cout << "W: Print request for device voltage has too many arguments. " << std::endl;
					std::cout << "W: Line: " << i << std::endl;
					std::cout << "W: Ignoring the extra argument." << std::endl;
					std::cout << std::endl;
				}
				label = tokens.at(2);
				if(label.find('_') != std::string::npos) {
					tokens = Misc::tokenize_delimeter(label, "_");
					label = tokens.back();
					for(int j = 0; j < tokens.size() - 1; j++) label += "|" + tokens.at(j);
				}
				else if(label.find('.') != std::string::npos) {
					std::replace(label.begin(), label.end(), '.', '|');
				}
				thisTrace.name = label;
				switch(label[0]) {
					case 'R':
						thisTrace.type = 'V';
						if(mObj.components.voltRes.count(label) != 0) {
							if(mObj.components.voltRes.at(label).posNCol == -1) {
								thisTrace.traceData = &sObj.results.xVect.at(mObj.components.voltRes.at(label).negNCol);
								traces.push_back(thisTrace);
							}
							else if(mObj.components.voltRes.at(label).negNCol == -1) {
								thisTrace.traceData = &sObj.results.xVect.at(mObj.components.voltRes.at(label).posNCol);
								traces.push_back(thisTrace);
							}
							else {
								std::transform(
									sObj.results.xVect.at(mObj.components.voltRes.at(label).posNCol).begin(),
									sObj.results.xVect.at(mObj.components.voltRes.at(label).posNCol).end(),
									sObj.results.xVect.at(mObj.components.voltRes.at(label).negNCol).begin(),
									std::back_inserter(thisTrace.calcData),
									std::minus<double>());
								thisTrace.pointer = false;
								traces.push_back(thisTrace);
							}
						}
						else if(mObj.components.phaseRes.count(label) != 0) {
							std::cout << "W: Request to print voltage for device " << label << std::endl;
							std::cout << "W: Phase mode simulation performed." << std::endl;
							std::cout << "W: Printing device phase instead." << std::endl;
							std::cout << std::endl;
							if(mObj.components.phaseRes.at(label).posNCol == -1) {
								thisTrace.traceData = &sObj.results.xVect.at(mObj.components.phaseRes.at(label).negNCol);
								traces.push_back(thisTrace);
							}
							else if(mObj.components.phaseRes.at(label).negNCol == -1) {
								thisTrace.traceData = &sObj.results.xVect.at(mObj.components.phaseRes.at(label).posNCol);
								traces.push_back(thisTrace);
							}
							else {
								std::transform(
									sObj.results.xVect.at(mObj.components.phaseRes.at(label).posNCol).begin(),
									sObj.results.xVect.at(mObj.components.phaseRes.at(label).posNCol).end(),
									sObj.results.xVect.at(mObj.components.phaseRes.at(label).negNCol).begin(),
									std::back_inserter(thisTrace.calcData),
									std::minus<double>());
								thisTrace.pointer = false;
								traces.push_back(thisTrace);
							}
						}
						else {
							std::cout << "W: Unknown device " << label << std::endl;
							std::cout << "W: Cannot print voltage for this device." << std::endl;
							std::cout << "W: Ignoring this print request." << std::endl;
							std::cout << std::endl;
						}
						break;
					case 'L':
						thisTrace.type = 'V';
						if(mObj.components.voltInd.count(label) != 0) {
							if(mObj.components.voltInd.at(label).posNCol == -1) {
								thisTrace.traceData = &sObj.results.xVect.at(mObj.components.voltInd.at(label).negNCol);
								traces.push_back(thisTrace);
							}
							else if(mObj.components.voltInd.at(label).negNCol == -1) {
								thisTrace.traceData = &sObj.results.xVect.at(mObj.components.voltInd.at(label).posNCol);
								traces.push_back(thisTrace);
							}
							else {
								std::transform(
									sObj.results.xVect.at(mObj.components.voltInd.at(label).posNCol).begin(),
									sObj.results.xVect.at(mObj.components.voltInd.at(label).posNCol).end(),
									sObj.results.xVect.at(mObj.components.voltInd.at(label).negNCol).begin(),
									std::back_inserter(thisTrace.calcData),
									std::minus<double>());
								thisTrace.pointer = false;
								traces.push_back(thisTrace);
							}
						}
						else if(mObj.components.phaseInd.count(label) != 0) {
							std::cout << "W: Request to print voltage for device " << label << std::endl;
							std::cout << "W: Phase mode simulation performed." << std::endl;
							std::cout << "W: Printing device phase instead." << std::endl;
							std::cout << std::endl;
							if(mObj.components.phaseInd.at(label).posNCol == -1) {
								thisTrace.traceData = &sObj.results.xVect.at(mObj.components.phaseInd.at(label).negNCol);
								traces.push_back(thisTrace);
							}
							else if(mObj.components.phaseInd.at(label).negNCol == -1) {
								thisTrace.traceData = &sObj.results.xVect.at(mObj.components.phaseInd.at(label).posNCol);
								traces.push_back(thisTrace);
							}
							else {
								std::transform(
									sObj.results.xVect.at(mObj.components.phaseInd.at(label).posNCol).begin(),
									sObj.results.xVect.at(mObj.components.phaseInd.at(label).posNCol).end(),
									sObj.results.xVect.at(mObj.components.phaseInd.at(label).negNCol).begin(),
									std::back_inserter(thisTrace.calcData),
									std::minus<double>());
								thisTrace.pointer = false;
								traces.push_back(thisTrace);
							}
						}
						else {
							std::cout << "W: Unknown device " << label << std::endl;
							std::cout << "W: Cannot print voltage for this device." << std::endl;
							std::cout << "W: Ignoring this print request." << std::endl;
							std::cout << std::endl;
						}
						break;
					case 'C':
						thisTrace.type = 'V';
						if(mObj.components.voltCap.count(label) != 0) {
							if(mObj.components.voltCap.at(label).posNCol == -1) {
								thisTrace.traceData = &sObj.results.xVect.at(mObj.components.voltCap.at(label).negNCol);
								traces.push_back(thisTrace);
							}
							else if(mObj.components.voltCap.at(label).negNCol == -1) {
								thisTrace.traceData = &sObj.results.xVect.at(mObj.components.voltCap.at(label).posNCol);
								traces.push_back(thisTrace);
							}
							else {
								std::transform(
									sObj.results.xVect.at(mObj.components.voltCap.at(label).posNCol).begin(),
									sObj.results.xVect.at(mObj.components.voltCap.at(label).posNCol).end(),
									sObj.results.xVect.at(mObj.components.voltCap.at(label).negNCol).begin(),
									std::back_inserter(thisTrace.calcData),
									std::minus<double>());
								thisTrace.pointer = false;
								traces.push_back(thisTrace);
							}
						}
						else if(mObj.components.phaseCap.count(label) != 0) {
							std::cout << "W: Request to print voltage for device " << label << std::endl;
							std::cout << "W: Phase mode simulation performed." << std::endl;
							std::cout << "W: Printing device phase instead." << std::endl;
							std::cout << std::endl;
							if(mObj.components.phaseCap.at(label).posNCol == -1) {
								thisTrace.traceData = &sObj.results.xVect.at(mObj.components.phaseCap.at(label).negNCol);
								traces.push_back(thisTrace);
							}
							else if(mObj.components.phaseCap.at(label).negNCol == -1) {
								thisTrace.traceData = &sObj.results.xVect.at(mObj.components.phaseCap.at(label).posNCol);
								traces.push_back(thisTrace);
							}
							else {
								std::transform(
									sObj.results.xVect.at(mObj.components.phaseCap.at(label).posNCol).begin(),
									sObj.results.xVect.at(mObj.components.phaseCap.at(label).posNCol).end(),
									sObj.results.xVect.at(mObj.components.phaseCap.at(label).negNCol).begin(),
									std::back_inserter(thisTrace.calcData),
									std::minus<double>());
								thisTrace.pointer = false;
								traces.push_back(thisTrace);
							}
						}
						else {
							std::cout << "W: Unknown device " << label << std::endl;
							std::cout << "W: Cannot print voltage for this device." << std::endl;
							std::cout << "W: Ignoring this print request." << std::endl;
							std::cout << std::endl;
						}
						break;
					case 'B':
						thisTrace.type = 'V';
						if(mObj.components.voltJJ.count(label) != 0) {
							if(mObj.components.voltJJ.at(label).posNCol == -1) {
								thisTrace.traceData = &sObj.results.xVect.at(mObj.components.voltJJ.at(label).negNCol);
								traces.push_back(thisTrace);
							}
							else if(mObj.components.voltJJ.at(label).negNCol == -1) {
								thisTrace.traceData = &sObj.results.xVect.at(mObj.components.voltJJ.at(label).posNCol);
								traces.push_back(thisTrace);
							}
							else {
								std::transform(
									sObj.results.xVect.at(mObj.components.voltJJ.at(label).posNCol).begin(),
									sObj.results.xVect.at(mObj.components.voltJJ.at(label).posNCol).end(),
									sObj.results.xVect.at(mObj.components.voltJJ.at(label).negNCol).begin(),
									std::back_inserter(thisTrace.calcData),
									std::minus<double>());
								thisTrace.pointer = false;
								traces.push_back(thisTrace);
							}
						}
						else if(mObj.components.phaseJJ.count(label) != 0) {
							std::cout << "W: Request to print voltage for device " << label << std::endl;
							std::cout << "W: Phase mode simulation performed." << std::endl;
							std::cout << "W: Printing device phase instead." << std::endl;
							std::cout << std::endl;
							if(mObj.components.phaseJJ.at(label).posNCol == -1) {
								thisTrace.traceData = &sObj.results.xVect.at(mObj.components.phaseJJ.at(label).negNCol);
								traces.push_back(thisTrace);
							}
							else if(mObj.components.phaseJJ.at(label).negNCol == -1) {
								thisTrace.traceData = &sObj.results.xVect.at(mObj.components.phaseJJ.at(label).posNCol);
								traces.push_back(thisTrace);
							}
							else {
								std::transform(
									sObj.results.xVect.at(mObj.components.phaseJJ.at(label).posNCol).begin(),
									sObj.results.xVect.at(mObj.components.phaseJJ.at(label).posNCol).end(),
									sObj.results.xVect.at(mObj.components.phaseJJ.at(label).negNCol).begin(),
									std::back_inserter(thisTrace.calcData),
									std::minus<double>());
								thisTrace.pointer = false;
								traces.push_back(thisTrace);
							}
						}
						else {
							std::cout << "W: Unknown device " << label << std::endl;
							std::cout << "W: Cannot print voltage for this device." << std::endl;
							std::cout << "W: Ignoring this print request." << std::endl;
							std::cout << std::endl;
						}
						break;
					case 'V':
						thisTrace.type = 'V';
						if(mObj.sources.count(label) != 0) {
							thisTrace.traceData = &mObj.sources.at(label);
							traces.push_back(thisTrace);
						}
						else {
							std::cout << "W: Unknown device " << label << std::endl;
							std::cout << "W: Cannot print voltage for this device." << std::endl;
							std::cout << "W: Ignoring this print request." << std::endl;
							std::cout << std::endl;
						}
						break;
					case 'I':
						std::cout << "W: Requesting voltage across a current source." << std::endl;
						std::cout << "W: Line: " << i << std::endl;
						std::cout << "W: This is invalid and the request will be ignored." << std::endl;
						std::cout << std::endl;
						break;
				}
			}
			else if(tokens.at(1) == "NODEV") {
				thisTrace.type = 'V';
				if (tokens.size() > 4) {
					std::cout << "W: Print requested with too many arguments." << std::endl;
					std::cout << "W: Line: " << i << std::endl;
					std::cout << "W: Excess arguments will be ignored." << std::endl;
					std::cout << std::endl;
				}
				else if (tokens.size() == 3) {
					label = tokens.at(2);
					if(label.find('_') != std::string::npos) {
						tokens = Misc::tokenize_delimeter(label, "_");
						label = tokens.back();
						for(int j = 0; j < tokens.size() - 1; j++) label += "|" + tokens.at(j);
					}
					else if(label.find('.') != std::string::npos) {
						std::replace(label.begin(), label.end(), '.', '|');
					}
					if(std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NV" + label) != mObj.columnNames.end()) {
						index1 = std::distance(mObj.columnNames.begin(), std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NV" + label));
						thisTrace.name = "NV_" + label;
						thisTrace.traceData = &sObj.results.xVect.at(index1);
						traces.push_back(thisTrace);
					}
					else if(std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NP" + label) != mObj.columnNames.end()) {
						std::cout << "W: Request to print nodal voltage for " << label << std::endl;
						std::cout << "W: Phase mode simulation performed." << std::endl;
						std::cout << "W: Printing nodal phase instead." << std::endl;
						std::cout << std::endl;
						thisTrace.type = 'P';
						index1 = std::distance(mObj.columnNames.begin(), std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NP" + label));
						thisTrace.name = "NP_" + label;
						thisTrace.traceData = &sObj.results.xVect.at(index1);
						traces.push_back(thisTrace);
					}
					else {
						std::cout << "W: Node " << label << " was not found in the circuit." << std::endl;
						std::cout << "W: This request for print will be ignored." << std::endl;
						std::cout << std::endl;
					}
				}
				else if (tokens.size() == 4) {
					label = tokens.at(2);
					label2 = tokens.at(3);
					if(label.find('_') != std::string::npos) {
						tokens = Misc::tokenize_delimeter(label, "_");
						label = tokens.back();
						for(int j = 0; j < tokens.size() - 1; j++) label += "|" + tokens.at(j);
					}
					else if(label.find('.') != std::string::npos) {
						std::replace(label.begin(), label.end(), '.', '|');
					}
					if(label2.find('_') != std::string::npos) {
						tokens = Misc::tokenize_delimeter(label2, "_");
						label2 = tokens.back();
						for(int j = 0; j < tokens.size() - 1; j++) label2 = label + "|" + tokens.at(j);
					}
					else if(label2.find('.') != std::string::npos) {
						std::replace(label2.begin(), label2.end(), '.', '|');
					}
					if(label == "0" || label == "GND") {
						if(std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NV" + label2) != mObj.columnNames.end()) {
							index1 = std::distance(mObj.columnNames.begin(), std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NV" + label2));
							thisTrace.name = "NV_" + label + "_" + label2;
							std::transform(
									sObj.results.xVect.at(index1).begin(),
									sObj.results.xVect.at(index1).end(),
									std::back_inserter(thisTrace.calcData),
									std::bind(std::multiplies<double>(),
										std::placeholders::_1,
										-1.0));
							thisTrace.pointer = false;
							traces.push_back(thisTrace);
						}
						else if(std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NP" + label2) != mObj.columnNames.end()) {
							std::cout << "W: Request to print nodal voltage from " << label << " to " << label2 << std::endl;
							std::cout << "W: Phase mode simulation performed." << std::endl;
							std::cout << "W: Printing nodal phase instead." << std::endl;
							std::cout << std::endl;
							thisTrace.type = 'P';
							index1 = std::distance(mObj.columnNames.begin(), std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NP" + label2));
							thisTrace.name = "NP_" + label + "_" + label2;
							std::transform(
									sObj.results.xVect.at(index1).begin(),
									sObj.results.xVect.at(index1).end(),
									std::back_inserter(thisTrace.calcData),
									std::bind(std::multiplies<double>(),
										std::placeholders::_1,
										-1.0));
							thisTrace.pointer = false;
							traces.push_back(thisTrace);
						}
						else {
							std::cout << "W: Node " << label2 << " was not found in the circuit." << std::endl;
							std::cout << "W: This request for print will be ignored." << std::endl;
							std::cout << std::endl;
						}
					}
					else if (label2 == "0" || label2 == "GND") {
						if(std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NV" + label) != mObj.columnNames.end()) {
							index1 = std::distance(mObj.columnNames.begin(), std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NV" + label));
							thisTrace.name = "NV_" + label + "_" + label2;
							thisTrace.traceData = &sObj.results.xVect.at(index1);
							traces.push_back(thisTrace);
						}
						else if(std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NP" + label) != mObj.columnNames.end()) {
							std::cout << "W: Request to print nodal voltage from " << label << " to " << label2 << std::endl;
							std::cout << "W: Phase mode simulation performed." << std::endl;
							std::cout << "W: Printing nodal phase instead." << std::endl;
							std::cout << std::endl;
							thisTrace.type = 'P';
							index1 = std::distance(mObj.columnNames.begin(), std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NP" + label));
							thisTrace.name = "NP_" + label + "_" + label2;
							thisTrace.traceData = &sObj.results.xVect.at(index1);
							traces.push_back(thisTrace);
						}
						else {
							std::cout << "W: Node " << label << " was not found in the circuit." << std::endl;
							std::cout << "W: This request for print will be ignored." << std::endl;
							std::cout << std::endl;
						}
					}
					else {
						if(std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NV" + label) == mObj.columnNames.end()) {
							if(std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NP" + label) == mObj.columnNames.end()) {
								std::cout << "W: Node " << label << " was not found in the circuit." << std::endl;
								std::cout << "W: This request for print will be ignored." << std::endl;
								std::cout << std::endl;
							}
						}
						else if(std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NV" + label2) == mObj.columnNames.end()) {
							if(std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NP" + label2) == mObj.columnNames.end()) {
								std::cout << "W: Node " << label2 << " was not found in the circuit." << std::endl;
								std::cout << "W: This request for print will be ignored." << std::endl;
								std::cout << std::endl;
							}
						}
						else if(std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NV" + label) != mObj.columnNames.end()) {
							index1 = std::distance(mObj.columnNames.begin(), std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NV" + label));
							index2 = std::distance(mObj.columnNames.begin(), std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NV" + label2));
							thisTrace.name = "NV_" + label + "_" + label2;
							std::transform(
									sObj.results.xVect.at(index1).begin(),
									sObj.results.xVect.at(index1).end(),
									sObj.results.xVect.at(index2).begin(),
									std::back_inserter(thisTrace.calcData),
									std::minus<double>());
							thisTrace.pointer = false;
							traces.push_back(thisTrace);
						}
						else if(std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NP" + label) != mObj.columnNames.end()) {
							std::cout << "W: Request to print nodal voltage from " << label << " to " << label2 << std::endl;
							std::cout << "W: Phase mode simulation performed." << std::endl;
							std::cout << "W: Printing nodal phase instead." << std::endl;
							std::cout << std::endl;
							thisTrace.type = 'P';
							index1 = std::distance(mObj.columnNames.begin(), std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NP" + label));
							index2 = std::distance(mObj.columnNames.begin(), std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NP" + label2));
							thisTrace.name = "NP_" + label + "_" + label2;
							std::transform(
									sObj.results.xVect.at(index1).begin(),
									sObj.results.xVect.at(index1).end(),
									sObj.results.xVect.at(index2).begin(),
									std::back_inserter(thisTrace.calcData),
									std::minus<double>());
							thisTrace.pointer = false;
							traces.push_back(thisTrace);
						}
					}
				}
				else {
					std::cout << "W: Invalid node voltage request found." << std::endl;
					std::cout << "W: Line: " << i << std::endl;
					std::cout << "W: This request for print will be ignored." << std::endl;
					std::cout << std::endl;
				}
			}
			else if(tokens.at(1) == "NODEP") {
				thisTrace.type = 'P';
				if (tokens.size() > 4) {
					std::cout << "W: Print requested with too many arguments." << std::endl;
					std::cout << "W: Line: " << i << std::endl;
					std::cout << "W: Excess arguments will be ignored." << std::endl;
					std::cout << std::endl;
				}
				else if (tokens.size() == 3) {
					label = tokens.at(2);
					if(label.find('_') != std::string::npos) {
						tokens = Misc::tokenize_delimeter(label, "_");
						label = tokens.back();
						for(int j = 0; j < tokens.size() - 1; j++) label += "|" + tokens.at(j);
					}
					else if(label.find('.') != std::string::npos) {
						std::replace(label.begin(), label.end(), '.', '|');
					}
					if(std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NP" + label) != mObj.columnNames.end()) {
						index1 = std::distance(mObj.columnNames.begin(), std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NP" + label));
						thisTrace.name = "NP_" + label;
						thisTrace.traceData = &sObj.results.xVect.at(index1);
						traces.push_back(thisTrace);
					}
					else if(std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NV" + label) != mObj.columnNames.end()) {
						std::cout << "W: Request to print nodal phase for " << label << std::endl;
						std::cout << "W: Voltage mode simulation performed." << std::endl;
						std::cout << "W: Printing nodal voltage instead." << std::endl;
						std::cout << std::endl;
						thisTrace.type = 'V';
						index1 = std::distance(mObj.columnNames.begin(), std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NV" + label));
						thisTrace.name = "NV_" + label;
						thisTrace.traceData = &sObj.results.xVect.at(index1);
						traces.push_back(thisTrace);
					}
					else {
						std::cout << "W: Node " << label << " was not found in the circuit." << std::endl;
						std::cout << "W: This request for print will be ignored." << std::endl;
						std::cout << std::endl;
					}
				}
				else if (tokens.size() == 4) {
					label = tokens.at(2);
					label2 = tokens.at(3);
					if(label.find('_') != std::string::npos) {
						tokens = Misc::tokenize_delimeter(label, "_");
						label = tokens.back();
						for(int j = 0; j < tokens.size() - 1; j++) label += "|" + tokens.at(j);
					}
					else if(label.find('.') != std::string::npos) {
						std::replace(label.begin(), label.end(), '.', '|');
					}
					if(label2.find('_') != std::string::npos) {
						tokens = Misc::tokenize_delimeter(label2, "_");
						label2 = tokens.back();
						for(int j = 0; j < tokens.size() - 1; j++) label2 = label + "|" + tokens.at(j);
					}
					else if(label2.find('.') != std::string::npos) {
						std::replace(label2.begin(), label2.end(), '.', '|');
					}
					if(label == "0" || label == "GND") {
						if(std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NP" + label2) != mObj.columnNames.end()) {
							index1 = std::distance(mObj.columnNames.begin(), std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NP" + label2));
							thisTrace.name = "NP_" + label + "_" + label2;
							std::transform(
									sObj.results.xVect.at(index1).begin(),
									sObj.results.xVect.at(index1).end(),
									std::back_inserter(thisTrace.calcData),
									std::bind(std::multiplies<double>(),
										std::placeholders::_1,
										-1.0));
							thisTrace.pointer = false;
							traces.push_back(thisTrace);
						}
						else if(std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NV" + label2) != mObj.columnNames.end()) {
							std::cout << "W: Request to print nodal phase from " << label << " to " << label2 << std::endl;
							std::cout << "W: Voltage mode simulation performed." << std::endl;
							std::cout << "W: Printing nodal voltage instead." << std::endl;
							std::cout << std::endl;
							thisTrace.type = 'V';
							index1 = std::distance(mObj.columnNames.begin(), std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NV" + label2));
							thisTrace.name = "NV_" + label + "_" + label2;
							std::transform(
									sObj.results.xVect.at(index1).begin(),
									sObj.results.xVect.at(index1).end(),
									std::back_inserter(thisTrace.calcData),
									std::bind(std::multiplies<double>(),
										std::placeholders::_1,
										-1.0));
							thisTrace.pointer = false;
							traces.push_back(thisTrace);
						}
						else {
							std::cout << "W: Node " << label2 << " was not found in the circuit." << std::endl;
							std::cout << "W: This request for print will be ignored." << std::endl;
							std::cout << std::endl;
						}
					}
					else if (label2 == "0" || label2 == "GND") {
						if(std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NP" + label) != mObj.columnNames.end()) {
							index1 = std::distance(mObj.columnNames.begin(), std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NP" + label));
							thisTrace.name = "NP_" + label + "_" + label2;
							thisTrace.traceData = &sObj.results.xVect.at(index1);
							traces.push_back(thisTrace);
						}
						else if(std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NV" + label) != mObj.columnNames.end()) {
							std::cout << "W: Request to print nodal phase from " << label << " to " << label2 << std::endl;
							std::cout << "W: Voltage mode simulation performed." << std::endl;
							std::cout << "W: Printing nodal voltage instead." << std::endl;
							std::cout << std::endl;
							thisTrace.type = 'V';
							index1 = std::distance(mObj.columnNames.begin(), std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NV" + label));
							thisTrace.name = "NV_" + label + "_" + label2;
							thisTrace.traceData = &sObj.results.xVect.at(index1);
							traces.push_back(thisTrace);
						}
						else {
							std::cout << "W: Node " << label << " was not found in the circuit." << std::endl;
							std::cout << "W: This request for print will be ignored." << std::endl;
							std::cout << std::endl;
						}
					}
					else {
						if(std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NV" + label) == mObj.columnNames.end()) {
							if(std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NP" + label) == mObj.columnNames.end()) {
								std::cout << "W: Node " << label << " was not found in the circuit." << std::endl;
								std::cout << "W: This request for print will be ignored." << std::endl;
								std::cout << std::endl;
							}
						}
						else if(std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NV" + label2) == mObj.columnNames.end()) {
							if(std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NP" + label2) == mObj.columnNames.end()) {
								std::cout << "W: Node " << label2 << " was not found in the circuit." << std::endl;
								std::cout << "W: This request for print will be ignored." << std::endl;
								std::cout << std::endl;
							}
						}
						else if(std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NV" + label) != mObj.columnNames.end()) {
							std::cout << "W: Request to print nodal phase from " << label << " to " << label2 << std::endl;
							std::cout << "W: Voltage mode simulation performed." << std::endl;
							std::cout << "W: Printing nodal voltage instead." << std::endl;
							std::cout << std::endl;
							thisTrace.type = 'V';
							index1 = std::distance(mObj.columnNames.begin(), std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NV" + label));
							index2 = std::distance(mObj.columnNames.begin(), std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NV" + label2));
							thisTrace.name = "NV_" + label + "_" + label2;
							std::transform(
									sObj.results.xVect.at(index1).begin(),
									sObj.results.xVect.at(index1).end(),
									sObj.results.xVect.at(index2).begin(),
									std::back_inserter(thisTrace.calcData),
									std::minus<double>());
							thisTrace.pointer = false;
							traces.push_back(thisTrace);
						}
						else if(std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NP" + label) != mObj.columnNames.end()) {
							index1 = std::distance(mObj.columnNames.begin(), std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NP" + label));
							index2 = std::distance(mObj.columnNames.begin(), std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NP" + label2));
							thisTrace.name = "NP_" + label + "_" + label2;
							std::transform(
									sObj.results.xVect.at(index1).begin(),
									sObj.results.xVect.at(index1).end(),
									sObj.results.xVect.at(index2).begin(),
									std::back_inserter(thisTrace.calcData),
									std::minus<double>());
							thisTrace.pointer = false;
							traces.push_back(thisTrace);
						}
					}
				}
				else {
					std::cout << "W: Invalid node voltage request found." << std::endl;
					std::cout << "W: Line: " << i << std::endl;
					std::cout << "W: This request for print will be ignored." << std::endl;
					std::cout << std::endl;
				}
			}
			else if(tokens.at(1) == "PHASE") {
				if(tokens.size() > 4) {
					std::cout << "W: Print request for device current has too many arguments. " << std::endl;
					std::cout << "W: Line: " << i << std::endl;
					std::cout << "W: Ignoring the extra argument." << std::endl;
					std::cout << std::endl;
				}
				label = tokens.at(2);
				if(label.find('_') != std::string::npos) {
					tokens = Misc::tokenize_delimeter(label, "_");
					label = tokens.back();
					for(int j = 0; j < tokens.size() - 1; j++) label += "|" + tokens.at(j);
				}
				else if(label.find('.') != std::string::npos) {
					std::replace(label.begin(), label.end(), '.', '|');
				}
				thisTrace.name = label;
				switch(label[0]) {
					case 'R':
						thisTrace.type = 'P';
						if(mObj.components.voltRes.count(label) != 0) {
							std::cout << "W: Requesting phase in a voltage simulation." << std::endl;
							std::cout << "W: Line: " << i << std::endl;
							std::cout << "W: This request will be ignored." << std::endl;
							std::cout << std::endl;
						}
						else if(mObj.components.phaseRes.count(label) != 0) {
							if(mObj.components.phaseRes.at(label).posNCol == -1) {
								thisTrace.traceData = &sObj.results.xVect.at(mObj.components.phaseRes.at(label).negNCol);
								traces.push_back(thisTrace);
							}
							else if(mObj.components.phaseRes.at(label).negNCol == -1) {
								thisTrace.traceData = &sObj.results.xVect.at(mObj.components.phaseRes.at(label).posNCol);
								traces.push_back(thisTrace);
							}
							else {
								std::transform(
									sObj.results.xVect.at(mObj.components.phaseRes.at(label).posNCol).begin(),
									sObj.results.xVect.at(mObj.components.phaseRes.at(label).posNCol).end(),
									sObj.results.xVect.at(mObj.components.phaseRes.at(label).negNCol).begin(),
									std::back_inserter(thisTrace.calcData),
									std::minus<double>());
								thisTrace.pointer = false;
								traces.push_back(thisTrace);
							}
						}
						else {
							std::cout << "W: Unknown device " << label << std::endl;
							std::cout << "W: Cannot print phase for this device." << std::endl;
							std::cout << "W: Ignoring this print request." << std::endl;
							std::cout << std::endl;
						}
						break;
					case 'L':
						thisTrace.type = 'P';
						if(mObj.components.voltInd.count(label) != 0) {
							std::cout << "W: Requesting phase in a voltage simulation." << std::endl;
							std::cout << "W: Line: " << i << std::endl;
							std::cout << "W: This request will be ignored." << std::endl;
							std::cout << std::endl;
						}
						else if(mObj.components.phaseInd.count(label) != 0) {
							if(mObj.components.phaseInd.at(label).posNCol == -1) {
								thisTrace.traceData = &sObj.results.xVect.at(mObj.components.phaseInd.at(label).negNCol);
								traces.push_back(thisTrace);
							}
							else if(mObj.components.phaseInd.at(label).negNCol == -1) {
								thisTrace.traceData = &sObj.results.xVect.at(mObj.components.phaseInd.at(label).posNCol);
								traces.push_back(thisTrace);
							}
							else {
								std::transform(
									sObj.results.xVect.at(mObj.components.phaseInd.at(label).posNCol).begin(),
									sObj.results.xVect.at(mObj.components.phaseInd.at(label).posNCol).end(),
									sObj.results.xVect.at(mObj.components.phaseInd.at(label).negNCol).begin(),
									std::back_inserter(thisTrace.calcData),
									std::minus<double>());
								thisTrace.pointer = false;
								traces.push_back(thisTrace);
							}
						}
						else {
							std::cout << "W: Unknown device " << label << std::endl;
							std::cout << "W: Cannot print phase for this device." << std::endl;
							std::cout << "W: Ignoring this print request." << std::endl;
							std::cout << std::endl;
						}
						break;
					case 'C':
						thisTrace.type = 'P';
						if(mObj.components.voltCap.count(label) != 0) {
							std::cout << "W: Requesting phase in a voltage simulation." << std::endl;
							std::cout << "W: Line: " << i << std::endl;
							std::cout << "W: This request will be ignored." << std::endl;
							std::cout << std::endl;
						}
						else if(mObj.components.phaseCap.count(label) != 0) {
							if(mObj.components.phaseCap.at(label).posNCol == -1) {
								thisTrace.traceData = &sObj.results.xVect.at(mObj.components.phaseCap.at(label).negNCol);
								traces.push_back(thisTrace);
							}
							else if(mObj.components.phaseCap.at(label).negNCol == -1) {
								thisTrace.traceData = &sObj.results.xVect.at(mObj.components.phaseCap.at(label).posNCol);
								traces.push_back(thisTrace);
							}
							else {
								std::transform(
									sObj.results.xVect.at(mObj.components.phaseCap.at(label).posNCol).begin(),
									sObj.results.xVect.at(mObj.components.phaseCap.at(label).posNCol).end(),
									sObj.results.xVect.at(mObj.components.phaseCap.at(label).negNCol).begin(),
									std::back_inserter(thisTrace.calcData),
									std::minus<double>());
								thisTrace.pointer = false;
								traces.push_back(thisTrace);
							}
						}
						else {
							std::cout << "W: Unknown device " << label << std::endl;
							std::cout << "W: Cannot print phase for this device." << std::endl;
							std::cout << "W: Ignoring this print request." << std::endl;
							std::cout << std::endl;
						}
						break;
					case 'B':
						thisTrace.type = 'P';
						if(mObj.components.voltJJ.count(label) != 0) {
							thisTrace.traceData = &sObj.results.xVect.at(mObj.components.voltJJ.at(label).phaseNCol);
							traces.push_back(thisTrace);
						}
						else if(mObj.components.phaseJJ.count(label) != 0) {
							if(mObj.components.phaseJJ.at(label).posNCol == -1) {
								thisTrace.traceData = &sObj.results.xVect.at(mObj.components.phaseJJ.at(label).negNCol);
								traces.push_back(thisTrace);
							}
							else if(mObj.components.phaseJJ.at(label).negNCol == -1) {
								thisTrace.traceData = &sObj.results.xVect.at(mObj.components.phaseJJ.at(label).posNCol);
								traces.push_back(thisTrace);
							}
							else {
								std::transform(
									sObj.results.xVect.at(mObj.components.phaseJJ.at(label).posNCol).begin(),
									sObj.results.xVect.at(mObj.components.phaseJJ.at(label).posNCol).end(),
									sObj.results.xVect.at(mObj.components.phaseJJ.at(label).negNCol).begin(),
									std::back_inserter(thisTrace.calcData),
									std::minus<double>());
								thisTrace.pointer = false;
								traces.push_back(thisTrace);
							}
						}
						else {
							std::cout << "W: Unknown device " << label << std::endl;
							std::cout << "W: Cannot print phase for this device." << std::endl;
							std::cout << "W: Ignoring this print request." << std::endl;
							std::cout << std::endl;
						}
						break;
					case 'V':
						std::cout << "W: Requesting phase of a voltage source." << std::endl;
						std::cout << "W: Line: " << i << std::endl;
						std::cout << "W: This is invalid and the request will be ignored." << std::endl;
						std::cout << std::endl;
						break;
					case 'P':
						thisTrace.type = 'P';
						if(mObj.sources.count(label) != 0) {
							thisTrace.traceData = &mObj.sources.at(label);
							traces.push_back(thisTrace);
						}
						else {
							std::cout << "W: Unknown device " << label << std::endl;
							std::cout << "W: Cannot print phase for this device." << std::endl;
							std::cout << "W: Ignoring this print request." << std::endl;
							std::cout << std::endl;
						}
						break;
					case 'I':
						std::cout << "W: Requesting phase of a current source." << std::endl;
						std::cout << "W: Line: " << i << std::endl;
						std::cout << "W: This is invalid and the request will be ignored." << std::endl;
						std::cout << std::endl;
						break;
				}
			}
		}
		else if(i.find("PLOT") != std::string::npos) {
			tokens = Misc::tokenize_space(i);
			for (int j = 1; j < tokens.size(); j++) {
				Trace thisTrace;
				if (tokens.at(j)[0] == 'V') {
					thisTrace.type = 'V';
					tokens2 = Misc::tokenize_delimeter(tokens.at(j), "V() ,");
					if(tokens2.size() == 1) {
						label = tokens2.at(0);
						if(label.find('_') != std::string::npos) {
							tokens2 = Misc::tokenize_delimeter(label, "_");
							label = tokens2.back();
							for(int k = 0; k < tokens2.size() - 1; k++) label += "|" + tokens2.at(k);
						}
						else if(label.find('.') != std::string::npos) {
							std::replace(label.begin(), label.end(), '.', '|');
						}
						thisTrace.name = label;
						switch(label[0]) {
							case 'R':
								thisTrace.type = 'V';
								if(mObj.components.voltRes.count(label) != 0) {
									if(mObj.components.voltRes.at(label).posNCol == -1) {
										thisTrace.traceData = &sObj.results.xVect.at(mObj.components.voltRes.at(label).negNCol);
										traces.push_back(thisTrace);
									}
									else if(mObj.components.voltRes.at(label).negNCol == -1) {
										thisTrace.traceData = &sObj.results.xVect.at(mObj.components.voltRes.at(label).posNCol);
										traces.push_back(thisTrace);
									}
									else {
										std::transform(
											sObj.results.xVect.at(mObj.components.voltRes.at(label).posNCol).begin(),
											sObj.results.xVect.at(mObj.components.voltRes.at(label).posNCol).end(),
											sObj.results.xVect.at(mObj.components.voltRes.at(label).negNCol).begin(),
											std::back_inserter(thisTrace.calcData),
											std::minus<double>());
										thisTrace.pointer = false;
										traces.push_back(thisTrace);
									}
								}
								else if(mObj.components.phaseRes.count(label) != 0) {
									std::cout << "W: Request to print voltage for device " << label << std::endl;
									std::cout << "W: Phase mode simulation performed." << std::endl;
									std::cout << "W: Printing device phase instead." << std::endl;
									std::cout << std::endl;
									if(mObj.components.phaseRes.at(label).posNCol == -1) {
										thisTrace.traceData = &sObj.results.xVect.at(mObj.components.phaseRes.at(label).negNCol);
										traces.push_back(thisTrace);
									}
									else if(mObj.components.phaseRes.at(label).negNCol == -1) {
										thisTrace.traceData = &sObj.results.xVect.at(mObj.components.phaseRes.at(label).posNCol);
										traces.push_back(thisTrace);
									}
									else {
										std::transform(
											sObj.results.xVect.at(mObj.components.phaseRes.at(label).posNCol).begin(),
											sObj.results.xVect.at(mObj.components.phaseRes.at(label).posNCol).end(),
											sObj.results.xVect.at(mObj.components.phaseRes.at(label).negNCol).begin(),
											std::back_inserter(thisTrace.calcData),
											std::minus<double>());
										thisTrace.pointer = false;
										traces.push_back(thisTrace);
									}
								}
								else {
									std::cout << "W: Unknown device " << label << std::endl;
									std::cout << "W: Cannot print voltage for this device." << std::endl;
									std::cout << "W: Ignoring this print request." << std::endl;
									std::cout << std::endl;
								}
								break;
							case 'L':
								thisTrace.type = 'V';
								if(mObj.components.voltInd.count(label) != 0) {
									if(mObj.components.voltInd.at(label).posNCol == -1) {
										thisTrace.traceData = &sObj.results.xVect.at(mObj.components.voltInd.at(label).negNCol);
										traces.push_back(thisTrace);
									}
									else if(mObj.components.voltInd.at(label).negNCol == -1) {
										thisTrace.traceData = &sObj.results.xVect.at(mObj.components.voltInd.at(label).posNCol);
										traces.push_back(thisTrace);
									}
									else {
										std::transform(
											sObj.results.xVect.at(mObj.components.voltInd.at(label).posNCol).begin(),
											sObj.results.xVect.at(mObj.components.voltInd.at(label).posNCol).end(),
											sObj.results.xVect.at(mObj.components.voltInd.at(label).negNCol).begin(),
											std::back_inserter(thisTrace.calcData),
											std::minus<double>());
										thisTrace.pointer = false;
										traces.push_back(thisTrace);
									}
								}
								else if(mObj.components.phaseInd.count(label) != 0) {
									std::cout << "W: Request to print voltage for device " << label << std::endl;
									std::cout << "W: Phase mode simulation performed." << std::endl;
									std::cout << "W: Printing device phase instead." << std::endl;
									std::cout << std::endl;
									if(mObj.components.phaseInd.at(label).posNCol == -1) {
										thisTrace.traceData = &sObj.results.xVect.at(mObj.components.phaseInd.at(label).negNCol);
										traces.push_back(thisTrace);
									}
									else if(mObj.components.phaseInd.at(label).negNCol == -1) {
										thisTrace.traceData = &sObj.results.xVect.at(mObj.components.phaseInd.at(label).posNCol);
										traces.push_back(thisTrace);
									}
									else {
										std::transform(
											sObj.results.xVect.at(mObj.components.phaseInd.at(label).posNCol).begin(),
											sObj.results.xVect.at(mObj.components.phaseInd.at(label).posNCol).end(),
											sObj.results.xVect.at(mObj.components.phaseInd.at(label).negNCol).begin(),
											std::back_inserter(thisTrace.calcData),
											std::minus<double>());
										thisTrace.pointer = false;
										traces.push_back(thisTrace);
									}
								}
								else {
									std::cout << "W: Unknown device " << label << std::endl;
									std::cout << "W: Cannot print voltage for this device." << std::endl;
									std::cout << "W: Ignoring this print request." << std::endl;
									std::cout << std::endl;
								}
								break;
							case 'C':
								thisTrace.type = 'V';
								if(mObj.components.voltCap.count(label) != 0) {
									if(mObj.components.voltCap.at(label).posNCol == -1) {
										thisTrace.traceData = &sObj.results.xVect.at(mObj.components.voltCap.at(label).negNCol);
										traces.push_back(thisTrace);
									}
									else if(mObj.components.voltCap.at(label).negNCol == -1) {
										thisTrace.traceData = &sObj.results.xVect.at(mObj.components.voltCap.at(label).posNCol);
										traces.push_back(thisTrace);
									}
									else {
										std::transform(
											sObj.results.xVect.at(mObj.components.voltCap.at(label).posNCol).begin(),
											sObj.results.xVect.at(mObj.components.voltCap.at(label).posNCol).end(),
											sObj.results.xVect.at(mObj.components.voltCap.at(label).negNCol).begin(),
											std::back_inserter(thisTrace.calcData),
											std::minus<double>());
										thisTrace.pointer = false;
										traces.push_back(thisTrace);
									}
								}
								else if(mObj.components.phaseCap.count(label) != 0) {
									std::cout << "W: Request to print voltage for device " << label << std::endl;
									std::cout << "W: Phase mode simulation performed." << std::endl;
									std::cout << "W: Printing device phase instead." << std::endl;
									std::cout << std::endl;
									if(mObj.components.phaseCap.at(label).posNCol == -1) {
										thisTrace.traceData = &sObj.results.xVect.at(mObj.components.phaseCap.at(label).negNCol);
										traces.push_back(thisTrace);
									}
									else if(mObj.components.phaseCap.at(label).negNCol == -1) {
										thisTrace.traceData = &sObj.results.xVect.at(mObj.components.phaseCap.at(label).posNCol);
										traces.push_back(thisTrace);
									}
									else {
										std::transform(
											sObj.results.xVect.at(mObj.components.phaseCap.at(label).posNCol).begin(),
											sObj.results.xVect.at(mObj.components.phaseCap.at(label).posNCol).end(),
											sObj.results.xVect.at(mObj.components.phaseCap.at(label).negNCol).begin(),
											std::back_inserter(thisTrace.calcData),
											std::minus<double>());
										thisTrace.pointer = false;
										traces.push_back(thisTrace);
									}
								}
								else {
									std::cout << "W: Unknown device " << label << std::endl;
									std::cout << "W: Cannot print voltage for this device." << std::endl;
									std::cout << "W: Ignoring this print request." << std::endl;
									std::cout << std::endl;
								}
								break;
							case 'B':
								thisTrace.type = 'V';
								if(mObj.components.voltJJ.count(label) != 0) {
									if(mObj.components.voltJJ.at(label).posNCol == -1) {
										thisTrace.traceData = &sObj.results.xVect.at(mObj.components.voltJJ.at(label).negNCol);
										traces.push_back(thisTrace);
									}
									else if(mObj.components.voltJJ.at(label).negNCol == -1) {
										thisTrace.traceData = &sObj.results.xVect.at(mObj.components.voltJJ.at(label).posNCol);
										traces.push_back(thisTrace);
									}
									else {
										std::transform(
											sObj.results.xVect.at(mObj.components.voltJJ.at(label).posNCol).begin(),
											sObj.results.xVect.at(mObj.components.voltJJ.at(label).posNCol).end(),
											sObj.results.xVect.at(mObj.components.voltJJ.at(label).negNCol).begin(),
											std::back_inserter(thisTrace.calcData),
											std::minus<double>());
										thisTrace.pointer = false;
										traces.push_back(thisTrace);
									}
								}
								else if(mObj.components.phaseJJ.count(label) != 0) {
									std::cout << "W: Request to print voltage for device " << label << std::endl;
									std::cout << "W: Phase mode simulation performed." << std::endl;
									std::cout << "W: Printing device phase instead." << std::endl;
									std::cout << std::endl;
									if(mObj.components.phaseJJ.at(label).posNCol == -1) {
										thisTrace.traceData = &sObj.results.xVect.at(mObj.components.phaseJJ.at(label).negNCol);
										traces.push_back(thisTrace);
									}
									else if(mObj.components.phaseJJ.at(label).negNCol == -1) {
										thisTrace.traceData = &sObj.results.xVect.at(mObj.components.phaseJJ.at(label).posNCol);
										traces.push_back(thisTrace);
									}
									else {
										std::transform(
											sObj.results.xVect.at(mObj.components.phaseJJ.at(label).posNCol).begin(),
											sObj.results.xVect.at(mObj.components.phaseJJ.at(label).posNCol).end(),
											sObj.results.xVect.at(mObj.components.phaseJJ.at(label).negNCol).begin(),
											std::back_inserter(thisTrace.calcData),
											std::minus<double>());
										thisTrace.pointer = false;
										traces.push_back(thisTrace);
									}
								}
								else {
									std::cout << "W: Unknown device " << label << std::endl;
									std::cout << "W: Cannot print voltage for this device." << std::endl;
									std::cout << "W: Ignoring this print request." << std::endl;
									std::cout << std::endl;
								}
								break;
							case 'V':
								thisTrace.type = 'V';
								if(mObj.sources.count(label) != 0) {
									thisTrace.traceData = &mObj.sources.at(label);
									traces.push_back(thisTrace);
								}
								else {
									std::cout << "W: Unknown device " << label << std::endl;
									std::cout << "W: Cannot print voltage for this device." << std::endl;
									std::cout << "W: Ignoring this print request." << std::endl;
									std::cout << std::endl;
								}
								break;
							case 'I':
								std::cout << "W: Requesting voltage across a current source." << std::endl;
								std::cout << "W: Line: " << i << std::endl;
								std::cout << "W: This is invalid and the request will be ignored." << std::endl;
								std::cout << std::endl;
								break;
							default:
								if(std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NV" + label) != mObj.columnNames.end()) {
									index1 = std::distance(mObj.columnNames.begin(), std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NV" + label));
									thisTrace.name = "NV_" + label;
									thisTrace.traceData = &sObj.results.xVect.at(index1);
									traces.push_back(thisTrace);
								}
								else if(std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NP" + label) != mObj.columnNames.end()) {
									std::cout << "W: Request to print nodal voltage for " << label << std::endl;
									std::cout << "W: Phase mode simulation performed." << std::endl;
									std::cout << "W: Printing nodal phase instead." << std::endl;
									std::cout << std::endl;
									thisTrace.type = 'P';
									index1 = std::distance(mObj.columnNames.begin(), std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NP" + label));
									thisTrace.name = "NP_" + label;
									thisTrace.traceData = &sObj.results.xVect.at(index1);
									traces.push_back(thisTrace);
								}
								else {
									std::cout << "W: Node " << label << " was not found in the circuit." << std::endl;
									std::cout << "W: This request for print will be ignored." << std::endl;
									std::cout << std::endl;
								}
								break;
						}
					}
					else {
						label = tokens2.at(0);
						label2 = tokens2.at(1);
						if(label.find('_') != std::string::npos) {
							tokens2 = Misc::tokenize_delimeter(label, "_");
							label = tokens2.back();
							for(int k = 0; k < tokens2.size() - 1; k++) label += "|" + tokens2.at(k);
						}
						else if(label.find('.') != std::string::npos) {
							std::replace(label.begin(), label.end(), '.', '|');
						}
						if(label2.find('_') != std::string::npos) {
							tokens2 = Misc::tokenize_delimeter(label2, "_");
							label2 = tokens2.back();
							for(int k = 0; k < tokens2.size() - 1; k++) label2 = label + "|" + tokens2.at(k);
						}
						else if(label2.find('.') != std::string::npos) {
							std::replace(label2.begin(), label2.end(), '.', '|');
						}
						if(label == "0" || label == "GND") {
							if(std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NV" + label2) != mObj.columnNames.end()) {
								index1 = std::distance(mObj.columnNames.begin(), std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NV" + label2));
								thisTrace.name = "NV_" + label + "_" + label2;
								std::transform(
										sObj.results.xVect.at(index1).begin(),
										sObj.results.xVect.at(index1).end(),
										std::back_inserter(thisTrace.calcData),
										std::bind(std::multiplies<double>(),
											std::placeholders::_1,
											-1.0));
								thisTrace.pointer = false;
								traces.push_back(thisTrace);
							}
							else if(std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NP" + label2) != mObj.columnNames.end()) {
								std::cout << "W: Request to print nodal voltage from " << label << " to " << label2 << std::endl;
								std::cout << "W: Phase mode simulation performed." << std::endl;
								std::cout << "W: Printing nodal phase instead." << std::endl;
								std::cout << std::endl;
								thisTrace.type = 'P';
								index1 = std::distance(mObj.columnNames.begin(), std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NP" + label2));
								thisTrace.name = "NP_" + label + "_" + label2;
								std::transform(
										sObj.results.xVect.at(index1).begin(),
										sObj.results.xVect.at(index1).end(),
										std::back_inserter(thisTrace.calcData),
										std::bind(std::multiplies<double>(),
											std::placeholders::_1,
											-1.0));
								thisTrace.pointer = false;
								traces.push_back(thisTrace);
							}
							else {
								std::cout << "W: Node " << label2 << " was not found in the circuit." << std::endl;
								std::cout << "W: This request for print will be ignored." << std::endl;
								std::cout << std::endl;
							}
						}
						else if (label2 == "0" || label2 == "GND") {
							if(std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NV" + label) != mObj.columnNames.end()) {
								index1 = std::distance(mObj.columnNames.begin(), std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NV" + label));
								thisTrace.name = "NV_" + label + "_" + label2;
								thisTrace.traceData = &sObj.results.xVect.at(index1);
								traces.push_back(thisTrace);
							}
							else if(std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NP" + label) != mObj.columnNames.end()) {
								std::cout << "W: Request to print nodal voltage from " << label << " to " << label2 << std::endl;
								std::cout << "W: Phase mode simulation performed." << std::endl;
								std::cout << "W: Printing nodal phase instead." << std::endl;
								std::cout << std::endl;
								thisTrace.type = 'P';
								index1 = std::distance(mObj.columnNames.begin(), std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NP" + label));
								thisTrace.name = "NP_" + label + "_" + label2;
								thisTrace.traceData = &sObj.results.xVect.at(index1);
								traces.push_back(thisTrace);
							}
							else {
								std::cout << "W: Node " << label << " was not found in the circuit." << std::endl;
								std::cout << "W: This request for print will be ignored." << std::endl;
								std::cout << std::endl;
							}
						}
						else {
							if(std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NV" + label) == mObj.columnNames.end()) {
								if(std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NP" + label) == mObj.columnNames.end()) {
									std::cout << "W: Node " << label << " was not found in the circuit." << std::endl;
									std::cout << "W: This request for print will be ignored." << std::endl;
									std::cout << std::endl;
								}
							}
							else if(std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NV" + label2) == mObj.columnNames.end()) {
								if(std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NP" + label2) == mObj.columnNames.end()) {
									std::cout << "W: Node " << label2 << " was not found in the circuit." << std::endl;
									std::cout << "W: This request for print will be ignored." << std::endl;
									std::cout << std::endl;
								}
							}
							else if(std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NV" + label) != mObj.columnNames.end()) {
								index1 = std::distance(mObj.columnNames.begin(), std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NV" + label));
								index2 = std::distance(mObj.columnNames.begin(), std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NV" + label2));
								thisTrace.name = "NV_" + label + "_" + label2;
								std::transform(
										sObj.results.xVect.at(index1).begin(),
										sObj.results.xVect.at(index1).end(),
										sObj.results.xVect.at(index2).begin(),
										std::back_inserter(thisTrace.calcData),
										std::minus<double>());
								thisTrace.pointer = false;
								traces.push_back(thisTrace);
							}
							else if(std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NP" + label) != mObj.columnNames.end()) {
								std::cout << "W: Request to print nodal voltage from " << label << " to " << label2 << std::endl;
								std::cout << "W: Phase mode simulation performed." << std::endl;
								std::cout << "W: Printing nodal phase instead." << std::endl;
								std::cout << std::endl;
								thisTrace.type = 'P';
								index1 = std::distance(mObj.columnNames.begin(), std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NP" + label));
								index2 = std::distance(mObj.columnNames.begin(), std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NP" + label2));
								thisTrace.name = "NP_" + label + "_" + label2;
								std::transform(
										sObj.results.xVect.at(index1).begin(),
										sObj.results.xVect.at(index1).end(),
										sObj.results.xVect.at(index2).begin(),
										std::back_inserter(thisTrace.calcData),
										std::minus<double>());
								thisTrace.pointer = false;
								traces.push_back(thisTrace);
							}
						}
					}
				}
				else if (tokens.at(j)[0] == 'C') {
					thisTrace.type = 'C';
					tokens2 = Misc::tokenize_delimeter(tokens.at(j), "C() ,");
					if(tokens2.size() == 1) {
						label = tokens2.at(0);
						if(label.find('_') != std::string::npos) {
							tokens2 = Misc::tokenize_delimeter(label, "_");
							label = tokens2.back();
							for(int k = 0; k < tokens2.size() - 1; k++) label += "|" + tokens2.at(k);
						}
						else if(label.find('.') != std::string::npos) {
							std::replace(label.begin(), label.end(), '.', '|');
						}
						thisTrace.name = label;
						switch(label[0]) {
							case 'R':
								thisTrace.type = 'C';
								if(mObj.components.voltRes.count(label) != 0) {
									if(mObj.components.voltRes.at(label).posNCol == -1) {
										std::transform(
											sObj.results.xVect.at(mObj.components.voltRes.at(label).negNCol).begin(),
											sObj.results.xVect.at(mObj.components.voltRes.at(label).negNCol).end(),
											std::back_inserter(thisTrace.calcData),
											std::bind(std::multiplies<double>(),
												std::placeholders::_1,
												(1 / mObj.components.voltRes.at(label).value)));
									}
									else if(mObj.components.voltRes.at(label).negNCol == -1) {
										std::transform(
											sObj.results.xVect.at(mObj.components.voltRes.at(label).posNCol).begin(),
											sObj.results.xVect.at(mObj.components.voltRes.at(label).posNCol).end(),
											std::back_inserter(thisTrace.calcData),
											std::bind(std::multiplies<double>(),
												std::placeholders::_1,
												(1 / mObj.components.voltRes.at(label).value)));
									}
									else {
										std::transform(
											sObj.results.xVect.at(mObj.components.voltRes.at(label).posNCol).begin(),
											sObj.results.xVect.at(mObj.components.voltRes.at(label).posNCol).end(),
											sObj.results.xVect.at(mObj.components.voltRes.at(label).negNCol).begin(),
											std::back_inserter(thisTrace.calcData),
											std::minus<double>());
										std::transform(
											thisTrace.calcData.begin(),
											thisTrace.calcData.end(),
											thisTrace.calcData.begin(),
											std::bind(std::multiplies<double>(),
												std::placeholders::_1,
												(1 / mObj.components.voltRes.at(label).value)));
									}
									thisTrace.pointer = false;
									traces.push_back(thisTrace);
								}
								else if(mObj.components.phaseRes.count(label) != 0) {
									thisTrace.traceData = &sObj.results.xVect.at(mObj.components.phaseRes.at(label).curNCol);
									traces.push_back(thisTrace);
								}
								else {
									std::cout << "W: Unknown device " << label << std::endl;
									std::cout << "W: Cannot print current for this device." << std::endl;
									std::cout << "W: Ignoring this print request." << std::endl;
									std::cout << std::endl;
								}
								break;
							case 'L':
								thisTrace.type = 'C';
								if(mObj.components.voltInd.count(label) != 0) {
									thisTrace.traceData = &sObj.results.xVect.at(mObj.components.voltInd.at(label).curNCol);
									traces.push_back(thisTrace);
								}
								else if(mObj.components.phaseInd.count(label) != 0) {
									thisTrace.traceData = &sObj.results.xVect.at(mObj.components.phaseInd.at(label).curNCol);
									traces.push_back(thisTrace);
								}
								else {
									std::cout << "W: Unknown device " << label << std::endl;
									std::cout << "W: Cannot print current for this device." << std::endl;
									std::cout << "W: Ignoring this print request." << std::endl;
									std::cout << std::endl;
								}
								break;
							case 'C':
								thisTrace.type = 'C';
								if(mObj.components.voltCap.count(label) != 0) {
									thisTrace.traceData = &sObj.results.xVect.at(mObj.components.voltCap.at(label).curNCol);
									traces.push_back(thisTrace);
								}
								else if(mObj.components.phaseCap.count(label) != 0) {
									thisTrace.traceData = &sObj.results.xVect.at(mObj.components.phaseCap.at(label).curNCol);
									traces.push_back(thisTrace);
								}
								else {
									std::cout << "W: Unknown device " << label << std::endl;
									std::cout << "W: Cannot print current for this device." << std::endl;
									std::cout << "W: Ignoring this print request." << std::endl;
									std::cout << std::endl;
								}
								break;
							case 'B':
								thisTrace.type = 'C';
								if(mObj.components.voltJJ.count(label) != 0) {
									thisTrace.traceData = &mObj.components.voltJJ.at(label).jjCur;
									traces.push_back(thisTrace);
								}
								else if(mObj.components.phaseJJ.count(label) != 0) {
									thisTrace.traceData = &mObj.components.phaseJJ.at(label).jjCur;
									traces.push_back(thisTrace);
								}
								else {
									std::cout << "W: Unknown device " << label << std::endl;
									std::cout << "W: Cannot print current for this device." << std::endl;
									std::cout << "W: Ignoring this print request." << std::endl;
									std::cout << std::endl;
								}
								break;
							case 'V':
								std::cout << "W: Requesting current through a voltage source." << std::endl;
								std::cout << "W: Line: " << i << std::endl;
								std::cout << "W: This is invalid and the request will be ignored." << std::endl;
								std::cout << std::endl;
								break;
							case 'I':
								thisTrace.type = 'C';
								if(mObj.sources.count(label) != 0) {
									thisTrace.traceData = &mObj.sources.at(label);
									traces.push_back(thisTrace);
								}
								else {
									std::cout << "W: Unknown device " << label << std::endl;
									std::cout << "W: Cannot print current for this device." << std::endl;
									std::cout << "W: Ignoring this print request." << std::endl;
									std::cout << std::endl;
								}
								break;
						}
					}
					else {
						std::cout << "W: Invalid request to plot current." << std::endl;
						std::cout << "W: Infringing line: " << i << std::endl;
						std::cout << std::endl;
					}
				}
				else if (tokens.at(j).find("#BRANCH") != std::string::npos) {
					thisTrace.type = 'C';
					tokens2 = Misc::tokenize_delimeter(tokens.at(j), " #");
					label = tokens2.at(0);
					if(label.find('_') != std::string::npos) {
						tokens2 = Misc::tokenize_delimeter(label, "_");
						label = tokens2.back();
						for(int k = 0; k < tokens2.size() - 1; k++) label += "|" + tokens2.at(k);
					}
					else if(label.find('.') != std::string::npos) {
						std::replace(label.begin(), label.end(), '.', '|');
					}
					thisTrace.name = label;
					switch(label[0]) {
						case 'R':
							thisTrace.type = 'C';
							if(mObj.components.voltRes.count(label) != 0) {
								if(mObj.components.voltRes.at(label).posNCol == -1) {
									std::transform(
										sObj.results.xVect.at(mObj.components.voltRes.at(label).negNCol).begin(),
										sObj.results.xVect.at(mObj.components.voltRes.at(label).negNCol).end(),
										std::back_inserter(thisTrace.calcData),
										std::bind(std::multiplies<double>(),
											std::placeholders::_1,
											(1 / mObj.components.voltRes.at(label).value)));
								}
								else if(mObj.components.voltRes.at(label).negNCol == -1) {
									std::transform(
										sObj.results.xVect.at(mObj.components.voltRes.at(label).posNCol).begin(),
										sObj.results.xVect.at(mObj.components.voltRes.at(label).posNCol).end(),
										std::back_inserter(thisTrace.calcData),
										std::bind(std::multiplies<double>(),
											std::placeholders::_1,
											(1 / mObj.components.voltRes.at(label).value)));
								}
								else {
									std::transform(
										sObj.results.xVect.at(mObj.components.voltRes.at(label).posNCol).begin(),
										sObj.results.xVect.at(mObj.components.voltRes.at(label).posNCol).end(),
										sObj.results.xVect.at(mObj.components.voltRes.at(label).negNCol).begin(),
										std::back_inserter(thisTrace.calcData),
										std::minus<double>());
									std::transform(
										thisTrace.calcData.begin(),
										thisTrace.calcData.end(),
										thisTrace.calcData.begin(),
										std::bind(std::multiplies<double>(),
											std::placeholders::_1,
											(1 / mObj.components.voltRes.at(label).value)));
								}
								thisTrace.pointer = false;
								traces.push_back(thisTrace);
							}
							else if(mObj.components.phaseRes.count(label) != 0) {
								thisTrace.traceData = &sObj.results.xVect.at(mObj.components.phaseRes.at(label).curNCol);
								traces.push_back(thisTrace);
							}
							else {
								std::cout << "W: Unknown device " << label << std::endl;
								std::cout << "W: Cannot print current for this device." << std::endl;
								std::cout << "W: Ignoring this print request." << std::endl;
								std::cout << std::endl;
							}
							break;
						case 'L':
							thisTrace.type = 'C';
							if(mObj.components.voltInd.count(label) != 0) {
								thisTrace.traceData = &sObj.results.xVect.at(mObj.components.voltInd.at(label).curNCol);
								traces.push_back(thisTrace);
							}
							else if(mObj.components.phaseInd.count(label) != 0) {
								thisTrace.traceData = &sObj.results.xVect.at(mObj.components.phaseInd.at(label).curNCol);
								traces.push_back(thisTrace);
							}
							else {
								std::cout << "W: Unknown device " << label << std::endl;
								std::cout << "W: Cannot print current for this device." << std::endl;
								std::cout << "W: Ignoring this print request." << std::endl;
								std::cout << std::endl;
							}
							break;
						case 'C':
							thisTrace.type = 'C';
							if(mObj.components.voltCap.count(label) != 0) {
								thisTrace.traceData = &sObj.results.xVect.at(mObj.components.voltCap.at(label).curNCol);
								traces.push_back(thisTrace);
							}
							else if(mObj.components.phaseCap.count(label) != 0) {
								thisTrace.traceData = &sObj.results.xVect.at(mObj.components.phaseCap.at(label).curNCol);
								traces.push_back(thisTrace);
							}
							else {
								std::cout << "W: Unknown device " << label << std::endl;
								std::cout << "W: Cannot print current for this device." << std::endl;
								std::cout << "W: Ignoring this print request." << std::endl;
								std::cout << std::endl;
							}
							break;
						case 'B':
							thisTrace.type = 'C';
							if(mObj.components.voltJJ.count(label) != 0) {
								thisTrace.traceData = &mObj.components.voltJJ.at(label).jjCur;
								traces.push_back(thisTrace);
							}
							else if(mObj.components.phaseJJ.count(label) != 0) {
								thisTrace.traceData = &mObj.components.phaseJJ.at(label).jjCur;
								traces.push_back(thisTrace);
							}
							else {
								std::cout << "W: Unknown device " << label << std::endl;
								std::cout << "W: Cannot print current for this device." << std::endl;
								std::cout << "W: Ignoring this print request." << std::endl;
								std::cout << std::endl;
							}
							break;
						case 'V':
							std::cout << "W: Requesting current through a voltage source." << std::endl;
							std::cout << "W: Line: " << i << std::endl;
							std::cout << "W: This is invalid and the request will be ignored." << std::endl;
							std::cout << std::endl;
							break;
						case 'I':
							thisTrace.type = 'C';
							if(mObj.sources.count(label) != 0) {
								thisTrace.traceData = &mObj.sources.at(label);
								traces.push_back(thisTrace);
							}
							else {
								std::cout << "W: Unknown device " << label << std::endl;
								std::cout << "W: Cannot print current for this device." << std::endl;
								std::cout << "W: Ignoring this print request." << std::endl;
								std::cout << std::endl;
							}
							break;
					}
				}
				else if (tokens.at(j)[0] == 'P') {
					thisTrace.type = 'P';
					tokens2 = Misc::tokenize_delimeter(tokens.at(j), "P() ,");
					if(tokens2.size() == 1) {
						label = tokens2.at(0);
						if(label.find('_') != std::string::npos) {
							tokens2 = Misc::tokenize_delimeter(label, "_");
							label = tokens2.back();
							for(int k = 0; k < tokens2.size() - 1; k++) label += "|" + tokens2.at(k);
						}
						else if(label.find('.') != std::string::npos) {
							std::replace(label.begin(), label.end(), '.', '|');
						}
						thisTrace.name = label;
						switch(label[0]) {
							case 'R':
								thisTrace.type = 'P';
								if(mObj.components.voltRes.count(label) != 0) {
									std::cout << "W: Requesting phase in a voltage simulation." << std::endl;
									std::cout << "W: Line: " << i << std::endl;
									std::cout << "W: This request will be ignored." << std::endl;
									std::cout << std::endl;
								}
								else if(mObj.components.phaseRes.count(label) != 0) {
									if(mObj.components.phaseRes.at(label).posNCol == -1) {
										thisTrace.traceData = &sObj.results.xVect.at(mObj.components.phaseRes.at(label).negNCol);
										traces.push_back(thisTrace);
									}
									else if(mObj.components.phaseRes.at(label).negNCol == -1) {
										thisTrace.traceData = &sObj.results.xVect.at(mObj.components.phaseRes.at(label).posNCol);
										traces.push_back(thisTrace);
									}
									else {
										std::transform(
											sObj.results.xVect.at(mObj.components.phaseRes.at(label).posNCol).begin(),
											sObj.results.xVect.at(mObj.components.phaseRes.at(label).posNCol).end(),
											sObj.results.xVect.at(mObj.components.phaseRes.at(label).negNCol).begin(),
											std::back_inserter(thisTrace.calcData),
											std::minus<double>());
										thisTrace.pointer = false;
										traces.push_back(thisTrace);
									}
								}
								else {
									std::cout << "W: Unknown device " << label << std::endl;
									std::cout << "W: Cannot print phase for this device." << std::endl;
									std::cout << "W: Ignoring this print request." << std::endl;
									std::cout << std::endl;
								}
								break;
							case 'L':
								thisTrace.type = 'P';
								if(mObj.components.voltInd.count(label) != 0) {
									std::cout << "W: Requesting phase in a voltage simulation." << std::endl;
									std::cout << "W: Line: " << i << std::endl;
									std::cout << "W: This request will be ignored." << std::endl;
									std::cout << std::endl;
								}
								else if(mObj.components.phaseInd.count(label) != 0) {
									if(mObj.components.phaseInd.at(label).posNCol == -1) {
										thisTrace.traceData = &sObj.results.xVect.at(mObj.components.phaseInd.at(label).negNCol);
										traces.push_back(thisTrace);
									}
									else if(mObj.components.phaseInd.at(label).negNCol == -1) {
										thisTrace.traceData = &sObj.results.xVect.at(mObj.components.phaseInd.at(label).posNCol);
										traces.push_back(thisTrace);
									}
									else {
										std::transform(
											sObj.results.xVect.at(mObj.components.phaseInd.at(label).posNCol).begin(),
											sObj.results.xVect.at(mObj.components.phaseInd.at(label).posNCol).end(),
											sObj.results.xVect.at(mObj.components.phaseInd.at(label).negNCol).begin(),
											std::back_inserter(thisTrace.calcData),
											std::minus<double>());
										thisTrace.pointer = false;
										traces.push_back(thisTrace);
									}
								}
								else {
									std::cout << "W: Unknown device " << label << std::endl;
									std::cout << "W: Cannot print phase for this device." << std::endl;
									std::cout << "W: Ignoring this print request." << std::endl;
									std::cout << std::endl;
								}
								break;
							case 'C':
								thisTrace.type = 'P';
								if(mObj.components.voltCap.count(label) != 0) {
									std::cout << "W: Requesting phase in a voltage simulation." << std::endl;
									std::cout << "W: Line: " << i << std::endl;
									std::cout << "W: This request will be ignored." << std::endl;
									std::cout << std::endl;
								}
								else if(mObj.components.phaseCap.count(label) != 0) {
									if(mObj.components.phaseCap.at(label).posNCol == -1) {
										thisTrace.traceData = &sObj.results.xVect.at(mObj.components.phaseCap.at(label).negNCol);
										traces.push_back(thisTrace);
									}
									else if(mObj.components.phaseCap.at(label).negNCol == -1) {
										thisTrace.traceData = &sObj.results.xVect.at(mObj.components.phaseCap.at(label).posNCol);
										traces.push_back(thisTrace);
									}
									else {
										std::transform(
											sObj.results.xVect.at(mObj.components.phaseCap.at(label).posNCol).begin(),
											sObj.results.xVect.at(mObj.components.phaseCap.at(label).posNCol).end(),
											sObj.results.xVect.at(mObj.components.phaseCap.at(label).negNCol).begin(),
											std::back_inserter(thisTrace.calcData),
											std::minus<double>());
										thisTrace.pointer = false;
										traces.push_back(thisTrace);
									}
								}
								else {
									std::cout << "W: Unknown device " << label << std::endl;
									std::cout << "W: Cannot print phase for this device." << std::endl;
									std::cout << "W: Ignoring this print request." << std::endl;
									std::cout << std::endl;
								}
								break;
							case 'B':
								thisTrace.type = 'P';
								if(mObj.components.voltJJ.count(label) != 0) {
									thisTrace.traceData = &sObj.results.xVect.at(mObj.components.voltJJ.at(label).phaseNCol);
									traces.push_back(thisTrace);
								}
								else if(mObj.components.phaseJJ.count(label) != 0) {
									if(mObj.components.phaseJJ.at(label).posNCol == -1) {
										thisTrace.traceData = &sObj.results.xVect.at(mObj.components.phaseJJ.at(label).negNCol);
										traces.push_back(thisTrace);
									}
									else if(mObj.components.phaseJJ.at(label).negNCol == -1) {
										thisTrace.traceData = &sObj.results.xVect.at(mObj.components.phaseJJ.at(label).posNCol);
										traces.push_back(thisTrace);
									}
									else {
										std::transform(
											sObj.results.xVect.at(mObj.components.phaseJJ.at(label).posNCol).begin(),
											sObj.results.xVect.at(mObj.components.phaseJJ.at(label).posNCol).end(),
											sObj.results.xVect.at(mObj.components.phaseJJ.at(label).negNCol).begin(),
											std::back_inserter(thisTrace.calcData),
											std::minus<double>());
										thisTrace.pointer = false;
										traces.push_back(thisTrace);
									}
								}
								else {
									std::cout << "W: Unknown device " << label << std::endl;
									std::cout << "W: Cannot print phase for this device." << std::endl;
									std::cout << "W: Ignoring this print request." << std::endl;
									std::cout << std::endl;
								}
								break;
							case 'V':
								std::cout << "W: Requesting phase of a voltage source." << std::endl;
								std::cout << "W: Line: " << i << std::endl;
								std::cout << "W: This is invalid and the request will be ignored." << std::endl;
								std::cout << std::endl;
								break;
							case 'I':
								std::cout << "W: Requesting phase of a current source." << std::endl;
								std::cout << "W: Line: " << i << std::endl;
								std::cout << "W: This is invalid and the request will be ignored." << std::endl;
								std::cout << std::endl;
								break;
							default:
								if(std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NP" + label) != mObj.columnNames.end()) {
									index1 = std::distance(mObj.columnNames.begin(), std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NP" + label));
									thisTrace.name = "NP_" + label;
									thisTrace.traceData = &sObj.results.xVect.at(index1);
									traces.push_back(thisTrace);
								}
								else if(std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NV" + label) != mObj.columnNames.end()) {
									std::cout << "W: Request to print nodal phase for " << label << std::endl;
									std::cout << "W: Voltage mode simulation performed." << std::endl;
									std::cout << "W: Printing nodal voltage instead." << std::endl;
									std::cout << std::endl;
									thisTrace.type = 'V';
									index1 = std::distance(mObj.columnNames.begin(), std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NV" + label));
									thisTrace.name = "NV_" + label;
									thisTrace.traceData = &sObj.results.xVect.at(index1);
									traces.push_back(thisTrace);
								}
								else {
									std::cout << "W: Node " << label << " was not found in the circuit." << std::endl;
									std::cout << "W: This request for print will be ignored." << std::endl;
									std::cout << std::endl;
								}
								break;
						}
					}
					else {
						label = tokens2.at(0);
						label2 = tokens2.at(1);
						if(label.find('_') != std::string::npos) {
							tokens2 = Misc::tokenize_delimeter(label, "_");
							label = tokens2.back();
							for(int k = 0; k < tokens2.size() - 1; k++) label += "|" + tokens2.at(k);
						}
						else if(label.find('.') != std::string::npos) {
							std::replace(label.begin(), label.end(), '.', '|');
						}
						if(label2.find('_') != std::string::npos) {
							tokens2 = Misc::tokenize_delimeter(label2, "_");
							label2 = tokens2.back();
							for(int k = 0; k < tokens2.size() - 1; k++) label2 = label + "|" + tokens2.at(k);
						}
						else if(label2.find('.') != std::string::npos) {
							std::replace(label2.begin(), label2.end(), '.', '|');
						}
						if(label == "0" || label == "GND") {
							if(std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NP" + label2) != mObj.columnNames.end()) {
								index1 = std::distance(mObj.columnNames.begin(), std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NP" + label2));
								thisTrace.name = "NP_" + label + "_" + label2;
								std::transform(
										sObj.results.xVect.at(index1).begin(),
										sObj.results.xVect.at(index1).end(),
										std::back_inserter(thisTrace.calcData),
										std::bind(std::multiplies<double>(),
											std::placeholders::_1,
											-1.0));
								thisTrace.pointer = false;
								traces.push_back(thisTrace);
							}
							else if(std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NV" + label2) != mObj.columnNames.end()) {
								std::cout << "W: Request to print nodal phase from " << label << " to " << label2 << std::endl;
								std::cout << "W: Voltage mode simulation performed." << std::endl;
								std::cout << "W: Printing nodal voltage instead." << std::endl;
								std::cout << std::endl;
								thisTrace.type = 'V';
								index1 = std::distance(mObj.columnNames.begin(), std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NV" + label2));
								thisTrace.name = "NV_" + label + "_" + label2;
								std::transform(
										sObj.results.xVect.at(index1).begin(),
										sObj.results.xVect.at(index1).end(),
										std::back_inserter(thisTrace.calcData),
										std::bind(std::multiplies<double>(),
											std::placeholders::_1,
											-1.0));
								thisTrace.pointer = false;
								traces.push_back(thisTrace);
							}
							else {
								std::cout << "W: Node " << label2 << " was not found in the circuit." << std::endl;
								std::cout << "W: This request for print will be ignored." << std::endl;
								std::cout << std::endl;
							}
						}
						else if (label2 == "0" || label2 == "GND") {
							if(std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NP" + label) != mObj.columnNames.end()) {
								index1 = std::distance(mObj.columnNames.begin(), std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NP" + label));
								thisTrace.name = "NP_" + label + "_" + label2;
								thisTrace.traceData = &sObj.results.xVect.at(index1);
								traces.push_back(thisTrace);
							}
							else if(std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NV" + label) != mObj.columnNames.end()) {
								std::cout << "W: Request to print nodal phase from " << label << " to " << label2 << std::endl;
								std::cout << "W: Voltage mode simulation performed." << std::endl;
								std::cout << "W: Printing nodal voltage instead." << std::endl;
								std::cout << std::endl;
								thisTrace.type = 'V';
								index1 = std::distance(mObj.columnNames.begin(), std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NV" + label));
								thisTrace.name = "NV_" + label + "_" + label2;
								thisTrace.traceData = &sObj.results.xVect.at(index1);
								traces.push_back(thisTrace);
							}
							else {
								std::cout << "W: Node " << label << " was not found in the circuit." << std::endl;
								std::cout << "W: This request for print will be ignored." << std::endl;
								std::cout << std::endl;
							}
						}
						else {
							if(std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NV" + label) == mObj.columnNames.end()) {
								if(std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NP" + label) == mObj.columnNames.end()) {
									std::cout << "W: Node " << label << " was not found in the circuit." << std::endl;
									std::cout << "W: This request for print will be ignored." << std::endl;
									std::cout << std::endl;
								}
							}
							else if(std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NV" + label2) == mObj.columnNames.end()) {
								if(std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NP" + label2) == mObj.columnNames.end()) {
									std::cout << "W: Node " << label2 << " was not found in the circuit." << std::endl;
									std::cout << "W: This request for print will be ignored." << std::endl;
									std::cout << std::endl;
								}
							}
							else if(std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NV" + label) != mObj.columnNames.end()) {
								std::cout << "W: Request to print nodal phase from " << label << " to " << label2 << std::endl;
								std::cout << "W: Voltage mode simulation performed." << std::endl;
								std::cout << "W: Printing nodal voltage instead." << std::endl;
								std::cout << std::endl;
								thisTrace.type = 'V';
								index1 = std::distance(mObj.columnNames.begin(), std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NV" + label));
								index2 = std::distance(mObj.columnNames.begin(), std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NV" + label2));
								thisTrace.name = "NV_" + label + "_" + label2;
								std::transform(
										sObj.results.xVect.at(index1).begin(),
										sObj.results.xVect.at(index1).end(),
										sObj.results.xVect.at(index2).begin(),
										std::back_inserter(thisTrace.calcData),
										std::minus<double>());
								thisTrace.pointer = false;
								traces.push_back(thisTrace);
							}
							else if(std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NP" + label) != mObj.columnNames.end()) {
								index1 = std::distance(mObj.columnNames.begin(), std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NP" + label));
								index2 = std::distance(mObj.columnNames.begin(), std::find(mObj.columnNames.begin(), mObj.columnNames.end(), "C_NP" + label2));
								thisTrace.name = "NP_" + label + "_" + label2;
								std::transform(
										sObj.results.xVect.at(index1).begin(),
										sObj.results.xVect.at(index1).end(),
										sObj.results.xVect.at(index2).begin(),
										std::back_inserter(thisTrace.calcData),
										std::minus<double>());
								thisTrace.pointer = false;
								traces.push_back(thisTrace);
							}
						}
					}
				}
				else if (tokens.at(j).find("TRAN") != std::string::npos) {}
				else if (tokens.at(j)[0] == '"') {
					std::cout << "W: Mathematical operations on output vectors are not yet supported." << std::endl;
					std::cout << "W: Ignoring plotting of " << tokens.at(j) << std::endl;
					std::cout << std::endl;
				}
				else {
					std::cout << "W: Unknown plot type " << i << std::endl;
					std::cout << "W: Ignoring request to plot." << std::endl;
					std::cout << std::endl;
				}
			}
		}
		else if(i.find("SAVE") != std::string::npos) {
			Trace thisTrace;
			tokens = Misc::tokenize_space(i);
			for (int k = 1; k < tokens.size(); k++) {
				index1 = tokens.at(k).find('@');
				if (index1 != std::string::npos) tokens.at(k) = tokens.at(k).substr(0, index1) + tokens.at(k).substr(index1 + 1);
				index1 = tokens.at(k).find(".at(");
				if (index1 != std::string::npos) tokens.at(k) = tokens.at(k).substr(0, index1);
				index1 = tokens.at(k).find('.');
				if (index1 != std::string::npos) {
					tokens.at(k) = tokens.at(k).substr(0, index1) + "|" + tokens.at(k).substr(index1 + 1);
				}
				index1 = tokens.at(k).find('[');
				if (index1 != std::string::npos) tokens.at(k) = tokens.at(k).substr(0, index1);
				/* If this is a current source */
				if (mObj.sources.count(tokens.at(k)) != 0) {
					thisTrace.type = 'C';
					thisTrace.name = tokens.at(k);
					thisTrace.traceData = &mObj.sources.at(tokens.at(k));
					traces.push_back(thisTrace);
				}
			}
		}
	}

}

void
Output::write_data(std::string &outname) {
	int loopsize = 0;
	std::ofstream outfile(outname);
	if (outfile.is_open()) {
		if (!traces.empty()) {
			if(traces.at(0).pointer)
				loopsize = traces.at(0).traceData->size();
			else
				loopsize = traces.at(0).calcData.size();
			outfile << "time"
				<< ",";
			for (int i = 0; i < traces.size() - 1; i++) {
				outfile << traces.at(i).name << ",";
			}
			outfile << traces.at(traces.size() - 1).name << "\n";
			for (int i = 0; i < loopsize; i++) {
				outfile << std::fixed << std::scientific << std::setprecision(16)
					<< timesteps->at(i) << ",";
				for (int j = 0; j < traces.size() - 1; j++) {
					if(traces.at(j).pointer)
						outfile << std::fixed << std::scientific << std::setprecision(16)
							<< traces.at(j).traceData->at(i) << ",";
					else
						outfile << std::fixed << std::scientific << std::setprecision(16)
							<< traces.at(j).calcData.at(i) << ",";
				}
				if(traces.at(traces.size() - 1).pointer)
					outfile << std::fixed << std::scientific << std::setprecision(16)
						<< traces.at(traces.size() - 1).traceData->at(i) << "\n";
				else
					outfile << std::fixed << std::scientific << std::setprecision(16)
						<< traces.at(traces.size() - 1).calcData.at(i) << "\n";
			}
			outfile.close();
		}
		else if (traces.empty()) {
			std::cout << "W: Nothing specified to save."
				<< std::endl;
			std::cout << std::endl;
			outfile.close();
		}
	}
	else {
		std::cout << "E: " << outname << " could not be opened for saving."
				<< std::endl;
		std::cout << std::endl;
	}
}

void
Output::write_legacy_data(std::string &outname) {
	int loopsize = 0;
	std::ofstream outfile(outname);
	if (outfile.is_open()) {
		if(!traces.empty()) {
			if(traces.at(0).pointer)
				loopsize = traces.at(0).traceData->size();
			else
				loopsize = traces.at(0).calcData.size();
			outfile << "time"
				<< " ";
			for (int i = 0; i < traces.size() - 1; i++) {
				outfile << traces.at(i).name << " ";
			}
			outfile << traces.at(traces.size() - 1).name << "\n";
			for (int i = 0; i < loopsize; i++) {
				outfile << std::fixed << std::scientific << std::setprecision(16)
					<< timesteps->at(i) << " ";
				for (int j = 0; j < traces.size() - 1; j++) {
					if(traces.at(j).pointer)
						outfile << std::fixed << std::scientific << std::setprecision(16)
							<< traces.at(j).traceData->at(i) << " ";
					else
						outfile << std::fixed << std::scientific << std::setprecision(16)
							<< traces.at(j).calcData.at(i) << " ";
				}
				if(traces.at(traces.size() - 1).pointer)
					outfile << std::fixed << std::scientific << std::setprecision(16)
						<< traces.at(traces.size() - 1).traceData->at(i) << "\n";
				else
					outfile << std::fixed << std::scientific << std::setprecision(16)
						<< traces.at(traces.size() - 1).calcData.at(i) << "\n";
			}
			outfile.close();
		}
		else if (traces.empty()) {
			std::cout << "W: Nothing specified to save."
				<< std::endl;
			std::cout << std::endl;
			outfile.close();
		}
	}
	else {
		std::cout << "E: " << outname << " could not be opened for saving."
				<< std::endl;
			std::cout << std::endl;
	}
}

void
Output::write_wr_data(std::string &outname) {
	std::string label;
	std::vector<std::string> tokens;
	int loopsize = 0;
	std::ofstream outfile(outname);
	if (outfile.is_open()) {
		if (!traces.empty()) {
			outfile << "Title: CKT1\n";
			std::time_t result = std::time(nullptr);
			outfile << "Date: " << std::asctime(std::localtime(&result));
			outfile << "Plotname: Transient analysis JoSIM\n";
			outfile << "Flags: real\n";
			outfile << "No. Variables: " << traces.size() + 1 << "\n";
			if(traces.at(0).pointer)
				loopsize = traces.at(0).traceData->size();
			else
				loopsize = traces.at(0).calcData.size();
			outfile << "No. Points: " << loopsize << "\n";
			outfile << "Command: version 4.3.8\n";
			outfile << "Variables:\n";
			outfile << " 0 time S\n";
			for (int i = 0; i < traces.size(); i++) {
				if(traces.at(i).type == 'V') {
					if(traces.at(i).name.substr(0, 2) == "NV") {
						tokens = Misc::tokenize_delimeter(traces.at(i).name, "_");
						if (tokens.size() > 2) outfile << " "
							<< i+1 << " v(" << tokens[1] << "," << tokens[2] <<") V\n";
						else outfile << " " << i+1 << " v(" << tokens[1] <<") V\n";
					}
					else outfile << " " << i+1 << " " << traces.at(i).name <<" V\n";
				}
				else if(traces.at(i).type == 'P') {
					if(traces.at(i).name.substr(0, 2) == "NP") {
						tokens = Misc::tokenize_delimeter(traces.at(i).name, "_");
						if (tokens.size() > 2) outfile << " "
							<< i+1 << " p(" << tokens[1] << "," << tokens[2] <<") P\n";
						else outfile << " " << i+1 << " p(" << tokens[1] <<") P\n";
					}
					else outfile << " " << i+1 << " " << traces.at(i).name <<" P\n";
				}
				else if(traces.at(i).type == 'C') {
					label = traces.at(i).name;
					std::replace(label.begin(), label.end(), '|', '.');
					outfile << " " << i+1 << " " << "@" << label << "[c] C\n";
				}
			}
			outfile << "Values:\n";
			for (int i = 0; i < loopsize; i++) {
				outfile << " " << i << " "
					<< std::fixed << std::scientific << std::setprecision(16)
					<< timesteps->at(i) << "\n";
				for (int j = 0; j < traces.size(); j++) {
					if(traces.at(j).pointer)
						outfile << " " << std::string( Misc::numDigits(i), ' ' )
							<< " " << std::fixed << std::scientific << std::setprecision(16)
							<< traces.at(j).traceData->at(i) << "\n";
					else
						outfile << " " << std::string( Misc::numDigits(i), ' ' )
							<< " " << std::fixed << std::scientific << std::setprecision(16)
							<< traces.at(j).calcData.at(i) << "\n";
				}
			}
			outfile.close();
		}
		else if (traces.empty()) {
			std::cout << "W: Nothing specified to save."
				<< std::endl;
			std::cout << std::endl;
			outfile.close();
		}
	}
	else {
		std::cout << "E: " << outname << " could not be opened for saving."
				<< std::endl;
			std::cout << std::endl;
	}
}

void
Output::write_cout(Matrix &mObj, Simulation &sObj) {
	int loopsize = 0;
	if(!traces.empty()) {
		if(traces.at(0).pointer)
			loopsize = traces.at(0).traceData->size();
		else
			loopsize = traces.at(0).calcData.size();
		std::cout << "time"
			<< " ";
		for (int i = 0; i < traces.size() - 1; i++) {
			std::cout << traces.at(i).name << " ";
		}
		std::cout << traces.at(traces.size() - 1).name << "\n";
		for (int i = 0; i < loopsize; i++) {
			std::cout << std::fixed << std::scientific << std::setprecision(16)
				<< timesteps->at(i) << " ";
			for (int j = 0; j < traces.size() - 1; j++) {
				if(traces.at(j).pointer)
					std::cout << std::fixed << std::scientific << std::setprecision(16)
						<< traces.at(j).traceData->at(i) << " ";
				else
					std::cout << std::fixed << std::scientific << std::setprecision(16)
						<< traces.at(j).calcData.at(i) << " ";
			}
			if(traces.at(traces.size() - 1).pointer)
				std::cout << std::fixed << std::scientific << std::setprecision(16)
					<< traces.at(traces.size() - 1).traceData->at(i) << "\n";
			else
				std::cout << std::fixed << std::scientific << std::setprecision(16)
					<< traces.at(traces.size() - 1).calcData.at(i) << "\n";
		}
	}
	else if (traces.empty()) {
		std::cout << "W: Nothing specified to output."
			<< std::endl;
		std::cout << "W: Printing all vectors."
			<< std::endl;
		std::cout << std::endl;
		std::cout << "time"
			<< " ";
		for (int i = 0; i < mObj.columnNames.size() - 1; i++) {
			std::cout << mObj.columnNames.at(i) << " ";
		}
		std::cout << mObj.columnNames.at(mObj.columnNames.size() - 1) << "\n";
		for (int i = 0; i < sObj.results.xVect.at(0).size(); i++) {
			std::cout << std::fixed << std::scientific << std::setprecision(16)
				<< timesteps->at(i) << " ";
			for (int j = 0; j < sObj.results.xVect.size() - 1; j++) {
				std::cout << std::fixed << std::scientific << std::setprecision(16)
					<< sObj.results.xVect.at(j).at(i) << " ";
			}
			std::cout << std::fixed << std::scientific << std::setprecision(16)
				<< sObj.results.xVect.at(sObj.results.xVect.size() - 1).at(i) << "\n";
		}
	}
}

int
Output::plot_traces(std::string &filename) {
	if (traces.size() > 0) {
	#ifdef USING_FLTK
		Fl_Window * win = new Fl_Window(1240, 768);
		Fl_Scroll * scroll = new Fl_Scroll(0, 0, win->w(), win->h());
		std::vector<Fl_Chart *> Charts;
		for (int i = 0; i < traces.size(); i++) {
			Charts.push_back(new Fl_Chart(20, 20 + (i * (scroll->h() / 3)), scroll->w() - 40, (scroll->h() / 3 - 20)));
			Charts[i]->type(FL_LINE_CHART);
			if(traces.at(0).pointer)
				for (int j = 0; j < traces.at(i).traceData->size(); j++)
					Charts[i]->add(traces.at(i).traceData->at(j));
			else
				for (int j = 0; j < traces.at(i).calcData.size(); j++)
					Charts[i]->add(traces.at(i).calcData.at(j));
			Charts[i]->color(FL_WHITE);
			Charts[i]->align(FL_ALIGN_INSIDE | FL_ALIGN_CENTER | FL_ALIGN_TOP);
			Charts[i]->copy_label(traces.at(i).name.c_str());
		}
		win->resizable(win);
		//win->label(filename);
		win->show();
		return(Fl::run());
	#elif USING_MATPLOTLIB
		if (traces.size() <= 4) {
			//plt::figure_size(800, 600);
			plt::figure();
			for (int i = 0; i < traces.size(); i++) {
				plt::subplot(traces.size(), 1, i + 1);
				plt::grid(true);
				if(traces.at(i).pointer)
					plt::plot(*timesteps, *traces.at(i).traceData);
				else
					plt::plot(*timesteps, traces.at(i).calcData);
				plt::title(traces.at(i).name.c_str());
				if (traces.at(i).type == 'V') plt::ylabel("Voltage (V)");
				else if (traces.at(i).type == 'C') plt::ylabel("Current (A)");
				else if (traces.at(i).type == 'P') plt::ylabel("Phase (rads)");
			}
			plt::xlabel("Time (s)");
			plt::tight_layout();
			plt::show();
			return 0;
		}
		else {
			for (int j = 0; j < traces.size(); j = j + 4) {
				int  i = j;
				//plt::figure_size(800, 600);
				plt::figure();
				while ((i < traces.size()) && (i < j + 4)) {
					plt::subplot(4, 1, (i - j) + 1);
					plt::grid(true);
					if(traces.at(i).pointer)
						plt::plot(*timesteps, *traces.at(i).traceData);
					else
						plt::plot(*timesteps, traces.at(i).calcData);
					plt::title(traces.at(i).name.c_str());
					if (traces.at(i).type == 'V') plt::ylabel("Voltage (V)");
					else if (traces.at(i).type == 'C') plt::ylabel("Current (A)");
					else if (traces.at(i).type == 'P') plt::ylabel("Phase (rads)");
					i++;
				}
				plt::xlabel("Time (s)");
				plt::tight_layout();
				plt::show(false);
			}
			plt::show();
			return 0;
		}
		#endif
	}
	else {
		std::cout << "W: Nothing specified to output."
			<< std::endl;
		std::cout << std::endl;
		return 0;
	}
	return 0;
}

int
Output::plot_all(std::string &filename, Matrix &mObj, Simulation &sObj) {
	#ifdef USING_FLTK
		Fl_Window * win = new Fl_Window(1240, 768);
		Fl_Scroll * scroll = new Fl_Scroll(0, 0, win->w(), win->h());
		std::vector<Fl_Chart *> Charts;
		for (int i = 0; i < mObj.columnNames.size(); i++) {
			Charts.push_back(new Fl_Chart(20, 20 + (i * (scroll->h() / 3)), scroll->w() - 40, (scroll->h() / 3 - 20)));
			Charts[i]->type(FL_LINE_CHART);
			for (int j = 0; j < sObj.results.xVect.at(i).size(); j++)
				Charts[i]->add(sObj.results.xVect.at(i).at(j));
			Charts[i]->color(FL_WHITE);
			Charts[i]->align(FL_ALIGN_INSIDE | FL_ALIGN_CENTER | FL_ALIGN_TOP);
			Charts[i]->copy_label(mObj.columnNames.at(i).c_str());
		}
		win->resizable(win);
		//win->label(filename);
		win->show();
		return(Fl::run());
	#elif USING_MATPLOTLIB
		if (mObj.columnNames.size() <= 4) {
			//plt::figure_size(800, 600);
			plt::figure();
			for (int i = 0; i < mObj.columnNames.size(); i++) {
				plt::subplot(mObj.columnNames.size(), 1, i + 1);
				plt::grid(true);
				plt::plot(*timesteps, sObj.results.xVect.at(i));
				plt::title(mObj.columnNames.at(i).c_str());
			}
			plt::xlabel("Time (s)");
			plt::tight_layout();
			plt::show();
			return 0;
		}
		else {
			for (int j = 0; j < mObj.columnNames.size(); j = j + 4) {
				int  i = j;
				//plt::figure_size(800, 600);
				plt::figure();
				while ((i < mObj.columnNames.size()) && (i < j + 4)) {
					plt::subplot(4, 1, (i - j) + 1);
					plt::grid(true);
					plt::plot(*timesteps, sObj.results.xVect.at(i));
					plt::title(mObj.columnNames.at(i).c_str());
					i++;
				}
				plt::xlabel("Time (s)");
				plt::tight_layout();
				plt::show(false);
			}
			plt::show();
			return 0;
		}
	#endif
	return 0;
}
