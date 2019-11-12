// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_J_ERRORS_H
#define JOSIM_J_ERRORS_H

#include <string>

enum class CLIErrors : int { 
  NO_ANALYSIS,
  NO_CONVENTION,
  NO_OUTPUT,
  NO_INPUT,
  UNKNOWN_SWITCH,
  TOO_FEW_ARGUMENTS,
  INVALID_ANALYSIS,
  INVALID_CONVENTION
};

enum class InputErrors : int {
  CANNOT_OPEN_FILE,
  MISSING_SUBCKT_IO,
  MISSING_SUBCKT_NAME,
  SUBCKT_CONTROLS,
  MISSING_MAIN,
  UNKNOWN_SUBCKT,
  EMPTY_FILE
};

enum class ComponentErrors : int {
  INVALID_COMPONENT_DECLARATION,
  BOTH_GROUND,
  GROUNDED_VOLTAGE_SOURCE,
  CAP_ERROR,
  IND_ERROR,
  RES_ERROR,
  JJ_ERROR,
  KMUT_ERROR,
  IS_ERROR,
  VS_ERROR,
  TRL_ERROR,
  LABEL_ERROR,
  MISSING_LABEL,
  MISSING_PNODE,
  MISSING_NNODE,
  MISSING_JJMODEL,
  MODEL_NOT_DEFINED,
  MODEL_AREA_NOT_GIVEN,
  DUPLICATE_LABEL,
  INVALID_SUBCIRCUIT_NODES,
  TIME_ERROR,
  MISSING_SUBCIRCUIT_NAME,
  MUT_ERROR,
  INVALID_EXPR,
  INVALID_TX_DEFINED,
  MISSING_INDUCTOR,
  UNKNOWN_DEVICE_TYPE,
  SPECIAL_CHARS
};

enum class ControlErrors : int {
  TRANS_ERROR,
  PRINT_ERROR,
  PLOT_ERROR,
  INV_CONTROL,
  DC_ERROR,
  AC_ERROR,
  PHASE_ERROR,
  NO_SIM,
  PRINT_TOO_MANY_ARGS,
  UNKNOWN_DEVICE,
  CURRENT_THROUGH_VOLT,
  VOLT_WHEN_PHASE,
  VOLT_ACROSS_CURRENT,
  NODEVOLT_WHEN_PHASE,
  UNKNOWN_NODE,
  INVALID_NODEV,
  NODEPHASE_WHEN_VOLT,
  INVALID_NODEP,
  PHASE_WHEN_VOLT,
  PHASE_OF_VOLT,
  PHASE_OF_CURRENT,
  INVALID_CURRENT,
  MATHOPS,
  UNKNOWN_PLOT,
  INVALID_OUTPUT_COMMAND,
  NODECURRENT
};

enum class ModelErrors : int {
  PARAM_TYPE_ERROR,
  UNKNOWN_MODEL_TYPE,
  BAD_MODEL_DEFINITION
};

enum class MatrixErrors : int {
  NON_SQUARE
};

enum class MiscErrors : int {
  STOD_ERROR
};

enum class FunctionErrors : int {
  INITIAL_VALUES,
  TOO_FEW_TIMESTEPS,
  TOO_FEW_VALUES,
  INITIAL_PULSE_VALUE,
  PULSE_TOO_FEW_ARGUMENTS,
  PULSE_VPEAK_ZERO,
  PULSE_RISE_TIME_ZERO,
  PULSE_FALL_TIME_ZERO,
  PULSE_WIDTH_ZERO,
  PULSE_REPEAT,
  SIN_TOO_FEW_ARGUMENTS,
  SIN_TOO_MANY_ARGUMENTS,
  SIN_VA_ZERO,
  CUS_TOO_FEW_ARGUMENTS,
  CUS_TOO_MANY_ARGUMENTS,
  CUS_SF_ZERO,
  CUS_WF_NOT_FOUND,
  NOISE_TOO_FEW_ARGUMENTS,
  NOISE_TOO_MANY_ARGUMENTS,
  NOISE_VA_ZERO,
  NOISE_VO_ZERO
};

enum class SimulationErrors : int {
  JJCAP_NOT_FOUND,
  JJICRIT_NOT_FOUND,
  JJPHASE_NODE_NOT_FOUND,
  INDUCTOR_CURRENT_NOT_FOUND,
  MATRIX_SINGULAR
};

enum class ParsingErrors : int {
  EXPRESSION_ARLEADY_DEFINED,
  UNIDENTIFIED_PART,
  MISMATCHED_PARENTHESIS,
  INVALID_RPN,
  INVALID_DECLARATION
};

class Errors {
public:
  static void cli_errors(int errorCode, const std::string &whatPart = "");

  static void input_errors(int errorCode, const std::string &whatPart = "");

  static void invalid_component_errors(int errorCode,
                                       const std::string &componentLabel);

  static void control_errors(int errorCode, const std::string &whatPart);

  [[noreturn]] static void model_errors(int errorCode,
                                        const std::string &whatPart);

  static void matrix_errors(int errorCode, const std::string &whatPart);

  [[noreturn]] static void misc_errors(int errorCode,
                                       const std::string &whatPart);

  static void function_errors(int errorCode, const std::string &whatPart);

  [[noreturn]] static void simulation_errors(int errorCode,
                                             const std::string &whatPart);

  static void parsing_errors(int errorCode, const std::string &whatPart);
};

[[noreturn]] void error_message (std::string &formattedMessage);

void warning_message (std::string &formattedMessage);

#endif
