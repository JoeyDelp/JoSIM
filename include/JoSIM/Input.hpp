// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_J_INPUT_H
#define JOSIM_J_INPUT_H

#include "j_misc.h"
#include "j_std_include.h"


#include "./AnalysisType.hpp"
#include "./InputType.hpp"
#include "./ParameterName.hpp"
#include "./Errors.hpp"

class Parameters {
public:
  std::vector<std::pair<std::string, std::string>> unparsedParams;
  std::unordered_map<JoSIM::ParameterName, double> parsedParams;
};

class Transient {
public:
  double prstep;
  double tstop;
  double tstart;
  double maxtstep;
  Transient() :
    prstep(1E-12),
    tstop(0.0),
    tstart(0.0),
    maxtstep(1E-12)
  {

  };
  std::size_t simsize() { return (tstop - tstart) / prstep; };
};

class Subcircuit {
public:
  std::vector<std::string> io;
  std::vector<std::pair<std::string, std::string>> lines;
  std::vector<std::string> subckts;
  int jjCount, compCount, subcktCounter;
  bool containsSubckt;
  Subcircuit() :
    jjCount(0),
    compCount(0),
    subcktCounter(0),
    containsSubckt(false)
  {

  };
};

class Netlist {
public:
  std::unordered_map<std::pair<std::string, std::string>, std::string,
                     pair_hash>
      models;
  std::unordered_map<std::string, Subcircuit> subcircuits;
  std::unordered_map<std::string, int> subcktLookup;
  std::vector<std::string> maindesign;
  std::vector<std::string> subckts;
  int jjCount, compCount, subcktCounter, nestedSubcktCount;
  bool containsSubckt;
  Netlist() :
    jjCount(0),
    compCount(0),
    subcktCounter(0),
    nestedSubcktCount(0),
    containsSubckt(false)
  {
  
  };
};

class Input {
public:
  Netlist netlist;
  Parameters parameters;
  Transient transSim;
  std::vector<std::string> fileLines, controls;
  std::vector<std::pair<std::string, std::string>> expNetlist;
  std::vector<std::string> relevantX;

  Input(JoSIM::AnalysisType analysis_type,
        JoSIM::InputType input_type = JoSIM::InputType::Jsim,
        bool verbose = false) : 
    argAnal(analysis_type), 
    argConv(input_type), 
    argVerb(verbose) 
    {

    };

  JoSIM::AnalysisType argAnal;
  JoSIM::InputType argConv;
  bool argVerb = false;

  void read_input_file(std::string &fileName,
                       std::vector<std::string> &fileLines);
  void split_netlist(std::vector<std::string> &fileLines,
                     std::vector<std::string> &controls, Parameters &parameters,
                     Netlist &netlist);
  void expand_subcircuits();
  void expand_maindesign();
};

#endif
