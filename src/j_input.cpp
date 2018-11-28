// Copyright (c) 2018 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#include "j_input.h"

void
check_model(std::string s, std::string sbcktName) {
	std::string modelTypes[] = {"JJ"};//, "MTJ", "NTRON", "CSHE"};
	std::vector<std::string> t;//, p;
	t = Misc::tokenize_delimeter(s, " \t(),");
	if(std::count(modelTypes, modelTypes+sizeof(modelTypes), t[2]) != 0) {
		if (sbcktName != "") iFile.models[sbcktName + "|" + t[1]] = s;
		else iFile.models[t[1]] = s;
	}
	else Errors::model_errors(UNKNOWN_MODEL_TYPE, t[2]);
}

void
InputFile::read_input_file(std::string iFileName) {
	std::string line;
	// Create a new input file stream using the specified file name
	std::fstream ifile(iFileName);
	// Check to see if the file is open
	if (ifile.is_open()) {
		// While not end of the file
		while (!ifile.eof()) {
			// Read each line into the line variable
			getline(ifile, line);
			// Transform the line variable to uppercase to simplify program
			std::transform(line.begin(), line.end(), line.begin(), toupper);
			// If not empty and ends with carret return, remove the character
			if (!line.empty() && line.back() == '\r')
				line.pop_back();
			// If the line is not empty push it back into the vector of lines
			if (!line.empty() && !Misc::starts_with(line, '*'))
				lines.push_back(line);
		}
	}
	// Error if file could not be opened
	else {
		Errors::error_handling(CANNOT_OPEN_FILE);
		exit(0);
	}
}

void
param_values::insertUParam(std::string paramName, std::string paramExpression, 
	std::string subcktName) {
	if(subcktName == "") {
		unparsedMap[paramName] = paramExpression;
	}
	else {
		paramName = paramName + "|" + subcktName;
		unparsedMap[paramName] = paramExpression;
	}
}

void
param_values::insertParam(std::string paramName, double paramValue, std::string subcktName) {
	if(subcktName == "") {
		paramMap[paramName] = paramValue;
	}
	else {
		paramName = paramName + "|" + subcktName;
		paramMap[paramName] = paramValue;
	}
}

double
param_values::returnParam(std::string paramName, std::string subcktName) {
	if(subcktName == "") {
		try {
			return paramMap.at(paramName);
		}
		catch (std::exception &e) {
			Errors::parsing_errors(UNIDENTIFIED_PART, paramName);
		}
	}
	else {
		paramName = paramName + "|" + subcktName;
		try {
			return paramMap.at(paramName);
		}
		catch (std::exception &e) {
			Errors::parsing_errors(UNIDENTIFIED_PART, paramName);
		}
	}
	return 0.0;
}

std::vector<std::string>
recurseSubckt(std::unordered_map<std::string, Subcircuit> subckts, 
			  std::string part) {
	std::vector<std::string> t, ckts;
	ckts.clear();
	if (subckts[part].containsSubckt) {
		for (auto i : subckts[part].subckts) {
			t = recurseSubckt(subckts, i);
			ckts.insert(ckts.end(), t.begin(), t.end());
		}
	}
	if (!subckts[part].subckts.empty())
		ckts.insert(ckts.end(), subckts[part].subckts.begin(), subckts[part].subckts.end());
	return ckts;
}

void
InputFile::circuit_to_segments() {
	bool startCkt = false;
	bool controlSection = false;
	std::string subcktName, varName;
	iFile.subCircuitCount = 0;
	std::vector<std::string> parameterTokens, tokens, v;
	for (const auto& i : iFile.lines) {
		tokens = Misc::tokenize_space(i);
		if (tokens[0] == ".SUBCKT") {
			subcktName = tokens[1];
			iFile.subcircuitSegments[subcktName].name = subcktName;
			iFile.subcircuitSegments[subcktName].io.assign(tokens.begin() + 2, tokens.end());
			startCkt = true;
			iFile.subCircuitCount++;
		}
		else if (startCkt) {
			if (tokens[0][0] != '.') {
				iFile.subcircuitSegments[subcktName].lines.push_back(i);
				if (tokens[0][0] != 'X') iFile.subcircuitSegments[subcktName].componentCount++;
				if (tokens[0][0] == 'B') iFile.subcircuitSegments[subcktName].jjCount++;
				if (tokens[0][0] == 'X') {
					iFile.subcircuitSegments[subcktName].containsSubckt = true;
					if (cArg.sbcktConv == 0) iFile.subcircuitSegments[subcktName].subckts.push_back(tokens[1]);
					else if (cArg.sbcktConv == 1) iFile.subcircuitSegments[subcktName].subckts.push_back(tokens.back());
				}
			}
			else if (tokens[0] == ".PARAM") {
				parameterTokens = Misc::tokenize_delimeter(i, " =");
				varName = parameterTokens[1];
				parameterTokens = Misc::tokenize_delimeter(i, "=");
				iFile.paramValues.insertUParam(varName,
					parameterTokens[1], subcktName);
				// Parser::parse_expression(varName,
				// 	parameterTokens[1], subcktName);
			}
			else if (tokens[0] == ".MODEL") {
				check_model(i, subcktName);
			}
			else if (tokens[0] == ".ENDS") {
				startCkt = false;
			}
		}
		else if (tokens[0][0] != '.' && !controlSection) {
			iFile.maincircuitSegment.push_back(i);
			if (tokens[0][0] != 'X') iFile.mainComponents++;
			if (tokens[0][0] == 'B') iFile.mainJJs++;
			if (tokens[0][0] == 'X') {
				if (cArg.sbcktConv == 0) iFile.subckts.push_back(tokens[1]);
				else if (cArg.sbcktConv == 1) iFile.subckts.push_back(tokens.back());
			}
		}
		else if (tokens[0] == ".CONTROL") {
			controlSection = true;
		}
		else if (!controlSection) {
			if (tokens[0] == ".MODEL") {
				check_model(i);
			}
			else if (tokens[0] == ".PARAM") {
				parameterTokens = Misc::tokenize_delimeter(i, " =");
				varName = parameterTokens[1];
				parameterTokens = Misc::tokenize_delimeter(i, "=");
				iFile.paramValues.insertUParam(varName,
					parameterTokens[1]);
				// Parser::parse_expression(varName, parameterTokens[1]);
			}
			else if (tokens[0] == ".ENDC")
				controlSection = false;
			else iFile.controlPart.push_back(i);
		}
		else if (controlSection) {
			if (tokens[0] == "MODEL") {
				check_model(i);
			}
			else if (tokens[0] == "PARAM") {
				parameterTokens = Misc::tokenize_delimeter(i, " =");
				varName = parameterTokens[1];
				parameterTokens = Misc::tokenize_delimeter(i, "=");
				iFile.paramValues.insertUParam(varName,
					parameterTokens[1]);
				// Parser::parse_expression(varName, parameterTokens[1]);
			}
			else if (tokens[0] == "ENDC")
				controlSection = false;
			else iFile.controlPart.push_back(i);
		}
	}
	for (auto i : iFile.paramValues.unparsedMap) {
		if(i.first.find("|") != std::string::npos)
			subcktName = i.first.substr(i.first.find("|") + 1, i.first.size() - 1);
		else subcktName = "";
		Parser::parse_expression(i.first, i.second, subcktName);
	}
	std::vector<std::string> mainSubckts = iFile.subckts;
	for (auto i : mainSubckts) {
		v = recurseSubckt(iFile.subcircuitSegments, i);
		if (!v.empty()) iFile.subckts.insert(iFile.subckts.end(), v.begin(), v.end());
		v.empty();
	}
	iFile.circuitComponentCount += iFile.mainComponents;
	iFile.circuitJJCount += iFile.mainJJs;
	for (int i = 0; i < iFile.subckts.size(); i++) {
		iFile.circuitComponentCount += iFile.subcircuitSegments[iFile.subckts[i]].componentCount;
		iFile.circuitJJCount += iFile.subcircuitSegments[iFile.subckts[i]].jjCount;
	}
}

void
InputFile::sub_in_subcircuits(std::vector<std::string>& segment, 
							  std::string label) {
	std::vector<std::string> tokens, duplicateSegment, io;
	std::string subckt,
		parString,
		modelLabel,
		i,
		origLabel = label;
	#pragma omp parallel for private(subckt, parString, modelLabel, i, origLabel, tokens, io, label)
	for (int o = 0; o < segment.size(); o++) {
		i = segment[o];
		if (i[0] == 'X') {
			tokens = Misc::tokenize_space(i);
			try {
				modelLabel = tokens[0];
				if (label.empty())
					label = tokens[0];
				else
					label.append("|" + tokens[0]);
				if (cArg.sbcktConv == 0) {
					if (tokens[1].find("|") != std::string::npos) subckt = tokens[1].substr(0, tokens[1].find_first_of("|"));
					else subckt = tokens[1];
					io.clear();
					io.insert(io.begin(), tokens.begin() + 2, tokens.end());
				}
				else if (cArg.sbcktConv == 1) {
					if (tokens.back().find("|") != std::string::npos) subckt = tokens.back().substr(0, tokens.back().find_first_of("|"));
					else subckt = tokens.back();
					io.clear();
					io.insert(io.begin(), tokens.begin() + 1, tokens.end() - 1);
				}
				if (iFile.subcircuitNameMap.count(label) == 0) {
					iFile.subcircuitNameMap[label] = subckt;
				}
				if (io.size() != iFile.subcircuitSegments[subckt].io.size())
					Errors::invalid_component_errors(INVALID_SUBCIRCUIT_NODES, label);
				for (auto j : iFile.subcircuitSegments[subckt].lines) {
					if (j[0] != 'X') {
						tokens = Misc::tokenize_space(j);
						tokens[0] = tokens[0] + "|" + label;
						if (std::find(iFile.subcircuitSegments[subckt].io.begin(),
							iFile.subcircuitSegments[subckt].io.end(),
							tokens[1]) !=
							iFile.subcircuitSegments[subckt].io.end()) {
							for (const auto& k : iFile.subcircuitSegments[subckt].io) {
								if (k == tokens[1]) {
									tokens[1] =
										io.at(Misc::index_of(iFile.subcircuitSegments[subckt].io, k));
									break;
								}
							}
							if (std::find(iFile.subcircuitSegments[subckt].io.begin(),
								iFile.subcircuitSegments[subckt].io.end(),
								tokens[2]) !=
								iFile.subcircuitSegments[subckt].io.end()) {
								for (const auto& k : iFile.subcircuitSegments[subckt].io) {
									if (k == tokens[2]) {
										tokens[2] =
											io.at(Misc::index_of(iFile.subcircuitSegments[subckt].io, k));
										break;
									}
								}
							}
							else {
								if (tokens[2] != "0" && tokens[2] != "GND")
									tokens[2] = tokens[2] + "|" + label;
							}
						}
						else if (std::find(iFile.subcircuitSegments[subckt].io.begin(),
							iFile.subcircuitSegments[subckt].io.end(),
							tokens[2]) !=
							iFile.subcircuitSegments[subckt].io.end()) {
							for (const auto& k : iFile.subcircuitSegments[subckt].io) {
								if (k == tokens[2])
									tokens[2] =
									io.at(Misc::index_of(iFile.subcircuitSegments[subckt].io, k));
							}
							if (tokens[1] != "0" && tokens[1] != "GND")
								tokens[1] = tokens[1] + "|" + label;
						}
						else {
							if (tokens[1] != "0" && tokens[1] != "GND")
								tokens[1] = tokens[1] + "|" + label;
							if (tokens[2] != "0" && tokens[2] != "GND")
								tokens[2] = tokens[2] + "|" + label;
						}
						if (j[0] == 'B') {
							if (tokens.size() > 5) {
								if (tokens[5].find("AREA=") != std::string::npos) {
									if (tokens[3] != "0" && tokens[3] != "GND")
										tokens[3] = tokens[3] + "|" + label;
									tokens[4] = subckt + "|" + tokens[4];
								}
							}
							else
								tokens[3] = subckt + "|" + tokens[3];
						}
						for (int k = 3; k < tokens.size(); k++) {
							if (tokens[0][0] == 'B') {
								if (tokens[k].find("AREA=") != std::string::npos)
									if (iFile.paramValues.paramMap.count(
											Misc::substring_after(tokens[k], "AREA=")
															+ "|" + subckt) != 0)
										tokens[k] = Misc::precise_to_string(
											iFile.paramValues.paramMap.at(
												Misc::substring_after(tokens[k], "AREA=")
												+ "|" + subckt));
							}
							else {
								if (tokens[k][0] == '(') {
									parString = tokens[k].substr(1);
									if (iFile.paramValues.paramMap.count(parString + "|" + subckt) != 0)
										tokens[k] = "(" + Misc::precise_to_string(
											iFile.paramValues.paramMap.at(parString + "|" + subckt));
								}
								else if (tokens[k].back() == ')') {
									parString = tokens[k].substr(0, tokens[k].size() - 1);
									if (iFile.paramValues.paramMap.count(parString + "|" + subckt) != 0)
										tokens[k] =
										Misc::precise_to_string(iFile.paramValues.paramMap.at(parString + "|" + subckt)) +
										")";
								}
								else if (tokens[k].find("PWL(") != std::string::npos) {
									parString = Misc::substring_after(tokens[k], "PWL(");
									if (iFile.paramValues.paramMap.count(parString + "|" + subckt) != 0)
										tokens[k] = "PWL(" + Misc::precise_to_string(
											iFile.paramValues.paramMap.at(parString + "|" + subckt));
								}
								else if (tokens[k].find("PULSE(") != std::string::npos) {
									parString = Misc::substring_after(tokens[k], "PULSE(");
									if (iFile.paramValues.paramMap.count(parString + "|" + subckt) != 0)
										tokens[k] =
										"PULSE(" +
										Misc::precise_to_string(iFile.paramValues.paramMap.at(parString + "|" + subckt));
								}
								else {
									if (iFile.paramValues.paramMap.count(tokens[k] + "|" + subckt) != 0)
										tokens[k] =
										Misc::precise_to_string(iFile.paramValues.paramMap.at(tokens[k] + "|" + subckt));
								}
							}
						}
						std::string line = tokens[0];
						for (int k = 1; k < tokens.size(); k++) {
							line = line + " " + tokens[k];
						}
	#pragma omp critical
						duplicateSegment.push_back(line);
					}
					else {
						tokens = Misc::tokenize_space(j);
						tokens[0] = tokens[0] + "|" + label;
						std::string line = tokens[0];
						for (int k = 1; k < tokens.size(); k++) {
							if (std::find(iFile.subcircuitSegments[subckt].io.begin(),
								iFile.subcircuitSegments[subckt].io.end(),
								tokens[k]) !=
								iFile.subcircuitSegments[subckt].io.end()) {
								for (auto l : iFile.subcircuitSegments[subckt].io) {
									if (l == tokens[k])
										tokens[k] =
										io.at(Misc::index_of(iFile.subcircuitSegments[subckt].io, l));
								}
							}
							else {
								if (tokens[k] != "0" && tokens[k] != "GND")
									tokens[k] = tokens[k] + "|" + label;
							}
						}
						for (int k = 1; k < tokens.size(); k++) {
							line = line + " " + tokens[k];
						}
	#pragma omp critical
						duplicateSegment.push_back(line);
					}
				}
			}
			catch (std::exception &e) {
			}
		}
		else
	#pragma omp critical
			duplicateSegment.push_back(i);
		label = origLabel;
	}
	#pragma omp barrier
	#pragma omp critical
	segment = duplicateSegment;
}