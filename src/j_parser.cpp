// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#include "JoSIM/j_parser.h"

// Possible functions that can be called
std::string funcsArray[] = { "COS",  "SIN",  "TAN",   "ACOS",  "ASIN",  "ATAN",
							 "COSH", "SINH", "TANH",  "ACOSH", "ASINH", "ATANH",
							 "EXP",  "LOG",  "LOG10", "SQRT",  "CBRT" };
std::unordered_map<std::string, double> consts = {{"C",299792458},
												  {"MU0",12.566370614E-7},
												  {"EPS0",8.854187817E-12},
												  {"PI",3.141592653589793}};
std::vector<std::string> funcs(funcsArray,
	funcsArray +
	sizeof(funcsArray) / sizeof(std::string));


double
Parser::parse_param(const std::string& expr, const std::unordered_map<std::pair<std::string,
				std::string>, double, pair_hash> &parsedParams,
				std::string subckt) {
	std::string expToEval = expr;
	expToEval.erase(std::remove_if(expToEval.begin(), expToEval.end(), isspace), expToEval.end());
	std::vector<std::string> rpnQueue, rpnQueueCopy, opStack;
	std::vector<char> qType, qTypeCopy;
	std::string partToEval;
	int opLoc, popCount = 0;
	double result = 0.0;
	while (!expToEval.empty()) {
		opLoc = expToEval.find_first_of("/*-+(){}[]^");
		if(opLoc == -1) {
			partToEval = expToEval;
		}
		else {
			if (expToEval.at(opLoc) == '-')
				if (opLoc != 0)
					if (expToEval[opLoc - 1] == 'E')
						opLoc = expToEval.find_first_of("/*-+(){}[]^", opLoc + 1);
			if (opLoc == 0)
				partToEval = expToEval.substr(0, opLoc + 1);
			else
				partToEval = expToEval.substr(0, opLoc);
		}
		if (isdigit(partToEval[0])) {
			rpnQueue.push_back(Misc::precise_to_string(Misc::modifier(partToEval)));
			qType.push_back('V');
		}
		else if (parsedParams.count(std::make_pair(partToEval, subckt)) != 0) {
			rpnQueue.push_back(Misc::precise_to_string(
				parsedParams.at(std::make_pair(partToEval, subckt))));
			qType.push_back('V');
		}
		else if (std::find(funcs.begin(), funcs.end(), partToEval) != funcs.end())
			opStack.push_back(partToEval);
		else if (consts.count(partToEval) != 0){
			rpnQueue.push_back(Misc::precise_to_string(consts[partToEval]));
			qType.push_back('V');
		}
		else if (partToEval.find_first_of("/*-+^") != std::string::npos) {
			while ((!opStack.empty()) &&
				(((prec_lvl(opStack.back()) == 4) ||
				(prec_lvl(opStack.back()) >= prec_lvl(partToEval))) &&
					(opStack.back().find_first_of("([{") == std::string::npos) &&
					(partToEval != "^"))) {
				rpnQueue.push_back(opStack.back());
				qType.push_back('O');
				opStack.pop_back();
			}
			opStack.push_back(partToEval);
		}
		else if (partToEval.find_first_of("([{") != std::string::npos)
			opStack.push_back(partToEval);
		else if (partToEval.find_first_of(")]}") != std::string::npos) {
			while ((!opStack.empty()) &&
				(opStack.back().find_first_of("([{") == std::string::npos)) {
				rpnQueue.push_back(opStack.back());
				qType.push_back('O');
				opStack.pop_back();
			}
			if ((!opStack.empty()) &&
				(opStack.back().find_first_of("([{") != std::string::npos))
				opStack.pop_back();
			else
				Errors::parsing_errors(MISMATCHED_PARENTHESIS, expr);
		}
		else
			Errors::parsing_errors(UNIDENTIFIED_PART, partToEval);
		if (opLoc == 0)
			expToEval = expToEval.substr(opLoc + 1);
		if (opLoc == -1)
			expToEval = "";
		else
			expToEval = expToEval.substr(opLoc);
	}
	if (expToEval.empty())
		while (!opStack.empty()) {
			if (opStack.back().find_first_of("([{") != std::string::npos)
				Errors::parsing_errors(MISMATCHED_PARENTHESIS, expr);
			else {
				rpnQueue.push_back(opStack.back());
				qType.push_back('O');
				opStack.pop_back();
			}
		}
	while (rpnQueue.size() > 1) {
		rpnQueueCopy.clear();
		qTypeCopy.clear();
		for (int i = 0; i < qType.size(); i++) {
			if (qType[i] == 'V') {
				rpnQueueCopy.push_back(rpnQueue[i]);
				qTypeCopy.push_back('V');
			}
			else if (qType[i] == 'O') {
				if (i == 0)
					Errors::parsing_errors(INVALID_RPN, expr);
				else if (i < 2) {
					rpnQueueCopy.pop_back();
					rpnQueueCopy.push_back(Misc::precise_to_string(parse_operator(
						rpnQueue[i], 0, Misc::modifier(rpnQueue[i - 1]), popCount)));
				}
				else {
					result = parse_operator(rpnQueue[i],
						Misc::modifier(rpnQueue[i - 2]),
						Misc::modifier(rpnQueue[i - 1]),
						popCount);
					for (int k = 0; k < popCount; k++)
						rpnQueueCopy.pop_back();
					if (popCount == 2)
						qTypeCopy.pop_back();
					rpnQueueCopy.push_back(Misc::precise_to_string(result));
				}
				if (rpnQueue.size() >= i) {
					rpnQueueCopy.insert(
						rpnQueueCopy.end(), rpnQueue.begin() + i + 1, rpnQueue.end());
					qTypeCopy.insert(qTypeCopy.end(), qType.begin() + i + 1, qType.end());
				}
				break;
			}
		}
		rpnQueue = rpnQueueCopy;
		qType = qTypeCopy;
	}
	return Misc::modifier(rpnQueue.back());
}

int
Parser::prec_lvl(std::string op)
{
	switch (op[0]) {
		// + and - are lowest level
	case '+':
	case '-':
		return 1;
		// * and / are higher level
	case '*':
	case '/':
		return 2;
		// ^ (pow) is highest level
	case '^':
		return 3;
		// default (functions) are max level
	default:
		return 4;
	}
	return 4;
}

double
Parser::parse_operator(std::string op, double val1, double val2, int& popCount)
{
	if (std::find(funcs.begin(), funcs.end(), op) != funcs.end()) {
		popCount = 1;
		if (op == "SIN")
			return sin(val2);
		else if (op == "COS")
			return cos(val2);
		else if (op == "TAN")
			return tan(val2);
		else if (op == "ASIN")
			return asin(val2);
		else if (op == "ACOS")
			return acos(val2);
		else if (op == "ATAN")
			return atan(val2);
		else if (op == "SINH")
			return sinh(val2);
		else if (op == "COSH")
			return cosh(val2);
		else if (op == "TANH")
			return tanh(val2);
		else if (op == "ASINH")
			return asinh(val2);
		else if (op == "ACOSH")
			return acosh(val2);
		else if (op == "ATANH")
			return atanh(val2);
		else if (op == "EXP")
			return exp(val2);
		else if (op == "LOG")
			return log(val2);
		else if (op == "SQRT")
			return sqrt(val2);
		else if (op == "CBRT")
			return cbrt(val2);
	}
	else {
		popCount = 2;
		switch (op[0]) {
		case '+':
			return val1 + val2;
		case '-':
			return val1 - val2;
		case '*':
			return val1 * val2;
		case '/':
			return val1 / val2;
		case '^':
			return pow(val1, val2);
		}
	}
	return 0.0;
}

void
Parser::parse_parameters(const std::vector<std::pair<std::string, std::string>> &unparsedParams,
						std::unordered_map<std::pair<std::string, std::string>,
						double, pair_hash> &parsedParams) {
	std::vector<std::string> tokens, paramTokens;
	std::string paramName, paramExp;
	double value;
	for (auto i : unparsedParams) {
		tokens = Misc::tokenize_space(i.second);
		if(tokens.size() > 1) {
			if(tokens.size() >= 2) {
				if(tokens.size() > 2) paramName = tokens.at(1);
				else paramName = tokens.at(1).substr(0, tokens.at(1).find_first_of('='));
				paramExp = i.second.substr(i.second.find_first_of("=") + 1);
				if(i.first == "") {
					value = parse_param(paramExp, parsedParams);
					parsedParams[std::make_pair(paramName, "")] = value;
				}
				else {
					value = parse_param(paramExp, parsedParams, i.first);
					parsedParams[std::make_pair(paramName, i.first)] = value;
				}
			}
			else {
				std::cout << "W: Missing parameter declaration in " << i.second << "." << std::endl;
				std::cout << "W: Please ensure that a valid .PARAM definition is declared." << std::endl;
				std::cout << "W: This line will be ignored." << std::endl;
			}
		}
		else {
			std::cout << "W: Missing parameter declaration in " << i.second << "." << std::endl;
			std::cout << "W: Please ensure that a valid .PARAM definition is declared." << std::endl;
			std::cout << "W: This line will be ignored." << std::endl;
		}
	}
}
