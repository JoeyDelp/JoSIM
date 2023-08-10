// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/AnalysisType.hpp"
#include "JoSIM/CliOptions.hpp"
#include "JoSIM/Errors.hpp"
#include "JoSIM/IV.hpp"
#include "JoSIM/Input.hpp"
#include "JoSIM/Matrix.hpp"
#include "JoSIM/Model.hpp"
#include "JoSIM/Noise.hpp"
#include "JoSIM/Output.hpp"
#include "JoSIM/Parameters.hpp"
#include "JoSIM/Simulation.hpp"
#include "JoSIM/Transient.hpp"
#include "JoSIM/Verbose.hpp"

using namespace JoSIM;

int main(int argc, const char** argv) {
  try {
    // Before anything. Display versioning info.
    CliOptions::version_info();
    // Parse input arguments for command line interface
    auto cli_options = CliOptions::parse(argc, argv);
    // Generate input object based on cli arguements
    Input iObj(cli_options);
    // Parse input file as specified by the cli arguments
    iObj.parse_input(cli_options.cir_file_name);
    // Parse any identified parameter values
    if (iObj.parameters.size() > 0) {
      parse_parameters(iObj.parameters);
    }
    // Parse any identified models
    for (const auto& i : iObj.netlist.models) {
      Model::parse_model(std::make_pair(i.second, i.first.second),
                         iObj.netlist.models_new, iObj.parameters);
    }
    // Expand nested subcircuits
    iObj.netlist.expand_subcircuits();
    // Expand main design using expanded subcircuits
    iObj.netlist.expand_maindesign();
    // Simulate IV curves if need be
    IV ivObj(iObj);
    // Identify the simulation parameters
    Transient::identify_simulation(iObj.controls, iObj.transSim,
                                   iObj.parameters);
    // Create matrix object
    Matrix mObj;
    // Create the matrix in csr format
    mObj.create_matrix(iObj);
    // Do verbosity
    Verbose::handle_verbosity(iObj.argVerb, iObj, mObj);
    //  Find the relevant traces to store
    find_relevant_traces(iObj, mObj);
    // Create a simulation object
    Simulation sObj(iObj, mObj);
    // Create an output object
    Output oObj(iObj, mObj, sObj);
    // Finish
    return 0;
  } catch (std::runtime_error& formattedMessage) {
    // Catch any thrown error messages
    Errors::error_message(formattedMessage.what());
  } catch (std::out_of_range& e) {
    // Catch any Out of Range exceptions and print something user readable
    Errors::oor();
  }
}
