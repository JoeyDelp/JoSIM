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
#include <algorithm>

namespace JoSIM {
class Input {
  private:
    std::vector<std::string> read_file(const std::string &fileName);
    std::vector<std::string> read_input();

  public:
  Netlist netlist;
  Transient transSim;
  std::vector<std::string> fileLines, controls;
  std::vector<std::string> relevantX;
  std::unordered_map<ParameterName, Parameter> parameters;

  Input(AnalysisType analysis_type = AnalysisType::Voltage,
        InputType input_type = InputType::Jsim,
        bool verbose = false) : 
    argAnal(analysis_type), 
    argConv(input_type), 
    argVerb(verbose) 
    {
      netlist.argConv = argConv;
    };

  AnalysisType argAnal;
  InputType argConv;
  bool argVerb = false;

  void parse_file(const std::string &fileName);
};
} // namespace JoSIM

#endif
