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

    Matrix mObj;
    Simulation sObj;
    Output oObj;

    parse_file(cli_options.cir_file_name, iObj);
    // Parse any identified parameter values
    if (iObj.parameters.parameters.size() > 0) {
      iObj.parameters.parse_parameters();
    }
    
    // Expand nested subcircuits
    iObj.netlist.expand_subcircuits();
    // Expand main design using expanded subcircuits
    iObj.netlist.expand_maindesign();

    identify_simulation(iObj.controls, iObj.transSim);

    if (iObj.argVerb)
      Verbose::print_expanded_netlist(iObj.netlist.expNetlist);
    // sObj.identify_simulation(iObj.controls, iObj.transSim.prstep,
    //                         iObj.transSim.tstop, iObj.transSim.tstart,
    //                         iObj.transSim.maxtstep);
    mObj.find_relevant_x(iObj);
    mObj.create_matrix(iObj);
    if (cli_options.analysis_type == JoSIM::AnalysisType::Voltage)
      sObj.trans_sim<JoSIM::AnalysisType::Voltage>(iObj, mObj);
      //sObj.transient_voltage_simulation(iObj, mObj);
    else if (cli_options.analysis_type == JoSIM::AnalysisType::Phase)
      sObj.trans_sim<JoSIM::AnalysisType::Phase>(iObj, mObj);
      // sObj.transient_phase_simulation(iObj, mObj);
    oObj.relevant_traces(iObj, mObj, sObj);

    if (cli_options.output_to_file) {
      if (cli_options.output_file_type == JoSIM::FileOutputType::Csv)
        oObj.write_data(cli_options.output_file_name, mObj, sObj);
      else if (cli_options.output_file_type == JoSIM::FileOutputType::Dat)
        oObj.write_legacy_data(cli_options.output_file_name, mObj, sObj);
      else if (cli_options.output_file_type == JoSIM::FileOutputType::WrSpice)
        oObj.write_wr_data(cli_options.output_file_name);
    }
    if (!cli_options.output_to_file)
      oObj.write_cout(mObj, sObj);
    return 0;
  } catch(std::string &formattedMessage) {
      error_message(formattedMessage);
  }
}
