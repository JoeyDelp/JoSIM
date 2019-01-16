// Copyright (c) 2018 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#include "j_args.h"

void Args::parse_arguments(int argc, char* argv[]) {
    if (argc <= 1) Errors::error_handling(TOO_FEW_ARGUMENTS);

    if (argv[argc - 1][0] == '-') {
        if(argv[argc - 1][1] == 'h'){
            display_help();
            exit(0);
        }
        else if (argv[argc - 1][1] == 'v'){
            version_info();
            exit(0);
        }
        else if (argv[argc - 1][1] == '-') {
            if(argv[argc - 1][2] == 'h') {
                display_help();
                exit(0);
            }
            else if (argv[argc - 1][2] == 'v') {
                version_info();
                exit(0);
            }
            else Errors::error_handling(FINAL_ARG_SWITCH);
        }
        else Errors::error_handling(FINAL_ARG_SWITCH);
    }
    else if (argv[argc - 1][0] != '-' && argv[argc - 1][1] != 'h') inName = argv[argc - 1];
    else Errors::error_handling(FINAL_ARG_SWITCH);

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            switch(argv[i][1]) {
            case 'a':
                if ((i + 1) == (argc - 1)) Errors::error_handling(TOO_FEW_ARGUMENTS);
                else if (argv[i + 1][0] == '-') Errors::error_handling(TOO_FEW_ARGUMENTS);
                else{
                    try {
                        analysisT = std::stoi(argv[i + 1]);
                        if (analysisT > 1) Errors::error_handling(INVALID_ANALYSIS);
                    }
                    catch (std::exception& e) {
                        Errors::error_handling(INVALID_ANALYSIS);
                    }
                }
                break;
            case 'c':
                if ((i + 1) == (argc - 1)) Errors::error_handling(TOO_FEW_ARGUMENTS);
                else if (argv[i + 1][0] == '-') Errors::error_handling(TOO_FEW_ARGUMENTS);
                else{
                    try {
                        sbcktConv = std::stoi(argv[i + 1]);
                        if (sbcktConv > 1) Errors::error_handling(INVALID_CONVENTION);
                    }
                    catch (std::exception& e) {
                        Errors::error_handling(INVALID_CONVENTION);
                    }
                }
                break;
            case 'g':
                std::cout << "Plotting engine: " <<
                #ifdef USING_FLTK
                        "FTLK"
                #elif USING_MATPLOTLIB
                        "MATPLOTLIB"
                #else
                        "NONE"
                #endif
                        << std::endl;
                #ifndef USING_FLTK
                    #ifndef USING_MATPLOTLIB
                        std::cout << "Plotting requested with no plotting engine."
                        << std::endl;
                        std::cout << "Request to plot will be ignored."
                        << std::endl;
                    #else
                        plotRes = true;
                    #endif
                #else
                    plotRes = true;
                #endif
                break;
            case 'h':
                display_help();
                break;
            case 'o':
                saveRes = true;
                if (((i + 1) == (argc - 1)) || (argv[i + 1][0] == '-')) {
                    outName = inName;
                    outName = outName.substr(0, outName.find_last_of('.')) + ".csv";
                    saveType = CSV;
                }
                else {
                    outName = argv[i + 1];
                    if(outName.find('.') != std::string::npos) {
                        std::string outExt = outName.substr(outName.find_last_of('.'),
                         outName.size() - 1);
                        std::transform(outExt.begin(), outExt.end(),
                         outExt.begin(), toupper);
                        if (outExt == ".CSV") saveType = CSV;
                        else if (outExt == ".DAT") saveType = DAT;
                        else saveType = WR;
                    }
                    else saveType = WR;
                } 
                break;
            case 'p':
                #ifdef USING_OPENMP
                    std::cout << "Parallelization is ENABLED" << std::endl;
                #else
                    std::cout << "Parallelization is DISABLED" << std::endl;
                #endif
                break;
            case 'V':
                verbose = true;
                break;
            case 'v':
                version_info();
                exit(0);
            }
        }
    }
}

void Args::display_help() {
    std::cout << "JoSIM help interface\n";
    std::cout << "====================\n";
    std::cout << std::setw(13) << std::left << "-a(nalysis)" << std::setw(3)
        << std::left << "|"
        << "Specifies the analysis type." << std::endl;
    std::cout << std::setw(13) << std::left << "  " << std::setw(3)
        << std::left << "|"
        << "0 for Voltage analysis (Default)." << std::endl;	
    std::cout << std::setw(13) << std::left << "  " << std::setw(3)
        << std::left << "|"
        << "1 for Phase analysis." << std::endl;

    std::cout << std::setw(13) << std::left << "-c(onvention)" << std::setw(3)
        << std::left << "|"
        << "Sets the subcircuit convention to left(0) or right(1)."
        << std::endl;
    std::cout << std::setw(13) << std::left << "  " << std::setw(3)
        << std::left << "|"
        << "Default is left. WRSpice (normal SPICE) use right."
        << std::endl;
    std::cout << std::setw(13) << std::left << "  " << std::setw(3)
        << std::left << "|"
        << "Eg. X01 SUBCKT 1 2 3     vs.     X01 1 2 3 SUBCKT"
        << std::endl;

    std::cout << std::setw(13) << std::left << "-g(raph)" << std::setw(3)
        << std::left << "|"
        << "Plot the requested results using a plotting library."
        << std::endl;
    std::cout << std::setw(13) << std::left << "  " << std::setw(3)
        << std::left << "|"
        << "If this is enabled with verbose mode all traces "
        "are plotted."
        << std::endl;

    std::cout << std::setw(13) << std::left << "-h(elp)" << std::setw(3)
        << std::left << "|"
        << "Displays this help menu" << std::endl;

    std::cout << std::setw(13) << std::left << "-o(utput)" << std::setw(3)
        << std::left << "|"
        << "Specify output file for simulation results (.csv)."
        << std::endl;
    std::cout << std::setw(13) << std::left << "  " << std::setw(3)
        << std::left << "|"
        << "Default will be output.csv if no file is specified."
        << std::endl;

    std::cout << std::setw(13) << std::left << "-p(arallel)" << std::setw(3)
        << std::left << "|"
        << "(EXPERIMENTAL) Enables parallelization of certain functions." << std::endl;
    std::cout << std::setw(13) << std::left << "  " << std::setw(3)
        << std::left << "|"
        << "Requires compilation with OPENMP switch enabled."
        << std::endl;
    std::cout << std::setw(13) << std::left << "  " << std::setw(3)
        << std::left << "|"
        << "Threshold applies, overhead on small circuits negates performance."
        << std::endl;

    std::cout << std::setw(13) << std::left << "-V(erbose)" << std::setw(3)
        << std::left << "|"
        << "Runs JoSIM in verbose mode." << std::endl;

    std::cout << std::setw(13) << std::left << "-v(ersion)" << std::setw(3)
        << std::left << "|"
        << "Displays the JoSIM version info only." << std::endl;

    std::cout << std::endl;
    std::cout << "Example command: josim -g -o ./output.csv test.cir"
        << std::endl;
    std::cout << std::endl;
    exit(0);
}

void Args::version_info() {
    std::cout << std::endl;
	std::cout
		<< "JoSIM: Josephson Junction Superconductive SPICE Circuit Simulator"
		<< std::endl;
	std::cout << "Copyright (C) 2018 by Johannes Delport (jdelport@sun.ac.za)"
		<< std::endl;
	std::cout << "v" << VERSION << " compiled on " << __DATE__ << " at " << __TIME__
		<< std::endl;
    std::cout << std::endl;
}