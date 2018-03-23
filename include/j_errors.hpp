#pragma once
#include "j_std_include.hpp"
/*
  General error codes
*/
#define DEF_ERROR 0
#define DEF_FILE_ERROR 1
#define LOG_ERROR 2
#define LOG_FILE_ERROR 3
#define OUTPUT_ERROR 4
#define OUTPUT_FILE_ERROR 5
#define INPUT_ERROR 6
#define INPUT_FILE_ERROR 7
#define UNKNOWN_SWITCH 8
#define CANNOT_OPEN_FILE 9
/*
  Component error codes
*/
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
/*
  Control error codes
*/
#define TRANS_ERROR 0
#define PRINT_ERROR 1
#define PLOT_ERROR 2
#define INV_CONTROL 3
#define DC_ERROR 4
#define AC_ERROR 5
#define PHASE_ERROR 6
#define NO_SIM 7
/*
  Model error codes
*/
#define PARAM_TYPE_ERROR 0
/*
  Matrix error codes
*/
#define NON_SQUARE 0
/*
  Misc error codes
*/
#define STOD_ERROR 0
/*
	Function error codes
*/
#define INITIAL_VALUES 0
#define TOO_FEW_TIMESTEPS 1
#define TOO_FEW_VALUES 2
/*
	Simulation error codes
*/
#define JJCAP_NOT_FOUND 0
#define JJICRIT_NOT_FOUND 1
#define JJPHASE_NODE_NOT_FOUND 2
#define INDUCTOR_CURRENT_NOT_FOUND 3
/*
	Plotting error codes
*/
#define NO_SUCH_PLOT_TYPE 0
#define NO_SUCH_DEVICE_FOUND 1
#define CURRENT_THROUGH_VOLTAGE_SOURCE 2

/*
	Function that manages different error codes. This function will be huge.
*/
void error_handling(int errorCode);
/*
	Invalid component decleration error function
*/
void invalid_component_errors(int errorCode, std::string componentLabel);
/*
	Missing simulation and incorrect control parameters specified
*/
void control_errors(int errorCode, std::string whatPart);
/*
	Model declaration error function
*/
void model_errors(int errorCode, std::string whatPart);
/*
	Matrix creation error function
*/
void matrix_errors(int errorCode, std::string whatPart);
/*
	Misc error function
*/
void misc_errors(int errorCode, std::string whatPart);
/*
	Function parser error function
*/
void function_errors(int errorCode, std::string whatPart);
/*
	Simulation error function
*/
void simulation_errors(int errorCode, std::string whatPart);
/*
	Plotting error function
*/
void plotting_errors(int errorCode, std::string whatPart);