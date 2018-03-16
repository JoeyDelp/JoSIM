#include "include/j_errors.hpp"

/*
  Function that manages different error codes. This function will be huge.
*/
void error_handling(int errorCode) {
  switch(errorCode) {
    case DEF_ERROR:
      std::cout << "E: Invalid definitions file specifier.\n";
      std::cout << "E: Specifier should be: -d=<path to file> or --definition=<path to file>\n";
	  exit(0);
    case DEF_FILE_ERROR:
      std::cout << "E: Invalid definitions file specified.\n";
      std::cout << "E: Specified file needs to be of extension .txt\n";
      exit(0);
    case LOG_ERROR:
      std::cout << "E: Invalid log file specifier.\n";
      std::cout << "E: Specifier should be: -l=<path to file> or --log=<path to file>\n";
      exit(0);
    case LOG_FILE_ERROR:
      std::cout << "E: Invalid log file specified.\n";
      std::cout << "E: Specified file needs to be of extension .txt\n";
      exit(0);
    case OUTPUT_ERROR:
      std::cout << "E: Invalid output file specifier.\n";
      std::cout << "E: Specifier should be: -o=<path to file> or --output=<path to file>\n";
      exit(0);
    case OUTPUT_FILE_ERROR:
      std::cout << "E: Invalid output file specified.\n";
      std::cout << "E: Specified file needs to be of extension .csv\n";
      exit(0);
    case INPUT_ERROR:
      std::cout << "E: Missing input file.\n";
      std::cout << "E: Final argument should be: <path to input file>\n";
      exit(0);
    case INPUT_FILE_ERROR:
      std::cout << "E: Invalid input file specified.\n";
      std::cout << "E: Specified file needs to be of extension .cir or .js\n";
      exit(0);
    case UNKNOWN_SWITCH:
      std::cout << "E: Unknown option specified. Please refer to the help menu.\n";
      exit(0);
    case CANNOT_OPEN_FILE:
      std::cout << "E: Input file " << INPUT_PATH << " cannout be found or opened.\n";
      std::cout << "E: Please ensure that the file exists and can be opened.\n";
      exit(0);
  }
}
/*
  Invalid component decleration error function
*/
void invalid_component_errors(int errorCode, std::string whatPart) {
	switch (errorCode) {
	case RES_ERROR:
		std::cout << "E: Resistor value error\n";
		std::cout << "E: Infringing line: " << whatPart << "\n";
		exit(0);
	case CAP_ERROR:
		std::cout << "E: Capacitor value error\n";
		std::cout << "E: Infringing line: " << whatPart << "\n";
		exit(0);
	case IND_ERROR:
		std::cout << "E: Inductor value error\n";
		std::cout << "E: Infringing line: " << whatPart << "\n";
		exit(0);
	case LABEL_ERROR:
		std::cout << "Invalid component label: " << whatPart << "\n";
		exit(0);
	case MISSING_LABEL:
		std::cout << "E: No component label. This should not happen.\n";
		std::cout << "E: Infringing line: " << whatPart << "\n";
		std::cout << "E: Please contact the developer as this is possibly a bug.\n";
		exit(0);
	case MISSING_PNODE:
		std::cout << "E: No positive node. This should not hapen.\n";
		std::cout << "E: Infringing line: " << whatPart << "\n";
		std::cout << "E: Please contact the developer as this is possibly a bug.\n";
		exit(0);
	case MISSING_NNODE:
		std::cout << "E: No negative node. This should not hapen.\n";
		std::cout << "E: Infringing line: " << whatPart << "\n";
		std::cout << "E: Please contact the developer as this is possibly a bug.\n";
		exit(0);
	case MISSING_JJMODEL:
		std::cout << "E: No junction model is specified for junction " << whatPart << "\n";
		exit(0);
	case MODEL_NOT_DEFINED:
		std::cout << "W: The specified model " << whatPart << " is not defined\n";
		std::cout << "W: Using default model as specified in the manual.\n";
	case MODEL_AREA_NOT_GIVEN:
		std::cout << "W: No area specified for junction " << whatPart << "\n";
		std::cout << "W: Using default: AREA=1.0\n";
	}

}
/*
  Missing simulation and incorrect control parameters specified
*/
void control_errors(int errorCode, std::string whatPart) {
  switch (errorCode) {
    case TRANS_ERROR:
		std::cout << "E: Invalid transient analysis specified. " << whatPart << "\n";
		exit(0);
    case PRINT_ERROR:
		exit(0);
    case PLOT_ERROR:
		exit(0);
    case INV_CONTROL:
		exit(0);
	case DC_ERROR:
		exit(0);
	case AC_ERROR:
		exit(0);
	case PHASE_ERROR:
		exit(0);
	case NO_SIM:
		std::cout << "E: No simulation type specified. Nothing will be simulated.\n";
		exit(0);
  }
}
/*
  Model declaration error function
*/
void model_errors(int errorCode, std::string whatPart) {
	switch (errorCode) {
	case PARAM_TYPE_ERROR:
		std::cout << "E: Unknown model parameter " << whatPart << " specified.\n";
		exit(0);
	}
}
/*
  Matrix creation error function
*/
void matrix_errors(int errorCode, std::string whatPart) {
	switch (errorCode) {
	case NON_SQUARE:
		std::cout << "E: Matrix is not square. Dimensions are " << whatPart << "\n";
		std::cout << "E: Please contact the developer as this is potentially a bug.\n";
		exit(0);
	}
}
/*
  Misc error function
*/
void misc_errors(int errorCode, std::string whatPart) {
	switch (errorCode) {
	case STOD_ERROR:
		std::cout << "E: Cannot convert string to double: " << whatPart << "\n";
		exit(0);
	}
}
/*
Function parser error function
*/
void function_errors(int errorCode, std::string whatPart) {
	switch (errorCode) {
	case INITIAL_VALUES:
		std::cout << "E: Invalid PWL definition found. The value of " << whatPart << " is expected to be 0\n";
		std::cout << "E: Please refer to the PWL definition: PWL(0 0 T1 V1 T2 V2 ... Tn Vn)\n";
		exit(0);
	case TOO_FEW_TIMESTEPS:
		std::cout << "E: Total timesteps specified do not match the values specified." << whatPart << " specified.\n";
		std::cout << "E: Please refer to the PWL definition: PWL(0 0 T1 V1 T2 V2 ... Tn Vn)\n";
		exit(0);
	case TOO_FEW_VALUES:
		std::cout << "E: Total values specified do not match the timesteps specified." << whatPart << " specified.\n";
		std::cout << "E: Please refer to the PWL definition: PWL(0 0 T1 V1 T2 V2 ... Tn Vn)\n";
		exit(0);
	}
}
/*
Simulation error function
*/
void simulation_errors(int errorCode, std::string whatPart) {
	switch (errorCode) {
	case JJCAP_NOT_FOUND:
		std::cout << "E: Capacitor value for " << whatPart << " could not be found.\n";
		std::cout << "E: This is a bug and the developer should be contacted. The program will abort.\n";
		exit(0);
	case JJICRIT_NOT_FOUND:
		std::cout << "E: Critical current value for " << whatPart << " could not be found.\n";
		std::cout << "E: This is a bug and the developer should be contacted. The program will abort.\n";
		exit(0);
	case INDUCTOR_CURRENT_NOT_FOUND:
		std::cout << "E: Inductor current not defined for " << whatPart << ". Matrix will have no solution.\n";
		std::cout << "E: This is a bug and the developer should be contacted. The program will abort.\n";
		exit(0);
	}
}