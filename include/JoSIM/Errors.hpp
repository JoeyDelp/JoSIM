// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_ERRORS_HPP
#define JOSIM_ERRORS_HPP

#include <string>

#include "JoSIM/TypeDefines.hpp"

namespace JoSIM {
enum class CLIErrors : int64_t {
  NO_ANALYSIS,
  NO_SOLVER,
  NO_INTEGRATION,
  NO_CONVENTION,
  NO_OUTPUT,
  NO_INPUT,
  UNKNOWN_SWITCH,
  UNKNOWN_OUTPUT_TYPE,
  TOO_FEW_ARGUMENTS,
  INVALID_ANALYSIS,
  INVALID_SOLVER,
  INVALID_INTEGRATION,
  INVALID_CONVENTION,
  INVALID_MINIMAL,
  INPUT_SAME_OUTPUT
};

enum class InputErrors : int64_t {
  CANNOT_OPEN_FILE,
  CYCLIC_INCLUDE,
  MISSING_SUBCKT_IO,
  MISSING_SUBCKT_NAME,
  SUBCKT_CONTROLS,
  MISSING_MAIN,
  UNKNOWN_SUBCKT,
  EMPTY_FILE,
  IO_MISMATCH,
  UNKNOWN_CONTROL,
  DUPLICATE_SUBCIRCUIT
};

enum class ComponentErrors : int64_t {
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
  INVALID_TX_RESOLUTION,
  MISSING_INDUCTOR,
  UNKNOWN_DEVICE_TYPE,
  SPECIAL_CHARS
};

enum class ControlErrors : int64_t {
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
  INVALID_FILE_COMMAND,
  INVALID_IV_COMMAND,
  IV_MODEL_NOT_FOUND,
  NODECURRENT
};

enum class ModelErrors : int64_t {
  PARAM_TYPE_ERROR,
  PARAM_PARENTHESIS,
  UNKNOWN_MODEL_TYPE,
  BAD_MODEL_DEFINITION
};

enum class MatrixErrors : int64_t { NON_SQUARE, SANITY_ERROR };

enum class MiscErrors : int64_t { STOD_ERROR };

enum class FunctionErrors : int64_t {
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
  CUS_UNKNOWN_IM,
  NOISE_TOO_FEW_ARGUMENTS,
  NOISE_TOO_MANY_ARGUMENTS,
  NOISE_VA_ZERO,
  NOISE_VO_ZERO,
  EXP_TOO_FEW_ARGUMENTS
};

enum class SimulationErrors : int64_t {
  JJCAP_NOT_FOUND,
  JJICRIT_NOT_FOUND,
  JJPHASE_NODE_NOT_FOUND,
  INDUCTOR_CURRENT_NOT_FOUND,
  MATRIX_SINGULAR,
  PHASEGUESS_TOO_LARGE
};

enum class ParsingErrors : int64_t {
  EXPRESSION_ARLEADY_DEFINED,
  UNIDENTIFIED_PART,
  MISMATCHED_PARENTHESIS,
  INVALID_RPN,
  INVALID_DECLARATION
};

enum class OutputErrors : int64_t { CANNOT_OPEN_FILE, NOTHING_SPECIFIED };

enum class NetlistErrors : int64_t { NO_SUCH_NODE, MISSING_IO };

enum class VerbosityErrors : int64_t { NO_SUCH_LEVEL, INVALID_VERBOSITY_LEVEL };

class Errors {
 public:
  static void cli_errors(CLIErrors errorCode, string_o message = std::nullopt);

  static void input_errors(InputErrors errorCode,
                           string_o message = std::nullopt);

  static void invalid_component_errors(ComponentErrors errorCode,
                                       string_o componentLabel = std::nullopt);

  static void control_errors(ControlErrors errorCode,
                             string_o message = std::nullopt);

  static void model_errors(ModelErrors errorCode,
                           string_o message = std::nullopt);

  [[noreturn]] static void matrix_errors(MatrixErrors errorCode,
                                         string_o message = std::nullopt);

  [[noreturn]] static void misc_errors(MiscErrors errorCode,
                                       string_o message = std::nullopt);

  static void function_errors(FunctionErrors errorCode,
                              string_o message = std::nullopt);

  [[noreturn]] static void simulation_errors(SimulationErrors errorCode,
                                             string_o message = std::nullopt);

  static void parsing_errors(ParsingErrors errorCode,
                             string_o message = std::nullopt);

  static void output_errors(OutputErrors errorCode,
                            string_o message = std::nullopt);

  [[noreturn]] static void netlist_errors(NetlistErrors errorCode,
                                          string_o message = std::nullopt);

  static void verbosity_errors(VerbosityErrors errorCode,
                               string_o message = std::nullopt);

  [[noreturn]] static void oor();

  [[noreturn]] static void error_message(const std::string& formattedMessage);

  static void warning_message(const std::string& formattedMessage);
};
}  // namespace JoSIM
#endif
