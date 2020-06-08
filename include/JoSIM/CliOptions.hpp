// Copyright (c) 2020 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_CLIOPTIONS_HPP
#define JOSIM_CLIOPTIONS_HPP

#include "JoSIM/AnalysisType.hpp"
#include "JoSIM/IntegrationType.hpp"
#include "JoSIM/FileOutputType.hpp"
#include "JoSIM/InputType.hpp"
#include "JoSIM/TypeDefines.hpp"

#include <optional>
#include <string>
#include <algorithm>

namespace JoSIM {

struct CliOptions {
  private:

  tokens_t argv_to_tokens(const int &argc, const char **argv);
  vector_pair_t<char_o, string_o> argument_pairs(const tokens_t &tokens);

  public:

  string_o cir_file_name;
  string_o output_file_name;

  InputType input_type = InputType::Jsim;
  AnalysisType analysis_type = AnalysisType::Voltage;
  IntegrationType integration_type = IntegrationType::Trapezoidal;
  FileOutputType output_file_type = FileOutputType::Csv;

  bool verbose = false;
  bool minimal = false;
  bool parallel = false;

  // helper functions
  static CliOptions parse(int argc, const char **argv);
  static void display_help();
  static void version_info();
};

} // namespace JoSIM

#endif // JOSIM_CLIOPTIONS_HPP
