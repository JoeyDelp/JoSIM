#pragma once
#include "std_include.hpp"

/* Input File Object Class */
class InputFile {
  std::vector<std::string> lines;
  public:
	std::vector<std::string> maincircuitSegment, controlPart, maincircuitModels;
	std::map<std::string, std::vector<std::string>> subcircuitSegments, subcircuitModels;
	std::map<std::string, int> subCircuitComponentCount, subCircuitJJCount, subCircuitContainsSubCicuit;
	int subCircuitCount, jjCount, componentCount, allCounted, circuitComponentCount, circuitJJCount;
	int simulationType = 4;
  /*
    Input File Constructor
  */
  InputFile(std::string iFileName);
  /*
    Split the circuit into subcircuits, main circuit and identify circuit stats
  */
  void circuit_to_segments(InputFile &iFile);
};
