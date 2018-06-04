// Copyright (c) 2018 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#include "j_std_include.hpp"

std::string OUTPUT_PATH, OUTPUT_FILE, OUTPUT_LEGACY_PATH, OUTPUT_LEGACY_FILE,
            INPUT_PATH, INPUT_FILE;
bool VERBOSE = false, 
      OUTPUT = false,
      OUTPUT_SPECIFIED = false,
      OUTPUT_LEGACY = false,
      OUTPUT_LEGACY_SPECIFIED = false,
      PLOTTING = false,
      DEVELOPER = false;
int subcktDepth,
    thisDepth = 1,
    overallDepth = 1;

/*
  JoSIM entry point
*/
int
main(int argc, char* argv[])
{
  /* Title, copyright and version information */
  std::cout << std::endl;
  std::cout
    << "JoSIM: Josephson Junction Superconductive SPICE Circuit Simulator"
    << std::endl;
  std::cout << "Copyright (C) 2018 by Johannes Delport (jdelport@sun.ac.za)"
            << std::endl;
  std::cout << "v" << VERSION << " compiled on " << __DATE__ << " at " << __TIME__
            << std::endl;
  std::cout << std::endl;
  /* End title and versioning info */

  /* Parse arguments */
  if (argc <= 1) error_handling(TOO_FEW_ARGUMENTS);
  // Loop through input arguments
  for (size_t i = 1; i < argc; i++) {
    // If argument string starts with a dash
    if (argv[i][0] == '-') {
      switch (argv[i][1]) {
        // Help menu
        case 'h' :
          std::cout << "JoSIM help interface\n";
          std::cout << "====================\n";
          std::cout << std::setw(5) << std::left << "-g" << std::setw(5)
                    << std::left << "|"
                    << "Plot the requested results using a plotting library"
                    << std::endl;
          std::cout << std::setw(5) << std::left << "  " << std::setw(5)
                    << std::left << "|"
                    << "If this is enabled with verbose mode then all traces "
                       "are plotted"
                    << std::endl;
          std::cout << std::setw(5) << std::left << "-h" << std::setw(5)
                    << std::left << "|"
                    << "Displays this help menu" << std::endl;
          std::cout << std::setw(5) << std::left << "-o" << std::setw(5)
                    << std::left << "|"
                    << "Specify output file for simulation results (.csv)"
                    << std::endl;
          std::cout << std::setw(5) << std::left << "  " << std::setw(5)
                    << std::left << "|"
                    << "Default will be output.csv if no file is specified"
                    << std::endl;
          std::cout << std::setw(5) << std::left << "-m" << std::setw(5)
                    << std::left << "|"
                    << "[Legacy] JSIM_N output file format (.dat)" << std::endl;
          std::cout << std::setw(5) << std::left << "  " << std::setw(5)
                    << std::left << "|"
                    << "Default will be output.dat if no file is specified"
                    << std::endl;
          std::cout << std::setw(5) << std::left << "-v" << std::setw(5)
                    << std::left << "|"
                    << "Runs JoSIM in verbose mode" << std::endl;
          std::cout << std::endl;
          std::cout << "Example command: josim -g -o ./output.csv test.cir"
                    << std::endl;
          std::cout << std::endl;
          exit(0);
        // Enables plotting
        case 'g' :
          // Complains if missing an input file
          if (i == (argc - 1)) error_handling(INPUT_ERROR);
          PLOTTING = true;
          break;
        // Output (csv) file path
        case 'o' :
          // Complains if missing an input file
          if (i == (argc - 1)) error_handling(INPUT_ERROR);
          // Output path is not specified, use default
          if (argv[i + 1][0] == '-') {
            OUTPUT_SPECIFIED = false;
            OUTPUT = true;
          } else {
            // If the next argument is not the final argument
            if ((i + 1) != (argc - 1)) {
              // Set the path and filename
              OUTPUT_PATH = argv[i + 1];
              OUTPUT_FILE = file_from_path(OUTPUT_PATH);
              // Error if filename does not end with .csv
              if (!has_suffix(OUTPUT_FILE, ".csv")) error_handling(OUTPUT_FILE_ERROR);
              // Let the user know which file will be written to
              std::cout << "Path specified for output file: " << OUTPUT_PATH
                        << std::endl;
              std::cout << "Output file specified as: " << OUTPUT_FILE
                        << std::endl;
              std::cout << std::endl;
              OUTPUT_SPECIFIED = true;
              OUTPUT = true;
            } else {
              // Output path is not specified, use default
              OUTPUT_SPECIFIED = false;
              OUTPUT = true;
            }
          }
          break;
        // Output (Legacy) file path
        case 'm' :
          // Complains if missing input file
          if (i == (argc - 1)) error_handling(INPUT_ERROR);
          // Output path not specified, use default
          if (argv[i + 1][0] == '-') {
            OUTPUT_LEGACY_SPECIFIED = false;
            OUTPUT_LEGACY = true;
          } else {
            // If the next argument is not the final argument
            if ((i + 1) != (argc - 1)) {
              OUTPUT_LEGACY_PATH = argv[i + 1];
              OUTPUT_LEGACY_FILE = file_from_path(OUTPUT_LEGACY_PATH);
              // Error if the filename specified does not end with .dat
              if (!has_suffix(OUTPUT_LEGACY_FILE, ".dat")) error_handling(OUTPUT_LEGACY_FILE_ERROR);
              // Tell the user which file will be written to
              std::cout << "Path specified for output file: "
                        << OUTPUT_LEGACY_PATH << std::endl;
              std::cout << "Output file specified as: " << OUTPUT_LEGACY_FILE
                        << std::endl;
              std::cout << std::endl;
              OUTPUT_LEGACY_SPECIFIED = true;
              OUTPUT_LEGACY = true;
            } else {
              // Output path not specified, use default
              OUTPUT_LEGACY_SPECIFIED = false;
              OUTPUT_LEGACY = true;
            }
          }
          break;
        // Enable verbose mode
        case 'v' :
          // Complain if this is the only argument
          if (i == (argc - 1)) error_handling(INPUT_ERROR);
          std::cout << "Verbose mode has been enabled" << std::endl;
          std::cout << std::endl;
          VERBOSE = true;
          break;
        default:
          // Complain if this is the only argument
          if (i == (argc - 1)) error_handling(INPUT_ERROR);
          // Complain if switch is not known, just ignore it
          error_handling(UNKNOWN_SWITCH);
          break; 
      }
    }
    // Check input file if valid suffix
    if (i == (argc - 1)) {
      // Set input filename and path
      INPUT_PATH = argv[i];
      INPUT_FILE = file_from_path(INPUT_PATH);
      // Complain if input file is not of the correct suffix
      if (!has_suffix(INPUT_FILE, ".cir"))
        if (!has_suffix(INPUT_FILE, ".js"))
          error_handling(INPUT_FILE_ERROR);
      // Inform the user which file was selected as input file
      std::cout << "Path specified for input file: " << INPUT_PATH << std::endl;
      std::cout << "Input file specified as: " << INPUT_FILE << std::endl;
      std::cout << std::endl;
      // Set default output file path to the same as that of input file
      if (OUTPUT && !OUTPUT_SPECIFIED) {
        OUTPUT_PATH = INPUT_PATH;
        std::string::size_type i = INPUT_PATH.find(INPUT_FILE);
        if (i != std::string::npos)
          OUTPUT_PATH.erase(i, INPUT_FILE.length());
        OUTPUT_PATH.append("output.csv");
      }
      if (OUTPUT_LEGACY && !OUTPUT_LEGACY_SPECIFIED) {
        OUTPUT_LEGACY_PATH = INPUT_PATH;
        std::string::size_type i = INPUT_PATH.find(INPUT_FILE);
        if (i != std::string::npos)
          OUTPUT_LEGACY_PATH.erase(i, INPUT_FILE.length());
        OUTPUT_LEGACY_PATH.append("output.dat");
      }
    }
  }
  /* End parse arguments */

  /* Start JoSIM */
  // Parse the input file into a variable
  InputFile iFile(INPUT_PATH);
  // Split up the parsed file into segments
  iFile.circuit_to_segments(iFile);
  // Find all the models in the parsed segments
  identify_models(iFile, models);
  // Identify the type of simulation
  identify_simulation(iFile);
  // Determine the maximum depth of subcircuits
  subcktDepth =
    subCircuitDepth(iFile.maincircuitSegment, iFile, thisDepth, overallDepth);
  // Substiture the subcircuits into their relevant positions
  for (int i = 0; i < subcktDepth; i++) iFile.sub_in_subcircuits(iFile, iFile.maincircuitSegment);
  // Print full circuit statistics if verbose, else simple statistics
  if (VERBOSE) circuit_stats(1, iFile);
  else circuit_stats(0, iFile);
  // Developer mode debugging the entire substituted main circuit
  if (DEVELOPER) {
    for (const auto& i : iFile.maincircuitSegment) {
      std::vector<std::string> tokens = tokenize_space(i);
      for (const auto& j : tokens) {
        if(j.length() < 15) std::cout << std::setw(15) << std::left << j;
        else std::cout << std::setw(j.length() + (30 - j.length())) << std::left << j;
      }
      std::cout << std::endl;
    }
  }
  // Parse the input file and create an A matrix
  matrix_A(iFile);
  // Decide which simulation to do based on commands specified
  if (iFile.simulationType == TRANSIENT)
    // Do a transient simulation
    transient_simulation();
  // If plotting is specified
  if (PLOTTING) {
    // No plotting engine compiled
#ifdef USING_NONE
    error_handling(NO_PLOT_COMPILE);
#else
    // Plot all possible traces in verbose
    if (VERBOSE) plot_all_traces();
    // Plot only specified traces
    else plot_traces(iFile);
#endif
  }
  // Print relevant output files
  if (OUTPUT) write_data(iFile);
  if (OUTPUT_LEGACY) write_legacy_data(iFile);
  // TODO: Print raw output file
  if (!OUTPUT && !OUTPUT_LEGACY) std::cout << "RESULTS: " << std::endl;
}
