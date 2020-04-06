// Copyright (c) 2020 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_PARAMETERS_HPP
#define JOSIM_PARAMETERS_HPP

#include "JoSIM/ParameterName.hpp"

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

namespace JoSIM {
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

  void create_parameter(const std::pair<std::string, std::string> &s,
                        std::unordered_map<ParameterName, Parameter> &parameters);

  double parse_param(const std::string &expr,
                      const std::unordered_map<ParameterName, Parameter> &params,
                      const std::string &subckt = "");

  int precedence_lvl(const std::string &op);

  double parse_operator(const std::string &op, 
                        double val1, 
                        double val2,
                        int &popCount);

  void parse_parameters(std::unordered_map<ParameterName, Parameter> &parameters);

  void update_parameters(std::unordered_map<ParameterName, Parameter> &parameters);

} // namespace JoSIM

#endif