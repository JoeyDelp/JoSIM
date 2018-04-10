#include "include/j_errors.hpp"

/*
  Function that manages different error codes. This function will be huge.
*/
void error_handling(int errorCode) {
  switch(errorCode) {
    case DEF_ERROR:
      std::cout << "E: Invalid definitions file specifier." << std::endl;
      std::cout << "E: Specifier should be: -d=<path to file> or --definition=<path to file>" << std::endl;
	  exit(0);
    case DEF_FILE_ERROR:
      std::cout << "E: Invalid definitions file specified." << std::endl;
      std::cout << "E: Specified file needs to be of extension .txt" << std::endl;
      exit(0);
    case LOG_ERROR:
      std::cout << "E: Invalid log file specifier." << std::endl;
      std::cout << "E: Specifier should be: -l=<path to file> or --log=<path to file>" << std::endl;
      exit(0);
    case LOG_FILE_ERROR:
      std::cout << "E: Invalid log file specified." << std::endl;
      std::cout << "E: Specified file needs to be of extension .txt" << std::endl;
      exit(0);
    case OUTPUT_ERROR:
      std::cout << "E: Invalid output file specifier." << std::endl;
      std::cout << "E: Specifier should be: -o=<path to file> or --output=<path to file>" << std::endl;
      exit(0);
    case OUTPUT_FILE_ERROR:
      std::cout << "E: Invalid output file specified." << std::endl;
      std::cout << "E: Specified file needs to be of extension .csv" << std::endl;
      exit(0);
	case OUTPUT_LEGACY_ERROR:
		std::cout << "E: Invalid output file specifier." << std::endl;
		std::cout << "E: Specifier should be: -m=<path to file>" << std::endl;
		exit(0);
	case OUTPUT_LEGACY_FILE_ERROR:
		std::cout << "E: Invalid output file specified." << std::endl;
		std::cout << "E: Specified file needs to be of extension .dat" << std::endl;
		exit(0);
    case INPUT_ERROR:
      std::cout << "E: Missing input file." << std::endl;
      std::cout << "E: Final argument should be: <path to input file>" << std::endl;
      exit(0);
    case INPUT_FILE_ERROR:
      std::cout << "E: Invalid input file specified." << std::endl;
      std::cout << "E: Specified file needs to be of extension .cir or .js" << std::endl;
      exit(0);
    case UNKNOWN_SWITCH:
      std::cout << "E: Unknown option specified. Please refer to the help menu." << std::endl;
      exit(0);
    case CANNOT_OPEN_FILE:
      std::cout << "E: Input file " << INPUT_PATH << " cannout be found or opened." << std::endl;
      std::cout << "E: Please ensure that the file exists and can be opened." << std::endl;
      exit(0);
  }
}
/*
  Invalid component decleration error function
*/
void invalid_component_errors(int errorCode, std::string whatPart) {
	switch (errorCode) {
	case RES_ERROR:
		std::cout << "E: Resistor value error" << std::endl;
		std::cout << "E: Infringing line: " << whatPart << std::endl;
		exit(0);
	case CAP_ERROR:
		std::cout << "E: Capacitor value error" << std::endl;
		std::cout << "E: Infringing line: " << whatPart << std::endl;
		exit(0);
	case IND_ERROR:
		std::cout << "E: Inductor value error" << std::endl;
		std::cout << "E: Infringing line: " << whatPart << std::endl;
		exit(0);
	case LABEL_ERROR:
		std::cout << "Invalid component label: " << whatPart << std::endl;
		exit(0);
	case MISSING_LABEL:
		std::cout << "E: No component label. This should not happen." << std::endl;
		std::cout << "E: Infringing line: " << whatPart << std::endl;
		std::cout << "E: Please contact the developer as this is possibly a bug." << std::endl;
		exit(0);
	case MISSING_PNODE:
		std::cout << "E: No positive node. This should not hapen." << std::endl;
		std::cout << "E: Infringing line: " << whatPart << std::endl;
		std::cout << "E: Please contact the developer as this is possibly a bug." << std::endl;
		exit(0);
	case MISSING_NNODE:
		std::cout << "E: No negative node. This should not hapen." << std::endl;
		std::cout << "E: Infringing line: " << whatPart << std::endl;
		std::cout << "E: Please contact the developer as this is possibly a bug." << std::endl;
		exit(0);
	case MISSING_JJMODEL:
		std::cout << "E: No junction model is specified for junction " << whatPart << std::endl;
		exit(0);
	case MODEL_NOT_DEFINED:
		std::cout << "W: The specified model " << whatPart << " is not defined" << std::endl;
		std::cout << "W: Using default model as specified in the manual." << std::endl;
		break;
	case MODEL_AREA_NOT_GIVEN:
		std::cout << "W: No area specified for junction " << whatPart << std::endl;
		std::cout << "W: Using default: AREA=1.0" << std::endl;
		break;
	case DUPLICATE_LABEL:
		std::cout << "E: Duplicate label " << whatPart << " detected." << std::endl;
		std::cout << "E: The program will now terminate. Please recheck the netlist." << std::endl;
		exit(0);
	case INVALID_SUBCIRCUIT_NODES:
		std::cout << "E: The nodes for label " << whatPart << " does not match the required nodes of the subcicuit." << std::endl;
		std::cout << "E: Please recheck the nodes required by the subcircuit and try again." << std::endl;
		exit(0);
	}

}
/*
  Missing simulation and incorrect control parameters specified
*/
void control_errors(int errorCode, std::string whatPart) {
  switch (errorCode) {
    case TRANS_ERROR:
		std::cout << "E: Invalid transient analysis specified. " << whatPart << std::endl;
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
		std::cout << "E: No simulation type specified. Nothing will be simulated." << std::endl;
		exit(0);
  }
}
/*
  Model declaration error function
*/
void model_errors(int errorCode, std::string whatPart) {
	switch (errorCode) {
	case PARAM_TYPE_ERROR:
		std::cout << "E: Unknown model parameter " << whatPart << " specified." << std::endl;
		exit(0);
	}
}
/*
  Matrix creation error function
*/
void matrix_errors(int errorCode, std::string whatPart) {
	switch (errorCode) {
	case NON_SQUARE:
		std::cout << "E: Matrix is not square. Dimensions are " << whatPart << std::endl;
		std::cout << "E: Please contact the developer as this is potentially a bug." << std::endl;
		exit(0);
	}
}
/*
  Misc error function
*/
void misc_errors(int errorCode, std::string whatPart) {
	switch (errorCode) {
	case STOD_ERROR:
		std::cout << "E: Cannot convert string to double: " << whatPart << std::endl;
		exit(0);
	}
}
/*
Function parser error function
*/
void function_errors(int errorCode, std::string whatPart) {
	switch (errorCode) {
	case INITIAL_VALUES:
		std::cout << "E: Invalid PWL definition found. The value of " << whatPart << " is expected to be 0" << std::endl;
		std::cout << "E: Please refer to the PWL definition: PWL(0 0 T1 V1 T2 V2 ... Tn Vn)" << std::endl;
		exit(0);
	case TOO_FEW_TIMESTEPS:
		std::cout << "E: Total timesteps specified do not match the values specified." << whatPart << " specified." << std::endl;
		std::cout << "E: Please refer to the PWL definition: PWL(0 0 T1 V1 T2 V2 ... Tn Vn)" << std::endl;
		exit(0);
	case TOO_FEW_VALUES:
		std::cout << "E: Total values specified do not match the timesteps specified." << whatPart << " specified." << std::endl;
		std::cout << "E: Please refer to the PWL definition: PWL(0 0 T1 V1 T2 V2 ... Tn Vn)" << std::endl;
		exit(0);
	case INITIAL_PULSE_VALUE:
		std::cout << "E: Invalid PULSE definition found. The value of " << whatPart << " is expected to be 0" << std::endl;
		std::cout << "E: Please refer to the PULSE definition: PULSE(0 V2 TD TR TF PW PER)" << std::endl;
		exit(0);
	case PULSE_TOO_FEW_ARGUMENTS:
		std::cout << "E: Total arguments specified do not match the required for PULSE." << whatPart << " specified." << std::endl;
		std::cout << "E: Please refer to the PULSE definition: PULSE(0 V2 TD TR TF PW PER)" << std::endl;
		exit(0);
	case PULSE_VPEAK_ZERO:
		std::cout << "W: PULSE peak voltage is 0.0, this renders the function redundant." << std::endl;
		std::cout << "W: Program will continue but PULSE command is reduntant." << std::endl;
		break;
	//case PULSE_RISE_TIME_ZERO:
	//	std::cout << "E: PULSE rise time cannot be zero." << whatPart << std::endl;
	//	std::cout << "E: " << std::endl;
	//	std::cout << std::endl;
	//case PULSE_FALL_TIME_ZERO:
	//	std::cout << "E: " << whatPart << std::endl;
	//	std::cout << "E: " << std::endl;
	//	std::cout << std::endl;
	case PULSE_WIDTH_ZERO:
		std::cout << "W: PULSE width is 0.0, this renders the function reduntant." << std::endl;
		std::cout << "W: Program will contunue but PULSE command is reduntant." << std::endl;
		break;
	case PULSE_REPEAT:
		std::cout << "W: PULSE repeat rate is 0.0, this is effectively a DC source." << std::endl;
		std::cout << "W: Program will continue, but this is most likely unwanted." << std::endl;
		break;
	}
}
/*
Simulation error function
*/
void simulation_errors(int errorCode, std::string whatPart) {
	switch (errorCode) {
	case JJCAP_NOT_FOUND:
		std::cout << "E: Capacitor value for " << whatPart << " could not be found." << std::endl;
		std::cout << "E: This is a bug and the developer should be contacted. The program will abort." << std::endl;
		exit(0);
	case JJICRIT_NOT_FOUND:
		std::cout << "E: Critical current value for " << whatPart << " could not be found." << std::endl;
		std::cout << "E: This is a bug and the developer should be contacted. The program will abort." << std::endl;
		exit(0);
	case JJPHASE_NODE_NOT_FOUND:
		std::cout << "E: Junction phase node not found for " << whatPart << "." << std::endl;
		std::cout << "E: This is a bug and the developer should be contacted. The program will abort." << std::endl;
		exit(0);
	case INDUCTOR_CURRENT_NOT_FOUND:
		std::cout << "E: Inductor current not defined for " << whatPart << ". Matrix will have no solution." << std::endl;
		std::cout << "E: This is a bug and the developer should be contacted. The program will abort." << std::endl;
		exit(0);
	}
}
/*
Plotting error function
*/
void plotting_errors(int errorCode, std::string whatPart) {
	switch (errorCode) {
	case NO_SUCH_PLOT_TYPE:
		std::cout << "W: Unknown plot type defined. " << whatPart << " is not a valid plot type." << std::endl;
		std::cout << "W: This request to print will be ignored." << std::endl;
		std::cout << std::endl;
		break;
	case NO_SUCH_DEVICE_FOUND:
		std::cout << "W: Device " << whatPart << " was not found in the device stack." << std::endl;
		std::cout << "W: This request to print will be ignored." << std::endl;
		std::cout << std::endl;
		break;
	case CURRENT_THROUGH_VOLTAGE_SOURCE:
		std::cout << "W: Attempting to find the current through " << whatPart << " which is a voltage source." << std::endl;
		std::cout << "W: This request to print will be ignored." << std::endl;
		std::cout << std::endl;
		break;
	}
}