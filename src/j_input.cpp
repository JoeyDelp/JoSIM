// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#include "JoSIM/j_input.h"

#include "JoSIM/InputType.hpp"

using namespace JoSIM;

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
						Parameters &parameters,
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
	for(const auto& i : netlist.subcircuits) {
		for (auto j : i.second.lines) {
			if (j.first[0] == 'X') {
				netlist.subcircuits.at(i.first).containsSubckt = true;
				netlist.nestedSubcktCount++;
			}
		}
	}
	while(netlist.nestedSubcktCount != 0) {
		for(const auto& i : netlist.subcircuits) {
			for (int j = 0; j < netlist.subcircuits.at(i.first).lines.size(); j++) {
				if(netlist.subcircuits.at(i.first).lines.at(j).first[0] == 'X') {
					tokens = Misc::tokenize_space(netlist.subcircuits.at(i.first).lines.at(j).first);
					label = tokens.at(0);
					if(argConv == InputType::Jsim) /* LEFT */ {
						subcktName = tokens.at(1);
						io.assign(tokens.begin() + 2, tokens.end());
					}
					else if (argConv == InputType::WrSpice) /* RIGHT */ {
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
								for (int m = 1; m < tokens.size(); m++) line += " " + tokens.at(m);
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
			if(argConv == InputType::Jsim) /* LEFT */ {
				subcktName = tokens.at(1);
				io.assign(tokens.begin() + 2, tokens.end());
			}
			else if (argConv == InputType::WrSpice) /* RIGHT */ {
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
					for (int m = 1; m < tokens.size(); m++) line += " " + tokens.at(m);
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
