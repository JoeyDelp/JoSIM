// Copyright (c) 2018 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#include "j_input.h"

void 
Input::parse_arguments(int argc, 
							char* argv[]) {
    if (argc <= 1) Errors::error_handling(TOO_FEW_ARGUMENTS);

    if (argv[argc - 1][0] == '-') {
        if(argv[argc - 1][1] == 'h'){
            display_help();
            exit(0);
        }
        else if (argv[argc - 1][1] == 'v'){
            version_info();
            exit(0);
        }
        else if (argv[argc - 1][1] == '-') {
            if(argv[argc - 1][2] == 'h') {
                display_help();
                exit(0);
            }
            else if (argv[argc - 1][2] == 'v') {
                version_info();
                exit(0);
            }
            else Errors::error_handling(FINAL_ARG_SWITCH);
        }
        else Errors::error_handling(FINAL_ARG_SWITCH);
    }
    else if (argv[argc - 1][0] != '-' && argv[argc - 1][1] != 'h') fileName = argv[argc - 1];
    else Errors::error_handling(FINAL_ARG_SWITCH);

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            switch(argv[i][1]) {
            case 'a':
                if ((i + 1) == (argc - 1)) Errors::error_handling(TOO_FEW_ARGUMENTS);
                else if (argv[i + 1][0] == '-') Errors::error_handling(TOO_FEW_ARGUMENTS);
                else{
                    try {
                        argAnal = std::stoi(argv[i + 1]);
                        if (argAnal > 1) Errors::error_handling(INVALID_ANALYSIS);
                    }
                    catch (std::exception& e) {
                        Errors::error_handling(INVALID_ANALYSIS);
                    }
                }
                break;
            case 'c':
                if ((i + 1) == (argc - 1)) Errors::error_handling(TOO_FEW_ARGUMENTS);
                else if (argv[i + 1][0] == '-') Errors::error_handling(TOO_FEW_ARGUMENTS);
                else{
                    try {
                        argConv = std::stoi(argv[i + 1]);
                        if (argConv > 1) Errors::error_handling(INVALID_CONVENTION);
                    }
                    catch (std::exception& e) {
                        Errors::error_handling(INVALID_CONVENTION);
                    }
                }
                break;
            case 'g':
                std::cout << "Plotting engine: " <<
                #ifdef USING_FLTK
                        "FTLK"
                #elif USING_MATPLOTLIB
                        "MATPLOTLIB"
                #else
                        "NONE"
                #endif
                        << std::endl;
                #ifndef USING_FLTK
                    #ifndef USING_MATPLOTLIB
                        std::cout << "Plotting requested with no plotting engine."
                        << std::endl;
                        std::cout << "Request to plot will be ignored."
                        << std::endl;
                    #else
                        argPlot = true;
                    #endif
                #else
                    plotRes = true;
                #endif
                break;
            case 'h':
                display_help();
                break;
            case 'o':
                argSave = true;
                if (((i + 1) == (argc - 1)) || (argv[i + 1][0] == '-')) {
                    argOutname = fileName;
                    argOutname = argOutname.substr(0, argOutname.find_last_of('.')) + ".csv";
                    argExt = CSV;
                }
                else {
                    argOutname = argv[i + 1];
                    if(argOutname.find('.') != std::string::npos) {
                        std::string outExt = argOutname.substr(argOutname.find_last_of('.'),
                         argOutname.size() - 1);
                        std::transform(outExt.begin(), outExt.end(),
                         outExt.begin(), toupper);
                        if (outExt == ".CSV") argExt = CSV;
                        else if (outExt == ".DAT") argExt = DAT;
                        else argExt = WR;
                    }
                    else argExt = WR;
                } 
                break;
            case 'p':
                #ifdef USING_OPENMP
                    std::cout << "Parallelization is ENABLED" << std::endl;
                #else
                    std::cout << "Parallelization is DISABLED" << std::endl;
                #endif
                break;
            case 'V':
                argVerb = true;
                break;
            case 'v':
                version_info();
                exit(0);
            }
        }
    }
}

void 
Input::display_help() {
    std::cout << "JoSIM help interface\n";
    std::cout << "====================\n";
    std::cout << std::setw(13) << std::left << "-a(nalysis)" << std::setw(3)
        << std::left << "|"
        << "Specifies the analysis type." << std::endl;
    std::cout << std::setw(13) << std::left << "  " << std::setw(3)
        << std::left << "|"
        << "0 for Voltage analysis (Default)." << std::endl;	
    std::cout << std::setw(13) << std::left << "  " << std::setw(3)
        << std::left << "|"
        << "1 for Phase analysis." << std::endl;

    std::cout << std::setw(13) << std::left << "-c(onvention)" << std::setw(3)
        << std::left << "|"
        << "Sets the subcircuit convention to left(0) or right(1)."
        << std::endl;
    std::cout << std::setw(13) << std::left << "  " << std::setw(3)
        << std::left << "|"
        << "Default is left. WRSpice (normal SPICE) use right."
        << std::endl;
    std::cout << std::setw(13) << std::left << "  " << std::setw(3)
        << std::left << "|"
        << "Eg. X01 SUBCKT 1 2 3     vs.     X01 1 2 3 SUBCKT"
        << std::endl;

    std::cout << std::setw(13) << std::left << "-g(raph)" << std::setw(3)
        << std::left << "|"
        << "Plot the requested results using a plotting library."
        << std::endl;
    std::cout << std::setw(13) << std::left << "  " << std::setw(3)
        << std::left << "|"
        << "If this is enabled with verbose mode all traces "
        "are plotted."
        << std::endl;

    std::cout << std::setw(13) << std::left << "-h(elp)" << std::setw(3)
        << std::left << "|"
        << "Displays this help menu" << std::endl;

    std::cout << std::setw(13) << std::left << "-o(utput)" << std::setw(3)
        << std::left << "|"
        << "Specify output file for simulation results (.csv)."
        << std::endl;
    std::cout << std::setw(13) << std::left << "  " << std::setw(3)
        << std::left << "|"
        << "Default will be output.csv if no file is specified."
        << std::endl;

    std::cout << std::setw(13) << std::left << "-p(arallel)" << std::setw(3)
        << std::left << "|"
        << "(EXPERIMENTAL) Enables parallelization of certain functions." << std::endl;
    std::cout << std::setw(13) << std::left << "  " << std::setw(3)
        << std::left << "|"
        << "Requires compilation with OPENMP switch enabled."
        << std::endl;
    std::cout << std::setw(13) << std::left << "  " << std::setw(3)
        << std::left << "|"
        << "Threshold applies, overhead on small circuits negates performance."
        << std::endl;

    std::cout << std::setw(13) << std::left << "-V(erbose)" << std::setw(3)
        << std::left << "|"
        << "Runs JoSIM in verbose mode." << std::endl;

    std::cout << std::setw(13) << std::left << "-v(ersion)" << std::setw(3)
        << std::left << "|"
        << "Displays the JoSIM version info only." << std::endl;

    std::cout << std::endl;
    std::cout << "Example command: josim -g -o ./output.csv test.cir"
        << std::endl;
    std::cout << std::endl;
    exit(0);
}

void 
Input::version_info() {
    std::cout << std::endl;
	std::cout
		<< "JoSIM: Josephson Junction Superconductive SPICE Circuit Simulator"
		<< std::endl;
	std::cout << "Copyright (C) 2018 by Johannes Delport (jdelport@sun.ac.za)"
		<< std::endl;
	std::cout << "v" << VERSION << " compiled on " << __DATE__ << " at " << __TIME__
		<< std::endl;
    std::cout << std::endl;
}

void
Input::read_input_file(std::string &fileName, 
					std::vector<std::string> &fileLines) {
	std::string line;
	std::fstream ifile(fileName);
	if (ifile.is_open()) {
		while (!ifile.eof()) {
			getline(ifile, line);
			std::transform(line.begin(), line.end(), line.begin(), toupper);
			if (!line.empty() && line.back() == '\r')
				line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
			if (!line.empty() && !Misc::starts_with(line, '*'))
				fileLines.push_back(line);
		}
	}
	else {
		Errors::error_handling(CANNOT_OPEN_FILE, fileName);
		exit(0);
	}
}

void
Input::split_netlist(std::vector<std::string> &fileLines, 
						std::vector<std::string> &controls, 
						Parameter &parameters, 
						Netlist &netlist) {
	bool subckt = false;
	bool control = false;
	std::string subcktName = "";
	std::vector<std::string> tokens;
	for (int i = 0; i < fileLines.size(); i++) {
		if(fileLines.at(i)[0] == '.') {
			if(fileLines.at(i).find(".SUBCKT") != std::string::npos) {
				subckt = true;
				netlist.containsSubckt = true;
				tokens = Misc::tokenize_space(fileLines.at(i));
				if (tokens.size() > 1) {
					if (tokens.size() > 2) {
						subcktName = tokens.at(1);
						for (int j = 2; j < tokens.size(); j++) {
							netlist.subcircuits[subcktName].io.push_back(tokens.at(j));
						}
					}
					else {
						std::cout << "E: Missing subcircuit io." << std::endl;
						std::cout << "E: Please recheck the netlist and try again." <<std::endl;
					}
				}
				else {
					std::cout << "E: Missing subcircuit name." << std::endl;
					std::cout << "E: Please recheck the netlist and try again." <<std::endl;
				}
			}
			else if(fileLines.at(i).find(".PARAM") != std::string::npos) {
				if(subckt) parameters.unparsedParams.push_back(std::make_pair(subcktName, fileLines.at(i)));
				else parameters.unparsedParams.push_back(std::make_pair("", fileLines.at(i)));
			}
			else if(fileLines.at(i).find(".CONTROL") != std::string::npos) {
				control = true;
			}
			else if(fileLines.at(i).find(".ENDS") != std::string::npos) {
				subckt = false;
			}
			else if(fileLines.at(i).find(".ENDC") != std::string::npos) {
				control = false;
			}
			else if(fileLines.at(i).find(".MODEL") != std::string::npos) {
				tokens = Misc::tokenize_space(fileLines.at(i));
				if(subckt) netlist.models[std::make_pair(tokens[1], subcktName)] = fileLines.at(i);
				else netlist.models[std::make_pair(tokens[1], "")] = fileLines.at(i);
			}
			else {
				if(!subckt) controls.push_back(fileLines.at(i));
				else {
					std::cout << "I: Subcircuit " << subcktName << " contains controls." << std::endl;
					std::cout << "I: Controls are reserved for the main design." << std::endl;
					std::cout << "I: These controls will be ignored." << std::endl;
				}
			}
		}
		else if (control) {
			if(fileLines.at(i).find(".PARAM") != std::string::npos) {
				if(subckt) parameters.unparsedParams.push_back(std::make_pair(subcktName, fileLines.at(i)));
				else parameters.unparsedParams.push_back(std::make_pair("", fileLines.at(i)));
			}
			else if(fileLines.at(i).find(".MODEL") != std::string::npos) {
				tokens = Misc::tokenize_space(fileLines.at(i));
				if(subckt) netlist.models[std::make_pair(tokens[1], subcktName)] = fileLines.at(i);
				else netlist.models[std::make_pair(tokens[1], "")] = fileLines.at(i);
			}
			else {
				if(!subckt) controls.push_back(fileLines.at(i));
				else {
					std::cout << "I: Subcircuit " << subcktName << " contains controls." << std::endl;
					std::cout << "I: Controls are reserved for the main design." << std::endl;
					std::cout << "I: These controls will be ignored." << std::endl;
				}
			}
		}
		else {
			if (subckt)
				netlist.subcircuits[subcktName].lines.push_back(std::make_pair(fileLines.at(i), subcktName));
			else 
				netlist.maindesign.push_back(fileLines.at(i));
		}
	}
	if(netlist.maindesign.empty()) {
		std::cout << "E: Missing main design in netlist." << std::endl;
		std::cout << "E: This design will not simulate without a main design." << std::endl;
	}
}

void 
Input::expand_subcircuits() {
	std::vector<std::string> tokens, io;
	std::vector<std::pair<std::string, std::string>> moddedLines;
	std::string subcktName, label, line;
	for(auto i : netlist.subcircuits) {
		for (auto j : i.second.lines) {
			if (j.first[0] == 'X') {
				netlist.subcircuits.at(i.first).containsSubckt = true;
				netlist.nestedSubcktCount++;
			}
		}
	}
	while(netlist.nestedSubcktCount != 0) {
		for(auto i : netlist.subcircuits) {
			for (int j = 0; j < netlist.subcircuits.at(i.first).lines.size(); j++) {
				if(netlist.subcircuits.at(i.first).lines.at(j).first[0] == 'X') {
					tokens = Misc::tokenize_space(netlist.subcircuits.at(i.first).lines.at(j).first);
					label = tokens.at(0);
					if(argConv == LEFT) { 
						subcktName = tokens.at(1);
						io.assign(tokens.begin() + 2, tokens.end());
					}
					else if (argConv == RIGHT) { 
						subcktName = tokens.back();
						io.assign(tokens.begin() + 1, tokens.end() - 1);
					}
					if(netlist.subcircuits.count(subcktName) != 0) {
						if(!netlist.subcircuits.at(subcktName).containsSubckt) {
							for (int k = 0; k < netlist.subcircuits.at(subcktName).lines.size(); k++) {
								tokens = Misc::tokenize_space(netlist.subcircuits.at(subcktName).lines.at(k).first);
								tokens[0] = tokens[0] + "|" + label;
								if(std::count(netlist.subcircuits.at(subcktName).io.begin(), netlist.subcircuits.at(subcktName).io.end(), tokens[1]) != 0) {
									for(int l = 0; l < netlist.subcircuits.at(subcktName).io.size(); l++) {
										if(tokens[1] == netlist.subcircuits.at(subcktName).io.at(l)) tokens[1] = io.at(l);
									}
								}
								else if (tokens[1] != "0" && tokens[1] != "GND") tokens[1] = tokens[1] + "|" + label;
								if(std::count(netlist.subcircuits.at(subcktName).io.begin(), netlist.subcircuits.at(subcktName).io.end(), tokens[2]) != 0) {
									for(int l = 0; l < netlist.subcircuits.at(subcktName).io.size(); l++) {
										if(tokens[2] == netlist.subcircuits.at(subcktName).io.at(l)) tokens[2] = io.at(l);
									}
								}
								else if (tokens[2] != "0" && tokens[2] != "GND") tokens[2] = tokens[2] + "|" + label;
								line = tokens[0];
								for (int m = 1; m < tokens.size(); m++) line = line + " " + tokens.at(m);
								moddedLines.push_back(std::make_pair(line, netlist.subcircuits.at(subcktName).lines.at(k).second));
							}
							netlist.subcircuits.at(i.first).lines.erase(netlist.subcircuits.at(i.first).lines.begin() + j);
							netlist.subcircuits.at(i.first).lines.insert(netlist.subcircuits.at(i.first).lines.begin() + j, moddedLines.begin(), moddedLines.end());
							moddedLines.clear();
							netlist.nestedSubcktCount--;
							netlist.subcircuits.at(i.first).containsSubckt = false;
						}
					}
					else {
						std::cout << "E: The subcircuit named " << subcktName << " was not found in the netlist." << std::endl;
						std::cout << "E: Please ensure all subcircuits exist and are correctly named." << std::endl;
						exit(0);
					}
				}
			}
		}
	}
}

void
Input::expand_maindesign() {
	std::vector<std::string> tokens, io;
	std::vector<std::pair<std::string, std::string>> moddedLines;
	std::string subcktName, label, line;
	for (int i = 0; i < netlist.maindesign.size(); i++) {
		if(netlist.maindesign.at(i)[0] == 'X') {
			tokens = Misc::tokenize_space(netlist.maindesign.at(i));
			label = tokens.at(0);
			if(argConv == LEFT) { 
				subcktName = tokens.at(1);
				io.assign(tokens.begin() + 2, tokens.end());
			}
			else if (argConv == RIGHT) { 
				subcktName = tokens.back();
				io.assign(tokens.begin() + 1, tokens.end() - 1);
			}
			if(netlist.subcircuits.count(subcktName) != 0) {
				for (int k = 0; k < netlist.subcircuits.at(subcktName).lines.size(); k++) {
					tokens = Misc::tokenize_space(netlist.subcircuits.at(subcktName).lines.at(k).first);
					tokens[0] = tokens[0] + "|" + label;
					if(std::count(netlist.subcircuits.at(subcktName).io.begin(), netlist.subcircuits.at(subcktName).io.end(), tokens[1]) != 0) {
						for(int l = 0; l < netlist.subcircuits.at(subcktName).io.size(); l++) {
							if(tokens[1] == netlist.subcircuits.at(subcktName).io.at(l)) tokens[1] = io.at(l);
						}
					}
					else if (tokens[1] != "0" && tokens[1] != "GND") tokens[1] = tokens[1] + "|" + label;
					if(std::count(netlist.subcircuits.at(subcktName).io.begin(), netlist.subcircuits.at(subcktName).io.end(), tokens[2]) != 0) {
						for(int l = 0; l < netlist.subcircuits.at(subcktName).io.size(); l++) {
							if(tokens[2] == netlist.subcircuits.at(subcktName).io.at(l)) tokens[2] = io.at(l);
						}
					}
					else if (tokens[2] != "0" && tokens[2] != "GND") tokens[2] = tokens[2] + "|" + label;
					line = tokens[0];
					for (int m = 1; m < tokens.size(); m++) line = line + " " + tokens.at(m);
					moddedLines.push_back(std::make_pair(line, netlist.subcircuits.at(subcktName).lines.at(k).second));
				}
				expNetlist.insert(expNetlist.end(), moddedLines.begin(), moddedLines.end());
				moddedLines.clear();
			}
			else {
				std::cout << "E: The subcircuit named " << subcktName << " was not found in the netlist." << std::endl;
				std::cout << "E: Please ensure all subcircuits exist and are correctly named." << std::endl;
				exit(0);
			}
		}
		else {
			expNetlist.push_back(std::make_pair(netlist.maindesign.at(i), ""));
		}
	}
}

// void
// check_model(std::string s, std::string sbcktName) {
// 	std::string modelTypes[] = {"JJ"};//, "MTJ", "NTRON", "CSHE"};
// 	std::vector<std::string> t;//, p;
// 	t = Misc::tokenize_delimeter(s, " \t(),");
// 	if(std::count(modelTypes, modelTypes+sizeof(modelTypes), t[2]) != 0) {
// 		if (sbcktName != "") iFile.models[sbcktName + "|" + t[1]] = s;
// 		else iFile.models[t[1]] = s;
// 	}
// 	else Errors::model_errors(UNKNOWN_MODEL_TYPE, t[2]);
// }

// void
// InputFile::read_input_file(std::string iFileName) {
// 	std::string line;
// 	// Create a new input file stream using the specified file name
// 	std::fstream ifile(iFileName);
// 	// Check to see if the file is open
// 	if (ifile.is_open()) {
// 		// While not end of the file
// 		while (!ifile.eof()) {
// 			// Read each line into the line variable
// 			getline(ifile, line);
// 			// Transform the line variable to uppercase to simplify program
// 			std::transform(line.begin(), line.end(), line.begin(), toupper);
// 			// If not empty and ends with carret return, remove the character
// 			if (!line.empty() && line.back() == '\r')
// 				line.pop_back();
// 			// If the line is not empty push it back into the vector of lines
// 			if (!line.empty() && !Misc::starts_with(line, '*'))
// 				lines.push_back(line);
// 		}
// 	}
// 	// Error if file could not be opened
// 	else {
// 		Errors::error_handling(CANNOT_OPEN_FILE);
// 		exit(0);
// 	}
// }

// void
// param_values::insertUParam(std::string paramName, std::string paramExpression, 
// 	std::string subcktName) {
// 	if(subcktName == "") {
// 		unparsedMap[paramName] = paramExpression;
// 	}
// 	else {
// 		paramName = paramName + "|" + subcktName;
// 		unparsedMap[paramName] = paramExpression;
// 	}
// }

// void
// param_values::insertParam(std::string paramName, double paramValue, std::string subcktName) {
// 	if(subcktName == "") {
// 		paramMap[paramName] = paramValue;
// 	}
// 	else {
// 		paramName = paramName + "|" + subcktName;
// 		paramMap[paramName] = paramValue;
// 	}
// }

// double
// param_values::returnParam(std::string paramName, std::string subcktName) {
// 	if(subcktName == "") {
// 		try {
// 			return paramMap.at(paramName);
// 		}
// 		catch (std::exception &e) {
// 			Errors::parsing_errors(UNIDENTIFIED_PART, paramName);
// 		}
// 	}
// 	else {
// 		paramName = paramName + "|" + subcktName;
// 		try {
// 			return paramMap.at(paramName);
// 		}
// 		catch (std::exception &e) {
// 			Errors::parsing_errors(UNIDENTIFIED_PART, paramName);
// 		}
// 	}
// 	return 0.0;
// }

// std::vector<std::string>
// recurseSubckt(std::unordered_map<std::string, Subcircuit> subckts, 
// 			  std::string part) {
// 	std::vector<std::string> t, ckts;
// 	ckts.clear();
// 	if (subckts[part].containsSubckt) {
// 		for (auto i : subckts[part].subckts) {
// 			t = recurseSubckt(subckts, i);
// 			ckts.insert(ckts.end(), t.begin(), t.end());
// 		}
// 	}
// 	if (!subckts[part].subckts.empty())
// 		ckts.insert(ckts.end(), subckts[part].subckts.begin(), subckts[part].subckts.end());
// 	return ckts;
// }

// void
// InputFile::circuit_to_segments() {
// 	bool startCkt = false;
// 	bool controlSection = false;
// 	std::string subcktName, varName;
// 	iFile.subCircuitCount = 0;
// 	std::vector<std::string> parameterTokens, tokens, v;
// 	for (int i = 0; i < iFile.lines.size(); i++) {
// 		if(i != iFile.lines.size() - 1) {
// 			if(iFile.lines.at(i + 1)[0] == '+') {
// 				iFile.lines.at(i) += iFile.lines.at(i+1).substr(1, iFile.lines.at(i+1).size() - 1);
// 				iFile.lines.erase(iFile.lines.begin() + i + 1);
// 			}
// 		}
// 		tokens = Misc::tokenize_space(iFile.lines.at(i));
// 		if (tokens[0] == ".SUBCKT") {
// 			subcktName = tokens[1];
// 			//iFile.subcircuitSegments[subcktName].name = subcktName;
// 			iFile.subcircuitSegments[subcktName].io.assign(tokens.begin() + 2, tokens.end());
// 			startCkt = true;
// 			iFile.subCircuitCount++;
// 		}
// 		else if (startCkt) {
// 			if (tokens[0][0] != '.') {
// 				iFile.subcircuitSegments[subcktName].lines.push_back(std::make_pair(iFile.lines.at(i),subcktName));
// 				if (tokens[0][0] != 'X') iFile.subcircuitSegments[subcktName].compCount++;
// 				if (tokens[0][0] == 'B') iFile.subcircuitSegments[subcktName].jjCount++;
// 				if (tokens[0][0] == 'X') {
// 					iFile.subcircuitSegments[subcktName].containsSubckt = true;
// 					if (cArg.sbcktConv == 0) iFile.subcircuitSegments[subcktName].subckts.push_back(tokens[1]);
// 					else if (cArg.sbcktConv == 1) iFile.subcircuitSegments[subcktName].subckts.push_back(tokens.back());
// 				}
// 			}
// 			else if (tokens[0] == ".PARAM") {
// 				parameterTokens = Misc::tokenize_delimeter(iFile.lines.at(i), " =");
// 				varName = parameterTokens[1];
// 				parameterTokens = Misc::tokenize_delimeter(iFile.lines.at(i), "=");
// 				iFile.paramValues.insertUParam(varName,
// 					parameterTokens[1], subcktName);
// 				// Parser::parse_expression(varName,
// 				// 	parameterTokens[1], subcktName);
// 			}
// 			else if (tokens[0] == ".MODEL") {
// 				check_model(iFile.lines.at(i), subcktName);
// 			}
// 			else if (tokens[0] == ".ENDS") {
// 				startCkt = false;
// 			}
// 		}
// 		else if (tokens[0][0] != '.' && !controlSection) {
// 			iFile.maincircuitSegment.push_back(iFile.lines.at(i));
// 			if (tokens[0][0] != 'X') iFile.mainComponents++;
// 			if (tokens[0][0] == 'B') iFile.mainJJs++;
// 			if (tokens[0][0] == 'X') {
// 				if (cArg.sbcktConv == 0) iFile.subckts.push_back(tokens[1]);
// 				else if (cArg.sbcktConv == 1) iFile.subckts.push_back(tokens.back());
// 			}
// 		}
// 		else if (tokens[0] == ".CONTROL") {
// 			controlSection = true;
// 		}
// 		else if (!controlSection) {
// 			if (tokens[0] == ".MODEL") {
// 				check_model(iFile.lines.at(i));
// 			}
// 			else if (tokens[0] == ".PARAM") {
// 				parameterTokens = Misc::tokenize_delimeter(iFile.lines.at(i), " =");
// 				varName = parameterTokens[1];
// 				parameterTokens = Misc::tokenize_delimeter(iFile.lines.at(i), "=");
// 				iFile.paramValues.insertUParam(varName,
// 					parameterTokens[1]);
// 				// Parser::parse_expression(varName, parameterTokens[1]);
// 			}
// 			else if (tokens[0] == ".ENDC")
// 				controlSection = false;
// 			else iFile.controlPart.push_back(iFile.lines.at(i));
// 		}
// 		else if (controlSection) {
// 			if (tokens[0] == "MODEL") {
// 				check_model(iFile.lines.at(i));
// 			}
// 			else if (tokens[0] == "PARAM") {
// 				parameterTokens = Misc::tokenize_delimeter(iFile.lines.at(i), " =");
// 				varName = parameterTokens[1];
// 				parameterTokens = Misc::tokenize_delimeter(iFile.lines.at(i), "=");
// 				iFile.paramValues.insertUParam(varName,
// 					parameterTokens[1]);
// 				// Parser::parse_expression(varName, parameterTokens[1]);
// 			}
// 			else if (tokens[0] == "ENDC")
// 				controlSection = false;
// 			else iFile.controlPart.push_back(iFile.lines.at(i));
// 		}
// 	}
// 	for (auto i : iFile.paramValues.unparsedMap) {
// 		if(i.first.find("|") != std::string::npos)
// 			subcktName = i.first.substr(i.first.find("|") + 1, i.first.size() - 1);
// 		else subcktName = "";
// 		Parser::parse_expression(i.first, i.second, subcktName);
// 	}
// 	std::vector<std::string> mainSubckts = iFile.subckts;
// 	for (auto i : mainSubckts) {
// 		v = recurseSubckt(iFile.subcircuitSegments, i);
// 		if (!v.empty()) iFile.subckts.insert(iFile.subckts.end(), v.begin(), v.end());
// 		v.empty();
// 	}
// 	iFile.circuitComponentCount += iFile.mainComponents;
// 	iFile.circuitJJCount += iFile.mainJJs;
// 	for (int i = 0; i < iFile.subckts.size(); i++) {
// 		iFile.circuitComponentCount += iFile.subcircuitSegments[iFile.subckts[i]].compCount;
// 		iFile.circuitJJCount += iFile.subcircuitSegments[iFile.subckts[i]].jjCount;
// 	}
// }

// void
// InputFile::sub_in_subcircuits(std::vector<std::string>& segment, 
// 							  std::string label) {
// 	std::vector<std::string> tokens, duplicateSegment, io;
// 	std::string subckt,
// 		parString,
// 		modelLabel,
// 		i,
// 		origLabel = label;
// 	#pragma omp parallel for private(subckt, parString, modelLabel, i, origLabel, tokens, io, label)
// 	for (int o = 0; o < segment.size(); o++) {
// 		i = segment[o];
// 		if (i[0] == 'X') {
// 			tokens = Misc::tokenize_space(i);
// 			try {
// 				modelLabel = tokens[0];
// 				if (label.empty())
// 					label = tokens[0];
// 				else
// 					label.append("|" + tokens[0]);
// 				if (cArg.sbcktConv == 0) {
// 					if (tokens[1].find("|") != std::string::npos) subckt = tokens[1].substr(0, tokens[1].find_first_of("|"));
// 					else subckt = tokens[1];
// 					io.clear();
// 					io.insert(io.begin(), tokens.begin() + 2, tokens.end());
// 				}
// 				else if (cArg.sbcktConv == 1) {
// 					if (tokens.back().find("|") != std::string::npos) subckt = tokens.back().substr(0, tokens.back().find_first_of("|"));
// 					else subckt = tokens.back();
// 					io.clear();
// 					io.insert(io.begin(), tokens.begin() + 1, tokens.end() - 1);
// 				}
// 				if (iFile.subcircuitNameMap.count(label) == 0) {
// 					iFile.subcircuitNameMap[label] = subckt;
// 				}
// 				if (io.size() != iFile.subcircuitSegments[subckt].io.size())
// 					Errors::invalid_component_errors(INVALID_SUBCIRCUIT_NODES, label);
// 				for (auto j : iFile.subcircuitSegments[subckt].lines) {
// 					if (j.first[0] != 'X') {
// 						tokens = Misc::tokenize_space(j.first);
// 						tokens[0] = tokens[0] + "|" + label;
// 						if (std::find(iFile.subcircuitSegments[subckt].io.begin(),
// 							iFile.subcircuitSegments[subckt].io.end(),
// 							tokens[1]) !=
// 							iFile.subcircuitSegments[subckt].io.end()) {
// 							for (const auto& k : iFile.subcircuitSegments[subckt].io) {
// 								if (k == tokens[1]) {
// 									tokens[1] =
// 										io.at(Misc::index_of(iFile.subcircuitSegments[subckt].io, k));
// 									break;
// 								}
// 							}
// 							if (std::find(iFile.subcircuitSegments[subckt].io.begin(),
// 								iFile.subcircuitSegments[subckt].io.end(),
// 								tokens[2]) !=
// 								iFile.subcircuitSegments[subckt].io.end()) {
// 								for (const auto& k : iFile.subcircuitSegments[subckt].io) {
// 									if (k == tokens[2]) {
// 										tokens[2] =
// 											io.at(Misc::index_of(iFile.subcircuitSegments[subckt].io, k));
// 										break;
// 									}
// 								}
// 							}
// 							else {
// 								if (tokens[2] != "0" && tokens[2] != "GND")
// 									tokens[2] = tokens[2] + "|" + label;
// 							}
// 						}
// 						else if (std::find(iFile.subcircuitSegments[subckt].io.begin(),
// 							iFile.subcircuitSegments[subckt].io.end(),
// 							tokens[2]) !=
// 							iFile.subcircuitSegments[subckt].io.end()) {
// 							for (const auto& k : iFile.subcircuitSegments[subckt].io) {
// 								if (k == tokens[2])
// 									tokens[2] =
// 									io.at(Misc::index_of(iFile.subcircuitSegments[subckt].io, k));
// 							}
// 							if (tokens[1] != "0" && tokens[1] != "GND")
// 								tokens[1] = tokens[1] + "|" + label;
// 						}
// 						else {
// 							if (tokens[1] != "0" && tokens[1] != "GND")
// 								tokens[1] = tokens[1] + "|" + label;
// 							if (tokens[2] != "0" && tokens[2] != "GND")
// 								tokens[2] = tokens[2] + "|" + label;
// 						}
// 						if (j.first[0] == 'B') {
// 							if (tokens.size() > 5) {
// 								if (tokens[5].find("AREA=") != std::string::npos) {
// 									if (tokens[3] != "0" && tokens[3] != "GND")
// 										tokens[3] = tokens[3] + "|" + label;
// 									tokens[4] = subckt + "|" + tokens[4];
// 								}
// 							}
// 							else
// 								tokens[3] = subckt + "|" + tokens[3];
// 						}
// 						for (int k = 3; k < tokens.size(); k++) {
// 							if (tokens[0][0] == 'B') {
// 								if (tokens[k].find("AREA=") != std::string::npos)
// 									if (iFile.paramValues.paramMap.count(
// 											Misc::substring_after(tokens[k], "AREA=")
// 															+ "|" + subckt) != 0)
// 										tokens[k] = "AREA=" + Misc::precise_to_string(
// 											iFile.paramValues.paramMap.at(
// 												Misc::substring_after(tokens[k], "AREA=")
// 												+ "|" + subckt));
// 							}
// 							else {
// 								if (tokens[k][0] == '(') {
// 									parString = tokens[k].substr(1);
// 									if (iFile.paramValues.paramMap.count(parString + "|" + subckt) != 0)
// 										tokens[k] = "(" + Misc::precise_to_string(
// 											iFile.paramValues.paramMap.at(parString + "|" + subckt));
// 								}
// 								else if (tokens[k].back() == ')') {
// 									parString = tokens[k].substr(0, tokens[k].size() - 1);
// 									if (iFile.paramValues.paramMap.count(parString + "|" + subckt) != 0)
// 										tokens[k] =
// 										Misc::precise_to_string(iFile.paramValues.paramMap.at(parString + "|" + subckt)) +
// 										")";
// 								}
// 								else if (tokens[k].find("PWL(") != std::string::npos) {
// 									parString = Misc::substring_after(tokens[k], "PWL(");
// 									if (iFile.paramValues.paramMap.count(parString + "|" + subckt) != 0)
// 										tokens[k] = "PWL(" + Misc::precise_to_string(
// 											iFile.paramValues.paramMap.at(parString + "|" + subckt));
// 								}
// 								else if (tokens[k].find("PULSE(") != std::string::npos) {
// 									parString = Misc::substring_after(tokens[k], "PULSE(");
// 									if (iFile.paramValues.paramMap.count(parString + "|" + subckt) != 0)
// 										tokens[k] =
// 										"PULSE(" +
// 										Misc::precise_to_string(iFile.paramValues.paramMap.at(parString + "|" + subckt));
// 								}
// 								else {
// 									if (iFile.paramValues.paramMap.count(tokens[k] + "|" + subckt) != 0)
// 										tokens[k] =
// 										Misc::precise_to_string(iFile.paramValues.paramMap.at(tokens[k] + "|" + subckt));
// 								}
// 							}
// 						}
// 						std::string line = tokens[0];
// 						for (int k = 1; k < tokens.size(); k++) {
// 							line = line + " " + tokens[k];
// 						}
// 	#pragma omp critical
// 						duplicateSegment.push_back(line);
// 					}
// 					else {
// 						tokens = Misc::tokenize_space(j.first);
// 						tokens[0] = tokens[0] + "|" + label;
// 						std::string line = tokens[0];
// 						for (int k = 1; k < tokens.size(); k++) {
// 							if (std::find(iFile.subcircuitSegments[subckt].io.begin(),
// 								iFile.subcircuitSegments[subckt].io.end(),
// 								tokens[k]) !=
// 								iFile.subcircuitSegments[subckt].io.end()) {
// 								for (auto l : iFile.subcircuitSegments[subckt].io) {
// 									if (l == tokens[k])
// 										tokens[k] =
// 										io.at(Misc::index_of(iFile.subcircuitSegments[subckt].io, l));
// 								}
// 							}
// 							else {
// 								if (tokens[k] != "0" && tokens[k] != "GND")
// 									tokens[k] = tokens[k] + "|" + label;
// 							}
// 						}
// 						for (int k = 1; k < tokens.size(); k++) {
// 							line = line + " " + tokens[k];
// 						}
// 	#pragma omp critical
// 						duplicateSegment.push_back(line);
// 					}
// 				}
// 			}
// 			catch (std::exception &e) {
// 			}
// 		}
// 		else
// 	#pragma omp critical
// 			duplicateSegment.push_back(i);
// 		label = origLabel;
// 	}
// 	#pragma omp barrier
// 	#pragma omp critical
// 	segment = duplicateSegment;
// }