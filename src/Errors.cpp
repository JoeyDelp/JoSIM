// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/Errors.hpp"

#include <iostream>

using namespace JoSIM;

void Errors::cli_errors(CLIErrors errorCode, string_o message) {
  std::string formattedMessage = "Command Line Interface\n";
  switch (errorCode) {
    case CLIErrors::NO_OUTPUT:
      formattedMessage +=
          "No output file name specified. Using default (output.csv).\n";
      formattedMessage +=
          "This file will be stored in the current working directory.";
      warning_message(formattedMessage);
      break;
    case CLIErrors::NO_INPUT:
      formattedMessage += "No input file was specified.\n";
      formattedMessage += "Continuing by reading from standard input.";
      warning_message(formattedMessage);
      break;
    case CLIErrors::UNKNOWN_SWITCH:
      formattedMessage += "Unknown option '" + message.value_or("") +
                          "' specified. Please refer to the help menu.";
      warning_message(formattedMessage);
      break;
    case CLIErrors::UNKNOWN_OUTPUT_TYPE:
      formattedMessage += "Unknown output type " + message.value_or("") +
                          " specified. CSV will be used instead.";
      warning_message(formattedMessage);
      break;
    case CLIErrors::TOO_FEW_ARGUMENTS:
      formattedMessage += "Missing input arguments\n";
      formattedMessage += "Usage: josim [options] input_netlist\n\n";
      formattedMessage += "For further help use the -h switch";
      throw std::runtime_error(formattedMessage);
    case CLIErrors::INVALID_ANALYSIS:
      formattedMessage +=
          "Invalid analysis type specified. 0 - Voltage, 1 - Phase.\n";
      formattedMessage += "Usage: josim [options] input_netlist\n\n";
      formattedMessage += "For further help use the -h switch";
      throw std::runtime_error(formattedMessage);
    case CLIErrors::INVALID_SOLVER:
      formattedMessage += "Invalid solver type specified. 0 - KLU, 1 - SLU.\n";
      formattedMessage += "Usage: josim [options] input_netlist\n\n";
      formattedMessage += "For further help use the -h switch";
      throw std::runtime_error(formattedMessage);
    case CLIErrors::INVALID_INTEGRATION:
      formattedMessage +=
          "Invalid integration method specified. 0 - Trapezoidal, 1 - Gear.\n";
      formattedMessage += "Usage: josim [options] input_netlist\n\n";
      formattedMessage += "For further help use the -h switch";
      throw std::runtime_error(formattedMessage);
    case CLIErrors::NO_ANALYSIS:
      formattedMessage +=
          "No analysis was specified. Reverting to default (0 - Voltage).\n";
      formattedMessage +=
          "Please refer to the help menu (-h) or manual for further "
          "information.";
      warning_message(formattedMessage);
      break;
    case CLIErrors::NO_SOLVER:
      formattedMessage +=
          "No solver was specified. Reverting to default (0 - KLU).\n";
      formattedMessage +=
          "Please refer to the help menu (-h) or manual for further "
          "information.";
      warning_message(formattedMessage);
      break;
    case CLIErrors::NO_INTEGRATION:
      formattedMessage += "No integration method specified\n.";
      formattedMessage += "Reverting to default (0 - Trapezoidal).\n";
      formattedMessage +=
          "Please refer to the help menu (-h) or manual for further "
          "information.";
      warning_message(formattedMessage);
      break;
    case CLIErrors::INVALID_CONVENTION:
      formattedMessage +=
          "Invalid subcircuit convention specified. 0 - JSIM, 1 - WRspice.\n";
      formattedMessage += "Usage: josim [options] input_netlist\n\n";
      formattedMessage += "For further help use the -h switch";
      throw std::runtime_error(formattedMessage);
    case CLIErrors::INPUT_SAME_OUTPUT:
      formattedMessage += "Output file name is the same as input file name.\n";
      formattedMessage +=
          "This will cause irreversible changes to input file.\n";
      formattedMessage += "Please choose a different output file name.";
      throw std::runtime_error(formattedMessage);
    case CLIErrors::NO_CONVENTION:
      formattedMessage +=
          "No convention was specified. Reverting to default (0 - JSIM).\n";
      formattedMessage +=
          "Please refer to the help menu (-h) or manual for further "
          "information.";
      warning_message(formattedMessage);
      break;
    case CLIErrors::INVALID_MINIMAL:
      formattedMessage +=
          "Invalid minimal output specified. 0 - Off, 1 - On.\n";
      formattedMessage += "Usage: josim [options] input_netlist\n\n";
      formattedMessage += "For further help use the -h switch";
      throw std::runtime_error(formattedMessage);
    default:
      formattedMessage += "Unknown handling error.\n";
      formattedMessage += "Please contact the developer.";
      throw std::runtime_error(formattedMessage);
  }
}

void Errors::input_errors(InputErrors errorCode, string_o message) {
  std::string formattedMessage = "Input\n";
  switch (errorCode) {
    case InputErrors::CANNOT_OPEN_FILE:
      formattedMessage += "Input file " + message.value_or("") +
                          " cannot be found or opened.\n";
      formattedMessage +=
          "Please ensure that the file exists and can be opened.\n\n";
      formattedMessage += "For further help use the -h switch";
      throw std::runtime_error(formattedMessage);
    case InputErrors::CYCLIC_INCLUDE:
      formattedMessage +=
          "Attempting to include file " + message.value_or("") + ".\n";
      formattedMessage += "This is the same file as input file.\n\n";
      formattedMessage += "Preventing cyclic includes.";
      throw std::runtime_error(formattedMessage);
    case InputErrors::MISSING_SUBCKT_IO:
      formattedMessage += "Missing subcircuit io.\n";
      formattedMessage += "Please recheck the netlist and try again.";
      throw std::runtime_error(formattedMessage);
    case InputErrors::MISSING_SUBCKT_NAME:
      formattedMessage += "Missing subcircuit name.\n";
      formattedMessage += "Please recheck the netlist and try again.";
      throw std::runtime_error(formattedMessage);
    case InputErrors::SUBCKT_CONTROLS:
      formattedMessage +=
          "Subcircuit " + message.value_or("") + " contains controls.\n";
      formattedMessage += "Controls are reserved for the main design.\n";
      formattedMessage += "These controls will be ignored.";
      warning_message(formattedMessage);
      break;
    case InputErrors::DUPLICATE_SUBCIRCUIT:
      formattedMessage +=
          "Subcircuit " + message.value_or("") + " already exist.\n";
      formattedMessage += "Overwriting the existing definition.\n";
      formattedMessage += "Please avoid subcircuit redefinitions.";
      warning_message(formattedMessage);
      break;
    case InputErrors::MISSING_MAIN:
      formattedMessage += "Missing main design in netlist.\n";
      formattedMessage +=
          "This design will not simulate without a main design.";
      throw std::runtime_error(formattedMessage);
    case InputErrors::UNKNOWN_SUBCKT:
      formattedMessage +=
          "The subcircuit in the following line was not found:\n";
      formattedMessage += message.value() + "\n";
      formattedMessage +=
          "Please ensure all subcircuits exist and are correctly named.";
      throw std::runtime_error(formattedMessage);
    case InputErrors::EMPTY_FILE:
      formattedMessage +=
          "The file \"" + message.value_or("") + "\" contains no lines.\n";
      formattedMessage +=
          "Please check the input file and ensure that the file is not empty.";
      throw std::runtime_error(formattedMessage);
    case InputErrors::IO_MISMATCH:
      formattedMessage += "The IO of line \"" + message.value_or("") +
                          "\" does not "
                          "match the subcircuit IO.\n";
      formattedMessage +=
          "Please check the line and ensure correct IO and "
          "that parameters do not contain spaces.";
      throw std::runtime_error(formattedMessage);
    case InputErrors::UNKNOWN_CONTROL:
      formattedMessage +=
          "The control \"" + message.value_or("") + "\" is not known.\n";
      formattedMessage +=
          "Please consult the syntax guide for a list of available controls.";
      throw std::runtime_error(formattedMessage);
    default:
      formattedMessage += "Unknown input error.\n";
      formattedMessage += "Please contact the developer.";
      throw std::runtime_error(formattedMessage);
  }
}

void Errors::invalid_component_errors(ComponentErrors errorCode,
                                      string_o message) {
  std::string formattedMessage = "Components\n";
  switch (errorCode) {
    case ComponentErrors::INVALID_COMPONENT_DECLARATION:
      formattedMessage += "Invalid component declaration detected.\n";
      formattedMessage += "Infringing line: " + message.value_or("") + "\n";
      formattedMessage +=
          "Please refer to the documentation for the correct notation.";
      throw std::runtime_error(formattedMessage);
    case ComponentErrors::BOTH_GROUND:
      formattedMessage +=
          "Both nodes are grounded for the following component.\n";
      formattedMessage += "Component: " + message.value_or("");
      warning_message(formattedMessage);
      break;
    case ComponentErrors::GROUNDED_VOLTAGE_SOURCE:
      formattedMessage += "Both nodes are grounded for the following source.\n";
      formattedMessage += "Component: " + message.value_or("");
      throw std::runtime_error(formattedMessage);
    case ComponentErrors::RES_ERROR:
      formattedMessage += "Resistor value error\n";
      formattedMessage += "Infringing line: " + message.value_or("");
      throw std::runtime_error(formattedMessage);
    case ComponentErrors::CAP_ERROR:
      formattedMessage += "Capacitor value error\n";
      formattedMessage += "Infringing line: " + message.value_or("");
      throw std::runtime_error(formattedMessage);
    case ComponentErrors::IND_ERROR:
      formattedMessage += "Inductor value error\n";
      formattedMessage += "Infringing line: " + message.value_or("");
      throw std::runtime_error(formattedMessage);
    case ComponentErrors::LABEL_ERROR:
      formattedMessage += "Invalid component label: " + message.value_or("");
      throw std::runtime_error(formattedMessage);
    case ComponentErrors::MISSING_LABEL:
      formattedMessage += "No component label. This should not happen.\n";
      formattedMessage += "Infringing line: " + message.value_or("") + "\n";
      formattedMessage +=
          "Please contact the developer as this is possibly a bug.";
      throw std::runtime_error(formattedMessage);
    case ComponentErrors::MISSING_PNODE:
      formattedMessage += "No positive node. This should not happen.\n";
      formattedMessage += "Infringing line: " + message.value_or("") + "\n";
      formattedMessage +=
          "Please contact the developer as this is possibly a bug.";
      throw std::runtime_error(formattedMessage);
    case ComponentErrors::MISSING_NNODE:
      formattedMessage += "No negative node. This should not happen.\n";
      formattedMessage += "Infringing line: " + message.value_or("") + "\n";
      formattedMessage +=
          "Please contact the developer as this is possibly a bug.";
      throw std::runtime_error(formattedMessage);
    case ComponentErrors::MISSING_JJMODEL:
      formattedMessage +=
          "No junction model is specified for junction " + message.value_or("");
      throw std::runtime_error(formattedMessage);
    case ComponentErrors::MODEL_NOT_DEFINED:
      formattedMessage += "The specified model is not defined.\n";
      formattedMessage += "Missing model: " + message.value_or("") + "\n";
      formattedMessage += "Using default model as specified in the manual.";
      warning_message(formattedMessage);
      break;
    case ComponentErrors::MODEL_AREA_NOT_GIVEN:
      formattedMessage +=
          "No area specified for junction " + message.value_or("") + "\n";
      formattedMessage += "Using default: AREA=1.0";
      warning_message(formattedMessage);
      break;
    case ComponentErrors::DUPLICATE_LABEL:
      formattedMessage +=
          "Duplicate label " + message.value_or("") + " detected.\n";
      formattedMessage +=
          "The program will now terminate. Please recheck the netlist.";
      throw std::runtime_error(formattedMessage);
    case ComponentErrors::INVALID_SUBCIRCUIT_NODES:
      formattedMessage += "The nodes of " + message.value_or("") +
                          " do not match the subcicuit nodes.\n";
      formattedMessage +=
          "Please recheck the nodes required by the subcircuit and try again.";
      throw std::runtime_error(formattedMessage);
    case ComponentErrors::TIME_ERROR:
      formattedMessage += "Time delay value error\n";
      formattedMessage += "Infringing line: " + message.value_or("");
      throw std::runtime_error(formattedMessage);
    case ComponentErrors::MISSING_SUBCIRCUIT_NAME:
      formattedMessage += "The subcircuit for " + message.value_or("") +
                          " was not found in the file.\n";
      formattedMessage += "Please recheck the subcircuit name and try again.";
      throw std::runtime_error(formattedMessage);
    case ComponentErrors::MUT_ERROR:
      formattedMessage += "Invalid mutual coupling definition found.\n";
      formattedMessage += "Infringing line: " + message.value_or("");
      throw std::runtime_error(formattedMessage);
    case ComponentErrors::INVALID_EXPR:
      formattedMessage += "Invalid expression statement found.\n";
      formattedMessage += "Infringing line: " + message.value_or("");
      throw std::runtime_error(formattedMessage);
    case ComponentErrors::INVALID_TX_DEFINED:
      formattedMessage += "Invalid definition for transmission line found.\n";
      formattedMessage += "Infringing line: " + message.value_or("");
      throw std::runtime_error(formattedMessage);
    case ComponentErrors::INVALID_TX_RESOLUTION:
      formattedMessage +=
          "Time delay for transmission line is less than simulation "
          "timestep.\n";
      formattedMessage +=
          "Please reduce timestep to less than any transmission line delay.\n";
      formattedMessage += "Infringing line: " + message.value_or("");
      throw std::runtime_error(formattedMessage);
    case ComponentErrors::MISSING_INDUCTOR:
      formattedMessage += "Invalid mutual coupling defined.";
      formattedMessage += "Missing inductor " + message.value_or("") + "\n";
      formattedMessage +=
          "Please ensure that " + message.value_or("") + " exists.";
      throw std::runtime_error(formattedMessage);
    case ComponentErrors::UNKNOWN_DEVICE_TYPE:
      formattedMessage += "Unkown device type " + message.value_or("") + "\n";
      formattedMessage +=
          "Please refer to the syntax guide for a list of device types.";
      throw std::runtime_error(formattedMessage);
    case ComponentErrors::SPECIAL_CHARS:
      formattedMessage +=
          "Label " + message.value_or("") + " contains special characters.\n";
      formattedMessage +=
          "The use of special characters in label names is not advised.\n";
      formattedMessage += "This might produce unexpected results.\n";
      formattedMessage += "Continuing operation.";
      warning_message(formattedMessage);
      break;
    default:
      formattedMessage += "Unknown invalid component error.\n";
      formattedMessage += "Please contact the developer.";
      throw std::runtime_error(formattedMessage);
  }
}

void Errors::control_errors(ControlErrors errorCode, string_o message) {
  std::string formattedMessage = "Controls\n";
  switch (errorCode) {
    case ControlErrors::TRANS_ERROR:
      formattedMessage += "Invalid transient analysis specified. " +
                          message.value_or("") + "\n";
      formattedMessage += "Substituting default parameters.\n";
      formattedMessage +=
          "Defaults: TSTEP=1PS TSTOP=1000PS prstart=0PS PRSTEP=1PS";
      warning_message(formattedMessage);
      break;
    case ControlErrors::PRINT_TOO_MANY_ARGS:
      formattedMessage +=
          "Print request for device current has too many arguments.\n";
      formattedMessage += "Line: " + message.value_or("") + "\n";
      formattedMessage += "Ignoring the extra argument.";
      warning_message(formattedMessage);
      break;
    case ControlErrors::PRINT_ERROR:
      throw std::runtime_error(formattedMessage);
    case ControlErrors::PLOT_ERROR:
      throw std::runtime_error(formattedMessage);
    case ControlErrors::INV_CONTROL:
      throw std::runtime_error(formattedMessage);
    case ControlErrors::DC_ERROR:
      throw std::runtime_error(formattedMessage);
    case ControlErrors::AC_ERROR:
      throw std::runtime_error(formattedMessage);
    case ControlErrors::PHASE_ERROR:
      throw std::runtime_error(formattedMessage);
    case ControlErrors::NO_SIM:
      formattedMessage +=
          "No simulation type specified. Nothing will be simulated.";
      throw std::runtime_error(formattedMessage);
    case ControlErrors::UNKNOWN_DEVICE:
      formattedMessage += "Unknown device/node " + message.value_or("") + "\n";
      formattedMessage += "Cannot store results for this device/node.\n";
      formattedMessage += "Ignoring this store request.";
      warning_message(formattedMessage);
      break;
    case ControlErrors::CURRENT_THROUGH_VOLT:
      formattedMessage += "Requesting current through a voltage source.\n";
      formattedMessage += "Line: " + message.value_or("") + "\n";
      formattedMessage += "This is invalid and the request will be ignored.";
      warning_message(formattedMessage);
      break;
    case ControlErrors::VOLT_WHEN_PHASE:
      formattedMessage +=
          "Request to store voltage for device " + message.value_or("") + "\n";
      formattedMessage += "Phase mode simulation performed.\n";
      formattedMessage += "Storing device phase instead.";
      warning_message(formattedMessage);
      break;
    case ControlErrors::VOLT_ACROSS_CURRENT:
      formattedMessage += "Requesting voltage across a current source.\n";
      formattedMessage += "Line: " + message.value_or("") + "\n";
      formattedMessage += "This is invalid and the request will be ignored.";
      warning_message(formattedMessage);
      break;
    case ControlErrors::NODEVOLT_WHEN_PHASE:
      formattedMessage +=
          "Request to store nodal voltage for " + message.value_or("") + "\n";
      formattedMessage += "Phase mode simulation performed.\n";
      formattedMessage += "Storing nodal phase instead.";
      warning_message(formattedMessage);
      break;
    case ControlErrors::NODECURRENT:
      formattedMessage +=
          "Request for current of " + message.value_or("") + " is invalid.\n";
      formattedMessage +=
          "Cannot find device or cannot store current of a node.";
      warning_message(formattedMessage);
      break;
    case ControlErrors::UNKNOWN_NODE:
      formattedMessage +=
          "Node " + message.value_or("") + " was not found in the circuit.\n";
      formattedMessage += "This request for store will be ignored.";
      warning_message(formattedMessage);
      break;
    case ControlErrors::NODEPHASE_WHEN_VOLT:
      formattedMessage +=
          "Request to store nodal phase for " + message.value_or("") + "\n";
      formattedMessage += "Voltage mode simulation performed.\n";
      formattedMessage += "Storing nodal voltage instead.";
      warning_message(formattedMessage);
      break;
    case ControlErrors::INVALID_NODEV:
      formattedMessage += "Invalid node voltage request found.\n";
      formattedMessage += "Line: " + message.value_or("") + "\n";
      formattedMessage += "This request for store will be ignored.";
      warning_message(formattedMessage);
      break;
    case ControlErrors::INVALID_NODEP:
      formattedMessage += "Invalid node phase request found.\n";
      formattedMessage += "Line: " + message.value_or("") + "\n";
      formattedMessage += "This request for store will be ignored.";
      warning_message(formattedMessage);
      break;
    case ControlErrors::PHASE_WHEN_VOLT:
      formattedMessage += "Requesting phase in a voltage simulation.\n";
      formattedMessage += "Line: " + message.value_or("") + "\n";
      formattedMessage += "This request will be ignored.";
      warning_message(formattedMessage);
      break;
    case ControlErrors::PHASE_OF_VOLT:
      formattedMessage += "Requesting phase of a voltage source.\n";
      formattedMessage += "Line: " + message.value_or("") + "\n";
      formattedMessage += "This is invalid and the request will be ignored.";
      warning_message(formattedMessage);
      break;
    case ControlErrors::PHASE_OF_CURRENT:
      formattedMessage += "Requesting phase of a current source.\n";
      formattedMessage += "Line: " + message.value_or("") + "\n";
      formattedMessage += "This is invalid and the request will be ignored.";
      warning_message(formattedMessage);
      break;
    case ControlErrors::INVALID_CURRENT:
      formattedMessage += "Invalid request to plot current.\n";
      formattedMessage += "Infringing line: " + message.value_or("");
      warning_message(formattedMessage);
      break;
    case ControlErrors::MATHOPS:
      formattedMessage +=
          "Mathematical operations on output vectors are not yet supported.\n";
      formattedMessage += "Ignoring plotting of " + message.value_or("");
      warning_message(formattedMessage);
      break;
    case ControlErrors::UNKNOWN_PLOT:
      formattedMessage += "Unknown plot type " + message.value_or("") + "\n";
      formattedMessage += "Ignoring request to plot.";
      warning_message(formattedMessage);
      break;
    case ControlErrors::INVALID_OUTPUT_COMMAND:
      formattedMessage += "Invalid request for output found.\n";
      formattedMessage += message.value_or("") + "\n";
      formattedMessage += "Ignoring request and continuing.";
      warning_message(formattedMessage);
      break;
    case ControlErrors::INVALID_FILE_COMMAND:
      formattedMessage += "Invalid request for file output found.\n";
      formattedMessage += message.value_or("") + "\n";
      formattedMessage += "Ignoring request and continuing.";
      warning_message(formattedMessage);
      break;
    case ControlErrors::INVALID_IV_COMMAND:
      formattedMessage += "Invalid request for IV curve generation found.\n";
      formattedMessage += message.value_or("") + "\n";
      formattedMessage += "Please refer to the manual for proper syntax.\n";
      formattedMessage += "Ignoring request and continuing.";
      warning_message(formattedMessage);
      break;
    case ControlErrors::IV_MODEL_NOT_FOUND:
      formattedMessage += "The requested model was not found.\n";
      formattedMessage += message.value_or("") + "\n";
      formattedMessage += "Please ensure the model exists in the netlist.\n";
      error_message(formattedMessage);
    default:
      formattedMessage +=
          "Unknown control error: " + message.value_or("") + "\n";
      formattedMessage += "Please contact the developer.";
      throw std::runtime_error(formattedMessage);
  }
}

void Errors::model_errors(ModelErrors errorCode, string_o message) {
  std::string formattedMessage = "Model\n";
  switch (errorCode) {
    case ModelErrors::PARAM_TYPE_ERROR:
      formattedMessage += "Unknown model parameter specified.\n";
      formattedMessage += "Model line: " + message.value_or("") + "\n";
      formattedMessage +=
          "Continuing with default model parameters.\n"
          "Please consult the syntax guide for more information.";
      warning_message(formattedMessage);
      break;
    case ModelErrors::PARAM_PARENTHESIS:
      formattedMessage += "Model parameter has non-closing parenthesis.\n";
      formattedMessage += "Model line: " + message.value_or("");
      throw std::runtime_error(formattedMessage);
    case ModelErrors::UNKNOWN_MODEL_TYPE:
      formattedMessage += "Unknown model type specified.\n";
      formattedMessage += "Model line: " + message.value_or("");
      throw std::runtime_error(formattedMessage);
    case ModelErrors::BAD_MODEL_DEFINITION:
      formattedMessage += "Bad model definition found.\n";
      formattedMessage += "Infringing line: " + message.value_or("");
      throw std::runtime_error(formattedMessage);
    default:
      formattedMessage += "Unknown model error: " + message.value_or("") + "\n";
      formattedMessage += "Please contact the developer.";
      throw std::runtime_error(formattedMessage);
  }
}

[[noreturn]] void Errors::matrix_errors(MatrixErrors errorCode,
                                        string_o message) {
  std::string formattedMessage = "Matrix\n";
  switch (errorCode) {
    case MatrixErrors::NON_SQUARE:
      formattedMessage +=
          "Matrix is not square. Dimensions are " + message.value_or("") + "\n";
      formattedMessage +=
          "Please contact the developer as this is potentially a bug.";
      throw std::runtime_error(formattedMessage);
    case MatrixErrors::SANITY_ERROR:
      formattedMessage += "Component " + message.value_or("") +
                          " created a value in the non-zero"
                          " matrix that is either 0, inf or NaN.\n";
      formattedMessage +=
          "This is an internal error and the developer should be contacted.";
      throw std::runtime_error(formattedMessage);
    default:
      formattedMessage +=
          "Unknown matrix error: " + message.value_or("") + "\n";
      formattedMessage += "Please contact the developer.";
      throw std::runtime_error(formattedMessage);
  }
}

[[noreturn]] void Errors::misc_errors(MiscErrors errorCode, string_o message) {
  std::string formattedMessage = "Miscellaneous\n";
  switch (errorCode) {
    case MiscErrors::STOD_ERROR:
      formattedMessage +=
          "Cannot convert string to double: " + message.value_or("");
      throw std::runtime_error(formattedMessage);
    default:
      formattedMessage += "Unknown misc error: " + message.value_or("") + "\n";
      formattedMessage += "Please contact the developer.";
      throw std::runtime_error(formattedMessage);
  }
}

void Errors::function_errors(FunctionErrors errorCode, string_o message) {
  std::string formattedMessage = "Function\n";
  switch (errorCode) {
    case FunctionErrors::INITIAL_VALUES:
      formattedMessage += "Invalid PWL definition found.\n";
      formattedMessage +=
          "The value of " + message.value_or("") + " is expected to be 0\n";
      formattedMessage += "Please refer to the PWL definition:\n";
      formattedMessage += "PWL(0 V0 T1 V1 T2 V2 ... Tn Vn)";
      warning_message(formattedMessage);
      break;
    case FunctionErrors::TOO_FEW_TIMESTEPS:
      formattedMessage +=
          "Total timesteps specified do not match the values specified.\n";
      formattedMessage += message.value_or("") + " specified.\n";
      formattedMessage += "Please refer to the PWL definition:\n";
      formattedMessage += "PWL(0 0 T1 V1 T2 V2 ... Tn Vn)";
      throw std::runtime_error(formattedMessage);
    case FunctionErrors::TOO_FEW_VALUES:
      formattedMessage +=
          "Total values specified do not match the timesteps specified.\n";
      formattedMessage += message.value_or("") + " specified.\n";
      formattedMessage += "Please refer to the PWL definition:\n";
      formattedMessage += "PWL(0 0 T1 V1 T2 V2 ... Tn Vn)";
      throw std::runtime_error(formattedMessage);
    case FunctionErrors::INITIAL_PULSE_VALUE:
      formattedMessage += "Invalid PULSE definition found.\n";
      formattedMessage +=
          "The value of " + message.value_or("") + " is expected to be 0\n";
      formattedMessage += "Please refer to the PULSE definition:\n";
      formattedMessage += "PULSE(0 V2 TD TR TF PW PER)";
      throw std::runtime_error(formattedMessage);
    case FunctionErrors::PULSE_TOO_FEW_ARGUMENTS:
      formattedMessage +=
          "Total arguments specified do not match the required for PULSE.\n";
      formattedMessage += message.value_or("") + " specified.\n";
      formattedMessage += "Please refer to the PULSE definition:\n";
      formattedMessage += "PULSE(0 V2 TD TR TF PW PER)";
      throw std::runtime_error(formattedMessage);
    case FunctionErrors::PULSE_VPEAK_ZERO:
      formattedMessage +=
          "PULSE peak voltage is 0.0, this renders the function redundant.\n";
      formattedMessage +=
          "Program will continue but PULSE command is redundant.";
      warning_message(formattedMessage);
      break;
    case FunctionErrors::PULSE_WIDTH_ZERO:
      formattedMessage +=
          "PULSE width is 0.0, this renders the function redundant.\n";
      formattedMessage +=
          "Program will continue but PULSE command is redundant.";
      warning_message(formattedMessage);
      break;
    case FunctionErrors::PULSE_REPEAT:
      formattedMessage +=
          "PULSE repeat rate is 0.0, this is effectively a DC source.\n";
      formattedMessage +=
          "Program will continue, but this is most likely unwanted.";
      warning_message(formattedMessage);
      break;
    case FunctionErrors::SIN_TOO_FEW_ARGUMENTS:
      formattedMessage +=
          "Total arguments specified do not match the required for SIN.\n";
      formattedMessage += message.value_or("") + " specified.\n";
      formattedMessage += "Please refer to the SIN definition:\n";
      formattedMessage += "SIN(VO VA FREQ TD THETA)";
      throw std::runtime_error(formattedMessage);
    case FunctionErrors::SIN_TOO_MANY_ARGUMENTS:
      formattedMessage +=
          "Total arguments specified do not match the required for SIN.\n";
      formattedMessage += message.value_or("") + " specified.\n";
      formattedMessage += "Please refer to the SIN definition:\n";
      formattedMessage += "SIN(VO VA FREQ TD THETA)";
      throw std::runtime_error(formattedMessage);
    case FunctionErrors::SIN_VA_ZERO:
      formattedMessage +=
          "SIN amplitude is 0.0, this renders the function redundant.\n";
      formattedMessage += "Program will continue but SIN command is redundant.";
      warning_message(formattedMessage);
      break;
    case FunctionErrors::CUS_TOO_FEW_ARGUMENTS:
      formattedMessage +=
          "Total arguments specified do not match the required for CUS.\n";
      formattedMessage += message.value_or("") + " specified.\n";
      formattedMessage += "Please refer to the CUS definition:\n";
      formattedMessage += "CUS(WaveFile.dat TS SF IM <TD PER>)";
      warning_message(formattedMessage);
      break;
    case FunctionErrors::CUS_TOO_MANY_ARGUMENTS:
      formattedMessage +=
          "Total arguments specified do not match the required for CUS.\n";
      formattedMessage += message.value_or("") + " specified.\n";
      formattedMessage += "Please refer to the CUS definition:\n";
      formattedMessage += "CUS(WaveFile.dat TS SF IM <TD PER>)";
      warning_message(formattedMessage);
      break;
    case FunctionErrors::CUS_SF_ZERO:
      formattedMessage +=
          "CUS scale factor is 0.0, this renders the function redundant.\n";
      formattedMessage += "Program will continue but SIN command is redundant.";
      warning_message(formattedMessage);
      break;
    case FunctionErrors::CUS_WF_NOT_FOUND:
      formattedMessage += "CUS waveform file was not found.\n";
      formattedMessage += message.value_or("") + " specified.\n";
      formattedMessage += "Program will terminate.";
      throw std::runtime_error(formattedMessage);
    case FunctionErrors::CUS_UNKNOWN_IM:
      formattedMessage += "CUS waveform has unknown interpolation method.\n";
      formattedMessage += message.value_or("") + " specified.\n";
      formattedMessage += "Program will terminate.";
      throw std::runtime_error(formattedMessage);
    case FunctionErrors::NOISE_TOO_FEW_ARGUMENTS:
      formattedMessage +=
          "Total arguments specified do not match the required for NOISE.\n";
      formattedMessage += message.value_or("") + " specified.\n";
      formattedMessage += "Please refer to the NOISE definition:\n";
      formattedMessage += "NOISE(0 VA TSTEP TD)";
      throw std::runtime_error(formattedMessage);
    case FunctionErrors::NOISE_TOO_MANY_ARGUMENTS:
      formattedMessage +=
          "Total arguments specified do not match the required for NOISE.\n";
      formattedMessage += message.value_or("") + " specified.\n";
      formattedMessage += "Please refer to the NOISE definition:\n";
      formattedMessage += "NOISE(0 VA TSTEP TD)";
      throw std::runtime_error(formattedMessage);
    case FunctionErrors::NOISE_VO_ZERO:
      formattedMessage +=
          "NOISE initial value is not 0.0, this needs to be zero.\n";
      formattedMessage +=
          "Program will continue but NOISE command is redundant.";
      warning_message(formattedMessage);
      break;
    case FunctionErrors::NOISE_VA_ZERO:
      formattedMessage +=
          "NOISE amplitude is 0.0, this renders the function redundant.\n";
      formattedMessage +=
          "Program will continue but NOISE command is redundant.";
      warning_message(formattedMessage);
      break;
    case FunctionErrors::EXP_TOO_FEW_ARGUMENTS:
      formattedMessage +=
          "Total arguments specified do not match the required for NOISE.\n";
      formattedMessage += message.value_or("") + " specified.\n";
      formattedMessage += "Please refer to the NOISE definition:\n";
      formattedMessage +=
          "EXP(V1 V2 TD1[0.0] TAU1[TSTEP] TD2[TD1+TSTEP] TAU2[TSTEP])";
      throw std::runtime_error(formattedMessage);
    default:
      formattedMessage +=
          "Unknown function error: " + message.value_or("") + "\n";
      formattedMessage += "Please contact the developer.";
      throw std::runtime_error(formattedMessage);
  }
}

[[noreturn]] void Errors::simulation_errors(SimulationErrors errorCode,
                                            string_o message) {
  std::string formattedMessage = "Simulation\n";
  switch (errorCode) {
    case SimulationErrors::JJCAP_NOT_FOUND:
      formattedMessage +=
          "Capacitance for " + message.value_or("") + " could not be found.\n";
      formattedMessage +=
          "This is a bug and the developer should be contacted.\n";
      formattedMessage += "The program will abort.";
      throw std::runtime_error(formattedMessage);
    case SimulationErrors::JJICRIT_NOT_FOUND:
      formattedMessage += "Critical current value for " + message.value_or("") +
                          " could not be found.\n";
      formattedMessage +=
          "This is a bug and the developer should be contacted.\n";
      formattedMessage += "The program will abort.";
      throw std::runtime_error(formattedMessage);
    case SimulationErrors::JJPHASE_NODE_NOT_FOUND:
      formattedMessage +=
          "Junction phase node not found for " + message.value_or("") + ".\n";
      formattedMessage +=
          "This is a bug and the developer should be contacted.\n";
      formattedMessage += "The program will abort.";
      throw std::runtime_error(formattedMessage);
    case SimulationErrors::INDUCTOR_CURRENT_NOT_FOUND:
      formattedMessage +=
          "Inductor current not defined for " + message.value_or("") + ".\n";
      formattedMessage += "Matrix will have no solution.\n";
      formattedMessage +=
          "This is a bug and the developer should be contacted.\n";
      formattedMessage += "The program will abort.";
      throw std::runtime_error(formattedMessage);
    case SimulationErrors::MATRIX_SINGULAR:
      formattedMessage += "Matrix is singular. Matrix will have no solution.\n";
      formattedMessage += "Please check the components in the netlist.\n";
      formattedMessage += "The program will abort.";
      throw std::runtime_error(formattedMessage);
    case SimulationErrors::PHASEGUESS_TOO_LARGE:
      formattedMessage +=
          "Junction " + message.value_or("") + " has gone too far.\n";
      formattedMessage += "This is a result of integration error.\n";
      formattedMessage += "Please reduce the timestep and try again.";
      throw std::runtime_error(formattedMessage);
    default:
      formattedMessage +=
          "Unknown simulation error: " + message.value_or("") + "\n";
      formattedMessage += "Please contact the developer.";
      throw std::runtime_error(formattedMessage);
  }
}

void Errors::parsing_errors(ParsingErrors errorCode, string_o message) {
  std::string formattedMessage = "Parsing\n";
  switch (errorCode) {
    case ParsingErrors::EXPRESSION_ARLEADY_DEFINED:
      formattedMessage +=
          "Expression duplication: " + message.value_or("") + "\n";
      formattedMessage += "Replacing.";
      warning_message(formattedMessage);
      break;
    case ParsingErrors::UNIDENTIFIED_PART:
      formattedMessage += "The following variables were not found/defined.\n";
      formattedMessage +=
          "Please ensure that these variables exist within the netlist.\n";
      formattedMessage += "Variables: \n" + message.value_or("");
      throw std::runtime_error(formattedMessage);
    case ParsingErrors::MISMATCHED_PARENTHESIS:
      formattedMessage +=
          "Mismatched parenthesis in expression: " + message.value_or("") +
          "\n";
      formattedMessage += "Please correct the expression before trying again.";
      throw std::runtime_error(formattedMessage);
    case ParsingErrors::INVALID_RPN:
      formattedMessage += "Invalid RPN detected.\n";
      formattedMessage +=
          "This might be an algorithm fault or an incorrect expression "
          "parse.\n";
      formattedMessage += "The expression in question: " + message.value_or("");
      throw std::runtime_error(formattedMessage);
    case ParsingErrors::INVALID_DECLARATION:
      formattedMessage +=
          "Missing parameter declaration in: " + message.value_or("") + "\n";
      formattedMessage +=
          "Please ensure that a valid .PARAM definition is declared.";
      throw std::runtime_error(formattedMessage);
    default:
      formattedMessage +=
          "Unknown parsing error: " + message.value_or("") + "\n";
      formattedMessage += "Please contact the developer.";
      throw std::runtime_error(formattedMessage);
  }
}

[[noreturn]] void Errors::netlist_errors(NetlistErrors errorCode,
                                         string_o message) {
  std::string formattedMessage = "Netlist\n";
  switch (errorCode) {
    case NetlistErrors::NO_SUCH_NODE:
      formattedMessage += "Node \"" + message.value_or("") +
                          "\" was not found in the netlist\n";
      formattedMessage += "Please check for any disconnections in the netlist";
      throw std::runtime_error(formattedMessage);
    case NetlistErrors::MISSING_IO:
      formattedMessage += "Missing I/O nodes for subcircuit:\n";
      formattedMessage += message.value_or("") + "\n";
      formattedMessage += "Please check for any disconnections in the netlist";
      throw std::runtime_error(formattedMessage);
    default:
      formattedMessage +=
          "Unknown netlist error: " + message.value_or("") + "\n";
      formattedMessage += "Please contact the developer.";
      throw std::runtime_error(formattedMessage);
  }
}

void Errors::output_errors(OutputErrors errorCode, string_o message) {
  std::string formattedMessage = "Output\n";
  switch (errorCode) {
    case OutputErrors::CANNOT_OPEN_FILE:
      formattedMessage += "Cannot open the requested file for output.\n";
      formattedMessage += "Please ensure write permission for the file: " +
                          message.value_or("");
      throw std::runtime_error(formattedMessage);
    case OutputErrors::NOTHING_SPECIFIED:
      formattedMessage += "Nothing specified to output.\n";
      formattedMessage += "Cannot create empty RAW file.";
      warning_message(formattedMessage);
      break;
  }
}

void Errors::verbosity_errors(VerbosityErrors errorCode, string_o message) {
  std::string formattedMessage = "Verbosity\n";
  switch (errorCode) {
    case VerbosityErrors::NO_SUCH_LEVEL:
      formattedMessage +=
          "No such verbosity level: " + message.value_or("") + "\n";
      formattedMessage += "Continuing with maximum verbosity.";
      warning_message(formattedMessage);
      break;
    case VerbosityErrors::INVALID_VERBOSITY_LEVEL:
      formattedMessage +=
          "Invalid verbosity specified: " + message.value_or("") + "\n";
      formattedMessage +=
          "Please specify a valid level. Levels are 0, 1, 2, 3.\n";
      throw std::runtime_error(formattedMessage);
  }
}

[[noreturn]] void Errors::oor() {
  std::cerr << "\nE: Out of range error. This is a bug." << std::endl;
  std::cerr << "E: Please contact the developer." << std::endl;
  std::cerr << std::endl;
  exit(-1);
}

[[noreturn]] void Errors::error_message(const std::string& formattedMessage) {
  std::cerr << "\nE: " << formattedMessage << std::endl;
  std::cerr << std::endl;
  exit(-1);
}

void Errors::warning_message(const std::string& formattedMessage) {
  std::cerr << "\nW: " << formattedMessage << std::endl;
  std::cerr << std::endl;
}