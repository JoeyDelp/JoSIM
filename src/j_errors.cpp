// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#include "JoSIM/j_errors.h"

void
Errors::cli_errors(int errorCode, const std::string& whatPart) {
	switch (errorCode) {
	case NO_OUTPUT:
		std::cerr << "W: No output file name specified. Using default (output.csv)."
			<< std::endl;
		std::cerr << "W: This file will be stored in the current working directory."
			<< std::endl;
		std::cerr << std::endl;
		break;
	case NO_INPUT:
		std::cerr << "E: No input file was specified. Simulator cannot continue."
			<< std::endl;
		std::cerr << "E: Please specify input and try again."
			<< std::endl;
		std::cerr << std::endl;
		exit(-1);
	case UNKNOWN_SWITCH:
		std::cerr << "W: Unknown option " << whatPart << " specified. Please refer to the help menu."
			<< std::endl;
		break;
	case TOO_FEW_ARGUMENTS:
		std::cerr << "E: Missing input arguments" << std::endl;
		std::cerr << "E: Usage: josim [options] input_netlist" << std::endl;
		std::cerr << std::endl;
		std::cerr << "I: For further help use the -h switch" << std::endl;
		exit(-1);
	case INVALID_ANALYSIS:
		std::cerr << "E: Invalid analysis type specified. 0 - Voltage, 1 - Phase."
			<< std::endl;
		std::cerr << "E: Usage: josim [options] input_netlist" << std::endl;
		std::cerr << std::endl;
		std::cerr << "I: For further help use the -h switch" << std::endl;
		exit(-1);
	case NO_ANALYSIS:
		std::cerr << "W: No analysis was specified. Reverting to default (0 - Voltage)."
			<< std::endl;
		std::cerr << "W: Please refer to the help menu (-h) or manual for further information."
			<< std::endl;
		std::cerr << std::endl;
		break;
	case INVALID_CONVENTION:
		std::cerr << "E: Invalid subcircuit convention specified. 0 - JSIM, 1 - WRspice."
			<< std::endl;
		std::cerr << "E: Usage: josim [options] input_netlist" << std::endl;
		std::cerr << std::endl;
		std::cerr << "I: For further help use the -h switch" << std::endl;
		exit(-1);
	case NO_CONVENTION:
		std::cerr << "W: No convention was specified. Reverting to default (0 - JSIM)."
			<< std::endl;
		std::cerr << "W: Please refer to the help menu (-h) or manual for further information."
			<< std::endl;
		std::cerr << std::endl;
		break;
	default:
		std::cerr << "E: Unknown handling error." << std::endl;
		std::cerr << "E: Please contact the developer." << std::endl;
		std::cerr << std::endl;
		exit(-1);
	}
}

void
Errors::input_errors(int errorCode, const std::string& whatPart) {
	switch(errorCode) {
	case CANNOT_OPEN_FILE:
		std::cerr << "E: Input file " << whatPart
			<< " cannot be found or opened." << std::endl;
		std::cerr << "E: Please ensure that the file exists and can be opened."
			<< std::endl;
		std::cerr << std::endl;
		std::cerr << "I: For further help use the -h switch" << std::endl;
		exit(-1);
	case MISSING_SUBCKT_IO:
		std::cerr << "E: Missing subcircuit io." 
			<< std::endl;
		std::cerr << "E: Please recheck the netlist and try again." 
			<<std::endl;
		std::cerr << std::endl;
		exit(-1);
	case MISSING_SUBCKT_NAME:
		std::cerr << "E: Missing subcircuit name." 
			<< std::endl;
		std::cerr << "E: Please recheck the netlist and try again." 
			<<std::endl;
		exit(-1);
	case SUBCKT_CONTROLS:
		std::cerr << "W: Subcircuit " << whatPart << " contains controls." 
			<< std::endl;
		std::cerr << "W: Controls are reserved for the main design." 
			<< std::endl;
		std::cerr << "W: These controls will be ignored." 
			<< std::endl;
		break;
	case MISSING_MAIN:
		std::cerr << "E: Missing main design in netlist." 
			<< std::endl;
		std::cerr << "E: This design will not simulate without a main design." 
			<< std::endl;
		exit(-1);
	case UNKNOWN_SUBCKT:
		std::cerr << "E: The subcircuit named " << whatPart 
			<< " was not found in the netlist." << std::endl;
		std::cerr << "E: Please ensure all subcircuits exist and are correctly named." 
			<< std::endl;
		exit(-1);
	}
}

void
Errors::invalid_component_errors(int errorCode, const std::string& whatPart) {
	switch (errorCode) {
	case RES_ERROR:
		std::cerr << "E: Resistor value error" << std::endl;
		std::cerr << "E: Infringing line: " << whatPart << std::endl;
		exit(-1);
	case CAP_ERROR:
		std::cerr << "E: Capacitor value error" << std::endl;
		std::cerr << "E: Infringing line: " << whatPart << std::endl;
		exit(-1);
	case IND_ERROR:
		std::cerr << "E: Inductor value error" << std::endl;
		std::cerr << "E: Infringing line: " << whatPart << std::endl;
		exit(-1);
	case LABEL_ERROR:
		std::cout << "Invalid component label: " << whatPart << std::endl;
		exit(-1);
	case MISSING_LABEL:
		std::cerr << "E: No component label. This should not happen."
			<< std::endl;
		std::cerr << "E: Infringing line: " << whatPart << std::endl;
		std::cerr << "E: Please contact the developer as this is possibly a bug."
			<< std::endl;
		exit(-1);
	case MISSING_PNODE:
		std::cerr << "E: No positive node. This should not happen." << std::endl;
		std::cerr << "E: Infringing line: " << whatPart << std::endl;
		std::cerr << "E: Please contact the developer as this is possibly a bug."
			<< std::endl;
		exit(-1);
	case MISSING_NNODE:
		std::cerr << "E: No negative node. This should not happen." << std::endl;
		std::cerr << "E: Infringing line: " << whatPart << std::endl;
		std::cerr << "E: Please contact the developer as this is possibly a bug."
			<< std::endl;
		exit(-1);
	case MISSING_JJMODEL:
		std::cerr << "E: No junction model is specified for junction " << whatPart
			<< std::endl;
		exit(-1);
	case MODEL_NOT_DEFINED:
		std::cerr << "W: The specified model " << whatPart << " is not defined"
			<< std::endl;
		std::cerr << "W: Using default model as specified in the manual."
			<< std::endl;
		break;
	case MODEL_AREA_NOT_GIVEN:
		std::cerr << "W: No area specified for junction " << whatPart
			<< std::endl;
		std::cerr << "W: Using default: AREA=1.0" << std::endl;
		break;
	case DUPLICATE_LABEL:
		std::cerr << "E: Duplicate label " << whatPart << " detected."
			<< std::endl;
		std::cout
			<< "E: The program will now terminate. Please recheck the netlist."
			<< std::endl;
		exit(-1);
	case INVALID_SUBCIRCUIT_NODES:
		std::cerr << "E: The nodes for label " << whatPart
			<< " does not match the required nodes of the subcicuit."
			<< std::endl;
		std::cerr << "E: Please recheck the nodes required by the subcircuit and "
			"try again."
			<< std::endl;
		exit(-1);
	case TIME_ERROR:
		std::cerr << "E: Time delay value error" << std::endl;
		std::cerr << "E: Infringing line: " << whatPart << std::endl;
		exit(-1);
	case MISSING_SUBCIRCUIT_NAME:
		std::cerr << "E: The subcircuit for " << whatPart
			<< " does not match any of the subcircuits found in the file."
			<< std::endl;
		std::cerr << "E: Please recheck the subcircuit name and try again."
			<< std::endl;
		exit(-1);
	case MUT_ERROR:
		std::cerr << "E: Missing mutual coupling factor. " << std::endl;
		std::cerr << "E: Infringing line: " << whatPart << std::endl;
		exit(-1);
	case INVALID_EXPR:
		std::cerr << "E: Invalid expression statement found. " << std::endl;
		std::cerr << "E: Infringing line: " << whatPart << std::endl;
		exit(-1);
	case INVALID_TX_DEFINED:
		std::cerr << "E: Invalid definition for transmission line found. "
			<< std::endl;
		std::cerr << "E: Infringing line: " << whatPart << std::endl;
		exit(-1);
	case MISSING_INDUCTOR:
		std::cerr << "E: Invalid mutual coupling defined. Missing inductor "
			<< whatPart << std::endl;
		std::cerr << "E: Please ensure that " << whatPart << " exists."
			<< std::endl;
		exit(-1);
	default:
		std::cerr << "E: Unknown invalid component error." << std::endl;
		std::cerr << "E: Please contact the developer." << std::endl;
		std::cerr << std::endl;
		exit(-1);
	}
}

void
Errors::control_errors(int errorCode, const std::string& whatPart) {
	switch (errorCode) {
	case TRANS_ERROR:
		std::cerr << "W: Invalid transient analysis specified. " << whatPart
			<< std::endl;
		std::cerr << "W: Substituting default parameters. " 
			<< std::endl;
		std::cerr << "W: Defaults: TSTEP=1PS TSTOP=1000PS TSTART=0PS MAXTSTEP=1PS" 
			<< std::endl;
		std::cerr << std::endl;
		break;
	case PRINT_TOO_MANY_ARGS:
		std::cerr << "W: Print request for device current has too many arguments."
			<< std::endl;
		std::cerr << "W: Line: " << whatPart 
			<< std::endl;
		std::cerr << "W: Ignoring the extra argument." 
			<< std::endl;
		std::cerr << std::endl;
		break;
	case PRINT_ERROR:
		exit(-1);
	case PLOT_ERROR:
		exit(-1);
	case INV_CONTROL:
		exit(-1);
	case DC_ERROR:
		exit(-1);
	case AC_ERROR:
		exit(-1);
	case PHASE_ERROR:
		exit(-1);
	case NO_SIM:
		std::cerr << "E: No simulation type specified. Nothing will be simulated."
			<< std::endl;
		exit(-1);
	case UNKNOWN_DEVICE:
		std::cerr << "W: Unknown device " << whatPart 
			<< std::endl;
		std::cerr << "W: Cannot print current for this device." 
			<< std::endl;
		std::cerr << "W: Ignoring this print request." 
			<< std::endl;
		std::cerr << std::endl;
		break;
	case CURRENT_THROUGH_VOLT:
		std::cerr << "W: Requesting current through a voltage source." 
			<< std::endl;
		std::cerr << "W: Line: " << whatPart
			<< std::endl;
		std::cerr << "W: This is invalid and the request will be ignored." 
			<< std::endl;
		std::cerr << std::endl;
		break;
	case VOLT_WHEN_PHASE:
		std::cerr << "W: Request to print voltage for device " << whatPart 
			<< std::endl;
		std::cerr << "W: Phase mode simulation performed." 
			<< std::endl;
		std::cerr << "W: Printing device phase instead." 
			<< std::endl;
		std::cerr << std::endl;
		break;
	case VOLT_ACROSS_CURRENT:
		std::cerr << "W: Requesting voltage across a current source." 
			<< std::endl;
		std::cerr << "W: Line: " << whatPart 
			<< std::endl;
		std::cerr << "W: This is invalid and the request will be ignored." 
			<< std::endl;
		std::cerr << std::endl;
		break;	
	case NODEVOLT_WHEN_PHASE:
		std::cerr << "W: Request to print nodal voltage for " << whatPart 
			<< std::endl;
		std::cerr << "W: Phase mode simulation performed." 
			<< std::endl;
		std::cerr << "W: Printing nodal phase instead." 
			<< std::endl;
		std::cerr << std::endl;
		break;
	case UNKNOWN_NODE:
		std::cerr << "W: Node " << whatPart << " was not found in the circuit." 
			<< std::endl;
		std::cerr << "W: This request for print will be ignored." 
			<< std::endl;
		std::cerr << std::endl;
		break;
	case NODEPHASE_WHEN_VOLT:
		std::cerr << "W: Request to print nodal phase for " << whatPart 
			<< std::endl;
		std::cerr << "W: Voltage mode simulation performed." 
			<< std::endl;
		std::cerr << "W: Printing nodal voltage instead." 
			<< std::endl;
		std::cerr << std::endl;
    break;
	case INVALID_NODEV:
		std::cerr << "W: Invalid node voltage request found." 
			<< std::endl;
		std::cerr << "W: Line: " << whatPart
			<< std::endl;
		std::cerr << "W: This request for print will be ignored." 
			<< std::endl;
		std::cerr << std::endl;
		break;
	case INVALID_NODEP:
		std::cerr << "W: Invalid node phase request found." 
			<< std::endl;
		std::cerr << "W: Line: " << whatPart
			<< std::endl;
		std::cerr << "W: This request for print will be ignored." 
			<< std::endl;
		std::cerr << std::endl;
		break;
	case PHASE_WHEN_VOLT:
		std::cerr << "W: Requesting phase in a voltage simulation." 
			<< std::endl;
		std::cerr << "W: Line: " << whatPart 
			<< std::endl;
		std::cerr << "W: This request will be ignored." 
			<< std::endl;
		std::cerr << std::endl;
		break;
	case PHASE_OF_VOLT:
		std::cerr << "W: Requesting phase of a voltage source." 
			<< std::endl;
		std::cerr << "W: Line: " << whatPart 
			<< std::endl;
		std::cerr << "W: This is invalid and the request will be ignored." 
			<< std::endl;
		std::cerr << std::endl;
		break;
	case PHASE_OF_CURRENT:
		std::cerr << "W: Requesting phase of a current source." 
			<< std::endl;
		std::cerr << "W: Line: " << whatPart 
			<< std::endl;
		std::cerr << "W: This is invalid and the request will be ignored." 
			<< std::endl;
		std::cerr << std::endl;
		break;
	case INVALID_CURRENT:
		std::cerr << "W: Invalid request to plot current." 
			<< std::endl;
		std::cerr << "W: Infringing line: " << whatPart 
			<< std::endl;
		std::cerr << std::endl;
    break;
	case MATHOPS:
		std::cerr << "W: Mathematical operations on output vectors are not yet supported." 
			<< std::endl;
		std::cerr << "W: Ignoring plotting of " << whatPart 
			<< std::endl;
		std::cerr << std::endl;
		break;
	case UNKNOWN_PLOT:
		std::cerr << "W: Unknown plot type " << whatPart 
			<< std::endl;
		std::cerr << "W: Ignoring request to plot." 
			<< std::endl;
		std::cerr << std::endl;
		break;
	default:
		std::cerr << "E: Unknown control error: " << whatPart << std::endl;
		std::cerr << "E: Please contact the developer." << std::endl;
		std::cerr << std::endl;
		exit(-1);
	}
}

[[noreturn]] void
Errors::model_errors(int errorCode, const std::string& whatPart) {
	switch (errorCode) {
	case PARAM_TYPE_ERROR:
		std::cerr << "E: Unknown model parameter " << whatPart << " specified."
			<< std::endl;
		exit(-1);
	case UNKNOWN_MODEL_TYPE:
		std::cerr << "E: Unknown model type " << whatPart << " specified." << std::endl;
		exit(-1);
	case BAD_MODEL_DEFINITION:
		std::cerr << "E: Bad model definition found." << std::endl;
		std::cerr << "E: Infringing line: " << whatPart << std::endl;
		exit(-1);
	default:
		std::cerr << "E: Unknown model error: " << whatPart << std::endl;
		std::cerr << "E: Please contact the developer." << std::endl;
		std::cerr << std::endl;
		exit(-1);
	}
}

void
Errors::matrix_errors(int errorCode, const std::string& whatPart) {
	switch (errorCode) {
	case NON_SQUARE:
		std::cerr << "E: Matrix is not square. Dimensions are " << whatPart
			<< std::endl;
		std::cout
			<< "E: Please contact the developer as this is potentially a bug."
			<< std::endl;
		exit(-1);
	default:
		std::cerr << "E: Unknown matrix error: " << whatPart << std::endl;
		std::cerr << "E: Please contact the developer." << std::endl;
		std::cerr << std::endl;
	}
}

[[noreturn]] void
Errors::misc_errors(int errorCode, const std::string& whatPart) {
	switch (errorCode) {
	case STOD_ERROR:
		std::cerr << "E: Cannot convert string to double: " << whatPart
			<< std::endl;
		exit(-1);
	default:
		std::cerr << "E: Unknown misc error: " << whatPart << std::endl;
		std::cerr << "E: Please contact the developer." << std::endl;
		std::cerr << std::endl;
		exit(-1);
	}
}

void
Errors::function_errors(int errorCode, const std::string& whatPart) {
	switch (errorCode) {
	case INITIAL_VALUES:
		std::cerr << "E: Invalid PWL definition found. The value of " << whatPart
			<< " is expected to be 0" << std::endl;
		std::cerr << "E: Please refer to the PWL definition: PWL(0 0 T1 V1 T2 V2 "
			"... Tn Vn)"
			<< std::endl;
		exit(-1);
	case TOO_FEW_TIMESTEPS:
		std::cout
			<< "E: Total timesteps specified do not match the values specified."
			<< whatPart << " specified." << std::endl;
		std::cerr << "E: Please refer to the PWL definition: PWL(0 0 T1 V1 T2 V2 "
			"... Tn Vn)"
			<< std::endl;
		exit(-1);
	case TOO_FEW_VALUES:
		std::cout
			<< "E: Total values specified do not match the timesteps specified."
			<< whatPart << " specified." << std::endl;
		std::cerr << "E: Please refer to the PWL definition: PWL(0 0 T1 V1 T2 V2 "
			"... Tn Vn)"
			<< std::endl;
		exit(-1);
	case INITIAL_PULSE_VALUE:
		std::cerr << "E: Invalid PULSE definition found. The value of "
			<< whatPart << " is expected to be 0" << std::endl;
		std::cerr << "E: Please refer to the PULSE definition: PULSE(0 V2 TD TR "
			"TF PW PER)"
			<< std::endl;
		exit(-1);
	case PULSE_TOO_FEW_ARGUMENTS:
		std::cout
			<< "E: Total arguments specified do not match the required for PULSE. "
			<< whatPart << " specified." << std::endl;
		std::cerr << "E: Please refer to the PULSE definition: PULSE(0 V2 TD TR "
			"TF PW PER)"
			<< std::endl;
		exit(-1);
	case PULSE_VPEAK_ZERO:
		std::cout
			<< "W: PULSE peak voltage is 0.0, this renders the function redundant."
			<< std::endl;
		std::cerr << "W: Program will continue but PULSE command is redundant."
			<< std::endl;
		break;
	case PULSE_WIDTH_ZERO:
		std::cerr << "W: PULSE width is 0.0, this renders the function redundant."
			<< std::endl;
		std::cerr << "W: Program will continue but PULSE command is redundant."
			<< std::endl;
		break;
	case PULSE_REPEAT:
		std::cout
			<< "W: PULSE repeat rate is 0.0, this is effectively a DC source."
			<< std::endl;
		std::cerr << "W: Program will continue, but this is most likely unwanted."
			<< std::endl;
		break;
	case SIN_TOO_FEW_ARGUMENTS:
		std::cout
			<< "E: Total arguments specified do not match the required for SIN. "
			<< whatPart << " specified." << std::endl;
		std::cerr << "E: Please refer to the SIN definition: SIN(VO VA FREQ TD THETA)"
			<< std::endl;
		exit(-1);
	case SIN_TOO_MANY_ARGUMENTS:
		std::cout
			<< "E: Total arguments specified do not match the required for SIN. "
			<< whatPart << " specified." << std::endl;
		std::cerr << "E: Please refer to the SIN definition: SIN(VO VA FREQ TD THETA)"
			<< std::endl;
		exit(-1);
	case SIN_VA_ZERO:
		std::cout
			<< "W: SIN amplitude is 0.0, this renders the function redundant."
			<< std::endl;
		std::cerr << "W: Program will continue but SIN command is redundant."
			<< std::endl;
		break;
	case CUS_TOO_FEW_ARGUMENTS:
		std::cout
			<< "E: Total arguments specified do not match the required for CUS. "
			<< whatPart << " specified." << std::endl;
		std::cerr << "E: Please refer to the CUS definition: CUS(WaveFile.dat TS SF IM <TD PER>)"
			<< std::endl;
		break;
	case CUS_TOO_MANY_ARGUMENTS:
		std::cout
			<< "E: Total arguments specified do not match the required for CUS. "
			<< whatPart << " specified." << std::endl;
		std::cerr << "E: Please refer to the CUS definition: CUS(WaveFile.dat TS SF IM <TD PER>)"
			<< std::endl;
		break;
	case CUS_SF_ZERO:
		std::cout
			<< "W: CUS scale factor is 0.0, this renders the function redundant."
			<< std::endl;
		std::cerr << "W: Program will continue but SIN command is redundant."
			<< std::endl;
		break;
	case CUS_WF_NOT_FOUND:
		std::cout
			<< "W: CUS waveform file was not found."
			<< whatPart << " specified." << std::endl;
		std::cerr << "W: Program will terminate."
			<< std::endl;
		exit(-1);
	default:
		std::cerr << "E: Unknown function error: " << whatPart << std::endl;
		std::cerr << "E: Please contact the developer." << std::endl;
		std::cerr << std::endl;
		exit(-1);
	}
}

[[noreturn]] void
Errors::simulation_errors(int errorCode, const std::string& whatPart) {
	switch (errorCode) {
	case JJCAP_NOT_FOUND:
		std::cerr << "E: Capacitor value for " << whatPart
			<< " could not be found." << std::endl;
		std::cerr << "E: This is a bug and the developer should be contacted. "
			"The program will abort."
			<< std::endl;
		exit(-1);
	case JJICRIT_NOT_FOUND:
		std::cerr << "E: Critical current value for " << whatPart
			<< " could not be found." << std::endl;
		std::cerr << "E: This is a bug and the developer should be contacted. "
			"The program will abort."
			<< std::endl;
		exit(-1);
	case JJPHASE_NODE_NOT_FOUND:
		std::cerr << "E: Junction phase node not found for " << whatPart << "."
			<< std::endl;
		std::cerr << "E: This is a bug and the developer should be contacted. "
			"The program will abort."
			<< std::endl;
		exit(-1);
	case INDUCTOR_CURRENT_NOT_FOUND:
		std::cerr << "E: Inductor current not defined for " << whatPart
			<< ". Matrix will have no solution." << std::endl;
		std::cerr << "E: This is a bug and the developer should be contacted. "
			"The program will abort."
			<< std::endl;
		exit(-1);
	case MATRIX_SINGULAR:
		std::cerr << "E: Matrix is singular. Matrix will have no solution." << std::endl;
		std::cerr << "E: Please check the components in the netlist. "
			"The program will abort."
			<< std::endl;
		exit(-1);
	default:
		std::cerr << "E: Unknown simulation error: " << whatPart << std::endl;
		std::cerr << "E: Please contact the developer." << std::endl;
		std::cerr << std::endl;
		exit(-1);
	}
}

void
Errors::parsing_errors(int errorCode, const std::string& whatPart) {
	switch (errorCode) {
	case EXPRESSION_ARLEADY_DEFINED:
		std::cerr << "W: Expression for " << whatPart
			<< " has already been defined. Overwriting previous expression."
			<< std::endl;
		std::cerr << "W: If this was unintentional, please revise netlist."
			<< std::endl;
		std::cerr << std::endl;
		break;
	case UNIDENTIFIED_PART:
		std::cerr << "W: Unknown function/variable defined. What is " << whatPart
			<< "?" << std::endl;
		std::cerr << "W: Please ensure variables are declared before being used."
			<< std::endl;
		std::cerr << std::endl;
		break;
	case MISMATCHED_PARENTHESIS:
		std::cerr << "E: Mismatched parenthesis in expression: " << whatPart
			<< std::endl;
		std::cerr << "E: Please correct the expression before trying again."
			<< std::endl;
		std::cerr << std::endl;
		exit(-1);
	case INVALID_RPN:
		std::cerr << "E: Invalid RPN detected. This might be an algorithm fault "
			"or an incorrect expression parse."
			<< std::endl;
		std::cerr << "E: The expression in question: " << whatPart << std::endl;
		std::cerr << std::endl;
		exit(-1);
	default:
		std::cerr << "E: Unknown parsing error: " << whatPart << std::endl;
		std::cerr << "E: Please contact the developer." << std::endl;
		std::cerr << std::endl;
		exit(-1);
	}
}
