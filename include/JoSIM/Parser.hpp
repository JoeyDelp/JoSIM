// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_J_PARSER_H
#define JOSIM_J_PARSER_H

#include "./Misc.hpp"
#include "./Input.hpp"


#include "./ParameterName.hpp"

class Parameters;

class Parser {
public:
  static double parse_param(
      const std::string &expr,
      const std::unordered_map<JoSIM::ParameterName, double> &parsedParams,
      const std::string &subckt = "");

  static int prec_lvl(const std::string &op);

  static double parse_operator(const std::string &op, double val1, double val2,
                               int &popCount);

  static void parse_parameters(Parameters &parameters);
};

#endif
