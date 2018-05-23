// Copyright (c) 2018 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#pragma once
#include "j_std_include.hpp"

void parse_expression(std::string expName, std::string expr);

int prec_lvl(std::string op);

double parse_operator(std::string op, double val1, double val2, int& popCount);