// Copyright (c) 2020 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_J_INPUT_H
#define JOSIM_J_INPUT_H

#include "JoSIM/TypeDefines.hpp"
#include "JoSIM/AnalysisType.hpp"
#include "JoSIM/InputType.hpp"
#include "JoSIM/ParameterName.hpp"
#include "JoSIM/Errors.hpp"
#include "JoSIM/Netlist.hpp"
#include "JoSIM/Misc.hpp"
#include "JoSIM/Transient.hpp"
#include "JoSIM/IntegrationType.hpp"
#include "JoSIM/LineInput.hpp"

#include <string>
#include <vector>
#include <algorithm>

namespace JoSIM {
class Input {
  public:
  Netlist netlist;
  Transient transSim;
  std::vector<tokens_t> fileLines, controls;
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

  std::vector<tokens_t> read_input(
    LineInput &input, string_o fileName = std::nullopt);
  void parse_input(string_o fileName = std::nullopt);

};
} // namespace JoSIM

#endif
