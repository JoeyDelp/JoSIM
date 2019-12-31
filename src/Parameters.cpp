// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/Parameters.hpp"
#include "JoSIM/Misc.hpp"
#include "JoSIM/Constants.hpp"
#include "JoSIM/Errors.hpp"

#include <iostream>
#include <limits>
#include <cmath>

void Parameters::create_parameter(const std::pair<std::string, std::string> &s,
                                  std::unordered_map<JoSIM::ParameterName, Parameter> &parameters) {
  Parameter temp;
  std::vector<std::string> tokens;

  // Validity check: Check to see if '=' exists
  if(s.second.find_first_of('=') == std::string::npos) {
    Errors::parsing_errors(ParsingErrors::INVALID_DECLARATION, s.second);
  }
  // Remove ".PARAM" prefix and ensure valid declaration
  tokens = Misc::tokenize_space_once(s.second);
  if(tokens.size() < 2) {
    Errors::parsing_errors(ParsingErrors::INVALID_DECLARATION, s.second);
  }
  // Split into parameter name and expression
  tokens = Misc::tokenize_delimiter(tokens.at(1), "=");
  // Trim trailing and leading white spaces
  Misc::rtrim(tokens.at(0));
  Misc::ltrim(tokens.at(1));

  temp.set_expression(tokens.at(1));

  if(parameters.count(JoSIM::ParameterName(tokens.at(0), s.first)) == 0) {
    parameters.insert({JoSIM::ParameterName(tokens.at(0), s.first), temp});
  } else {
    Errors::parsing_errors(ParsingErrors::EXPRESSION_ARLEADY_DEFINED, s.second);
  }
}

// Possible functions that can be called
const std::vector<std::string> funcs = {
    "COS",   "SIN",   "TAN",   "ACOS", "ASIN", "ATAN",  "COSH", "SINH", "TANH",
    "ACOSH", "ASINH", "ATANH", "EXP",  "LOG",  "LOG10", "SQRT", "CBRT"
};

double Parameters::parse_param(
    const std::string &expr,
    const std::unordered_map<JoSIM::ParameterName, Parameter> &params,
    const std::string &subckt) 
  {
  // Initialize the expression to evaluate
  std::string expToEval = expr;
  // Remove any and all whitespace characters
  expToEval.erase(std::remove_if(expToEval.begin(), expToEval.end(), isspace),
                  expToEval.end());

  std::vector<std::string> rpnQueue, rpnQueueCopy, opStack;
  std::vector<char> qType, qTypeCopy;
  std::string partToEval;
  int popCount = 0;
  double result;

  // Evaluate expression piece by piece until it is empy
  while (!expToEval.empty()) {
    // Find the position of the first operator
    int opLoc = expToEval.find_first_of("/*-+(){}[]^");
    // If the position of the first operator is at the start 
    if (opLoc == -1) {
      partToEval = expToEval;
    } else {
      if (expToEval.at(opLoc) == '-')
        if (opLoc != 0)
          if (expToEval[opLoc - 1] == 'E')
            opLoc = expToEval.find_first_of("/*-+(){}[]^", opLoc + 1);
      if (opLoc == 0)
        partToEval = expToEval.substr(0, opLoc + 1);
      else
        partToEval = expToEval.substr(0, opLoc);
    }
    // Handle a numerical value
    if (isdigit(partToEval[0])) {
      rpnQueue.push_back(Misc::precise_to_string(Misc::modifier(partToEval)));
      qType.push_back('V');
    // If it is not a digit, check that it is not a parameter in local scope
    //} else if (params.count(JoSIM::ParameterName(partToEval, subckt)) != 0) {
    } else if ([&]() { 
        if(params.count(JoSIM::ParameterName(partToEval, subckt)) != 0)
          if(params.at(JoSIM::ParameterName(partToEval, subckt)).get_value())
            return true;
        return false;}() ) {
      rpnQueue.push_back(Misc::precise_to_string(
          params.at(JoSIM::ParameterName(partToEval, subckt)).get_value().value()));
      qType.push_back('V');
    // If it is not in local scope, check that it is not a parameter in global scope
    //} else if (params.count(JoSIM::ParameterName(partToEval, "")) != 0) {
    } else if ([&]() { 
        if(params.count(JoSIM::ParameterName(partToEval, "")) != 0)
          if(params.at(JoSIM::ParameterName(partToEval, "")).get_value())
            return true;
        return false;}() ) {
      rpnQueue.push_back(Misc::precise_to_string(
          params.at(JoSIM::ParameterName(partToEval, "")).get_value().value()));
      qType.push_back('V');
    // If it is not in either, check if it is not a function name such as SIN, COS, TAN, etc.
    } else if (std::find(funcs.begin(), funcs.end(), partToEval) != funcs.end()) {
      opStack.push_back(partToEval);
    // If not a function, check if it is not a defined constant
    } else if (Misc::string_constant(partToEval) != 0.0) {
      rpnQueue.push_back(Misc::precise_to_string(Misc::string_constant(partToEval)));
      qType.push_back('V');
    // If not a constant, check if it is an operator in the operator list
    } else if (partToEval.find_first_of("/*-+^") != std::string::npos) {
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
    // Check parenthesis opening
    } else if (partToEval.find_first_of("([{") != std::string::npos) {
      opStack.push_back(partToEval);
    // Check parenthesis close
    } else if (partToEval.find_first_of(")]}") != std::string::npos) {
      while ((!opStack.empty()) &&
             (opStack.back().find_first_of("([{") == std::string::npos)) {
        rpnQueue.push_back(opStack.back());
        qType.push_back('O');
        opStack.pop_back();
      }
      if ((!opStack.empty()) &&
          (opStack.back().find_first_of("([{") != std::string::npos)) {
        opStack.pop_back();
      } else {
        Errors::parsing_errors(ParsingErrors::MISMATCHED_PARENTHESIS, expr);
      }
    // If it is none of the above then the function is not valid or a parameter is used which has not been parsed
    } else {
      return std::numeric_limits<double>::quiet_NaN();
    }
    // Adjust the next part to be evaluated
    if (opLoc == 0) {
      expToEval = expToEval.substr(1);
    } else if (opLoc == -1) {
      expToEval = "";
    } else {
      expToEval = expToEval.substr(opLoc);
    }
  }
  if (expToEval.empty())
    while (!opStack.empty()) {
      if (opStack.back().find_first_of("([{") != std::string::npos)
        Errors::parsing_errors(ParsingErrors::MISMATCHED_PARENTHESIS, expr);
      else {
        rpnQueue.push_back(opStack.back());
        qType.push_back('O');
        opStack.pop_back();
      }
    }
  while (rpnQueue.size() > 1) {
    rpnQueueCopy.clear();
    qTypeCopy.clear();
    for (int i = 0; i < qType.size(); ++i) {
      if (qType[i] == 'V') {
        rpnQueueCopy.push_back(rpnQueue[i]);
        qTypeCopy.push_back('V');
      } else if (qType[i] == 'O') {
        if (i == 0)
          Errors::parsing_errors(ParsingErrors::INVALID_RPN, expr);
        else if (i < 2) {
          rpnQueueCopy.pop_back();
          rpnQueueCopy.push_back(Misc::precise_to_string(parse_operator(
              rpnQueue[i], 0, Misc::modifier(rpnQueue[i - 1]), popCount)));
        } else {
          result = parse_operator(rpnQueue[i], Misc::modifier(rpnQueue[i - 2]),
                                  Misc::modifier(rpnQueue[i - 1]), popCount);
          for (int k = 0; k < popCount; ++k)
            rpnQueueCopy.pop_back();
          if (popCount == 2)
            qTypeCopy.pop_back();
          rpnQueueCopy.push_back(Misc::precise_to_string(result));
        }
        if (rpnQueue.size() >= i) {
          rpnQueueCopy.insert(rpnQueueCopy.end(), rpnQueue.begin() + i + 1,
                              rpnQueue.end());
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

int Parameters::prec_lvl(const std::string &op) {
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

double Parameters::parse_operator(const std::string &op, double val1, double val2,
                              int &popCount) {
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
  } else {
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

void Parameters::parse_parameters(std::unordered_map<JoSIM::ParameterName, Parameter> &parameters) {
  double value;
  int parsedCounter = 0;

  while(parsedCounter < parameters.size()) {
    int previous_counter = parsedCounter;
    for(auto &i : parameters) {
      if(!i.second.get_value()) {
        value = parse_param(i.second.get_expression(), parameters, i.first.subcircuit());
        if(!std::isnan(value)) {
          i.second.set_value(value);
          parsedCounter++;
        }
      }
    }
    if (previous_counter == parsedCounter) {
      std::string unknownParams = "";
      for (auto &i : parameters) {
        if (!i.second.get_value()) {
          unknownParams += i.first.name() + " " + i.first.subcircuit() + "\n";
        }
      }
      Errors::parsing_errors(ParsingErrors::UNIDENTIFIED_PART, unknownParams);
    }
  }

}

