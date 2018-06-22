// Copyright (c) 2018 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef J_INPUT_H
#define J_INPUT_H
#include "j_std_include.hpp"

#define UTYPE -1
#define RCSJ 0
#define MTJ 1
#define NTRON 2
#define CSHE 3

/* RCSJ Model */
class model_rcsj
{
public:
  std::string rtype, cct, vg, delv, icon, rzero, rnormal, cap, icrit;
  // Default RCSJ model constructor
  model_rcsj()
  {
    rtype = "0.0";
    cct = "0.0";
    vg = "2.8E-3";
    delv = "0.1E-3";
    icon = "1E-3";
    rzero = "30";
    rnormal = "5";
    cap = "2.5E-12";
    icrit = "1E-3";
  }
};
/* MTJ Model */
class model_mtj
{
public:
  std::string modelname;
};
/* NTRON Model */
class model_ntron
{
public:
  std::string modelname;
};
/* NTRON Model */
class model_cshe
{
public:
  std::string modelname;
};

/* Model object */
class Model
{
public:
  std::string modelname;
  int modelType = UTYPE;
  model_rcsj jj;
  model_mtj mtj;
  model_ntron ntron;
  model_cshe cshe;
};
/* Subcircuit object */
class Subcircuit
{
public:
  std::string name;
  std::vector<std::string> io;
  std::vector<std::string> lines;
  std::vector<std::string> subckts;
  int componentCount = 0,
      jjCount = 0;
  std::unordered_map<std::string, double> parVal;
  std::unordered_map<std::string, Model> subcktModels;
  bool containsSubckt = false;
};
/*
  Model parsing. Split a model line into parameters
*/
void parse_model(std::string s, std::unordered_map<std::string, Model>& m);
/*
  Recursive function, for subcircuit traversal
*/
std::vector<std::string> recurseSubckt(std::unordered_map<std::string, Subcircuit> subckts, std::string part);

/* Input File Object Class */
class InputFile
{
  std::vector<std::string> lines;

public:
  std::vector<std::string> maincircuitSegment, controlPart, maincircuitModels, subckts;
  std::unordered_map<std::string, std::vector<std::string>> subcircuitModels;
  std::unordered_map<std::string, Subcircuit> subcircuitSegments;
  std::unordered_map<std::string, Model> mainModels;
  std::unordered_map<std::string, int> subCircuitComponentCount,
    subCircuitJJCount, subCircuitContainsSubCircuit;
  std::unordered_map<std::string, double> parVal;
  std::unordered_map<std::string, std::string> subcircuitNameMap;
  int subCircuitCount = 0,
    jjCount = 0,
    componentCount = 0,
    allCounted = 0,
    circuitComponentCount = 0, 
    circuitJJCount = 0, 
    mainJJs = 0, 
    mainComponents = 0;
  int simulationType = 4;
  /*
    Input File Constructor
  */
  InputFile(std::string iFileName);
  /*
    Split the circuit into subcircuits, main circuit and identify circuit stats
  */
  void circuit_to_segments(InputFile& iFile);
  /*
        Substitute subcircuits into the main circuit to create a full main
     circuit
  */
  void sub_in_subcircuits(InputFile& iFile,
                          std::vector<std::string>& segment,
                          std::string label = "");
};
#endif
