#ifdef WIN32
#include "include/j_std_include.hpp"
#else
#include "j_std_include.hpp"
#endif

#define VERSION 0.5

#ifndef WIN32
    #define EXEC josim.exe
#else
    #define EXEC josim
#endif

std::string OUTPUT_PATH, OUTPUT_FILE, OUTPUT_LEGACY_PATH, OUTPUT_LEGACY_FILE, INPUT_PATH, INPUT_FILE;
bool VERBOSE = false;
bool OUTPUT = false;
bool OUTPUT_SPECIFIED = false;
bool OUTPUT_LEGACY = false;
bool OUTPUT_LEGACY_SPECIFIED = false;
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
        std::cout << "For further help use the -h switch" << std::endl;
        exit(0);
    }
	/* Loop through input arguments */
    for (size_t i = 1; i < argc; i++) {
        std::string swString = argv[i];
		/* Initialize the switch char */
        char swChar = '\0';
		/* If argument string starts with a dash */
		if (swString[0] == '-') { 
			swChar = swString[1];
			switch (swChar) {
			/* Display the help menu and then exit. Can be the only arguement sepcified.*/
			case 'h':
				std::cout << "Help menu for JoSIM\n";
				std::cout << "===================\n";
				std::cout << std::setw(5) << std::left << "-g" << std::setw(5) << std::left << "|" << "Plot the requested results with FLTK plotting window" << std::endl;
				std::cout << std::setw(5) << std::left << "  " << std::setw(5) << std::left << "|" << "If this is enabled with VERBOSE mode then all traces are plotted" << std::endl;
				std::cout << std::setw(5) << std::left << "-h" << std::setw(5) << std::left << "|" << "Displays this help menu" << std::endl;
				std::cout << std::setw(5) << std::left << "-o" << std::setw(5) << std::left << "|" << "Specify output file for simulation results (.csv)" << std::endl;
				std::cout << std::setw(5) << std::left << "  " << std::setw(5) << std::left << "|" << "Default will be output.csv if no file is specified" << std::endl;
				std::cout << std::setw(5) << std::left << "-m" << std::setw(5) << std::left << "|" << "[Legacy] JSIM_N output file format (.dat)" << std::endl;
				std::cout << std::setw(5) << std::left << "  " << std::setw(5) << std::left << "|" << "Default will be output.dat if no file is specified" << std::endl;
				std::cout << std::setw(5) << std::left << "-v" << std::setw(5) << std::left << "|" << "Runs JoSIM in verbose mode" << std::endl;
				std::cout << std::endl;
				std::cout << "Example command: josim -g -o ./output.csv test.cir" << std::endl;
				std::cout << std::endl;
				exit(0);
				break;
			/* Enables plotting using built in FLTK library. */
			case 'g':
				if (i == (argc - 1)) {
					error_handling(INPUT_ERROR);
				}
				PLOTTING = true;
				break;
			/* Specifies location for a output file. Cannot be the only argument. Will complain if it is.*/
			case 'o':
				if (i == (argc - 1)) {
					error_handling(INPUT_ERROR);
				}
				if (argv[i + 1][0] == '-') {
					OUTPUT_SPECIFIED = false;
					OUTPUT = true;
				}
				else {
					if ((i + 1) != (argc - 1)) {
						OUTPUT_PATH = argv[i + 1];
						OUTPUT_FILE = file_from_path(OUTPUT_PATH);
						if (!has_suffix(OUTPUT_FILE, ".csv")) {
							error_handling(OUTPUT_FILE_ERROR);
						}
						std::cout << "Path specified for output file: " << OUTPUT_PATH << std::endl;
						std::cout << "Output file specified as: " << OUTPUT_FILE << std::endl;
						std::cout << std::endl;
						OUTPUT_SPECIFIED = true;
						OUTPUT = true;
					}
					else {
						OUTPUT_SPECIFIED = false;
						OUTPUT = true;
					}
				}
				break;
			/* Specifies location for a output file. Cannot be the only argument. Will complain if it is.*/
			case 'm':
				if (i == (argc - 1)) {
					error_handling(INPUT_ERROR);
				}
				if (argv[i + 1][0] == '-') {
					OUTPUT_LEGACY_SPECIFIED = false;
					OUTPUT_LEGACY = true;
				}
				else {
					if ((i + 1) != (argc - 1)) {
						OUTPUT_LEGACY_PATH = argv[i + 1];
						OUTPUT_LEGACY_FILE = file_from_path(OUTPUT_LEGACY_PATH);
						if (!has_suffix(OUTPUT_LEGACY_FILE, ".csv")) {
							error_handling(OUTPUT_LEGACY_FILE_ERROR);
						}
						std::cout << "Path specified for output file: " << OUTPUT_LEGACY_PATH << std::endl;
						std::cout << "Output file specified as: " << OUTPUT_LEGACY_FILE << std::endl;
						std::cout << std::endl;
						OUTPUT_LEGACY_SPECIFIED = true;
						OUTPUT_LEGACY = true;
					}
					else {
						OUTPUT_LEGACY_SPECIFIED = false;
						OUTPUT_LEGACY = true;
					}
				}
				break;
			/* Enables verbose program mode. Cannot be the only argument. Will complain if it is.*/
			case 'v':
				if (i == (argc - 1)) {
					error_handling(INPUT_ERROR);
				}
				std::cout << "Verbose mode has been enabled" << std::endl;
				std::cout << std::endl;
				VERBOSE = true;
				break;
				/* Breaks if any other switch is found that is not an option.*/
			default:
				if (i == (argc - 1)) break;
				error_handling(UNKNOWN_SWITCH);
			}
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
		  if (OUTPUT && !OUTPUT_SPECIFIED) {
			  OUTPUT_PATH = INPUT_PATH;
			  std::string::size_type i = INPUT_PATH.find(INPUT_FILE);
			  if (i != std::string::npos) OUTPUT_PATH.erase(i, INPUT_FILE.length());
			  OUTPUT_PATH = OUTPUT_PATH + "output.csv";
		  }
		  if (OUTPUT_LEGACY && !OUTPUT_LEGACY_SPECIFIED) {
			  OUTPUT_LEGACY_PATH = INPUT_PATH;
			  std::string::size_type i = INPUT_PATH.find(INPUT_FILE);
			  if (i != std::string::npos) OUTPUT_LEGACY_PATH.erase(i, INPUT_FILE.length());
			  OUTPUT_LEGACY_PATH = OUTPUT_LEGACY_PATH + "output.dat";
		  }
        }
    }
    /*Finished handling input arguments. Now setup simulation based on arguments*/
    InputFile iFile(INPUT_PATH);
    iFile.circuit_to_segments(iFile);
    if(VERBOSE) circuit_stats(1, iFile);
    else circuit_stats(0, iFile);
	model_rcsj_functions::identify_models(iFile, models);
	identify_simulation(iFile);
	//bool subcktFound = true;
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
	if (OUTPUT) {
		write_data(iFile);
	}
	if (OUTPUT_LEGACY) {
		write_legacy_data(iFile);
	}
}
