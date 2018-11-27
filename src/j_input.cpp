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
	// Variable declarations
	bool startCkt = false;
	bool controlSection = false;
	std::string subcktName, varName;
	iFile.subCircuitCount = 0;
	std::vector<std::string> parameterTokens, tokens, v;
	// Identify the segments of the circuit
	for (const auto& i : iFile.lines) {
		// Clear the tokens to remove any garbage
		tokens.clear();
		// Split the line into tokens
		tokens = Misc::tokenize_space(i);
		// If the line in question is the start of a subcircuit
		if (tokens[0] == ".SUBCKT") {
			// The second token will/should be the subcircuit name
			subcktName = tokens[1];
			// The rest of the tokens will be the IO labels
			iFile.subcircuitSegments[subcktName].name = subcktName;
			iFile.subcircuitSegments[subcktName].io.assign(tokens.begin() + 2, tokens.end());
			// Set the subcircuit flag to true
			startCkt = true;
			// Increase the subcircuit counter
			iFile.subCircuitCount++;
		}
		// Else if the subcircuit flag is set
		else if (startCkt) {
			// If the line is not a control card
			if (tokens[0][0] != '.') {
				// Push the line back to the corresponding subcircuit vector
				iFile.subcircuitSegments[subcktName].lines.push_back(i);
				if (tokens[0][0] != 'X') iFile.subcircuitSegments[subcktName].componentCount++;
				if (tokens[0][0] == 'B') iFile.subcircuitSegments[subcktName].jjCount++;
				if (tokens[0][0] == 'X') {
					iFile.subcircuitSegments[subcktName].containsSubckt = true;
					if (cArg.sbcktConv == 0) iFile.subcircuitSegments[subcktName].subckts.push_back(tokens[1]);
					else if (cArg.sbcktConv == 1) iFile.subcircuitSegments[subcktName].subckts.push_back(tokens.back());
				}
			}
			// Else if the control card is a parameter
			else if (tokens[0] == ".PARAM") {
				// Tokenize the string using a space and an '=' sign
				// Elementary method will improve in the future
				parameterTokens = Misc::tokenize_delimeter(i, " =");
				// Parse the expression to the right of the '=' sign and save as
				// variable of name to the left of '=' sign + subcircuit name
				varName = parameterTokens[1];
				parameterTokens = Misc::tokenize_delimeter(i, "=");
				Parser::parse_expression(varName,
					parameterTokens[1], iFile.subcircuitSegments[subcktName].parVal, iFile.parVal,
					subcktName);
			}
			// Else if the control card is a model
			else if (tokens[0] == ".MODEL") {
				// Add the model to the corresponding subcircuit models vector
				check_model(i, subcktName);
			}
			// Else if the control card ends the subcircuit
			else if (tokens[0] == ".ENDS") {
				// Set the subcircuit flag to false
				startCkt = false;
				// Mark the location of the last subcircuit within the lines vector
			}
		}
		// Else if the line is not a control card and not part of the control
		// section
		else if (tokens[0][0] != '.' && !controlSection) {
			// Push the line back to the corresponding subcircuit vector
			iFile.maincircuitSegment.push_back(i);
			if (tokens[0][0] != 'X') iFile.mainComponents++;
			if (tokens[0][0] == 'B') iFile.mainJJs++;
			if (tokens[0][0] == 'X') {
				if (cArg.sbcktConv == 0) iFile.subckts.push_back(tokens[1]);
				else if (cArg.sbcktConv == 1) iFile.subckts.push_back(tokens.back());
			}
		}
		// Else if this is the start of the control section
		else if (tokens[0] == ".CONTROL") {
			// Set control section flag to true
			controlSection = true;
		}
		// Else if not part of the control section
		else if (!controlSection) {
			// Identify the models in the main part of the circuit
			if (tokens[0] == ".MODEL") {
				check_model(i);
			}
			// Identify the parametes in the main part of the circuit
			else if (tokens[0] == ".PARAM") {
				// Tokenize the string using a space and an '=' sign
				// Elementary method will improve in the future
				parameterTokens = Misc::tokenize_delimeter(i, " =");
				// Parse the expression to the right of the '=' sign and save as
				// variable of name to the left of '=' sign
				varName = parameterTokens[1];
				parameterTokens = Misc::tokenize_delimeter(i, "=");
				Parser::parse_expression(varName, parameterTokens[1], iFile.parVal, iFile.parVal);
			}
			// Identify the end of the control section
			else if (tokens[0] == ".ENDC")
				controlSection = false;
			// Identify the controls in the main part of the circuit
			else iFile.controlPart.push_back(i);
		}
		else if (controlSection) {
			// Identify the models in the main part of the circuit
			if (tokens[0] == "MODEL") {
				check_model(i);
			}
			// Identify the parametes in the main part of the circuit
			else if (tokens[0] == "PARAM") {
				// Tokenize the string using a space and an '=' sign
				// Elementary method will improve in the future
				parameterTokens = Misc::tokenize_delimeter(i, " =");
				// Parse the expression to the right of the '=' sign and save as
				// variable of name to the left of '=' sign
				varName = parameterTokens[1];
				parameterTokens = Misc::tokenize_delimeter(i, "=");
				Parser::parse_expression(varName, parameterTokens[1], iFile.parVal, iFile.parVal);
			}
			// Identify the end of the control section
			else if (tokens[0] == "ENDC")
				controlSection = false;
			// Identify the controls in the main part of the circuit
			else iFile.controlPart.push_back(i);
		}
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
	// Loop through the specified segment
	#pragma omp parallel for private(subckt, parString, modelLabel, i, origLabel, tokens, io, label)
	for (int o = 0; o < segment.size(); o++) {
		i = segment[o];
		// If the line in the segment is identified as a subcircuit
		if (i[0] == 'X') {
			tokens = Misc::tokenize_space(i);
			try {
				// Identify the subcircuit label
				modelLabel = tokens[0];
				// Check to see if it is a subcircuit within a subcircuit so that
				// labeling can be done right
				if (label.empty())
					label = tokens[0];
				else
					label.append("|" + tokens[0]);
				// Depending on subcircuit convention, use 2nd or last token
				if (cArg.sbcktConv == 0) {
					// Identify the type of subcircuit
					if (tokens[1].find("|") != std::string::npos) subckt = tokens[1].substr(0, tokens[1].find_first_of("|"));
					else subckt = tokens[1];
					// The rest of the tokens should only be the IO nodes which will be
					// matched to the corresponding subcircuit IO
					io.clear();
					io.insert(io.begin(), tokens.begin() + 2, tokens.end());
				}
				else if (cArg.sbcktConv == 1) {
					// Identify the type of subcircuit
					if (tokens.back().find("|") != std::string::npos) subckt = tokens.back().substr(0, tokens.back().find_first_of("|"));
					else subckt = tokens.back();
					// The rest of the tokens should only be the IO nodes which will be
					// matched to the corresponding subcircuit IO
					io.clear();
					io.insert(io.begin(), tokens.begin() + 1, tokens.end() - 1);
				}
				// Create the subcircuit name map to identify subcircuits by their labels
				if (iFile.subcircuitNameMap.count(label) == 0) {
					iFile.subcircuitNameMap[label] = subckt;
				}
				// Check whether the correct amount of nodes was specified for the
				// subcircuit
				if (io.size() != iFile.subcircuitSegments[subckt].io.size())
					Errors::invalid_component_errors(INVALID_SUBCIRCUIT_NODES, label);
				// Loop through the lines of the identified subcircuit segment
				for (auto j : iFile.subcircuitSegments[subckt].lines) {
					// If the line is not a subcircuit line
					if (j[0] != 'X') {
						// Tokenize the line so that tokens can be altered
						tokens = Misc::tokenize_space(j);
						// Append the overarching subcircuit label to the label using a
						// underscore. Eg L01 becomes L01_X01 or L01_X01_X02 depending on
						// the depth
						tokens[0] = tokens[0] + "|" + label;
						// Check if the second token (positive node) is identified as IO
						// node
						if (std::find(iFile.subcircuitSegments[subckt].io.begin(),
							iFile.subcircuitSegments[subckt].io.end(),
							tokens[1]) !=
							iFile.subcircuitSegments[subckt].io.end()) {
							for (const auto& k : iFile.subcircuitSegments[subckt].io) {
								if (k == tokens[1]) {
									// Sub the token for corresponding overarching subcircuit IO
									// node
									tokens[1] =
										io.at(Misc::index_of(iFile.subcircuitSegments[subckt].io, k));
									break;
								}
							}
							// Check if the third token (negative node) is identified as IO
							// node
							if (std::find(iFile.subcircuitSegments[subckt].io.begin(),
								iFile.subcircuitSegments[subckt].io.end(),
								tokens[2]) !=
								iFile.subcircuitSegments[subckt].io.end()) {
								for (const auto& k : iFile.subcircuitSegments[subckt].io) {
									if (k == tokens[2]) {
										// Sub the token for corresponding overarching subcircuit IO
										// node
										tokens[2] =
											io.at(Misc::index_of(iFile.subcircuitSegments[subckt].io, k));
										break;
									}
								}
							}
							// Negative node is not IO
							else {
								if (tokens[2] != "0" && tokens[2] != "GND")
									tokens[2] = tokens[2] + "|" + label;
							}
							//if (tokens[2] != "0" && tokens[2] != "GND")
								//tokens[2] = tokens[2] + "|" + label;
						}
						// If the third token (negative node) is identified as IO node
						else if (std::find(iFile.subcircuitSegments[subckt].io.begin(),
							iFile.subcircuitSegments[subckt].io.end(),
							tokens[2]) !=
							iFile.subcircuitSegments[subckt].io.end()) {
							for (const auto& k : iFile.subcircuitSegments[subckt].io) {
								// Sub the token for corresponding overarching subcircuit IO
								// node
								if (k == tokens[2])
									tokens[2] =
									io.at(Misc::index_of(iFile.subcircuitSegments[subckt].io, k));
							}
							// If the negative node was identified as IO then the positive
							// node will not be IO as well. Append label
							if (tokens[1] != "0" && tokens[1] != "GND")
								tokens[1] = tokens[1] + "|" + label;
						}
						// If neither nodes are IO append the label
						else {
							if (tokens[1] != "0" && tokens[1] != "GND")
								tokens[1] = tokens[1] + "|" + label;
							if (tokens[2] != "0" && tokens[2] != "GND")
								tokens[2] = tokens[2] + "|" + label;
						}
						// If the device is a junction then the 4th or 5th (WRSpice)
						// token will be the model therefore append label so it can
						// later be identified.
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
						// Now for the tricky part:
						// Subbing in the values if they are parameterized within the
						// subcircuit.
						for (int k = 3; k < tokens.size(); k++) {
							// If the line is a junction then look of the AREA part as this
							// is the only thing that can be parameterized (FOR NOW)
							if (tokens[0][0] == 'B') {
								if (tokens[k].find("AREA=") != std::string::npos)
									if (iFile.parVal.find(subckt + "|" +
										Misc::substring_after(tokens[k], "AREA=")) !=
										iFile.parVal.end())
										tokens[k] = Misc::precise_to_string(
											iFile.parVal.at(subckt + "|" +
												Misc::substring_after(tokens[k], "AREA=")));
							}
							// Else loop through all the remaining tokens subbing in
							// parameter values if they exist
							else {
								// If token starts with a brace
								if (tokens[k][0] == '(') {
									// Remove the brace
									parString = tokens[k].substr(1);
									// Check if the value is a parameter value
									if (iFile.parVal.find(subckt + "_" + parString) != iFile.parVal.end())
										// Replace if it is and put back the brace
										tokens[k] = "(" + Misc::precise_to_string(
											iFile.parVal.at(subckt + "_" + parString));
									// Else if the token ends with a brace
								}
								else if (tokens[k].back() == ')') {
									// Remove the brace
									parString = tokens[k].substr(0, tokens[k].size() - 1);
									// Check if the value is a parameter value
									if (iFile.parVal.find(subckt + "_" + parString) != iFile.parVal.end())
										// Replace if it is and put back the brace
										tokens[k] =
										Misc::precise_to_string(iFile.parVal.at(subckt + "_" + parString)) +
										")";
									// If the token starts with something like PWL OR PULSE
								}
								else if (tokens[k].find("PWL(") != std::string::npos) {
									parString = Misc::substring_after(tokens[k], "PWL(");
									if (iFile.parVal.find(subckt + "_" + parString) != iFile.parVal.end())
										tokens[k] = "PWL(" + Misc::precise_to_string(
											iFile.parVal.at(subckt + "_" + parString));
								}
								else if (tokens[k].find("PULSE(") != std::string::npos) {
									parString = Misc::substring_after(tokens[k], "PULSE(");
									if (iFile.parVal.find(subckt + "_" + parString) != iFile.parVal.end())
										tokens[k] =
										"PULSE(" +
										Misc::precise_to_string(iFile.parVal.at(subckt + "_" + parString));
									// Else check the token and replace if it is a parementer
								}
								else {
									if (iFile.parVal.find(subckt + "_" + tokens[k]) != iFile.parVal.end())
										tokens[k] =
										Misc::precise_to_string(iFile.parVal.at(subckt + "_" + tokens[k]));
								}
							}
						}
						// Recreate string from the tokens
						std::string line = tokens[0];
						for (int k = 1; k < tokens.size(); k++) {
							line = line + " " + tokens[k];
						}
	#pragma omp critical
						duplicateSegment.push_back(line);
					}
					// Else if line is a subcircuit (subcircuit in subcircuit)
					else {
						// Tokenize the line
						tokens = Misc::tokenize_space(j);
						// Attach the subcircuit label to the label
						tokens[0] = tokens[0] + "|" + label;
						// The string to reconstruct the line
						std::string line = tokens[0];
						// Loop through tokens replacing
						for (int k = 1; k < tokens.size(); k++) {
							// If node is IO
							if (std::find(iFile.subcircuitSegments[subckt].io.begin(),
								iFile.subcircuitSegments[subckt].io.end(),
								tokens[k]) !=
								iFile.subcircuitSegments[subckt].io.end()) {
								// Loop through subcircuit IO
								for (auto l : iFile.subcircuitSegments[subckt].io) {
									if (l == tokens[k])
										// Rplace IO with top level nodes
										tokens[k] =
										io.at(Misc::index_of(iFile.subcircuitSegments[subckt].io, l));
								}
							}
							else {
								// If node is not ground, append label
								if (tokens[k] != "0" && tokens[k] != "GND")
									tokens[k] = tokens[k] + "|" + label;
							}
						}
						// Reconstruct the line
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