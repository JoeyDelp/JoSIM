// Copyright (c) 2018 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#include "j_output.h"

void
Output::relevant_traces(Input &iObj, Matrix &mObj, Simulation &sObj) {
	std::vector<std::string> tokens;
	std::string label;
	for (auto i : iObj.controls) {
		Trace thisTrace;
		if(i.find("PRINT") != std::string::npos) {
			tokens = Misc::tokenize_space(i);
			if(tokens.at(1) == "DEVI") {
				if(tokens.size() > 4) {
					std::cout << "W: Print request for device current has too many arguments. " << std::endl;
					std::cout << "W: Line: " << i << std::endl;
					std::cout << "W: Ignoring the extra argument." << std::endl;
					std::cout << std::endl;
				}
				label = tokens.at(2);
				if(label.find("_") != std::string::npos) {
					tokens = Misc::tokenize_delimeter(label, "_");
					label = tokens.back();
					for(int j = 0; j < tokens.size() - 1; j++) label = label + "|" + tokens.at(j);
				}
				switch(label[0]) {
					case 'R':
						thisTrace.name = label;
						thisTrace.type = 'C';
						if(mObj.components.voltRes.count(label) != 0) {
							if(mObj.components.voltRes.at(label).posNCol == -1) {
								std::transform(
									sObj.results.xVect.at(mObj.components.voltRes.at(label).negNCol).begin(), 
									sObj.results.xVect.at(mObj.components.voltRes.at(label).negNCol).end(), 
									thisTrace.calcData.begin(), 
									std::bind(std::multiplies<double>(), 
										std::placeholders::_1, 
										(1 / mObj.components.voltRes.at(label).value)));
							}
							else if(mObj.components.voltRes.at(label).negNCol == -1) {
								std::transform(
									sObj.results.xVect.at(mObj.components.voltRes.at(label).posNCol).begin(), 
									sObj.results.xVect.at(mObj.components.voltRes.at(label).posNCol).end(), 
									thisTrace.calcData.begin(), 
									std::bind(std::multiplies<double>(), 
										std::placeholders::_1, 
										(1 / mObj.components.voltRes.at(label).value)));
							}
							else {
								std::transform(
									sObj.results.xVect.at(mObj.components.voltRes.at(label).posNCol).begin(), 
									sObj.results.xVect.at(mObj.components.voltRes.at(label).posNCol).end(), 
									sObj.results.xVect.at(mObj.components.voltRes.at(label).negNCol).begin(), 
									thisTrace.calcData.begin(), 
									std::minus<double>());
								std::transform(
									thisTrace.calcData.begin(), 
									thisTrace.calcData.end(), 
									thisTrace.calcData.begin(), 
									std::bind(std::multiplies<double>(), 
										std::placeholders::_1, 
										(1 / mObj.components.voltRes.at(label).value)));
							}
							thisTrace.traceData = &thisTrace.calcData;
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
						thisTrace.name = label;
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
						thisTrace.name = label;
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
					case 'J':
						thisTrace.name = label;
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
						thisTrace.name = label;
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
				if(label.find("_") != std::string::npos) {
					tokens = Misc::tokenize_delimeter(label, "_");
					label = tokens.back();
					for(int j = 0; j < tokens.size() - 1; j++) label = label + "|" + tokens.at(j);
				}
				switch(label[0]) {
					case 'R':
						thisTrace.name = label;
						thisTrace.type = 'V';
						if(mObj.components.voltRes.count(label) != 0) {
							if(mObj.components.voltRes.at(label).posNCol == -1) {
								thisTrace.traceData = &sObj.results.xVect.at(mObj.components.voltRes.at(label).negNCol);
							}
							else if(mObj.components.voltRes.at(label).negNCol == -1) {
								thisTrace.traceData = &sObj.results.xVect.at(mObj.components.voltRes.at(label).posNCol);
							}
							else {
								std::transform(
									sObj.results.xVect.at(mObj.components.voltRes.at(label).posNCol).begin(), 
									sObj.results.xVect.at(mObj.components.voltRes.at(label).posNCol).end(), 
									sObj.results.xVect.at(mObj.components.voltRes.at(label).negNCol).begin(), 
									thisTrace.calcData.begin(), 
									std::minus<double>());
								thisTrace.traceData = &thisTrace.calcData;									
							}
							traces.push_back(thisTrace);
						}
						else if(mObj.components.phaseRes.count(label) != 0) {
							///////////////////////////// TODO: Find voltage from phase results
						}
						else {
							std::cout << "W: Unknown device " << label << std::endl;
							std::cout << "W: Cannot print voltage for this device." << std::endl;
							std::cout << "W: Ignoring this print request." << std::endl;
							std::cout << std::endl;
						}
						break;
					case 'L':
						thisTrace.name = label;
						thisTrace.type = 'V';
						if(mObj.components.voltInd.count(label) != 0) {
							if(mObj.components.voltInd.at(label).posNCol == -1) {
								thisTrace.traceData = &sObj.results.xVect.at(mObj.components.voltInd.at(label).negNCol);
							}
							else if(mObj.components.voltInd.at(label).negNCol == -1) {
								thisTrace.traceData = &sObj.results.xVect.at(mObj.components.voltInd.at(label).posNCol);
							}
							else {
								std::transform(
									sObj.results.xVect.at(mObj.components.voltInd.at(label).posNCol).begin(), 
									sObj.results.xVect.at(mObj.components.voltInd.at(label).posNCol).end(), 
									sObj.results.xVect.at(mObj.components.voltInd.at(label).negNCol).begin(), 
									thisTrace.calcData.begin(), 
									std::minus<double>());
								thisTrace.traceData = &thisTrace.calcData;									
							}
							traces.push_back(thisTrace);
						}
						else if(mObj.components.phaseInd.count(label) != 0) {
							///////////////////////////// TODO: Find voltage from phase results
						}
						else {
							std::cout << "W: Unknown device " << label << std::endl;
							std::cout << "W: Cannot print voltage for this device." << std::endl;
							std::cout << "W: Ignoring this print request." << std::endl;
							std::cout << std::endl;
						}
						break;
					case 'C':
						thisTrace.name = label;
						thisTrace.type = 'V';
						if(mObj.components.voltCap.count(label) != 0) {
							if(mObj.components.voltCap.at(label).posNCol == -1) {
								thisTrace.traceData = &sObj.results.xVect.at(mObj.components.voltCap.at(label).negNCol);
							}
							else if(mObj.components.voltCap.at(label).negNCol == -1) {
								thisTrace.traceData = &sObj.results.xVect.at(mObj.components.voltCap.at(label).posNCol);
							}
							else {
								std::transform(
									sObj.results.xVect.at(mObj.components.voltCap.at(label).posNCol).begin(), 
									sObj.results.xVect.at(mObj.components.voltCap.at(label).posNCol).end(), 
									sObj.results.xVect.at(mObj.components.voltCap.at(label).negNCol).begin(), 
									thisTrace.calcData.begin(), 
									std::minus<double>());
								thisTrace.traceData = &thisTrace.calcData;									
							}
							traces.push_back(thisTrace);
						}
						else if(mObj.components.phaseCap.count(label) != 0) {
							///////////////////////////// TODO: Find voltage from phase results
						}
						else {
							std::cout << "W: Unknown device " << label << std::endl;
							std::cout << "W: Cannot print voltage for this device." << std::endl;
							std::cout << "W: Ignoring this print request." << std::endl;
							std::cout << std::endl;
						}
						break;
					case 'J':
						thisTrace.name = label;
						thisTrace.type = 'V';
						if(mObj.components.voltJJ.count(label) != 0) {
							if(mObj.components.voltJJ.at(label).posNCol == -1) {
								thisTrace.traceData = &sObj.results.xVect.at(mObj.components.voltJJ.at(label).negNCol);
							}
							else if(mObj.components.voltJJ.at(label).negNCol == -1) {
								thisTrace.traceData = &sObj.results.xVect.at(mObj.components.voltJJ.at(label).posNCol);
							}
							else {
								std::transform(
									sObj.results.xVect.at(mObj.components.voltJJ.at(label).posNCol).begin(), 
									sObj.results.xVect.at(mObj.components.voltJJ.at(label).posNCol).end(), 
									sObj.results.xVect.at(mObj.components.voltJJ.at(label).negNCol).begin(), 
									thisTrace.calcData.begin(), 
									std::minus<double>());
								thisTrace.traceData = &thisTrace.calcData;									
							}
							traces.push_back(thisTrace);
						}
						else if(mObj.components.phaseJJ.count(label) != 0) {
							///////////////////////////// TODO: Find voltage from phase results
						}
						else {
							std::cout << "W: Unknown device " << label << std::endl;
							std::cout << "W: Cannot print voltage for this device." << std::endl;
							std::cout << "W: Ignoring this print request." << std::endl;
							std::cout << std::endl;
						}
						break;
					case 'V':
						thisTrace.name = label;
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
			else if(tokens.at(1) == "NODEV") {}
			else if(tokens.at(1) == "NODEP") {}
			else if(tokens.at(1) == "PHASE") {
				
			}
		}
		else if(i.find("PLOT") != std::string::npos) {
			tokens = Misc::tokenize_space(i);
		}
		else if(i.find("SAVE") != std::string::npos) {
			tokens = Misc::tokenize_space(i);
		}
	}

}

/*
	 Function that writes the output file as requested by the user
*/
// void
// Output::write_data(InputFile& iFile) {
// 	std::vector<std::string> traceLabel;
// 	std::vector<std::vector<double>> traceData;
// 	if(cArg.analysisT == VANALYSIS) Plot::traces_to_plot(iFile, iFile.controlPart, traceLabel, traceData);
// 	else if (cArg.analysisT == PANALYSIS) Plot::phase_traces_to_plot(iFile, iFile.controlPart, traceLabel, traceData);
// 	std::ofstream outfile(cArg.outName);
// 	if (outfile.is_open()) {
// 		if (!traceLabel.empty()) {
// 			outfile << "time"
// 				<< ",";
// 			for (int i = 0; i < traceLabel.size() - 1; i++) {
// 				outfile << traceLabel[i] << ",";
// 			}
// 			outfile << traceLabel.at(traceLabel.size() - 1) << "\n";
// 			for (int i = 0; i < traceData[0].size(); i++) {
// 				outfile << std::fixed << std::scientific << std::setprecision(16)
// 					<< iFile.timeAxis[i] << ",";
// 				for (int j = 0; j < traceData.size() - 1; j++) {
// 					outfile << std::fixed << std::scientific << std::setprecision(16)
// 						<< traceData[j][i] << ",";
// 				}
// 				outfile << std::fixed << std::scientific << std::setprecision(16)
// 					<< traceData.at(traceData.size() - 1)[i] << "\n";
// 			}
// 			outfile.close();
// 		}
// 		else if (traceLabel.empty()) {
// 			std::cout << "W: Nothing specified to save. Saving all traces."
// 				<< std::endl;
// 			outfile << "time"
// 				<< ",";
// 			for (int i = 0; i < iFile.matA.columnNames.size() - 1; i++) {
// 				outfile << iFile.matA.columnNames[i] << ",";
// 			}
// 			outfile << iFile.matA.columnNames.at(iFile.matA.columnNames.size() - 1) << "\n";
// 			for (int i = 0; i < iFile.xVect[0].size(); i++) {
// 				outfile << std::fixed << std::scientific << std::setprecision(16)
// 					<< iFile.timeAxis[i] << ",";
// 				for (int j = 0; j < iFile.xVect.size() - 1; j++) {
// 					outfile << std::fixed << std::scientific << std::setprecision(16)
// 						<< iFile.xVect[j][i] << ",";
// 				}
// 				outfile << std::fixed << std::scientific << std::setprecision(16)
// 					<< iFile.xVect.at(iFile.xVect.size() - 1)[i] << "\n";
// 			}
// 			outfile.close();
// 		}
// 	}
// }
// /*
// 	Function that writes a legacy output file in JSIM_N format
// */
// void
// Output::write_legacy_data(InputFile& iFile) {
// 	std::string label;
// 	std::vector<std::string> traceLabel, tokens;
// 	std::vector<std::vector<double>> traceData;
// 	if(cArg.analysisT == VANALYSIS) Plot::traces_to_plot(iFile, iFile.controlPart, traceLabel, traceData);
// 	else if (cArg.analysisT == PANALYSIS) Plot::phase_traces_to_plot(iFile, iFile.controlPart, traceLabel, traceData);
// 	std::ofstream outfile(cArg.outName);
// 	if (outfile.is_open()) {
// 		outfile << "time"
// 			<< " ";
// 		for (int i = 0; i < traceLabel.size() - 1; i++) {
// 			tokens = Misc::tokenize_space(traceLabel[i]);
// 			label = tokens[0];
// 			for (int j = 1; j < tokens.size(); j++) label = label + "_" + tokens[j];
// 			outfile << label << " ";
// 		}
// 		tokens = Misc::tokenize_space(traceLabel.at(traceLabel.size() - 1));
// 		label = tokens[0];
// 		for (int j = 1; j < tokens.size(); j++) label = label + "_" + tokens[j];
// 		outfile << label << "\n";
// 		for (int i = 0; i < traceData[0].size(); i++) {
// 			outfile << std::fixed << std::scientific << std::setprecision(16)
// 				<< iFile.timeAxis[i] << " ";
// 			for (int j = 0; j < traceData.size() - 1; j++) {
// 				outfile << std::fixed << std::scientific << std::setprecision(16)
// 					<< traceData[j][i] << " ";
// 			}
// 			outfile << std::fixed << std::scientific << std::setprecision(16)
// 				<< traceData.at(traceData.size() - 1)[i] << "\n";
// 		}
// 		outfile.close();
// 	}
// }
// /*
// 	Function that writes a wr output file for opening in WRspice
// */
// void 
// Output::write_wr_data(InputFile &iFile) {
// 	std::string label;
// 	std::vector<std::string> traceLabel, tokens;
// 	std::vector<std::vector<double>> traceData;
// 	if(cArg.analysisT == VANALYSIS) Plot::traces_to_plot(iFile, iFile.controlPart, traceLabel, traceData);
// 	else if (cArg.analysisT == PANALYSIS) Plot::phase_traces_to_plot(iFile, iFile.controlPart, traceLabel, traceData);
// 	std::ofstream outfile(cArg.outName);
// 	if (outfile.is_open()) {
// 		outfile << "Title: CKT1\n";
// 		std::time_t result = std::time(nullptr);
// 		outfile << "Date: " << std::asctime(std::localtime(&result));
// 		outfile << "Plotname: Transient analysis JoSIM\n";
// 		outfile << "Flags: real\n";
// 		outfile << "No. Variables: " << traceData.size() + 1 << "\n";
// 		outfile << "No. Points: " << traceData.at(0).size() << "\n";
// 		outfile << "Command: version 4.3.8\n";
// 		outfile << "Variables:\n";
// 		outfile << " 0 time S\n";
// 		for (int i = 0; i < traceLabel.size(); i++) {
// 			tokens = Misc::tokenize_space(traceLabel[i]);
// 			if(tokens[0] == "NODE") {
// 				if(tokens[1] == "VOLTAGE") {
// 					if (tokens.size() > 3) outfile << " " 
// 						<< i+1 << " v(" << tokens[2] << "," << tokens[4] <<") V\n";
// 					else outfile << " " << i+1 << " v(" << tokens[2] <<") V\n";
// 				}
// 				else if(tokens[1] == "PHASE") {
// 					if (tokens.size() > 3) outfile << " " 
// 						<< i+1 << " p(" << tokens[2] << "," << tokens[4] <<") P\n";
// 					else outfile << " " << i+1 << " p(" << tokens[2] <<") P\n";
// 				}
// 			}
// 			else if(tokens[0] == "DEVICE") {
// 				if(tokens[1] == "VOLTAGE") {
// 					outfile << " " << i+1 << " " << tokens[2] <<" V\n";
// 				}
// 				else if(tokens[1] == "CURRENT") {
// 					outfile << " " << i+1 << " " << tokens[2] <<" C\n";
// 				}
// 				else if(tokens[1] == "PHASE") {
// 					outfile << " " << i+1 << " " << tokens[2] <<" P\n";
// 				}
// 			}
// 			else if(tokens[0] == "PHASE") {
// 				outfile << " " << i+1 << " " << tokens[1] <<" P\n";
// 			}
// 			else if(tokens[0] == "CURRENT") {
// 				std::replace(tokens[1].begin(), tokens[1].end(), '|', '.');
// 				outfile << " " << i+1 << " " << "@" << tokens[1] << "[c] C\n"; 
// 			}
// 		}
// 		outfile << "Values:\n";
// 		for (int i = 0; i < traceData[0].size(); i++) {
// 			outfile << " " << i << " " 
// 				<< std::fixed << std::scientific << std::setprecision(16)
// 				<< iFile.timeAxis[i] << "\n";
// 			for (int j = 0; j < traceData.size(); j++) {
// 				outfile << " " << std::string( Misc::numDigits(i), ' ' ) 
// 					<< " " << std::fixed << std::scientific << std::setprecision(16)
// 					<< traceData.at(j).at(i) << "\n";
// 			}
// 		}
// 		outfile.close();
// 	}
// }
// /*
// 	Function that writes the output to cout as requested by the user
// */
// void
// Output::write_cout(InputFile& iFile) {
// 	std::vector<std::string> traceLabel;
// 	std::vector<std::vector<double>> traceData;
// 	if(cArg.analysisT == VANALYSIS) Plot::traces_to_plot(iFile, iFile.controlPart, traceLabel, traceData);
// 	else if (cArg.analysisT == PANALYSIS) Plot::phase_traces_to_plot(iFile, iFile.controlPart, traceLabel, traceData);
// 	if (!traceLabel.empty()) {
// 		std::cout << "time"
// 			<< " ";
// 		for (int i = 0; i < traceLabel.size() - 1; i++) {
// 			std::cout << traceLabel[i] << " ";
// 		}
// 		std::cout << traceLabel.at(traceLabel.size() - 1) << "\n";
// 		for (int i = 0; i < traceData[0].size(); i++) {
// 			std::cout << std::fixed << std::scientific << std::setprecision(16)
// 				<< iFile.timeAxis[i] << " ";
// 			for (int j = 0; j < traceData.size() - 1; j++) {
// 				std::cout << std::fixed << std::scientific << std::setprecision(16)
// 					<< traceData[j][i] << " ";
// 			}
// 			std::cout << std::fixed << std::scientific << std::setprecision(16)
// 				<< traceData.at(traceData.size() - 1)[i] << "\n";
// 		}
// 	}
// 	else if (traceLabel.empty()) {
// 		std::cout << "W: Nothing specified to output."
// 			<< std::endl;
// 	}
// }
