// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_J_INPUT_H
#define JOSIM_J_INPUT_H

#include "./AnalysisType.hpp"
#include "./InputType.hpp"
#include "./ParameterName.hpp"
#include "./Errors.hpp"
#include "./Netlist.hpp"
#include "./Misc.hpp"
#include "./Transient.hpp"

#include <string>
#include <vector>

class Input {
public:
  Netlist netlist;
  Transient transSim;
  std::vector<std::string> fileLines, controls;
  std::vector<std::string> relevantX;
  std::unordered_map<JoSIM::ParameterName, Parameter> parameters;

  Input(JoSIM::AnalysisType analysis_type = JoSIM::AnalysisType::Voltage,
        JoSIM::InputType input_type = JoSIM::InputType::Jsim,
        bool verbose = false) : 
    argAnal(analysis_type), 
    argConv(input_type), 
    argVerb(verbose) 
    {
      netlist.argConv = argConv;
    };

  JoSIM::AnalysisType argAnal;
  JoSIM::InputType argConv;
  bool argVerb = false;

  static std::vector<std::string> read_file(const std::string &fileName);
  static void parse_file(const std::string &fileName, Input &iObj);
};

#endif
