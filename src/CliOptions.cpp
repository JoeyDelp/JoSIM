// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/CliOptions.hpp"

#include <cstring>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>

#include "JoSIM/Errors.hpp"
#include "JoSIM/Misc.hpp"
#include "JoSIM/TypeDefines.hpp"

using namespace JoSIM;

tokens_t CliOptions::argv_to_tokens(const int64_t& argc, const char** argv) {
  // Tokens variable of size argc to store arguments
  tokens_t tokens(argc - 1);
  // If argument count is less or equal to one
  if (argc <= 1) {
    // Complain that josim was run without arguments
    Errors::cli_errors(CLIErrors::TOO_FEW_ARGUMENTS);
  }
  // Loop through arguments, starting at 1 (0 is josim-cli)
  for (int64_t i = 1; i < argc; ++i) {
    // Add the arguments at the appropriate position
    tokens.at(i - 1) = std::string(argv[i]);
  }
  // Return the tokens
  return tokens;
}

vector_pair_t<char_o, string_o> CliOptions::argument_pairs(
    const tokens_t& tokens) {
  // Variable to store the argument pairs
  vector_pair_t<char_o, string_o> ap;
  // Loop through the argument tokens
  for (auto t : tokens) {
    // Ensure that the argument is not e.g. "--analysis=1"
    tokens_t tempT = Misc::tokenize(t, "=", true, true);
    if (tempT.size() > 2) {
      // If there is more than one "=" in the argument this is invalid
      Errors::cli_errors(CLIErrors::UNKNOWN_SWITCH, t);
    }
    // If the string is a switch, remove the --
    if (tempT.at(0).at(0) == '-') {
      // Remove all '-' chars, indicating a switch.
      tempT.at(0).erase(
          std::remove(tempT.at(0).begin(), tempT.at(0).end(), '-'),
          tempT.at(0).end());
      // Store the switch identifying char 'a' for "analysis"
      ap.emplace_back(std::make_pair(tempT.at(0).at(0), std::nullopt));
    } else {
      // Argument to the switch in first token
      if (tempT.size() > 1) {
        ap.back().second = tempT.at(1);
      }
      // If the argument pairs aren't empty
      if (ap.size() != 0) {
        // If the previous switch was alone then this must be the argument
        if (!ap.back().second) {
          ap.back().second = tempT.at(0);
          // If the last pair has a value then this must be file name
        } else {
          ap.emplace_back(std::make_pair(std::nullopt, tempT.at(0)));
        }
        // Else if they are empty then this is the input file name
      } else {
        ap.emplace_back(std::make_pair(std::nullopt, tempT.at(0)));
      }
    }
  }
  // Sanity check
  if (ap.size() == 1) {
    // If there is only one pair with both switch and value populated
    if (ap.at(0).first && ap.at(0).second) {
      // We are most likely misinterpreting the input file as the value
      ap.emplace_back(std::make_pair(std::nullopt, ap.at(0).second));
      // Split the pair into 2, one the switch and one the input
      ap.at(0).second = std::nullopt;
    }
  }
  return ap;
}

CliOptions CliOptions::parse(int64_t argc, const char** argv) {
  // Variable where all the CLI options will be stored
  CliOptions out;
  // Parse and generate argument pairs from tokens
  vector_pair_t<char_o, string_o> ap =
      out.argument_pairs(out.argv_to_tokens(argc, argv));
  // Loop through the argument pairs
  for (auto i : ap) {
    // If the argument is not accompanied by any switch
    if (!i.first) {
      // This argument is the file name
      out.cir_file_name = i.second;
      auto path = std::filesystem::path(i.second.value());
      // Else if the switch is not empty
    } else {
      // Use the char in a case statment
      switch (i.first.value()) {
          // Set analysis type
        case 'a':
          if (!i.second) {
            Errors::cli_errors(CLIErrors::NO_ANALYSIS);
          } else if (i.second.value() == "0") {
            out.analysis_type = AnalysisType::Voltage;
          } else if (i.second.value() == "1") {
            out.analysis_type = AnalysisType::Phase;
          } else {
            Errors::cli_errors(CLIErrors::INVALID_ANALYSIS);
          }
          break;
          // Show help menu
        case 'h':
          display_help();
          exit(0);
          // Set input to standard input
        case 'i':
          out.cir_file_name = std::nullopt;
          break;
          // Enable minimal reporting
        case 'm':
          try {
            out.minimal = std::stoi(i.second.value_or("1"));
          } catch (const std::invalid_argument& ia) {
            Errors::cli_errors(CLIErrors::INVALID_MINIMAL, i.second.value());
          }
          break;
          // Set output file
        case 'o':
          // If no file name is specified but output is specified
          if (!i.second) {
            // Store the output in a file called output.csv at cwd
            out.output_file = OutputFile(
                std::filesystem::current_path().append("output.csv").string());
            // If a file name was given
          } else {
            out.output_file = OutputFile(i.second.value());
          }
          break;
          // Enable parallel processing (EXPERIMENTAL)
        case 'p':
#ifdef _OPENMP
          std::cout << "Parallelization is ENABLED" << std::endl;
#else
          std::cout << "Parallelization is DISABLED" << std::endl;
#endif
          break;
          // Enable verbose mode
        case 'V':
          try {
            out.verbose = std::stoi(i.second.value_or("1"));
          } catch (const std::invalid_argument& ia) {
            Errors::verbosity_errors(VerbosityErrors::INVALID_VERBOSITY_LEVEL,
                                     i.second.value());
          }
          break;
          // Show version info
        case 'v':
          exit(0);
          break;
          // Unknown switch was specified
        default:
          Errors::cli_errors(CLIErrors::UNKNOWN_SWITCH,
                             std::string(1, i.first.value()));
      }
    }
  }
  // Do a few sanity checks
  // If the input was not specified
  if (!out.cir_file_name) {
    // Complain and assume standard input
    Errors::cli_errors(CLIErrors::NO_INPUT);
  }
  // If an output file name was specified
  if (out.output_file) {
    // But this matches the input file name
    if (out.output_file.value().name() == out.cir_file_name.value()) {
      // Complain and exit since continuing will overwrite input file
      Errors::cli_errors(CLIErrors::INPUT_SAME_OUTPUT);
    }
  }
  // If minimal flag is not specified
  if (!out.minimal) {
    // Print to screen the input and output locations
    if (out.cir_file_name) {
      std::cout << "Input: " << out.cir_file_name.value() << std::endl;
    } else {
      std::cout << "Input: "
                << "standard input" << std::endl;
    }
    std::cout << std::endl;
    if (out.output_file) {
      std::cout << "Output: " << out.output_file.value().name() << std::endl;
      std::cout << std::endl;
    }
  }

  return out;
}

void CliOptions::display_help() {
  // Format the help interface
  // Preamble
  std::cout << "JoSIM help interface\n";
  std::cout << "====================\n";
  // Analysis type
  // ---------------------------------------------------------------------------
  std::cout << std::setw(16) << std::left << "-a" << std::setw(3) << std::left
            << "|"
            << "Specifies the analysis type." << std::endl;
  std::cout << std::setw(16) << std::left << "--analysis=" << std::setw(3)
            << std::left << "|"
            << "0 for Voltage analysis." << std::endl;
  std::cout << std::setw(16) << std::left << "  " << std::setw(3) << std::left
            << "|"
            << "1 for Phase analysis (Default)." << std::endl;
  std::cout << std::setw(16) << std::left << "  " << std::setw(3) << std::left
            << "|" << std::endl;
  // Help menu
  // ---------------------------------------------------------------------------
  std::cout << std::setw(16) << std::left << "-h" << std::setw(3) << std::left
            << "|"
            << "Displays this help menu" << std::endl;
  std::cout << std::setw(16) << std::left << "--help" << std::setw(3)
            << std::left << "|"
            << " " << std::endl;
  std::cout << std::setw(16) << std::left << "  " << std::setw(3) << std::left
            << "|" << std::endl;
  // Input from standard in
  // ---------------------------------------------------------------------------
  std::cout << std::setw(16) << std::left << "-i" << std::setw(3) << std::left
            << "|"
            << "Input circuit netlist from standard input." << std::endl;
  std::cout << std::setw(16) << std::left << "--input" << std::setw(3)
            << std::left << "|"
            << "This command ignores any additional input file specified."
            << std::endl;
  std::cout << std::setw(16) << std::left << "  " << std::setw(3) << std::left
            << "|"
            << "Reads from standard input until the "
            << ".end command or EOF character is specified." << std::endl;
  std::cout << std::setw(16) << std::left << "  " << std::setw(3) << std::left
            << "|" << std::endl;
  // Minimal output
  // ---------------------------------------------------------------------------
  std::cout << std::setw(16) << std::left << "-m" << std::setw(3) << std::left
            << "|"
            << "Disables most output." << std::endl;
  std::cout << std::setw(16) << std::left << "--minimal" << std::setw(3)
            << std::left << "|"
            << " " << std::endl;
  std::cout << std::setw(16) << std::left << "  " << std::setw(3) << std::left
            << "|" << std::endl;
  // Output
  // ---------------------------------------------------------------------------
  std::cout << std::setw(16) << std::left << "-o" << std::setw(3) << std::left
            << "|"
            << "Specify output file for simulation results (.csv)."
            << std::endl;
  std::cout << std::setw(16) << std::left << "--output=" << std::setw(3)
            << std::left << "|"
            << "Default will be output.csv if no file is specified."
            << std::endl;
  std::cout << std::setw(16) << std::left << "  " << std::setw(3) << std::left
            << "|" << std::endl;
  // Parallel Processing
  // ---------------------------------------------------------------------------
  std::cout << std::setw(16) << std::left << "-p" << std::setw(3) << std::left
            << "|"
            << "(EXPERIMENTAL) Enables parallelization of certain functions."
            << std::endl;
  std::cout << std::setw(16) << std::left << "--parallel" << std::setw(3)
            << std::left << "|"
            << "Requires compilation with OPENMP switch enabled." << std::endl;
  std::cout
      << std::setw(16) << std::left << "  " << std::setw(3) << std::left << "|"
      << "Threshold applies, overhead on small circuits negates performance."
      << std::endl;
  std::cout << std::setw(16) << std::left << "  " << std::setw(3) << std::left
            << "|" << std::endl;
  // Verbose switch
  // ---------------------------------------------------------------------------
  std::cout << std::setw(16) << std::left << "-V" << std::setw(3) << std::left
            << "|"
            << "Runs JoSIM in verbose mode." << std::endl;
  std::cout << std::setw(16) << std::left << "--Verbose=" << std::setw(3)
            << std::left << "|"
            << "Defaults to minimal(1), can be medium(2) or maximum(3)"
            << std::endl;
  std::cout << std::setw(16) << std::left << "  " << std::setw(3) << std::left
            << "|" << std::endl;
  // Version info
  // ---------------------------------------------------------------------------
  std::cout << std::setw(16) << std::left << "-v" << std::setw(3) << std::left
            << "|"
            << "Displays the JoSIM version info only." << std::endl;
  std::cout << std::setw(16) << std::left << "--version" << std::setw(3)
            << std::left << "|"
            << " " << std::endl;
  std::cout << std::setw(16) << std::left << "  " << std::setw(3) << std::left
            << "|" << std::endl;

  // ---------------------------------------------------------------------------
  std::cout << std::endl;
  std::cout << "Example command: josim -o ./output.csv test.cir" << std::endl;
  std::cout << std::endl;
  exit(0);
}

void CliOptions::version_info() {
  std::cout << std::endl;
  std::cout
      << "JoSIM: Josephson Junction Superconductive SPICE Circuit Simulator"
      << std::endl;
  std::cout << "Copyright (C) 2020 by Johannes Delport (jdelport@sun.ac.za)"
            << std::endl;
  std::cout << "v" << VERSION << "." << GIT_COMMIT_HASH << " compiled on "
            << __DATE__ << " at " << __TIME__ << std::endl;
#ifndef NDEBUG
  std::cout << "(Debug)" << std::endl;
#endif
  std::cout << std::endl;
}
