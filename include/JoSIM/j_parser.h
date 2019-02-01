// Copyright (c) 2018 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef J_PARSER_H_
#define J_PARSER_H_
#include "j_std_include.h"
#include "j_misc.h"

class Parser {
	private:
		static std::unordered_map<std::string, double> _dummy_map;
	public:
		// static
		// void
		// parse_expression(std::string expName,
		// 	std::string expr,
		// 	std::string subckt = "");

		// static
		// double
		// parse_return_expression(std::string expr, std::string subckt = "");

		static
		double
		parse_param(std::string expr, std::unordered_map<std::pair<std::string, 
				std::string>, double, pair_hash> parsedParams, 
				std::string subckt = "");

		static
		int
		prec_lvl(std::string op);

		static
		double
		parse_operator(std::string op, double val1, double val2, int& popCount);

		static
		void
		parse_parameters(std::vector<std::pair<std::string, std::string>> unparsedParams,
		std::unordered_map<std::pair<std::string, std::string>, double, pair_hash> &parsedParams);
};
#endif