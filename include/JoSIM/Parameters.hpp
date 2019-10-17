// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_PARAMETERS_HPP
#define JOSIM_PARAMETERS_HPP

#include "JoSIM/ParameterName.hpp"

#include <string>
#include <vector>
#include <unordered_map>

class Parameter {
  private:
    std::string parameterName;
    std::string expression;
    std::string subcircuit;
    double value;
  public:
    Parameter() :
      subcircuit(""),
      value(0.0)
      { };
};

class Parameters {
public:
  std::vector<std::pair<std::string, std::string>> unparsedParams;
  std::unordered_map<JoSIM::ParameterName, double> parsedParams;
};

#endif