// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef J_PARSER_H_
#define J_PARSER_H_

#include "./j_misc.h"
#include "./j_std_include.h"
#include "./j_input.h"

#include "./ParameterName.hpp"

class Parameters;

class Parser {
public:
  static double parse_param(
      const std::string &expr,
      const std::unordered_map<JoSIM::ParameterName, double> &parsedParams,
      std::string subckt = "");

  static int prec_lvl(std::string op);

  static double parse_operator(std::string op, double val1, double val2,
                               int &popCount);

  static void parse_parameters(Parameters &parameters);
};

#endif
