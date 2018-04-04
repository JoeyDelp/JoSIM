#pragma once
#include "include/j_std_include.hpp"

#define VERSION 0.5

#ifndef WIN32
    #define EXEC josim.exe
#else
    #define EXEC josim
#endif

std::string DEFINITIONS_PATH, DEFINITIONS_FILE, LOG_PATH, LOG_FILE, OUTPUT_PATH, OUTPUT_FILE, INPUT_PATH, INPUT_FILE;
bool VERBOSE = false;
bool LOGGING = false;
bool OUTPUT = false;
bool DEFINITIONS = false;
bool PLOTTING = false;
bool DEVELOPER = false;

int main(int argc, char *argv[]) {
    std::cout << std::endl;
    std::cout << "JoSIM: Josephson Junction Superconductive SPICE Circuit Simulator" << std::endl;
    std::cout << "Copyright (C) 2017 by Johannes Delport (jdelport@sun.ac.za)" << std::endl;
    std::cout << std::endl;
    /*
        Parse all the input arguments. All options are switch based, final argument is netlist.
    */
    if (argc <= 1) {
        std::cout << "Missing input arguments" << std::endl;
        std::cout << "Usage: josim [options] input_netlist" << std::endl;
        std::cout << std::endl;
        std::cout << "For further help use the -h or --help switch" << std::endl;
        exit(0);
    }
    for (size_t i = 1; i < argc; i++) {
        std::string swString = argv[i];
        char swChar = '\0';
        if(swString[1] == '-') swChar = swString[2];
        else if(swString[0] != '-') swChar = '\0';
        else swChar = swString[1];
        switch (swChar) {
          /* Display the help menu and then exit. Can be the only arguement sepcified.*/
          case 'h':
            std::cout << "Help menu for JoSIM\n";
            std::cout << "===================\n";
			std::cout << std::setw(25) << std::left << "-d or --definition:" << "Specifiy path to definitions file on the system" << std::endl;
			std::cout << std::setw(25) << std::left << "-g or --graph:" << "Plot the requested results with FLTK plotting window" << std::endl;
			std::cout << std::setw(25) << std::left << "" << "If this is enabled with VERBOSE mode then all traces are plotted" << std::endl;
			std::cout << std::setw(25) << std::left << "-h or --help:" << "Displays tSpecifies location to a defitions filehis help menu" << std::endl;
			std::cout << std::setw(25) << std::left << "-l or --log\t\t: Specify path for the JoSIM log file (.txt)" << std::endl;
			std::cout << std::setw(25) << std::left << "-o or --output\t\t: Specify output file for simulation results (.csv)" << std::endl;
			std::cout << std::setw(25) << std::left << "-v or --verbose\t\t: Runs JoSIM in verbose mode" << std::endl;
            std::cout << std::endl;
            std::cout << "Example command: josim -d=/path/to/defintions.txt --log=/path/to/log.txt -o=./output.csv test.cir" << std::endl;
			std::cout << std::endl;
            exit(0);
            break;
          /* Specifies location to a defitions file. Cannot be the only argument. Will complain if it is.*/
          case 'd':
            if(i == (argc - 1)) {
              error_handling(INPUT_ERROR);
              exit(0);
            }
            if(swString.find("=") == std::string::npos) {
              error_handling(DEF_ERROR);
              exit(0);
            }
            DEFINITIONS_PATH = swString.substr(swString.find("=") + 1);
            DEFINITIONS_FILE = file_from_path(DEFINITIONS_PATH);
            if(!has_suffix(DEFINITIONS_FILE, ".txt")) {
              error_handling(DEF_FILE_ERROR);
              exit(0);
            }
            std::cout << "Path specified for definitions file: " << DEFINITIONS_PATH << std::endl;
            std::cout << "Definitions file specified as: " << DEFINITIONS_FILE << std::endl;
            std::cout << "\n";
            DEFINITIONS = true;
            break;
		  case 'g':
			  PLOTTING = true;
			  break;
          /* Specifies location for a log file. Cannot be the only argument. Will complain if it is.*/
          case 'l':
            if(i == (argc - 1)) {
              error_handling(INPUT_ERROR);
              exit(0);
            }
            if(swString.find("=") == std::string::npos) {
              error_handling(LOG_ERROR);
              exit(0);
            }
            LOG_PATH = swString.substr(swString.find("=") + 1);
            LOG_FILE = file_from_path(LOG_PATH);
            if(!has_suffix(LOG_FILE, ".txt")) {
              error_handling(LOG_FILE_ERROR);
              exit(0);
            }
            std::cout << "Path specified for log file: " << LOG_PATH << std::endl;
            std::cout << "Log file specified as: " << LOG_FILE << std::endl;
            std::cout << std::endl;
            LOGGING = true;
            break;
          /* Specifies location for a output file. Cannot be the only argument. Will complain if it is.*/
          case 'o':
            if(i == (argc - 1)) {
              error_handling(INPUT_ERROR);
            }
            if(swString.find("=") == std::string::npos) {
              error_handling(OUTPUT_ERROR);
            }
            OUTPUT_PATH = swString.substr(swString.find("=") + 1);
            OUTPUT_FILE = file_from_path(OUTPUT_PATH);
            if(!has_suffix(OUTPUT_FILE, ".csv")) {
              error_handling(OUTPUT_FILE_ERROR);
            }
            std::cout << "Path specified for output file: " << OUTPUT_PATH << std::endl;
            std::cout << "Output file specified as: " << OUTPUT_FILE << std::endl;
            std::cout << std::endl;
            if(i == (argc - 1)) {
              error_handling(INPUT_ERROR);
            }
            OUTPUT = true;
            break;
          /* Enables verbose program mode. Cannot be the only argument. Will complain if it is.*/
          case 'v':
            if(i == (argc - 1)) {
              error_handling(INPUT_ERROR);
            }
            std::cout << "Verbose mode has been enabled" << std::endl;
            std::cout << std::endl;
            VERBOSE = true;
            break;
          /* Breaks if any other switch is found that is not an option.*/
          default:
            if(i == (argc - 1)) break;
            error_handling(UNKNOWN_SWITCH);
        }
        /* Check the final argument to see if it is a valid input file. Break if it is not*/
        if(i == (argc - 1)) {
          INPUT_PATH = swString;
          INPUT_FILE = file_from_path(INPUT_PATH);
		  /* Break if the input file is not of the correct extension*/
          if(!has_suffix(INPUT_FILE, ".cir")) {
			  if (!has_suffix(INPUT_FILE, ".js")) {
				  error_handling(INPUT_FILE_ERROR);
			  }
          }
          std::cout << "Path specified for input file: " << INPUT_PATH << std::endl;
          std::cout << "Input file specified as: " << INPUT_FILE << std::endl;
          std::cout << std::endl;
        }
    }
    /*Finished handling input arguments. Now setup simulation based on arguments*/
    InputFile iFile(INPUT_PATH);
    iFile.circuit_to_segments(iFile);
    if(VERBOSE) circuit_stats(1, iFile);
    else circuit_stats(0, iFile);
	model_rcsj_functions::identify_models(iFile, models);
	identify_simulation(iFile);
	bool subcktFound = true;
	int thisDepth, overallDepth;
	thisDepth = 1;
	overallDepth = 1;
	int subcktDepth = subCircuitDepth(iFile.maincircuitSegment, iFile, thisDepth, overallDepth);
	for (int i = 0; i < subcktDepth; i++) {
		iFile.sub_in_subcircuits(iFile, iFile.maincircuitSegment);
	}
	/* Debugging tool that can only be activated in the code for checking final netlist after subcircuit substitution */
	if (DEVELOPER) {
		for (auto i : iFile.maincircuitSegment) {
			std::vector<std::string> tokens = tokenize_space(i);
			for (auto j : tokens) {
				std::cout << std::setw(15) << std::left << j;
			}
			std::cout << std::endl;
		}
	}
	/* Create A matrix from final netlist */
	matrix_A(iFile);
	/* Do transient simulation */
	transient_simulation();
	if (PLOTTING) {
		if (VERBOSE) plot_all_traces();
		else plot_traces(iFile);
	}
}
