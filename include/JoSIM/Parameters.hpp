// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_PARAMETERS_HPP
#define JOSIM_PARAMETERS_HPP

#include "JoSIM/ParameterName.hpp"

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

class Parameter {
  private:
    std::string expression_;
    std::optional<double> value_;

  public:
    Parameter() {};

    void set_expression(const std::string &s) { expression_ = s; };
    void set_value(const double &v) { value_ = v; };

    std::string get_expression() const { return expression_; };
    std::optional<double> get_value() const { return value_; };
};

namespace JoSIM {
class Parameters {
public:
  static void create_parameter(const std::pair<std::string, std::string> &s,
                              std::unordered_map<JoSIM::ParameterName, Parameter> &parameters);

  static double parse_param(
      const std::string &expr,
      const std::unordered_map<JoSIM::ParameterName, Parameter> &params,
      const std::string &subckt = "");

  static int prec_lvl(const std::string &op);

  static double parse_operator(const std::string &op, double val1, double val2,
                              int &popCount);

  static void parse_parameters(std::unordered_map<JoSIM::ParameterName, Parameter> &parameters);
};
}

#endif