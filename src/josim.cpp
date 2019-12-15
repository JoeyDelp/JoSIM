// Copyright (c) 2019 Johannes Delport
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


int main(int argc, const char **argv) {

  try {
    // Before anything. Display versioning info.
    JoSIM::CliOptions::version_info();
    // Parse input arguments for command line interface
    auto cli_options = JoSIM::CliOptions::parse(argc, argv);
    // Generate input object based on cli arguements
    Input iObj(cli_options.analysis_type, 
              cli_options.input_type,
              cli_options.verbose);    
    // Parse input file as specified by the cli arguments
    Input::parse_file(cli_options.cir_file_name, iObj);
    // Parse any identified parameter values
    if (iObj.parameters.size() > 0) {
      Parameters::parse_parameters(iObj.parameters);
    }
    // Parse any identified models
    for (const auto &i : iObj.netlist.models) {
      Model::parse_model(std::make_pair(i.second, i.first.second), iObj.netlist.models_new, iObj.parameters);
    }
    // Expand nested subcircuits
    iObj.netlist.expand_subcircuits();
    // Expand main design using expanded subcircuits
    iObj.netlist.expand_maindesign();
    // Identify the simulation parameters
    Transient::identify_simulation(iObj.controls, iObj.transSim);
    // If verbose mode was requested, print the expanded netlist
    if (iObj.argVerb)
      Verbose::print_expanded_netlist(iObj.netlist.expNetlist);

    Matrix mObj;
    // Create the matrix in csr format
    mObj.create_matrix(iObj);

    // Find the relevant traces to store
    RelevantTrace::find_relevant_traces(iObj.controls, mObj);
    
    Simulation sObj;

    sObj.trans_sim_new(iObj, mObj);
    
    Output oObj;

    std::vector<std::vector<std::string>> output = Output::write_output(iObj, mObj, sObj);

    if (cli_options.output_file_name) {
      if (cli_options.output_file_type == JoSIM::FileOutputType::Csv) {
        Output::format_csv_or_dat(cli_options.output_file_name.value(), output, ',');
      } else if (cli_options.output_file_type == JoSIM::FileOutputType::Dat) {
        Output::format_csv_or_dat(cli_options.output_file_name.value(), output, ' ');
      } else if (cli_options.output_file_type == JoSIM::FileOutputType::WrSpice) {
        Output::format_raw(cli_options.output_file_name.value(), output);
      }
    } else {
      Output::format_cout(output);
    }

    return 0;
  } catch(std::string &formattedMessage) {
      Errors::error_message(formattedMessage);
  }
}
