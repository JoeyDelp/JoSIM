// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_CLIOPTIONS_HPP
#define JOSIM_CLIOPTIONS_HPP

#include <algorithm>
#include <optional>
#include <string>

#include "JoSIM/AnalysisType.hpp"
#include "JoSIM/FileOutputType.hpp"
#include "JoSIM/TypeDefines.hpp"

namespace JoSIM {

struct CliOptions {
 private:
  tokens_t argv_to_tokens(const int64_t& argc, const char** argv);
  vector_pair_t<char_o, string_o> argument_pairs(const tokens_t& tokens);

 public:
  string_o cir_file_name;
  std::optional<OutputFile> output_file;

  AnalysisType analysis_type = AnalysisType::Phase;

  int64_t verbose = 0;
  bool minimal = false;
  bool parallel = false;

  // helper functions
  static CliOptions parse(int64_t argc, const char** argv);
  static void display_help();
  static void version_info();
};

}  // namespace JoSIM

#endif  // JOSIM_CLIOPTIONS_HPP
