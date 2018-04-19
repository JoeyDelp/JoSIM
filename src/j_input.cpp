#ifdef WIN32
#include "include/j_input.hpp"
#else
#include "j_input.hpp"
#endif

/* Input File Constructor*/
InputFile::InputFile(std::string iFileName) {
  std::string line;
  std::fstream ifile(iFileName);
  if(ifile.is_open()) {
    while(!ifile.eof()) {
      getline(ifile, line);
      std::transform(line.begin(), line.end(), line.begin(), toupper);
      lines.push_back(line);
    }
  }
  else {
    error_handling(CANNOT_OPEN_FILE);
    exit(0);
  }
}

/*
  Split the circuit into subcircuits, main circuit and identify circuit stats
*/
void InputFile::circuit_to_segments(InputFile& iFile) {
  bool startCkt = false;
  std::string subcktName;
  int posLastSubCkt = -1;
  int counter = 0;
  /* Identify the subcircuits in the circuit and push them to a map of subcircuits*/
  counter = 0;
  subCircuitCount = 0;
  for(auto i : lines) {
    if (!starts_with(i, '*')) {
		std::vector<std::string> tokens;
      if(i.find(".SUBCKT") != std::string::npos) {
		  tokens = tokenize_space(i);
		  subcktName = tokens[1];
		  for (int j = 2; j < tokens.size(); j++) {
			  subcircuitSegments[subcktName].io.push_back(tokens[j]);
		  }
		  startCkt = true;
        subCircuitCount++;
      }
      if(startCkt) if(!i.empty()) subcircuitSegments[subcktName].lines.push_back(i);
	  if (startCkt) {
		  if (starts_with(i, '.')) {
			  if (i.find("END") == std::string::npos) {
				  if (i.find("MODEL") != std::string::npos) {
					  subcircuitModels[subcktName].push_back(i);
				  }
			  }
		  }
	  }
      if(i.find(".ENDS") != std::string::npos) {
        startCkt = false;
        posLastSubCkt = counter;
      }
    }
    counter++;
  }  
  std::map<std::string, std::string> subCircuitCounted;
  /* Identify the main part of the circuit and push it to a string vector*/
  counter = 0;
  for (auto i : lines) {
    if (counter == (posLastSubCkt + 1)) startCkt = true;
    if(startCkt) {
      if (!starts_with(i, '*')) {
        if(!i.empty()) if(!starts_with(i, '.')) maincircuitSegment.push_back(i);
		/* Identify the controls in the main part of the circuit */
        if(starts_with(i, '.')) if(i.find("END") == std::string::npos) if(i.find("MODEL") == std::string::npos) controlPart.push_back(i);
		/* Identify the models in the main part of the circuit */
		if (starts_with(i, '.')) if (i.find("END") == std::string::npos) if (i.find("MODEL") != std::string::npos) maincircuitModels.push_back(i);
      }
    }
    counter++;
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
	std::string subckt;
	std::vector<std::string> duplicateSegment;
	std::string modelLabel;
	std::string origLabel = label;
	for (auto i : segment) {
		if (i[0] == 'X') {
			tokens = tokenize_space(i);
			try {
				modelLabel = tokens[0];
				if (label == "") label = tokens[0];
				else label = label + "_" + tokens[0];
				subckt = tokens[1];
				std::vector<std::string> io(tokens.begin() + 2, tokens.end());
				if (io.size() != iFile.subcircuitSegments[subckt].io.size()) invalid_component_errors(INVALID_SUBCIRCUIT_NODES, label);
				for (auto j : iFile.subcircuitSegments[subckt].lines) {
					if (j[0] != '.') {
						if (j[0] != 'X') {
							tokens = tokenize_space(j);
							tokens[0] = tokens[0] + "_" + label;
							if (std::find(iFile.subcircuitSegments[subckt].io.begin(), iFile.subcircuitSegments[subckt].io.end(), tokens[1]) != iFile.subcircuitSegments[subckt].io.end()) {
								for (auto k : iFile.subcircuitSegments[subckt].io) {
									if (k == tokens[1]) tokens[1] = io[index_of(iFile.subcircuitSegments[subckt].io, k)];
								}
								if (tokens[2] != "0" && tokens[2] != "GND") tokens[2] = tokens[2] + "_" + label;
							}
							else if (std::find(iFile.subcircuitSegments[subckt].io.begin(), iFile.subcircuitSegments[subckt].io.end(), tokens[2]) != iFile.subcircuitSegments[subckt].io.end()) {
								for (auto k : iFile.subcircuitSegments[subckt].io) {
									if (k == tokens[2]) tokens[2] = io[index_of(iFile.subcircuitSegments[subckt].io, k)];
								}
								if (tokens[1] != "0" && tokens[1] != "GND") tokens[1] = tokens[1] + "_" + label;
							}
							else {
								if (tokens[1] != "0" && tokens[1] != "GND") tokens[1] = tokens[1] + "_" + label;
								if (tokens[2] != "0" && tokens[2] != "GND") tokens[2] = tokens[2] + "_" + label;
								if (j[0] == 'B') {
									tokens[3] = subckt + "_" + tokens[3];
								}
							}
							std::string line = tokens[0];
							for (int k = 1; k < tokens.size(); k++) {
								line = line + " " + tokens[k];
							}
							duplicateSegment.push_back(line);
						}
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
