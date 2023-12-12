// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_J_INPUT_H
#define JOSIM_J_INPUT_H

#include <algorithm>
#include <string>
#include <vector>

#include "JoSIM/AnalysisType.hpp"
#include "JoSIM/CliOptions.hpp"
#include "JoSIM/Errors.hpp"
#include "JoSIM/FileOutputType.hpp"
#include "JoSIM/LineInput.hpp"
#include "JoSIM/Misc.hpp"
#include "JoSIM/Netlist.hpp"
#include "JoSIM/ParameterName.hpp"
#include "JoSIM/Transient.hpp"
#include "JoSIM/TypeDefines.hpp"

namespace JoSIM {
class Input {
 public:
  Netlist netlist;
  Transient transSim;
  std::optional<double> globalTemp;
  double neB = 1E12;
  std::vector<tokens_t> fileLines, controls;
  std::vector<std::string> relevantX;
  std::unordered_map<ParameterName, Parameter> parameters;
  std::optional<OutputFile> cli_output_file;
  std::vector<OutputFile> output_files;
  std::optional<std::string> fileParentPath;
  AnalysisType argAnal;
  int64_t argVerb;
  bool argMin;

  Input(AnalysisType at = AnalysisType::Phase, int64_t verb = 0,
        bool min = false)
      : argAnal(at), argVerb(verb), argMin(min){};
  Input(CliOptions& cli_options) {
    argAnal = cli_options.analysis_type;
    argVerb = cli_options.verbose;
    argMin = cli_options.minimal;
    cli_output_file = cli_options.output_file;
  }

  std::vector<tokens_t> read_input(LineInput& input,
                                   string_o fileName = std::nullopt);
  void parse_input(string_o fileName = std::nullopt);
  void syntax_check_controls(std::vector<tokens_t>& controls);
};
}  // namespace JoSIM

#endif
