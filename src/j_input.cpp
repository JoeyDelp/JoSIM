// Copyright (c) 2019 Johannes Delport
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

	version_info();
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
                    argPlot = true;
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
                #ifdef _OPENMP
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
	std::cout << "Copyright (C) 2019 by Johannes Delport (jdelport@sun.ac.za)"
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
		exit(-1);
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
						exit(-1);
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
				exit(-1);
			}
		}
		else {
			expNetlist.push_back(std::make_pair(netlist.maindesign.at(i), ""));
		}
	}
}
