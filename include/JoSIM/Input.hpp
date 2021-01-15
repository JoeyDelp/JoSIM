// Copyright (c) 2021 Johannes Delport
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
#include "JoSIM/LineInput.hpp"

#include <string>
#include <vector>
#include <algorithm>

namespace JoSIM {
class Input {
  public:
  Netlist netlist;
  Transient transSim;
  std::optional<double> globalTemp, neB;
  std::vector<tokens_t> fileLines, controls;
  std::vector<std::string> relevantX;
  std::unordered_map<ParameterName, Parameter> parameters;

  Input(AnalysisType analysis_type = AnalysisType::Voltage,
        int verbose = 0,
        bool minimal = false) : 
    argAnal(analysis_type), 
    argVerb(verbose),
    argMin(minimal) 
    {};

  AnalysisType argAnal;
  int argVerb = 0;
  bool argMin = false;

  std::vector<tokens_t> read_input(
    LineInput &input, string_o fileName = std::nullopt);
  void parse_input(string_o fileName = std::nullopt);
  void syntax_check_controls(std::vector<tokens_t> &controls);

};
} // namespace JoSIM

#endif
