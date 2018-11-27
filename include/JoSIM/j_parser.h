// Copyright (c) 2018 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef J_PARSER_H_
#define J_PARSER_H_
#include "j_std_include.h"

class Parser {
	private:
		static std::unordered_map<std::string, double> _dummy_map;
	public:
		static
		void
		parse_expression(std::string expName,
			std::string expr,
			std::unordered_map<std::string, double>& parVal,
			std::unordered_map<std::string, double>& globalParVal,
			std::string subckt = "NONE");

		static
		double
		parse_return_expression(std::string expr);

		static
		double
		parse_return_expression(std::string expr, 
			std::unordered_map<std::string, double>& parVal, 
			std::unordered_map<std::string, double>& globalParVal);

		static
		int
		prec_lvl(std::string op);

		static
		double
		parse_operator(std::string op, double val1, double val2, int& popCount);
};
#endif