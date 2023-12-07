// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/Parameters.hpp"

#include <cmath>
#include <iostream>
#include <limits>

#include "JoSIM/Constants.hpp"
#include "JoSIM/Errors.hpp"
#include "JoSIM/Misc.hpp"

using namespace JoSIM;

void JoSIM::create_parameter(const tokens_t& s, param_map& parameters,
                             string_o subc) {
  // Temporary parameter which will be stored
  Parameter temp;
  // Check to ensure declaration is valid
  if (s.size() < 2) {
    Errors::parsing_errors(ParsingErrors::INVALID_DECLARATION,
                           Misc::vector_to_string(s));
  }
  // Check to see if '=' exists
  if (Misc::vector_to_string(s).find('=') == std::string::npos) {
    Errors::parsing_errors(ParsingErrors::INVALID_DECLARATION,
                           Misc::vector_to_string(s));
  }
  // Split the line/tokens int64_to the parameter name and expression
  tokens_t tokens(s.begin() + 1, s.end());
  // 1st token is parameter name, 2nd is the expression
  tokens = Misc::tokenize(Misc::vector_to_string(tokens, ""), "=", true, true);
  // Set the expresion for the temporary parameter
  temp.set_expression(tokens.at(1));
  // Warn user that duplicate parameter exists, this overwrites previous
  if (parameters.count(ParameterName(tokens.at(0), subc)) != 0) {
    Errors::parsing_errors(ParsingErrors::EXPRESSION_ARLEADY_DEFINED,
                           Misc::vector_to_string(s));
  }
  // Insert the parameter int64_to a map of known parameters for parsing
  parameters.insert({ParameterName(tokens.at(0), subc), temp});
}

// Possible functions that can be called
const std::vector<std::string> funcs = {
    "COS",   "SIN",   "TAN",   "ACOS", "ASIN", "ATAN",  "COSH", "SINH", "TANH",
    "ACOSH", "ASINH", "ATANH", "EXP",  "LOG",  "LOG10", "SQRT", "CBRT"};

int64_t JoSIM::precedence_lvl(const std::string& op) {
  switch (op.at(0)) {
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

double JoSIM::parse_param(const std::string& expr, const param_map& params,
                          string_o subc, bool single) {
  // Initialize the expression to evaluate
  std::string expToEval = expr;
  // Sanity check, prepend 0 to a string where the value is eg. .5 to vorm 0.5
  if (expToEval.front() == '.') {
    expToEval = "0" + expToEval;
  }
  // Remove any and all whitespace characters
  expToEval.erase(std::remove_if(expToEval.begin(), expToEval.end(), isspace),
                  expToEval.end());
  // Stacks used in the shunting yard
  std::vector<std::string> rpnQueue, rpnQueueCopy, opStack;
  // Create two copies of the type each element in the queue is
  std::vector<char> qType, qTypeCopy;
  // Substring reference to part of the expression
  std::string partToEval;
  // Counter
  int64_t popCount = 0;
  // Variable for the result to be returned
  double result;
  // Evaluate expression piece by piece until it is empy
  while (!expToEval.empty()) {
    // First test the expression to see if it is a parameter
    if (params.count(ParameterName(expToEval, subc)) != 0) {
      if (!params.at(ParameterName(expToEval, subc)).get_value()) {
        if (!single) {
          // Return NaN to indicate this ocurred
          return std::numeric_limits<double>::quiet_NaN();
        } else {
          Errors::parsing_errors(ParsingErrors::UNIDENTIFIED_PART, expr);
        }
      } else
        expToEval = Misc::precise_to_string(
            params.at(ParameterName(expToEval, subc)).get_value().value());
    } else if (params.count(ParameterName(expToEval, std::nullopt)) != 0) {
      if (!params.at(ParameterName(expToEval, std::nullopt)).get_value()) {
        if (!single) {
          // Return NaN to indicate this ocurred
          return std::numeric_limits<double>::quiet_NaN();
        } else {
          Errors::parsing_errors(ParsingErrors::UNIDENTIFIED_PART, expr);
        }
      } else
        expToEval = Misc::precise_to_string(
            params.at(ParameterName(expToEval, std::nullopt))
                .get_value()
                .value());
    }
    // Find the position of the first operator
    int64_t opLoc = expToEval.find_first_of("/*-+(){}[]^");
    // If no operator is found
    if (opLoc == -1) {
      // The part to evaluate is the entire experssion
      partToEval = expToEval;
      // If an operator is found
    } else {
      // If the operator is either a '-' or a '+'
      if (expToEval.at(opLoc) == '-' || expToEval.at(opLoc) == '+') {
        // If this operator is not the start of the string
        if (opLoc != 0) {
          // Do a few checks
          bool digitBeforeE = false;
          bool eBefore = false;
          bool digitAfter = false;
          // If the character preceeding this operator is an E
          if (expToEval[opLoc - 1] == 'E') eBefore = true;
          // If the character after the operator is a digit
          if (opLoc != expToEval.length() - 1) {
            if (std::isdigit(expToEval[opLoc + 1])) digitAfter = true;
          }
          // If the char before E is a digit
          if ((opLoc - 1) != 0) {
            if (std::isdigit(expToEval[opLoc - 2])) digitBeforeE = true;
          }
          if (eBefore && digitAfter && digitBeforeE) {
            // Find the next operator since this operator is not an operator
            opLoc = expToEval.find_first_of("/*-+(){}[]^", opLoc + 1);
          }
        } else {
          // If the character after the operator is a digit but not before
          if ((opLoc != expToEval.length() - 1) && !qType.empty()) {
            if (qType.back() != 'V') {
              if (std::isdigit(expToEval[opLoc + 1])) {
                // Find next operator since this operator is not an operator
                opLoc = expToEval.find_first_of("/*-+(){}[]^", opLoc + 1);
                // Do a few checks
                bool digitBeforeE = false;
                bool eBefore = false;
                bool digitAfter = false;
                // If the character preceeding this operator is an E
                if (expToEval[opLoc - 1] == 'E') eBefore = true;
                // If the character after the operator is a digit
                if (opLoc != expToEval.length() - 1) {
                  if (std::isdigit(expToEval[opLoc + 1])) digitAfter = true;
                }
                // If the char before E is a digit
                if ((opLoc - 1) != 0) {
                  if (std::isdigit(expToEval[opLoc - 2])) digitBeforeE = true;
                }
                if (eBefore && digitAfter && digitBeforeE) {
                  // Find next operator since this operator is not an operator
                  opLoc = expToEval.find_first_of("/*-+(){}[]^", opLoc + 1);
                }
              }
            }
          }
        }
      }
      // If the operator is at the start of the string
      if (opLoc == 0) {
        // The part to evaluate is the the operator
        partToEval = expToEval.substr(0, opLoc + 1);
        // If the operator is further along
      } else {
        // The part to evaluate is from the start to the the operator
        partToEval = expToEval.substr(0, opLoc);
      }
    }
    // Handle a numerical value
    if (isdigit(partToEval.at(0)) ||
        ((partToEval.at(0) == '-' || partToEval.at(0) == '+') &&
         partToEval.size() > 1)) {
      // Add the value to the RPN queue
      rpnQueue.push_back(Misc::precise_to_string(Misc::modifier(partToEval)));
      // Identify the type as a value
      qType.push_back('V');
      // If it is not a digit, check that it is not a parameter
    } else if ([&]() {
                 // Check if the parameter exists
                 if (params.count(ParameterName(partToEval, subc)) != 0) {
                   if (params.at(ParameterName(partToEval, subc)).get_value()) {
                     return true;
                   }
                   return false;
                 }
                 return false;
               }()) {
      // If the parameter exists then add to the queue
      rpnQueue.push_back(Misc::precise_to_string(
          params.at(ParameterName(partToEval, subc)).get_value().value()));
      // Identify the type as a value
      qType.push_back('V');
      // Else check that it is not a function name such as SIN, COS, TAN, etc.
    } else if (std::find(funcs.begin(), funcs.end(), partToEval) !=
               funcs.end()) {
      // Add it to the operator stack if it is
      opStack.push_back(partToEval);
      // If not a function, check if it is not a defined constant
    } else if (Misc::string_constant(partToEval) != 0.0) {
      // Add this defined constant to the RPN stack
      rpnQueue.push_back(
          Misc::precise_to_string(Misc::string_constant(partToEval)));
      // Identify the type as a value
      qType.push_back('V');
      // If not a constant, check if it is an operator in the operator list
    } else if (partToEval.find_first_of("/*-+^") != std::string::npos) {
      // While the operator stack is not empty
      while (
          (!opStack.empty()) &&
          (((precedence_lvl(opStack.back()) == 4) ||
            (precedence_lvl(opStack.back()) >= precedence_lvl(partToEval))) &&
           (opStack.back().find_first_of("([{") == std::string::npos) &&
           (partToEval != "^"))) {
        // Add the operator to the RPN stack
        rpnQueue.push_back(opStack.back());
        // Identify the type as operator
        qType.push_back('O');
        // Remove the operator from the operator stack
        opStack.pop_back();
      }
      // Add the part to be evaluated to the operator stack
      opStack.push_back(partToEval);
      // Check parenthesis opening
    } else if (partToEval.find_first_of("([{") != std::string::npos) {
      // Add the parenthesis opening to the operator stack
      opStack.push_back(partToEval);
      // Check parenthesis close
    } else if (partToEval.find_first_of(")]}") != std::string::npos) {
      // While operator stack is not empty and the back is not a opening
      while ((!opStack.empty()) &&
             (opStack.back().find_first_of("([{") == std::string::npos)) {
        // Add the operator to the RPN stack
        rpnQueue.push_back(opStack.back());
        // Identify it as an operator
        qType.push_back('O');
        // Remove the operator from the operator stack
        opStack.pop_back();
      }
      // Id the stack is not empty and the back is a closing
      if ((!opStack.empty()) &&
          (opStack.back().find_first_of("([{") != std::string::npos)) {
        // Remove from the operator stack
        opStack.pop_back();
        // If this point is reached we have mismatched parenthesis
      } else {
        Errors::parsing_errors(ParsingErrors::MISMATCHED_PARENTHESIS, expr);
      }
      // Function is invalid or a parameter is used which is unparsed
    } else {
      if (!single) {
        // Return NaN to indicate this ocurred
        return std::numeric_limits<double>::quiet_NaN();
      } else {
        Errors::parsing_errors(ParsingErrors::UNIDENTIFIED_PART, expr);
      }
    }
    // Adjust the next part to be evaluated
    if (opLoc == 0) {
      // If the operator is at 0, substring the rest of the expression
      expToEval = expToEval.substr(1);
      // If operator is not found then the remaining expression is empty
    } else if (opLoc == -1) {
      expToEval = "";
      // Else substring until the next operator location
    } else {
      expToEval = expToEval.substr(opLoc);
    }
  }
  // If the remaining expresiotn is empty
  if (expToEval.empty())
    // If the operator stack is not empty (it should be)
    while (!opStack.empty()) {
      // If the last operator is an opening parenthesis
      if (opStack.back().find_first_of("([{") != std::string::npos)
        // We have mismatched parenthesis, complain
        Errors::parsing_errors(ParsingErrors::MISMATCHED_PARENTHESIS, expr);
      else {
        // Add the operator to the RPN stack
        rpnQueue.push_back(opStack.back());
        // Identify it as an operator
        qType.push_back('O');
        // Remove the operator from the operator stack
        opStack.pop_back();
      }
    }
  // Perform Reverse Polish Notation expansion on RPN stack
  while (rpnQueue.size() > 1) {
    // Clear the copies
    rpnQueueCopy.clear();
    qTypeCopy.clear();
    // Loop the the queue type
    for (int64_t i = 0; i < qType.size(); ++i) {
      // If the queue type is value
      if (qType[i] == 'V') {
        // Add the value to the copy of the RPN
        rpnQueueCopy.push_back(rpnQueue[i]);
        // Add the type to the copy of the qType
        qTypeCopy.push_back('V');
        // If the type is operator
      } else if (qType[i] == 'O') {
        // If the operator is the first item in the queue
        if (i == 0) {
          // We have an invalid queue type. Should always be VVO or VO
          Errors::parsing_errors(ParsingErrors::INVALID_RPN, expr);
          // If we are less than 2 deep in the stack
        } else if (i < 2) {
          // Remove the previous value from the copy
          rpnQueueCopy.pop_back();
          // Process the operator on the value and add to the copy
          rpnQueueCopy.push_back(Misc::precise_to_string(parse_operator(
              rpnQueue[i], 0, Misc::modifier(rpnQueue[i - 1]), popCount)));
          // Now proceed as normal with RPN expansion
        } else {
          // Evaluate the operator on the two values
          result = parse_operator(rpnQueue[i], Misc::modifier(rpnQueue[i - 2]),
                                  Misc::modifier(rpnQueue[i - 1]), popCount);
          // Remove the two values from the copy
          for (int64_t k = 0; k < popCount; ++k) {
            rpnQueueCopy.pop_back();
          }
          // If we removed two
          if (popCount == 2) {
            // Remove the type from the copy of type stack
            qTypeCopy.pop_back();
          }
          // Add the result value to the RPN queue
          rpnQueueCopy.push_back(Misc::precise_to_string(result));
        }
        // If the RPN queue is greater or equal to i
        if (rpnQueue.size() >= i) {
          // Insert the rest of the stacks to the copies
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
  // The final value should be the one we want, return this
  return Misc::modifier(rpnQueue.back());
}

double JoSIM::parse_operator(const std::string& op, double val1, double val2,
                             int64_t& popCount) {
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
    switch (op.at(0)) {
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

void JoSIM::parse_parameters(param_map& parameters) {
  // Double parsed value that will be stored for each parameter
  double value;
  // Counter to ensure that we do not get stuck in a loop
  int64_t parsedCounter = 0;
  // Parse parameters while counter is less than total parameter count
  while (parsedCounter < parameters.size()) {
    // Set previous counter to counter to do sanity check
    int64_t previous_counter = parsedCounter;
    // Loop through the parameters parsing them if possible
    for (auto& i : parameters) {
      // If the parameter does not yet have a value (double)
      if (!i.second.get_value()) {
        // Parse this parameter if expression if possible
        value = parse_param(i.second.get_expression(), parameters,
                            i.first.subcircuit(), false);
        // If the returned value is not NaN
        if (!std::isnan(value)) {
          // Set the parameter value (double) to the parsed value (double)
          i.second.set_value(value);
          // Increase the counter
          parsedCounter++;
        }
      }
    }
    // If we reach this then there are parameters that could not be parsed
    if (previous_counter == parsedCounter) {
      // Temporary string that will contain the parameter to complain about
      std::string unknownParams;
      // Loop through the parameters
      for (auto& i : parameters) {
        // If there are parameters with no value (double)
        if (!i.second.get_value()) {
          // Create a string with the name and subcircuit
          unknownParams +=
              i.first.name() + " " + i.first.subcircuit().value_or("") + "\n";
        }
      }
      // Complain about all the unknown parameters only once
      Errors::parsing_errors(ParsingErrors::UNIDENTIFIED_PART, unknownParams);
    }
  }
}

void JoSIM::update_parameters(param_map& parameters) {
  // Reset all the paraemeter values (doubles)
  for (auto i : parameters) {
    i.second.set_value(std::nan(""));
  }
  // Parse all the parameters again
  parse_parameters(parameters);
}

void JoSIM::expand_inline_parameters(std::pair<tokens_t, string_o>& s,
                                     param_map& parameters) {
  int_o oPos, cPos;
  // Loop through all the provided tokens, expanding any parameters
  for (int64_t i = 0; i < s.first.size(); ++i) {
    // If there exists a opening curly parenthesis, an expression exists
    if (s.first.at(i).find("{") != std::string::npos) {
      oPos = i;
    }
    // If there exists a closing curly parenthesis, an expression exists
    if (s.first.at(i).find("}") != std::string::npos) {
      cPos = i;
    }
  }
  // If an expression opening was found
  if (oPos) {
    //  Then a closing brace must exist
    if (!cPos) {
      // Complain if it doesn't
      Errors::parsing_errors(ParsingErrors::INVALID_DECLARATION,
                             Misc::vector_to_string(s.first));
    }
    if (cPos.value() != oPos.value()) {
      s.first.at(oPos.value()) =
          Misc::vector_to_string(tokens_t{s.first.begin() + oPos.value(),
                                          s.first.begin() + cPos.value() + 1},
                                 "");
      s.first.erase(s.first.begin() + oPos.value() + 1,
                    s.first.begin() + cPos.value() + 1);
    }
    tokens_t temp =
        Misc::tokenize(s.first.at(oPos.value()), "{}", true, true, 2);
    double value = parse_param(temp.back(), parameters, s.second, false);
    // If the returned value is not NaN
    if (std::isnan(value)) {
      // Complain of invalid parameter expression
      Errors::parsing_errors(ParsingErrors::UNIDENTIFIED_PART,
                             Misc::vector_to_string(s.first));
    }
    // Erase the expression part of the tokens
    s.first.erase(s.first.begin() + oPos.value(),
                  s.first.begin() + oPos.value() + 1);
    if (temp.size() > 1) {
      // Insert the double value in the place of the expression
      s.first.insert(
          s.first.begin() + oPos.value(),
          Misc::vector_to_string(
              tokens_t{temp.front(), Misc::precise_to_string(value)}, ""));
    } else {
      s.first.insert(s.first.begin() + oPos.value(),
                     Misc::precise_to_string(value));
    }
  }
  // If an expression closing was found
  if (cPos) {
    //  Then a opening brace must exist
    if (!oPos) {
      // Complain if it doesn't
      Errors::parsing_errors(ParsingErrors::INVALID_DECLARATION,
                             Misc::vector_to_string(s.first));
    }
  }
}