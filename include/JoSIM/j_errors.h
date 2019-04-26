// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_J_ERRORS_H
#define JOSIM_J_ERRORS_H
#include "j_std_include.h"

#define NO_ANALYSIS 0
#define NO_CONVENTION 1
#define NO_OUTPUT 2
#define NO_INPUT 3
#define UNKNOWN_SWITCH 4
#define TOO_FEW_ARGUMENTS 5
#define INVALID_ANALYSIS 6
#define INVALID_CONVENTION 7

#define CANNOT_OPEN_FILE 0
#define MISSING_SUBCKT_IO 1
#define MISSING_SUBCKT_NAME 2
#define SUBCKT_CONTROLS 3
#define MISSING_MAIN 4
#define UNKNOWN_SUBCKT 5

#define CAP_ERROR 0
#define IND_ERROR 1
#define RES_ERROR 2
#define JJ_ERROR 3
#define KMUT_ERROR 4
#define IS_ERROR 5
#define VS_ERROR 6
#define TRL_ERROR 7
#define LABEL_ERROR 8
#define MISSING_LABEL 9
#define MISSING_PNODE 10
#define MISSING_NNODE 11
#define MISSING_JJMODEL 12
#define MODEL_NOT_DEFINED 13
#define MODEL_AREA_NOT_GIVEN 14
#define DUPLICATE_LABEL 15
#define INVALID_SUBCIRCUIT_NODES 16
#define TIME_ERROR 17
#define MISSING_SUBCIRCUIT_NAME 18
#define MUT_ERROR 19
#define INVALID_EXPR 20
#define INVALID_TX_DEFINED 21
#define MISSING_INDUCTOR 22

#define TRANS_ERROR 0
#define PRINT_ERROR 1
#define PLOT_ERROR 2
#define INV_CONTROL 3
#define DC_ERROR 4
#define AC_ERROR 5
#define PHASE_ERROR 6
#define NO_SIM 7
#define PRINT_TOO_MANY_ARGS 8
#define UNKNOWN_DEVICE 9
#define CURRENT_THROUGH_VOLT 10
#define VOLT_WHEN_PHASE 11
#define VOLT_ACROSS_CURRENT 12
#define NODEVOLT_WHEN_PHASE 13
#define UNKNOWN_NODE 14
#define INVALID_NODEV 15
#define NODEPHASE_WHEN_VOLT 16
#define INVALID_NODEP 17
#define PHASE_WHEN_VOLT 18
#define PHASE_OF_VOLT 19
#define PHASE_OF_CURRENT 20
#define INVALID_CURRENT 21
#define MATHOPS 22
#define UNKNOWN_PLOT 23
#define INVALID_OUTPUT_COMMAND 24

#define PARAM_TYPE_ERROR 0
#define UNKNOWN_MODEL_TYPE 1
#define BAD_MODEL_DEFINITION 2

#define NON_SQUARE 0

#define STOD_ERROR 0

#define INITIAL_VALUES 0
#define TOO_FEW_TIMESTEPS 1
#define TOO_FEW_VALUES 2
#define INITIAL_PULSE_VALUE 3
#define PULSE_TOO_FEW_ARGUMENTS 4
#define PULSE_VPEAK_ZERO 5
#define PULSE_RISE_TIME_ZERO 6
#define PULSE_FALL_TIME_ZERO 7
#define PULSE_WIDTH_ZERO 8
#define PULSE_REPEAT 9
#define SIN_TOO_FEW_ARGUMENTS 10
#define SIN_TOO_MANY_ARGUMENTS 11
#define SIN_VA_ZERO 12
#define CUS_TOO_FEW_ARGUMENTS 13
#define CUS_TOO_MANY_ARGUMENTS 14
#define CUS_SF_ZERO 15
#define CUS_WF_NOT_FOUND 16

#define JJCAP_NOT_FOUND 0
#define JJICRIT_NOT_FOUND 1
#define JJPHASE_NODE_NOT_FOUND 2
#define INDUCTOR_CURRENT_NOT_FOUND 3
#define MATRIX_SINGULAR 4

#define EXPRESSION_ARLEADY_DEFINED 0
#define UNIDENTIFIED_PART 1
#define MISMATCHED_PARENTHESIS 2
#define INVALID_RPN 3

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
#endif
