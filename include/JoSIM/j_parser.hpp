// Copyright (c) 2018 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef J_PARSER_H
#define J_PARSER_H
#include "j_std_include.hpp"

void
parse_expression(std::string expName,
	std::string expr,
	std::unordered_map<std::string, double>& parVal,
	std::unordered_map<std::string, double>& globalParVal,
	std::string subckt = "NONE");

int
prec_lvl(std::string op);

double
parse_operator(std::string op, double val1, double val2, int& popCount);
#endif