#include "include/j_input.hpp"

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
      if(i.find(".SUBCKT") != std::string::npos) {
        subcktName = i.substr(i.find(".SUBCKT ") + 8);
        subcktName = subcktName.substr(0, subcktName.find(' '));
        startCkt = true;
        subCircuitCount++;
      }
      if(startCkt) if(!i.empty()) subcircuitSegments[subcktName].push_back(i);
	  if(startCkt) if(starts_with(i, '.')) if (i.find("END") == std::string::npos) if (i.find("MODEL") == std::string::npos) subcircuitModels[subcktName].push_back(i);
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
    for (auto j : i.second) {
      count_component(j, iFile, i.first);
    }
  }
  int mapValueCount = map_value_count(subCircuitContainsSubCicuit, 1);
  while (mapValueCount != 0) {
    for (auto i : subcircuitSegments) {
      allCounted = 1;
      count_subcircuit_component(i.second, iFile, i.first);
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
