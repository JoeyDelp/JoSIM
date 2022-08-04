// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_PARAMETERS_HPP
#define JOSIM_PARAMETERS_HPP

#include <cmath>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "JoSIM/ParameterName.hpp"
#include "JoSIM/TypeDefines.hpp"

namespace JoSIM {

class Parameter {
 private:
  std::string expression_;
  double_o value_;

 public:
  Parameter(){};

  void set_expression(const std::string& s) { expression_ = s; };
  void reset_value() { value_ = std::nullopt; }
  void set_value(const double& v) { value_ = v; };

  std::string get_expression() const { return expression_; };
  double_o get_value() const { return value_; };
};

// Shorthand for long type
using param_map = std::unordered_map<ParameterName, Parameter>;

void expand_inline_parameters(std::vector<tokens_t, string_o>& s,
                              param_map& parameters);

void create_parameter(const tokens_t& s, param_map& parameters,
                      string_o subc = std::nullopt);

double parse_param(const std::string& expr, const param_map& params,
                   string_o subc = std::nullopt, bool single = true);

int64_t precedence_lvl(const std::string& op);

double parse_operator(const std::string& op, double val1, double val2,
                      int64_t& popCount);

void parse_parameters(param_map& parameters);

void update_parameters(param_map& parameters);

void expand_inline_parameters(std::pair<tokens_t, string_o>& s,
                              param_map& parameters);

}  // namespace JoSIM

#endif