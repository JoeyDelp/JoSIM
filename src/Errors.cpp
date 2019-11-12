// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/Errors.hpp"

#include <iostream>

void Errors::cli_errors(int errorCode, const std::string &whatPart) {
  std::string formattedMessage = "Command Line Interface\n";
  switch (errorCode) {
  case static_cast<int>(CLIErrors::NO_OUTPUT):
    formattedMessage += "No output file name specified. Using default (output.csv).\n";
    formattedMessage += "This file will be stored in the current working directory.";
    warning_message(formattedMessage);
    break;
  case static_cast<int>(static_cast<int>(CLIErrors::NO_INPUT)):
    formattedMessage += "No input file was specified. Simulator cannot continue.\n";
    formattedMessage += "Please specify input and try again.";
    throw formattedMessage;
  case static_cast<int>(CLIErrors::UNKNOWN_SWITCH):
    formattedMessage += "Unknown option " + whatPart + " specified. Please refer to the help menu.";
    warning_message(formattedMessage);
    break;
  case static_cast<int>(CLIErrors::TOO_FEW_ARGUMENTS):
    formattedMessage += "Missing input arguments\n";
    formattedMessage += "Usage: josim [options] input_netlist\n\n";
    formattedMessage += "For further help use the -h switch";
    throw formattedMessage;
  case static_cast<int>(CLIErrors::INVALID_ANALYSIS):
    formattedMessage += "Invalid analysis type specified. 0 - Voltage, 1 - Phase.\n";
    formattedMessage += "Usage: josim [options] input_netlist\n\n";
    formattedMessage += "For further help use the -h switch";
    throw formattedMessage;
  case static_cast<int>(CLIErrors::NO_ANALYSIS):
    formattedMessage += "No analysis was specified. Reverting to default (0 - Voltage).\n";
    formattedMessage += "Please refer to the help menu (-h) or manual for further information.";
    warning_message(formattedMessage);
    break;
  case static_cast<int>(CLIErrors::INVALID_CONVENTION):
    formattedMessage += "Invalid subcircuit convention specified. 0 - JSIM, 1 - WRspice.\n";
    formattedMessage += "Usage: josim [options] input_netlist\n\n";
    formattedMessage += "For further help use the -h switch";
    throw formattedMessage;
  case static_cast<int>(CLIErrors::NO_CONVENTION):
    formattedMessage += "No convention was specified. Reverting to default (0 - JSIM).\n";
    formattedMessage += "Please refer to the help menu (-h) or manual for further information.";
    warning_message(formattedMessage);
    break;
  default:
    formattedMessage += "Unknown handling error.\n";
    formattedMessage += "Please contact the developer.";
    throw formattedMessage;
  }
}

void Errors::input_errors(int errorCode, const std::string &whatPart) {
  std::string formattedMessage = "Input\n";
  switch (errorCode) {
  case static_cast<int>(InputErrors::CANNOT_OPEN_FILE):
    formattedMessage += "Input file " + whatPart + " cannot be found or opened.\n";
    formattedMessage += "Please ensure that the file exists and can be opened.\n\n";
    formattedMessage += "For further help use the -h switch";
    throw formattedMessage;
  case static_cast<int>(InputErrors::MISSING_SUBCKT_IO):
    formattedMessage += "Missing subcircuit io.\n";
    formattedMessage += "Please recheck the netlist and try again.";
    throw formattedMessage;
  case static_cast<int>(InputErrors::MISSING_SUBCKT_NAME):
    formattedMessage += "Missing subcircuit name.\n";
    formattedMessage += "Please recheck the netlist and try again.";
    throw formattedMessage;
  case static_cast<int>(InputErrors::SUBCKT_CONTROLS):
    formattedMessage += "Subcircuit " + whatPart + " contains controls.\n";
    formattedMessage += "Controls are reserved for the main design.\n";
    formattedMessage += "These controls will be ignored.";
    warning_message(formattedMessage);
    break;
  case static_cast<int>(InputErrors::MISSING_MAIN):
    formattedMessage += "Missing main design in netlist.\n";
    formattedMessage += "This design will not simulate without a main design.";
    throw formattedMessage;
  case static_cast<int>(InputErrors::UNKNOWN_SUBCKT):
    formattedMessage += "The subcircuit named " + whatPart + " was not found in the netlist.\n";
    formattedMessage += "Please ensure all subcircuits exist and are correctly named.";
    throw formattedMessage;
  case static_cast<int>(InputErrors::EMPTY_FILE):
    formattedMessage += "The file \"" + whatPart + "\" contains no readable lines.\n";
    formattedMessage += "Please check the input file and ensure that the file is not empty.";
    throw formattedMessage;
  }
}

void Errors::invalid_component_errors(int errorCode,
                                      const std::string &whatPart) {
  std::string formattedMessage = "Components\n";
  switch (errorCode) {
  case static_cast<int>(ComponentErrors::INVALID_COMPONENT_DECLARATION):
    formattedMessage += "Invalid component declaration detected.\n";
    formattedMessage += "Infringing line: " + whatPart + "\n";
    formattedMessage += "Please refer to the documentation for the correct notation.";
    throw formattedMessage;
  case static_cast<int>(ComponentErrors::BOTH_GROUND):
    formattedMessage += "Both nodes are grounded for the following component.\n";
    formattedMessage += "Component: " + whatPart;
    warning_message(formattedMessage);
    break;
  case static_cast<int>(ComponentErrors::GROUNDED_VOLTAGE_SOURCE):
    formattedMessage += "Both nodes are grounded for the following source.\n";
    formattedMessage += "Component: " + whatPart;
    throw formattedMessage;
  case static_cast<int>(ComponentErrors::RES_ERROR):
    formattedMessage += "Resistor value error\n";
    formattedMessage += "Infringing line: " + whatPart;
    throw formattedMessage;
  case static_cast<int>(ComponentErrors::CAP_ERROR):
    formattedMessage += "Capacitor value error\n";
    formattedMessage += "Infringing line: " + whatPart;
    throw formattedMessage;
  case static_cast<int>(ComponentErrors::IND_ERROR):
    formattedMessage += "Inductor value error\n";
    formattedMessage += "Infringing line: " + whatPart;
    throw formattedMessage;
  case static_cast<int>(ComponentErrors::LABEL_ERROR):
    formattedMessage += "Invalid component label: " + whatPart;
    throw formattedMessage;
  case static_cast<int>(ComponentErrors::MISSING_LABEL):
    formattedMessage += "No component label. This should not happen.\n";
    formattedMessage += "Infringing line: " + whatPart + "\n";
    formattedMessage += "Please contact the developer as this is possibly a bug.";
    throw formattedMessage;
  case static_cast<int>(ComponentErrors::MISSING_PNODE):
    formattedMessage += "No positive node. This should not happen.\n";
    formattedMessage += "Infringing line: " + whatPart + "\n";
    formattedMessage += "Please contact the developer as this is possibly a bug.";
    throw formattedMessage;
  case static_cast<int>(ComponentErrors::MISSING_NNODE):
    formattedMessage += "No negative node. This should not happen.\n";
    formattedMessage += "Infringing line: " + whatPart + "\n";
    formattedMessage += "Please contact the developer as this is possibly a bug.";
    throw formattedMessage;
  case static_cast<int>(ComponentErrors::MISSING_JJMODEL):
    formattedMessage += "No junction model is specified for junction " + whatPart;
    throw formattedMessage;
  case static_cast<int>(ComponentErrors::MODEL_NOT_DEFINED):
    formattedMessage += "The specified model is not defined.\n";
    formattedMessage += "Missing model: " + whatPart + "\n";
    formattedMessage += "Using default model as specified in the manual.";
    warning_message(formattedMessage);
    break;
  case static_cast<int>(ComponentErrors::MODEL_AREA_NOT_GIVEN):
    formattedMessage += "No area specified for junction " + whatPart + "\n";
    formattedMessage += "Using default: AREA=1.0";
    warning_message(formattedMessage);
    break;
  case static_cast<int>(ComponentErrors::DUPLICATE_LABEL):
    formattedMessage += "Duplicate label " + whatPart + " detected.\n";
    formattedMessage += "The program will now terminate. Please recheck the netlist.";
    throw formattedMessage;
  case static_cast<int>(ComponentErrors::INVALID_SUBCIRCUIT_NODES):
    formattedMessage += "The nodes for label " + whatPart + " does not match the required nodes of the subcicuit.\n";
    formattedMessage += "Please recheck the nodes required by the subcircuit and try again.";
    throw formattedMessage;
  case static_cast<int>(ComponentErrors::TIME_ERROR):
    formattedMessage += "Time delay value error\n";
    formattedMessage += "Infringing line: " + whatPart;
    throw formattedMessage;
  case static_cast<int>(ComponentErrors::MISSING_SUBCIRCUIT_NAME):
    formattedMessage += "The subcircuit for " + whatPart + " does not match any of the subcircuits found in the file.\n";
    formattedMessage += "Please recheck the subcircuit name and try again.";
    throw formattedMessage;
  case static_cast<int>(ComponentErrors::MUT_ERROR):
    formattedMessage += "Invalid mutual coupling definition found.\n";
    formattedMessage += "Infringing line: " + whatPart;
    throw formattedMessage;
  case static_cast<int>(ComponentErrors::INVALID_EXPR):
    formattedMessage += "Invalid expression statement found.\n";
    formattedMessage += "Infringing line: " + whatPart;
    throw formattedMessage;
  case static_cast<int>(ComponentErrors::INVALID_TX_DEFINED):
    formattedMessage += "Invalid definition for transmission line found.\n";
    formattedMessage += "Infringing line: " + whatPart;
    throw formattedMessage;
  case static_cast<int>(ComponentErrors::MISSING_INDUCTOR):
    formattedMessage += "Invalid mutual coupling defined. Missing inductor " + whatPart + "\n";
    formattedMessage += "Please ensure that " + whatPart + " exists.";
    throw formattedMessage;
  case static_cast<int>(ComponentErrors::UNKNOWN_DEVICE_TYPE):
    formattedMessage += "Unkown device type " + whatPart + "\n";
    formattedMessage += "Please refer to the syntax guide for a list of available device types.";
    throw formattedMessage;
  case static_cast<int>(ComponentErrors::SPECIAL_CHARS):
    formattedMessage += "Label " + whatPart + " contains special characters.\n";
    formattedMessage += "The use of special characters in label names is not advised.\n";
    formattedMessage += "This might produce unexpected results.\n";
    formattedMessage += "Continuing operation.";
    warning_message(formattedMessage);
    break;
  default:
    formattedMessage += "Unknown invalid component error.\n";
    formattedMessage += "Please contact the developer.";
    throw formattedMessage;
  }
}

void Errors::control_errors(int errorCode, const std::string &whatPart) {
  std::string formattedMessage = "Controls\n";
  switch (errorCode) {
  case static_cast<int>(ControlErrors::TRANS_ERROR):
    formattedMessage += "Invalid transient analysis specified. " + whatPart + "\n";
    formattedMessage += "Substituting default parameters.\n";
    formattedMessage += "Defaults: TSTEP=1PS TSTOP=1000PS TSTART=0PS MAXTSTEP=1PS";
    warning_message(formattedMessage);
    break;
  case static_cast<int>(ControlErrors::PRINT_TOO_MANY_ARGS):
    formattedMessage += "Print request for device current has too many arguments.\n";
    formattedMessage += "Line: " + whatPart + "\n";
    formattedMessage += "Ignoring the extra argument.";
    warning_message(formattedMessage);
    break;
  case static_cast<int>(ControlErrors::PRINT_ERROR):
    throw formattedMessage;
  case static_cast<int>(ControlErrors::PLOT_ERROR):
    throw formattedMessage;
  case static_cast<int>(ControlErrors::INV_CONTROL):
    throw formattedMessage;
  case static_cast<int>(ControlErrors::DC_ERROR):
    throw formattedMessage;
  case static_cast<int>(ControlErrors::AC_ERROR):
    throw formattedMessage;
  case static_cast<int>(ControlErrors::PHASE_ERROR):
    throw formattedMessage;
  case static_cast<int>(ControlErrors::NO_SIM):
    formattedMessage += "No simulation type specified. Nothing will be simulated.";
    throw formattedMessage;
  case static_cast<int>(ControlErrors::UNKNOWN_DEVICE):
    formattedMessage += "Unknown device " + whatPart + "\n";
    formattedMessage += "Cannot store results for this device.\n";
    formattedMessage += "Ignoring this store request.";
    warning_message(formattedMessage);
    break;
  case static_cast<int>(ControlErrors::CURRENT_THROUGH_VOLT):
    formattedMessage += "Requesting current through a voltage source.\n";
    formattedMessage += "Line: " + whatPart + "\n";
    formattedMessage += "This is invalid and the request will be ignored.";
    warning_message(formattedMessage);
    break;
  case static_cast<int>(ControlErrors::VOLT_WHEN_PHASE):
    formattedMessage += "Request to store voltage for device " + whatPart + "\n";
    formattedMessage += "Phase mode simulation performed.\n";
    formattedMessage += "Storing device phase instead.";
    warning_message(formattedMessage);
    break;
  case static_cast<int>(ControlErrors::VOLT_ACROSS_CURRENT):
    formattedMessage += "Requesting voltage across a current source.\n";
    formattedMessage += "Line: " + whatPart + "\n";
    formattedMessage += "This is invalid and the request will be ignored.";
    warning_message(formattedMessage);
    break;
  case static_cast<int>(ControlErrors::NODEVOLT_WHEN_PHASE):
    formattedMessage += "Request to store nodal voltage for " + whatPart + "\n";
    formattedMessage += "Phase mode simulation performed.\n";
    formattedMessage += "Storing nodal phase instead.";
    warning_message(formattedMessage);
    break;
  case static_cast<int>(ControlErrors::NODECURRENT):
    formattedMessage += "Request to store nodal current for " + whatPart + "\n";
    formattedMessage += "Cannot store current of a node.";
    warning_message(formattedMessage);
    break;
  case static_cast<int>(ControlErrors::UNKNOWN_NODE):
    formattedMessage += "Node " + whatPart + " was not found in the circuit.\n";
    formattedMessage += "This request for store will be ignored.";
    warning_message(formattedMessage);
    break;
  case static_cast<int>(ControlErrors::NODEPHASE_WHEN_VOLT):
    formattedMessage += "Request to store nodal phase for " + whatPart + "\n";
    formattedMessage += "Voltage mode simulation performed.\n";
    formattedMessage += "Storing nodal voltage instead.";
    warning_message(formattedMessage);
    break;
  case static_cast<int>(ControlErrors::INVALID_NODEV):
    formattedMessage += "Invalid node voltage request found.\n";
    formattedMessage += "Line: " + whatPart + "\n";
    formattedMessage += "This request for store will be ignored.";
    warning_message(formattedMessage);
    break;
  case static_cast<int>(ControlErrors::INVALID_NODEP):
    formattedMessage += "Invalid node phase request found.\n";
    formattedMessage += "Line: " + whatPart + "\n";
    formattedMessage += "This request for store will be ignored.";
    warning_message(formattedMessage);
    break;
  case static_cast<int>(ControlErrors::PHASE_WHEN_VOLT):
    formattedMessage += "Requesting phase in a voltage simulation.\n";
    formattedMessage += "Line: " + whatPart + "\n";
    formattedMessage += "This request will be ignored.";
    warning_message(formattedMessage);
    break;
  case static_cast<int>(ControlErrors::PHASE_OF_VOLT):
    formattedMessage += "Requesting phase of a voltage source.\n";
    formattedMessage += "Line: " + whatPart + "\n";
    formattedMessage += "This is invalid and the request will be ignored.";
    warning_message(formattedMessage);
    break;
  case static_cast<int>(ControlErrors::PHASE_OF_CURRENT):
    formattedMessage += "Requesting phase of a current source.\n";
    formattedMessage += "Line: " + whatPart + "\n";
    formattedMessage += "This is invalid and the request will be ignored.";
    warning_message(formattedMessage);
    break;
  case static_cast<int>(ControlErrors::INVALID_CURRENT):
    formattedMessage += "Invalid request to plot current.\n";
    formattedMessage += "Infringing line: " + whatPart;
    warning_message(formattedMessage);
    break;
  case static_cast<int>(ControlErrors::MATHOPS):
    formattedMessage += "Mathematical operations on output vectors are not yet supported.\n";
    formattedMessage += "Ignoring plotting of " + whatPart;
    warning_message(formattedMessage);
    break;
  case static_cast<int>(ControlErrors::UNKNOWN_PLOT):
    formattedMessage += "Unknown plot type " + whatPart + "\n";
    formattedMessage += "Ignoring request to plot.";
    warning_message(formattedMessage);
    break;
  case static_cast<int>(ControlErrors::INVALID_OUTPUT_COMMAND):
    formattedMessage += "Invalid request for output found.\n";
    formattedMessage += "Line:" + whatPart + "\n";
    formattedMessage += "Ignoring request and continuing.";
    warning_message(formattedMessage);
    break;
  default:
    formattedMessage += "Unknown control error: " + whatPart + "\n";
    formattedMessage += "Please contact the developer.";
    throw formattedMessage;
  }
}

[[noreturn]] void Errors::model_errors(int errorCode,
                                       const std::string &whatPart) {
  std::string formattedMessage = "Model\n";                                         
  switch (errorCode) {
  case static_cast<int>(ModelErrors::PARAM_TYPE_ERROR):
    formattedMessage += "Unknown model parameter specified.\n";
    formattedMessage += "Model line: " + whatPart + "\n";
    formattedMessage += "Please refer to the model definition in the documentation";
    throw formattedMessage;
  case static_cast<int>(ModelErrors::UNKNOWN_MODEL_TYPE):
    formattedMessage += "Unknown model type specified.\n";
    formattedMessage += "Model line: " + whatPart;
    throw formattedMessage;
  case static_cast<int>(ModelErrors::BAD_MODEL_DEFINITION):
    formattedMessage += "Bad model definition found.\n";
    formattedMessage += "Infringing line: " + whatPart;
    throw formattedMessage;
  default:
    formattedMessage += "Unknown model error: " + whatPart + "\n";
    formattedMessage += "Please contact the developer.";
    throw formattedMessage;
  }
}

void Errors::matrix_errors(int errorCode, const std::string &whatPart) {
  std::string formattedMessage = "Matrix\n";                                         
  switch (errorCode) {
  case static_cast<int>(MatrixErrors::NON_SQUARE):
    formattedMessage += "Matrix is not square. Dimensions are " + whatPart + "\n";
    formattedMessage += "Please contact the developer as this is potentially a bug.";
    throw formattedMessage;
  default:
    formattedMessage += "Unknown matrix error: " + whatPart + "\n";
    formattedMessage += "Please contact the developer.";
    throw formattedMessage;
  }
}

[[noreturn]] void Errors::misc_errors(int errorCode,
                                      const std::string &whatPart) {
  std::string formattedMessage = "Miscellaneous\n";                                                                                 
  switch (errorCode) {
  case static_cast<int>(MiscErrors::STOD_ERROR):
    formattedMessage += "Cannot convert string to double: " + whatPart;
    throw formattedMessage;
  default:
    formattedMessage += "Unknown misc error: " + whatPart + "\n";
    formattedMessage += "Please contact the developer.";
    throw formattedMessage;
  }
}

void Errors::function_errors(int errorCode, const std::string &whatPart) {
  std::string formattedMessage = "Function\n";                                         
  switch (errorCode) {
  case static_cast<int>(FunctionErrors::INITIAL_VALUES):
    formattedMessage += "Invalid PWL definition found. The value of " + whatPart + " is expected to be 0\n";
    formattedMessage += "Please refer to the PWL definition: PWL(0 0 T1 V1 T2 V2 ... Tn Vn)";
    warning_message(formattedMessage);
    break;
  case static_cast<int>(FunctionErrors::TOO_FEW_TIMESTEPS):
    formattedMessage += "Total timesteps specified do not match the values specified." + whatPart + " specified.\n";
    formattedMessage += "Please refer to the PWL definition: PWL(0 0 T1 V1 T2 V2 ... Tn Vn)";
    throw formattedMessage;
  case static_cast<int>(FunctionErrors::TOO_FEW_VALUES):
    formattedMessage += "Total values specified do not match the timesteps specified." + whatPart + " specified.\n";
    formattedMessage += "Please refer to the PWL definition: PWL(0 0 T1 V1 T2 V2 ... Tn Vn)";
    throw formattedMessage;
  case static_cast<int>(FunctionErrors::INITIAL_PULSE_VALUE):
    formattedMessage += "Invalid PULSE definition found. The value of " + whatPart + " is expected to be 0\n";
    formattedMessage += "Please refer to the PULSE definition: PULSE(0 V2 TD TR TF PW PER)";
    throw formattedMessage;
  case static_cast<int>(FunctionErrors::PULSE_TOO_FEW_ARGUMENTS):
    formattedMessage += "Total arguments specified do not match the required for PULSE. " + whatPart + " specified.\n";
    formattedMessage += "Please refer to the PULSE definition: PULSE(0 V2 TD TR TF PW PER)";
    throw formattedMessage;
  case static_cast<int>(FunctionErrors::PULSE_VPEAK_ZERO):
    formattedMessage += "PULSE peak voltage is 0.0, this renders the function redundant.\n";
    formattedMessage += "Program will continue but PULSE command is redundant.";
    warning_message(formattedMessage);
    break;
  case static_cast<int>(FunctionErrors::PULSE_WIDTH_ZERO):
    formattedMessage += "PULSE width is 0.0, this renders the function redundant.\n";
    formattedMessage += "Program will continue but PULSE command is redundant.";
    warning_message(formattedMessage);
    break;
  case static_cast<int>(FunctionErrors::PULSE_REPEAT):
    formattedMessage += "PULSE repeat rate is 0.0, this is effectively a DC source.\n";
    formattedMessage += "Program will continue, but this is most likely unwanted.";
    warning_message(formattedMessage);
    break;
  case static_cast<int>(FunctionErrors::SIN_TOO_FEW_ARGUMENTS):
    formattedMessage += "Total arguments specified do not match the required for SIN. " + whatPart + " specified.\n";
    formattedMessage += "Please refer to the SIN definition: SIN(VO VA FREQ TD THETA)";
    throw formattedMessage;
  case static_cast<int>(FunctionErrors::SIN_TOO_MANY_ARGUMENTS):
    formattedMessage += "Total arguments specified do not match the required for SIN. " + whatPart + " specified.\n";
    formattedMessage += "Please refer to the SIN definition: SIN(VO VA FREQ TD THETA)";
    throw formattedMessage;
  case static_cast<int>(FunctionErrors::SIN_VA_ZERO):
    formattedMessage += "SIN amplitude is 0.0, this renders the function redundant.\n";
    formattedMessage += "Program will continue but SIN command is redundant.";
    warning_message(formattedMessage);
    break;
  case static_cast<int>(FunctionErrors::CUS_TOO_FEW_ARGUMENTS):
    formattedMessage += "Total arguments specified do not match the required for CUS. " + whatPart + " specified.\n";
    formattedMessage += "Please refer to the CUS definition: CUS(WaveFile.dat TS SF IM <TD PER>)";
    warning_message(formattedMessage);
    break;
  case static_cast<int>(FunctionErrors::CUS_TOO_MANY_ARGUMENTS):
    formattedMessage += "Total arguments specified do not match the required for CUS. " + whatPart + " specified.\n";
    formattedMessage += "Please refer to the CUS definition: CUS(WaveFile.dat TS SF IM <TD PER>)";
    warning_message(formattedMessage);
    break;
  case static_cast<int>(FunctionErrors::CUS_SF_ZERO):
    formattedMessage += "CUS scale factor is 0.0, this renders the function redundant.\n";
    formattedMessage += "Program will continue but SIN command is redundant.";
    warning_message(formattedMessage);
    break;
  case static_cast<int>(FunctionErrors::CUS_WF_NOT_FOUND):
    formattedMessage += "CUS waveform file was not found." + whatPart + " specified.\n";
    formattedMessage += "Program will terminate.";
    throw formattedMessage;
  case static_cast<int>(FunctionErrors::NOISE_TOO_FEW_ARGUMENTS):
    formattedMessage += "Total arguments specified do not match the required for NOISE. " + whatPart + " specified.\n";
    formattedMessage += "Please refer to the NOISE definition: NOISE(0 VA TSTEP TD)";
    throw formattedMessage;
  case static_cast<int>(FunctionErrors::NOISE_TOO_MANY_ARGUMENTS):
    formattedMessage += "Total arguments specified do not match the required for NOISE. " + whatPart + " specified.\n";
    formattedMessage += "Please refer to the NOISE definition: NOISE(0 VA TSTEP TD)";
    throw formattedMessage;
  case static_cast<int>(FunctionErrors::NOISE_VO_ZERO):
    formattedMessage += "NOISE initial value is not 0.0, this needs to be zero.\n";
    formattedMessage += "Program will continue but NOISE command is redundant.";
    warning_message(formattedMessage);
    break;
  case static_cast<int>(FunctionErrors::NOISE_VA_ZERO):
    formattedMessage += "NOISE amplitude is 0.0, this renders the function redundant.\n";
    formattedMessage += "Program will continue but NOISE command is redundant.";
    warning_message(formattedMessage);
    break;
  default:
    formattedMessage += "Unknown function error: " + whatPart + "\n";
    formattedMessage += "Please contact the developer.";
    throw formattedMessage;
  }
}

[[noreturn]] void Errors::simulation_errors(int errorCode,
                                            const std::string &whatPart) {
  std::string formattedMessage = "Simulation\n";                                         
  switch (errorCode) {
  case static_cast<int>(SimulationErrors::JJCAP_NOT_FOUND):
    formattedMessage += "Capacitor value for " + whatPart + " could not be found.\n";
    formattedMessage += "This is a bug and the developer should be contacted. The program will abort.";
    throw formattedMessage;
  case static_cast<int>(SimulationErrors::JJICRIT_NOT_FOUND):
    formattedMessage += "Critical current value for " + whatPart + " could not be found.\n";
    formattedMessage += "This is a bug and the developer should be contacted. The program will abort.";
    throw formattedMessage;
  case static_cast<int>(SimulationErrors::JJPHASE_NODE_NOT_FOUND):
    formattedMessage += "Junction phase node not found for " + whatPart + ".\n";
    formattedMessage += "This is a bug and the developer should be contacted. The program will abort.";
    throw formattedMessage;
  case static_cast<int>(SimulationErrors::INDUCTOR_CURRENT_NOT_FOUND):
    formattedMessage += "Inductor current not defined for " + whatPart + ". Matrix will have no solution.\n";
    formattedMessage += "This is a bug and the developer should be contacted. The program will abort.";
    throw formattedMessage;
  case static_cast<int>(SimulationErrors::MATRIX_SINGULAR):
    formattedMessage += "Matrix is singular. Matrix will have no solution.\n";
    formattedMessage += "Please check the components in the netlist. The program will abort.";
    throw formattedMessage;
  default:
    formattedMessage += "Unknown simulation error: " + whatPart + "\n";
    formattedMessage += "Please contact the developer.";
    throw formattedMessage;
  }
}

void Errors::parsing_errors(int errorCode, const std::string &whatPart) {
  std::string formattedMessage = "Parsing\n";                                         
  switch (errorCode) {
  case static_cast<int>(ParsingErrors::EXPRESSION_ARLEADY_DEFINED):
    formattedMessage += "Expression duplication: " + whatPart + "\n";
    formattedMessage += "Please revise netlist.";
    throw formattedMessage;
  case static_cast<int>(ParsingErrors::UNIDENTIFIED_PART):
    formattedMessage += "The following variables were not found/defined.\n";
    formattedMessage += "Please ensure that these variables exist within the netlist.\n";
    formattedMessage += "Variables: \n" + whatPart;
    throw formattedMessage;
  case static_cast<int>(ParsingErrors::MISMATCHED_PARENTHESIS):
    formattedMessage += "Mismatched parenthesis in expression: " + whatPart + "\n";
    formattedMessage += "Please correct the expression before trying again.";
    throw formattedMessage;
  case static_cast<int>(ParsingErrors::INVALID_RPN):
    formattedMessage += "Invalid RPN detected. This might be an algorithm fault or an incorrect expression parse.\n";
    formattedMessage += "The expression in question: " + whatPart;
    throw formattedMessage;
  case static_cast<int>(ParsingErrors::INVALID_DECLARATION):
    formattedMessage += "Missing parameter declaration in: " + whatPart + "\n";
    formattedMessage += "Please ensure that a valid .PARAM definition is declared.";
    throw formattedMessage;
  default:
    formattedMessage += "Unknown parsing error: " + whatPart + "\n";
    formattedMessage += "Please contact the developer.";
    throw formattedMessage;
  }
}

[[noreturn]] void error_message (std::string &formattedMessage) {
  std::cerr << "E: " << formattedMessage << std::endl;
  std::cerr << std::endl;
  exit(-1);
}

void warning_message (std::string &formattedMessage) {
  std::cerr << "W: " << formattedMessage << std::endl;
  std::cerr << std::endl;
}