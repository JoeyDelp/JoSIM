#ifndef JOSIM_CLIOPTIONS_HPP
#define JOSIM_CLIOPTIONS_HPP

#include "./AnalysisType.hpp"
#include "./FileOutputType.hpp"
#include "./InputType.hpp"

#include <string>

namespace JoSIM {

struct CliOptions {
  std::string cir_file_name;
  InputType input_type = InputType::Jsim;
  AnalysisType analysis_type = AnalysisType::Voltage;
  bool verbose = false;
  bool parallel = false;

  // Replace with optional in C++17
  bool output_to_file = false;
  std::string output_file_name;
  FileOutputType output_file_type;

  // Deprecated
  bool plot = false;

  // helper functions
  static CliOptions parse(int argc, const char ** argv);
  static void display_help();
  static void version_info();
};

} // namespace JoSIM

#endif // JOSIM_CLIOPTIONS_HPP
