// Copyright (c) 2018 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef J_INPUT_H
#define J_INPUT_H
#include "j_std_include.hpp"

/* Subcircuit object */
class Subcircuit {
public:
	std::vector<std::string> io;
	std::vector<std::string> lines;
};

extern std::unordered_map<std::string, double> parVal;

/* Input File Object Class */
class InputFile {
  std::vector<std::string> lines;
  public:
	std::vector<std::string> maincircuitSegment, controlPart, maincircuitModels;
	std::unordered_map<std::string, std::vector<std::string>> subcircuitModels;
	std::unordered_map<std::string, Subcircuit> subcircuitSegments;
	std::unordered_map<std::string, int> subCircuitComponentCount, subCircuitJJCount, subCircuitContainsSubCicuit;
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
  /*
	Substitute subcircuits into the main circuit to create a full main circuit
  */
  void sub_in_subcircuits(InputFile& iFile, std::vector<std::string>& segment, std::string label = "");
};
#endif
