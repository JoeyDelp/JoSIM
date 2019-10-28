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
  Parameters parameters;
  Transient transSim;
  std::vector<std::string> fileLines, controls;
  std::vector<std::string> relevantX;

  Input(JoSIM::AnalysisType analysis_type,
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
};

std::vector<std::string> read_file(const std::string &fileName);
void parse_file(std::string &fileName, Input &iObj);

#endif
