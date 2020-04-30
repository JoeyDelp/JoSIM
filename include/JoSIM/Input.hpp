// Copyright (c) 2020 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_J_INPUT_H
#define JOSIM_J_INPUT_H

#include "JoSIM/AnalysisType.hpp"
#include "JoSIM/InputType.hpp"
#include "JoSIM/ParameterName.hpp"
#include "JoSIM/Errors.hpp"
#include "JoSIM/Netlist.hpp"
#include "JoSIM/Misc.hpp"
#include "JoSIM/Transient.hpp"
#include "JoSIM/IntegrationType.hpp"

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
        IntegrationType int_type = IntegrationType::Trapezoidal,
        bool verbose = false,
        bool minimal = false) : 
    argAnal(analysis_type), 
    argConv(input_type), 
    argInt(int_type),
    argVerb(verbose),
    argMin(minimal) 
    {
      netlist.argConv = argConv;
    };

  AnalysisType argAnal;
  InputType argConv;
  IntegrationType argInt;
  bool argVerb = false;
  bool argMin = false;

  void parse_file(const std::string &fileName);
};
} // namespace JoSIM

#endif
