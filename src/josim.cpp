// Copyright (c) 2020 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/AnalysisType.hpp"
#include "JoSIM/Matrix.hpp"
#include "JoSIM/Output.hpp"
#include "JoSIM/Parameters.hpp"
#include "JoSIM/Simulation.hpp"
#include "JoSIM/Verbose.hpp"
#include "JoSIM/CliOptions.hpp"
#include "JoSIM/Input.hpp"
#include "JoSIM/Errors.hpp"
#include "JoSIM/Transient.hpp"
#include "JoSIM/Model.hpp"

using namespace JoSIM;

int main(int argc, 
          const char **argv) {
  try {
    // Before anything. Display versioning info.
    CliOptions::version_info();
    // Parse input arguments for command line interface
    auto cli_options = CliOptions::parse(argc, argv);
    // Generate input object based on cli arguements
    Input iObj(cli_options.analysis_type,
              cli_options.verbose,
              cli_options.minimal);    
    // Parse input file as specified by the cli arguments
    iObj.parse_input(cli_options.cir_file_name);
    // Parse any identified parameter values
    if (iObj.parameters.size() > 0) {
      parse_parameters(iObj.parameters);
    }
    // Parse any identified models
    for (const auto &i : iObj.netlist.models) {
      Model::parse_model(
        std::make_pair(i.second, i.first.second), iObj.netlist.models_new, 
        iObj.parameters);
    }
    // Expand nested subcircuits
    iObj.netlist.expand_subcircuits();
    // Expand main design using expanded subcircuits
    iObj.netlist.expand_maindesign();
    // Identify the simulation parameters
    Transient::identify_simulation(iObj.controls, iObj.transSim);
    // Create matrix object
    Matrix mObj;
    // Create the matrix in csr format
    mObj.create_matrix(iObj);
    // Do verbosity
    Verbose::handle_verbosity(iObj.argVerb, iObj, mObj);
    // Dump expanded Netlist since it is no longer needed
    iObj.netlist.expNetlist.clear();
    iObj.netlist.expNetlist.shrink_to_fit();
    // Find the relevant traces to store
    find_relevant_traces(iObj.controls, mObj);
    // Create a simulation object
    Simulation sObj(iObj, mObj);
    // Create an output object
    Output oObj;
    // Write the output in type agnostic format
    oObj.write_output(iObj, mObj, sObj);
    // Format the output into the relevant type
    if (cli_options.output_file_name) {
      if (cli_options.output_file_type == FileOutputType::Csv) {
        oObj.Output::format_csv_or_dat(cli_options.output_file_name.value(), ',');
      } else if (cli_options.output_file_type == FileOutputType::Dat) {
        oObj.Output::format_csv_or_dat(cli_options.output_file_name.value(), ' ');
      } else if (cli_options.output_file_type == FileOutputType::Raw) {
        oObj.Output::format_raw(cli_options.output_file_name.value());
      }
    } else {
      oObj.Output::format_cout(iObj.argMin);
    }
    // Finish
    return 0;
  } catch(std::runtime_error &formattedMessage) {
    // Catch any thrown error messages
    Errors::error_message(formattedMessage.what());
  } catch(std::out_of_range &e) {
    // Catch any Out of Range exceptions and print something user readable
    Errors::oor();
  }
}
