// Copyright (c) 2018 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#include "j_input.hpp"

/*
	Input File Constructor
*/
InputFile::InputFile(std::string iFileName) {
  std::string line;
	// Create a new input file stream using the specified file name
  std::fstream ifile(iFileName);
	// Check to see if the file is open
  if(ifile.is_open()) {
		// While not end of the file
    while(!ifile.eof()) {
			// Read each line into the line variable
      getline(ifile, line);
			// Transform the line vairable to uppercase to simplify the rest of the program
      std::transform(line.begin(), line.end(), line.begin(), toupper);
			// If the line is not empty and ends with a carret return character, remove the character
			if (!line.empty() && line.back() == '\r') line.pop_back();
			// If the line is not empty push it back into the vector of lines
      if (!line.empty() && !starts_with(line, '*')) lines.push_back(line);
    }
  }
	// Error if file could not be opened
  else {
    error_handling(CANNOT_OPEN_FILE);
    exit(0);
  }
}

// Global variabele to store all the parameter values in
std::unordered_map<std::string, double> parVal;

/*
  Split the circuit into subcircuits, main circuit and identify circuit stats
*/
void InputFile::circuit_to_segments(InputFile& iFile) {
	// Variable declarations
  bool startCkt = false;
	bool controlSection = false;
  std::string subcktName;
  subCircuitCount = 0;
	std::vector<std::string> parameterTokens, tokens;
	// Identify the segments of the circuit
  for(auto i : lines) {
		// Clear the tokens to remove any garbage
		tokens.clear();
		// Split the line into tokens
		tokens = tokenize_space(i);
		// If the line in question is the start of a subcircuit
		if(tokens[0] == ".SUBCKT") {
			// The second token will/should be the subcircuit name
			subcktName = tokens[1];
			// The rest of the tokens will be the IO labels
			for (int j = 2; j < tokens.size(); j++) {
				subcircuitSegments[subcktName].io.push_back(tokens[j]);
			}
			// Set the subcircuit flag to true
			startCkt = true;
			// Increase the subcircuit counter
			subCircuitCount++;
		}
		// Else if the subcircuit flag is set
		else if(startCkt) {
			// If the line is not a control card
			if (!starts_with(i, '.')) {
				// Push the line back to the corresponding subcircuit vector
				subcircuitSegments[subcktName].lines.push_back(i);
			}
			// Else if the control card is a parameter
			else if (tokens[0] == ".PARAM") {
				// Tokenize the string using a space and an '=' sign
				// Elementary method will improve in the future
				parameterTokens = tokenize_delimeter(i, " =");
				// Parse the expression to the right of the '=' sign and save as variable of name to the left of '=' sign + subcircuit name
				parse_expression(subcktName + "_" + parameterTokens[1], parameterTokens[2], subcktName);
			}
			// Else if the control card is a model
			else if (tokens[0] == ".MODEL") {
				// Add the model to the corresponding subcircuit models vector
				subcircuitModels[subcktName].push_back(i);
			}
			// Else if the control card ends the subcircuit
			else if(tokens[0] == ".ENDS") {
				// Set the subcircuit flag to false
				startCkt = false;
				// Mark the location of the last subcircuit within the lines vector
			}
		}
		// Else if the line is not a control card and not part of the control section
		else if (!starts_with(i, '.') && !controlSection) {
			// Push the line back to the corresponding subcircuit vector
			maincircuitSegment.push_back(i);
		}
		// Else if this is the start of the control section
		else if (tokens[0] == ".CONTROL") {
			// Set control section flag to true
			controlSection = true;
		}
		// Else if not part of the control section
		else if(!controlSection) {
			// Identify the models in the main part of the circuit
			if(tokens[0] == ".MODEL") maincircuitModels.push_back(i);
			// Identify the parametes in the main part of the circuit
			else if (tokens[0] == ".PARAM") {
				// Tokenize the string using a space and an '=' sign
				// Elementary method will improve in the future
				parameterTokens = tokenize_delimeter(i, " =");
				// Parse the expression to the right of the '=' sign and save as variable of name to the left of '=' sign 
				parse_expression(parameterTokens[1], parameterTokens[2]);
			}
			// Identify the end of the control section
			else if (tokens[0] == ".ENDC") controlSection = false;
			// Identified the end of the circuit file
			else if (tokens[0] == ".END") break;
			// Identify the controls in the main part of the circuit
			else controlPart.push_back(i);
		}
		else if (controlSection) {
			// Identify the models in the main part of the circuit
			if (tokens[0] == "MODEL") maincircuitModels.push_back(i);
			// Identify the parametes in the main part of the circuit
			else if (tokens[0] == "PARAM") {
				// Tokenize the string using a space and an '=' sign
				// Elementary method will improve in the future
				parameterTokens = tokenize_delimeter(i, " =");
				// Parse the expression to the right of the '=' sign and save as variable of name to the left of '=' sign 
				parse_expression(parameterTokens[1], parameterTokens[2]);
			}
			// Identify the end of the control section					
			else if (tokens[0] == "ENDC") controlSection = false;
			// Identify the controls in the main part of the circuit
			else controlPart.push_back(i);
		}
  }
  /* Attempt to count the components in the circuit*/
  /* First the subcircuits */
  for (auto i : subcircuitSegments) {
    subCircuitComponentCount[i.first] = 0;
    subCircuitContainsSubCicuit[i.first] = 0;
    for (auto j : i.second.lines) {
      count_component(j, iFile, i.first);
    }
  }
  int mapValueCount = map_value_count(subCircuitContainsSubCicuit, 1);
  while (mapValueCount != 0) {
    for (auto i : subcircuitSegments) {
      allCounted = 1;
      count_subcircuit_component(i.second.lines, iFile, i.first);
      if (allCounted == 1) subCircuitContainsSubCicuit[i.first] = 0;
    }
    mapValueCount = map_value_count(subCircuitContainsSubCicuit, 1);
    if(VERBOSE) {
		for (auto i : subCircuitContainsSubCicuit) {
			std::cout << i.first << ": " << i.second << '\n';
		}
      std::cout << "Subcircuits to be counted: " << mapValueCount << '\n';
      std::cout << '\n';
    }
  }
  /* Now the main circuit */
  circuitComponentCount = 0;
  for (auto i : maincircuitSegment) {
    count_component(i, iFile);
  }

}

/*
	Substitute subcircuits into the main circuit to create a full main circuit
*/
void InputFile::sub_in_subcircuits(InputFile& iFile, std::vector<std::string>& segment, std::string label) {
	std::vector<std::string> tokens;
	std::string subckt, parString;
	std::vector<std::string> duplicateSegment;
	std::string modelLabel;
	std::string origLabel = label;
	std::vector<std::string> io;
	// Loop through the specified segment
	for (auto i : segment) {
		// If the line in the segment is identified as a subcircuit
		if (i[0] == 'X') {
			tokens = tokenize_space(i);
			try {
				// Identify the subcircuit label
				modelLabel = tokens[0];
				// Check to see if it is a subcircuit within a subcircuit so that labeling can be done right
				if (label == "") label = tokens[0];
				else label = label + "_" + tokens[0];
				// This section is tricky. We need to check if the subcircuit name is at the end or the begining of the line
				// Check if the second token can be identified as a subcircuit name. If yes then
				if(iFile.subcircuitSegments.find(tokens[1]) != iFile.subcircuitSegments.end()) {
					// Identify the type of subcircuit
					subckt = tokens[1];
					// The rest of the tokens should only be the IO nodes which will be matched to the corresponding subcircuit IO
					io.clear();
					io.insert(io.begin(), tokens.begin() + 2, tokens.end());
				}
				else if (iFile.subcircuitSegments.find(tokens.back()) != iFile.subcircuitSegments.end()) {
					// Identify the type of subcircuit
					subckt = tokens.back();
					// The rest of the tokens should only be the IO nodes which will be matched to the corresponding subcircuit IO
					io.clear();
					io.insert(io.begin(), tokens.begin() + 1, tokens.end() - 1);
				}
				else {
					// The subcircuit name was not found therefore error out
					invalid_component_errors(MISSING_SUBCIRCUIT_NAME, i);
				}
				// Check whether the correct amount of nodes was specified for the subcircuit
				if (io.size() != iFile.subcircuitSegments[subckt].io.size()) invalid_component_errors(INVALID_SUBCIRCUIT_NODES, label);
				// Loop through the lines of the identified subcircuit segment
				for (auto j : iFile.subcircuitSegments[subckt].lines) {
					// ignore control cards for the subcircuit. They shouldn't be there in any case
					if (j[0] != '.') {
						// If the line is not a subcircuit line
						if (j[0] != 'X') {
							// Tokenize the line so that tokens can be altered
							tokens = tokenize_space(j);
							// Append the overarching subcircuit label to the label using a underscore. 
							// Eg L01 becomes L01_X01 or L01_X01_X02 depending on the depth
							tokens[0] = tokens[0] + "_" + label;
							// Check if the second token (positive node) is identified as IO node
							if (std::find(iFile.subcircuitSegments[subckt].io.begin(), iFile.subcircuitSegments[subckt].io.end(), tokens[1]) != iFile.subcircuitSegments[subckt].io.end()) {
								for (auto k : iFile.subcircuitSegments[subckt].io) {
									if (k == tokens[1]) {
										// Sub the token for corresponding overarching subcircuit IO node
										tokens[1] = io[index_of(iFile.subcircuitSegments[subckt].io, k)];
										break;
									}
								}
								// If the positive node was identified as IO then the negative node will not be IO as well. Append label
								if (tokens[2] != "0" && tokens[2] != "GND") tokens[2] = tokens[2] + "_" + label;
							}
							// If the third token (negative node) is identified as IO node
							else if (std::find(iFile.subcircuitSegments[subckt].io.begin(), iFile.subcircuitSegments[subckt].io.end(), tokens[2]) != iFile.subcircuitSegments[subckt].io.end()) {
								for (auto k : iFile.subcircuitSegments[subckt].io) {
									// Sub the token for corresponding overarching subcircuit IO node
									if (k == tokens[2]) tokens[2] = io[index_of(iFile.subcircuitSegments[subckt].io, k)];
								}
								// If the negative node was identified as IO then the positive node will not be IO as well. Append label
								if (tokens[1] != "0" && tokens[1] != "GND") tokens[1] = tokens[1] + "_" + label;
							}
							// If neither nodes are IO append the label
							else {
								if (tokens[1] != "0" && tokens[1] != "GND") tokens[1] = tokens[1] + "_" + label;
								if (tokens[2] != "0" && tokens[2] != "GND") tokens[2] = tokens[2] + "_" + label;
								// If the device is a junction then the 4th or 5th (WRSpice) token will be the model therefore append label so it can later be identified.
								if (j[0] == 'B') {
									if(tokens.size() > 5) {
										if(tokens[5].find("AREA=") != std::string::npos) {
											if (tokens[3] != "0" && tokens[3] != "GND") tokens[3] = tokens[3] + "_" + label;
											tokens[4] = subckt + "_" + tokens[4];
										}
									}
									else tokens[3] = subckt + "_" + tokens[3];
								}
							}
							// Now for the tricky part:
							// Subbing in the values if they are parameterized within the subcircuit.
							for (int k = 3; k < tokens.size(); k++) {
								// If the line is a junction then look of the AREA part as this is the only thing that can be parameterized (FOR NOW)
								if(tokens[0][0] == 'B') {
									if(tokens[k].find("AREA=") != std::string::npos)
										if(parVal.find(subckt + "_" + substring_after(tokens[k], "AREA=")) != parVal.end())
											tokens[k] = precise_to_string(parVal[subckt + "_" + substring_after(tokens[k], "AREA=")]);
								}
								// Else loop through all the remaining tokens subbing in parameter values if they exist
								else {
									if(tokens[k][0] == '(') {
										parString = tokens[k].substr(1);
										if(parVal.find(subckt + "_" + parString) != parVal.end())
												tokens[k] = "(" + precise_to_string(parVal[subckt + "_" + parString]);
									}
									else if (tokens[k].back() == ')') {
										parString = tokens[k].substr(0, tokens[k].size()-1);
										if(parVal.find(subckt + "_" + parString) != parVal.end())
												tokens[k] = precise_to_string(parVal[subckt + "_" + parString]) + ")";
									}
									else if(tokens[k].find("PWL(") != std::string::npos) {
										parString = substring_after(tokens[k], "PWL(");
										if(parVal.find(subckt + "_" + parString) != parVal.end())
												tokens[k] = "PWL(" + precise_to_string(parVal[subckt + "_" + parString]);
									}
									else if(tokens[k].find("PULSE(") != std::string::npos) {
										parString = substring_after(tokens[k], "PULSE(");
										if(parVal.find(subckt + "_" + parString) != parVal.end())
												tokens[k] = "PULSE(" + precise_to_string(parVal[subckt + "_" + parString]);
									}
									else {
										if(parVal.find(subckt + "_" + tokens[k]) != parVal.end())
												tokens[k] = precise_to_string(parVal[subckt + "_" + tokens[k]]);
									}
								}
							}
							std::string line = tokens[0];
							for (int k = 1; k < tokens.size(); k++) {
								line = line + " " + tokens[k];
							}
							duplicateSegment.push_back(line);
						}
						// Else if line is a subcircuit
						else {
							tokens = tokenize_space(j);
							tokens[0] = tokens[0] + "_" + label;
							std::string line = tokens[0];
							for (int k = 2; k < tokens.size(); k++) {
								if (std::find(iFile.subcircuitSegments[subckt].io.begin(), iFile.subcircuitSegments[subckt].io.end(), tokens[k]) != iFile.subcircuitSegments[subckt].io.end()) {
									for (auto l : iFile.subcircuitSegments[subckt].io) {
										if (l == tokens[k]) tokens[k] = io[index_of(iFile.subcircuitSegments[subckt].io, l)];
									}
								}
								else {
									if (tokens[k] != "0" && tokens[k] != "GND") tokens[k] = tokens[k] + "_" + label;
								}
							}
							for (int k = 1; k < tokens.size(); k++) {
								line = line + " " + tokens[k];
							}
							duplicateSegment.push_back(line);
						}
					}
				}
			}
			catch (std::out_of_range) {}
		}
		else duplicateSegment.push_back(i);
		label = origLabel;
	}
	segment = duplicateSegment;
}
