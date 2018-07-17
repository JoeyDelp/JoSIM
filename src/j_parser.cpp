// Copyright (c) 2018 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#include "j_parser.hpp"

// Possible functions that can be called
std::string funcsArray[] = { "COS",  "SIN",  "TAN",   "ACOS",  "ASIN",  "ATAN",
                             "COSH", "SINH", "TANH",  "ACOSH", "ASINH", "ATANH",
                             "EXP",  "LOG",  "LOG10", "SQRT",  "CBRT" };
std::vector<std::string> funcs(funcsArray,
                               funcsArray +
                                 sizeof(funcsArray) / sizeof(std::string));

void
parse_expression(std::string expName, std::string expr, std::unordered_map<std::string, double>& parVal, std::unordered_map<std::string, double>& globalParVal, std::string subckt)
{
  if (parVal.find("expName") != parVal.end())
    parsing_errors(EXPRESSION_ARLEADY_DEFINED, expName);
  std::string expToEval = expr;
  std::vector<std::string> rpnQueue, rpnQueueCopy, opStack;
  std::vector<char> qType, qTypeCopy;
  std::string partToEval;
  int opLoc, popCount = 0;
  double result = 0.0;
  // While there are tokens to read
  while (!expToEval.empty()) {
    // Read a token
    opLoc = expToEval.find_first_of("/*-+(){}[]^");
    // Check to make sure it's not scientific notation
    if (expToEval[opLoc] == '-')
      if (opLoc != 0)
        if (expToEval[opLoc - 1] == 'E')
          opLoc = expToEval.find_first_of("/*-+(){}[]^", opLoc + 1);
    // If operator location is zero substring after the operator
    if (opLoc == 0)
      partToEval = expToEval.substr(0, opLoc + 1);
    else
      partToEval = expToEval.substr(0, opLoc);
    // If token is a number
    if (isdigit(partToEval[0])) {
      // Push number to RPN Queue
      rpnQueue.push_back(precise_to_string(modifier(partToEval)));
      qType.push_back('V');
    }
    // Else if token is a variable with a value
    else if ((parVal.find(partToEval) != parVal.end()) ||
             (globalParVal.find(partToEval) != globalParVal.end())) {
      // Check if subckt variable or not
      if (subckt != "NONE") {
        // Push variable to RPN Queue
        rpnQueue.push_back(
          precise_to_string(parVal[partToEval]));
        qType.push_back('V');
      } else {
        // Push variable to RPN Queue
        rpnQueue.push_back(precise_to_string(parVal[partToEval]));
        qType.push_back('V');
      }
    }
    // Else if token is a function
    else if (std::find(funcs.begin(), funcs.end(), partToEval) != funcs.end())
      // Push to Operator Stack
      opStack.push_back(partToEval);
    // Else if operator
    else if (partToEval.find_first_of("/*-+^") != std::string::npos) {
      /* While (((Function at top of operator stack) OR (Operator at top of
         operator stack with greater precedence) OR (Operator at top of operator
         stack with equal precedence and left associative)) AND (Operator at top
         of operator stack is not a left bracket))
     */
      while ((!opStack.empty()) &&
             (((prec_lvl(opStack.back()) == 4) ||
               (prec_lvl(opStack.back()) >= prec_lvl(partToEval))) &&
              (opStack.back().find_first_of("([{") == std::string::npos) &&
              (partToEval != "^"))) {
        // Pop operators from the operator stack onto RPN Queue
        rpnQueue.push_back(opStack.back());
        qType.push_back('O');
        opStack.pop_back();
      }
      // Push operator to stack
      opStack.push_back(partToEval);
    }
    // Else if token is left bracket
    else if (partToEval.find_first_of("([{") != std::string::npos)
      // Push to operator stack
      opStack.push_back(partToEval);
    // Else if token is right bracket
    else if (partToEval.find_first_of(")]}") != std::string::npos) {
      // While operator at top of operator stack is not left bracket
      while ((!opStack.empty()) &&
             (opStack.back().find_first_of("([{") == std::string::npos)) {
        // Pop operator from operator stack to RPN Queue
        rpnQueue.push_back(opStack.back());
        qType.push_back('O');
        opStack.pop_back();
      }
      // Pop the left bracket from the operator stack
      if ((!opStack.empty()) &&
          (opStack.back().find_first_of("([{") != std::string::npos))
        opStack.pop_back();
      else
        parsing_errors(MISMATCHED_PARENTHESIS, expr);
    }
    // Else unknown function
    else
      parsing_errors(UNIDENTIFIED_PART, partToEval);
    // If operator location is zero substring after the operator
    if (opLoc == 0)
      expToEval = expToEval.substr(opLoc + 1);
    if (opLoc == -1)
      expToEval = "";
    else
      expToEval = expToEval.substr(opLoc);
  }
  // If there are no more tokens to read
  if (expToEval.empty())
    // While there are still operators on the operator stack
    while (!opStack.empty()) {

      if (opStack.back().find_first_of("([{") != std::string::npos)
        parsing_errors(MISMATCHED_PARENTHESIS, expr);
      else {
        // Pop the operator from the operator stack to the RPN Queue
        rpnQueue.push_back(opStack.back());
        qType.push_back('O');
        opStack.pop_back();
      }
    }
  // Now that we have the RPN notation for the expression we can solve it and
  // store it
  while (rpnQueue.size() > 1) {
    // Clear copies
    rpnQueueCopy.clear();
    qTypeCopy.clear();
    // Loop through the type queue looking for operators
    for (int i = 0; i < qType.size(); i++) {
      // If a value is identified
      if (qType[i] == 'V') {
        // Push the value to the RPN Queue Copy
        rpnQueueCopy.push_back(rpnQueue[i]);
        // Push the value type to the type queue copy
        qTypeCopy.push_back('V');
      }
      // Else if an operator is identified
      else if (qType[i] == 'O') {
        // If i is 0 then no values only operator meaning invalid RPN
        if (i == 0)
          parsing_errors(INVALID_RPN, expr);
        // Else if i is less than 2 (meaning only 1 value identified)
        else if (i < 2) {
          rpnQueueCopy.pop_back();
          rpnQueueCopy.push_back(precise_to_string(parse_operator(
            rpnQueue[i], 0, modifier(rpnQueue[i - 1]), popCount)));
        }
        // Else 2 or more values identified
        else {
          result = parse_operator(rpnQueue[i],
                                  modifier(rpnQueue[i - 2]),
                                  modifier(rpnQueue[i - 1]),
                                  popCount);
          for (int k = 0; k < popCount; k++)
            rpnQueueCopy.pop_back();
          if (popCount == 2)
            qTypeCopy.pop_back();
          rpnQueueCopy.push_back(precise_to_string(result));
        }
        // Append the rest of the Queues if there is anything left
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
  parVal[expName] = modifier(rpnQueue.back());
}

int
prec_lvl(std::string op)
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
parse_operator(std::string op, double val1, double val2, int& popCount)
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
