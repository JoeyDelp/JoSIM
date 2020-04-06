// Copyright (c) 2020 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_CLIOPTIONS_HPP
#define JOSIM_CLIOPTIONS_HPP

#include "JoSIM/AnalysisType.hpp"
#include "JoSIM/IntegrationType.hpp"
#include "JoSIM/FileOutputType.hpp"
#include "JoSIM/InputType.hpp"

#include <optional>
#include <string>
#include <algorithm>

namespace JoSIM {

struct CliOptions {
  std::string cir_file_name;
  InputType input_type = InputType::Jsim;
  AnalysisType analysis_type = AnalysisType::Voltage;
  IntegrationType integration_type = IntegrationType::Trapezoidal;
  bool verbose = false;
  bool parallel = false;
  bool standardin = false;

  std::optional<std::string> output_file_name;
  FileOutputType output_file_type;

  // helper functions
  static CliOptions parse(int argc, 
                          const char **argv);
  static void display_help();
  static void version_info();
};

} // namespace JoSIM

#endif // JOSIM_CLIOPTIONS_HPP
