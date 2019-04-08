#include "JoSIM/CliOptions.hpp"

#include "JoSIM/j_errors.h"
#include "JoSIM/j_globals.h"

using namespace JoSIM;

CliOptions CliOptions::parse(int argc, const char **argv) {
  CliOptions out;

  if (argc <= 1) {
    version_info();
    Errors::error_handling(TOO_FEW_ARGUMENTS);
  }

  if (argv[argc - 1][0] == '-') {
    if (argv[argc - 1][1] == 'h') {
      version_info();
      display_help();
      exit(0);
    } else if (argv[argc - 1][1] == 'v') {
      version_info();
      exit(0);
    } else if (argv[argc - 1][1] == '-') {
      if (argv[argc - 1][2] == 'h') {
        version_info();
        display_help();
        exit(0);
      } else if (argv[argc - 1][2] == 'v') {
        version_info();
        exit(0);
      } else {
        version_info();
        Errors::error_handling(FINAL_ARG_SWITCH);
      }
    } 
    else {
      version_info();
      Errors::error_handling(FINAL_ARG_SWITCH);
    }
  } else if (argv[argc - 1][0] != '-' && argv[argc - 1][1] != 'h')
    out.cir_file_name = argv[argc - 1];
  else {
    version_info();
    Errors::error_handling(FINAL_ARG_SWITCH);
  }

  version_info();
  for (int i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      switch (argv[i][1]) {
      case 'a':
        if ((i + 1) == (argc - 1))
          Errors::error_handling(TOO_FEW_ARGUMENTS);
        else if (argv[i + 1][0] == '-')
          Errors::error_handling(TOO_FEW_ARGUMENTS);
        else {
          try {
            int argAnal = std::stoi(argv[i + 1]);
            out.analysis_type = analysis_type_from_int(argAnal);
          } catch (std::exception &e) {
            Errors::error_handling(INVALID_ANALYSIS);
          }
        }
        break;
      case 'c':
        if ((i + 1) == (argc - 1))
          Errors::error_handling(TOO_FEW_ARGUMENTS);
        else if (argv[i + 1][0] == '-')
          Errors::error_handling(TOO_FEW_ARGUMENTS);
        else {
          try {
            int argConv = std::stoi(argv[i + 1]);
            out.input_type = input_type_from_int(argConv);
          } catch (std::exception &e) {
            Errors::error_handling(INVALID_CONVENTION);
          }
        }
        break;
      case 'h':
        display_help();
        break;
      case 'o':
        out.output_to_file = true;
        if (((i + 1) == (argc - 1)) || (argv[i + 1][0] == '-')) {
          out.output_file_name = out.cir_file_name;
          out.output_file_name =
              out.output_file_name.substr(
                  0, out.output_file_name.find_last_of('.')) +
              ".csv";
          out.output_file_type = FileOutputType::Csv;
        } else {
          out.output_file_name = argv[i + 1];
          if (out.output_file_name.find('.') != std::string::npos) {
            std::string outExt = out.output_file_name.substr(
                out.output_file_name.find_last_of('.'),
                out.output_file_name.size() - 1);
            std::transform(outExt.begin(), outExt.end(), outExt.begin(),
                           toupper);
            if (outExt == ".CSV")
              out.output_file_type = FileOutputType::Csv;
            else if (outExt == ".DAT")
              out.output_file_type = FileOutputType::Dat;
            else
              out.output_file_type = FileOutputType::WrSpice;
          } else
            out.output_file_type = FileOutputType::WrSpice;
        }
        break;
      case 'p':
#ifdef _OPENMP
        std::cout << "Parallelization is ENABLED" << std::endl;
#else
        std::cout << "Parallelization is DISABLED" << std::endl;
#endif
        break;
      case 'V':
        out.verbose = true;
        break;
      case 'v':
        version_info();
        exit(0);
      }
    }
  }

  return out;
}

void CliOptions::display_help() {
  std::cout << "JoSIM help interface\n";
  std::cout << "====================\n";
  std::cout << std::setw(13) << std::left << "-a(nalysis)" << std::setw(3)
            << std::left << "|"
            << "Specifies the analysis type." << std::endl;
  std::cout << std::setw(13) << std::left << "  " << std::setw(3) << std::left
            << "|"
            << "0 for Voltage analysis (Default)." << std::endl;
  std::cout << std::setw(13) << std::left << "  " << std::setw(3) << std::left
            << "|"
            << "1 for Phase analysis." << std::endl;

  std::cout << std::setw(13) << std::left << "-c(onvention)" << std::setw(3)
            << std::left << "|"
            << "Sets the subcircuit convention to left(0) or right(1)."
            << std::endl;
  std::cout << std::setw(13) << std::left << "  " << std::setw(3) << std::left
            << "|"
            << "Default is left. WRSpice (normal SPICE) use right."
            << std::endl;
  std::cout << std::setw(13) << std::left << "  " << std::setw(3) << std::left
            << "|"
            << "Eg. X01 SUBCKT 1 2 3     vs.     X01 1 2 3 SUBCKT" << std::endl;

  std::cout << std::setw(13) << std::left << "-h(elp)" << std::setw(3)
            << std::left << "|"
            << "Displays this help menu" << std::endl;

  std::cout << std::setw(13) << std::left << "-o(utput)" << std::setw(3)
            << std::left << "|"
            << "Specify output file for simulation results (.csv)."
            << std::endl;
  std::cout << std::setw(13) << std::left << "  " << std::setw(3) << std::left
            << "|"
            << "Default will be output.csv if no file is specified."
            << std::endl;

  std::cout << std::setw(13) << std::left << "-p(arallel)" << std::setw(3)
            << std::left << "|"
            << "(EXPERIMENTAL) Enables parallelization of certain functions."
            << std::endl;
  std::cout << std::setw(13) << std::left << "  " << std::setw(3) << std::left
            << "|"
            << "Requires compilation with OPENMP switch enabled." << std::endl;
  std::cout
      << std::setw(13) << std::left << "  " << std::setw(3) << std::left << "|"
      << "Threshold applies, overhead on small circuits negates performance."
      << std::endl;

  std::cout << std::setw(13) << std::left << "-V(erbose)" << std::setw(3)
            << std::left << "|"
            << "Runs JoSIM in verbose mode." << std::endl;

  std::cout << std::setw(13) << std::left << "-v(ersion)" << std::setw(3)
            << std::left << "|"
            << "Displays the JoSIM version info only." << std::endl;

  std::cout << std::endl;
  std::cout << "Example command: josim -o ./output.csv test.cir"
            << std::endl;
  std::cout << std::endl;
  exit(0);
}

void CliOptions::version_info() {
  std::cout << std::endl;
  std::cout
      << "JoSIM: Josephson Junction Superconductive SPICE Circuit Simulator"
      << std::endl;
  std::cout << "Copyright (C) 2019 by Johannes Delport (jdelport@sun.ac.za)"
            << std::endl;
  std::cout << "v" << VERSION << " compiled on " << __DATE__ << " at "
            << __TIME__ << std::endl;
  std::cout << std::endl;
}
