// Copyright (c) 2020 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_FUNCTION_HPP
#define JOSIM_FUNCTION_HPP

#include "Input.hpp"

#include <vector>

namespace JoSIM {
  namespace Function {
    std::vector<double> parse_function(
      const std::string &str, Input &iObj, const string_o &subckt);  
    void parse_pwl(
      const std::vector<std::string> &tokens, std::vector<double> &functionOfT, 
      const Input &iObj, const string_o &subckt);
    void parse_pulse(
      const std::vector<std::string> &tokens, std::vector<double> &functionOfT, 
      const Input &iObj, const string_o &subckt);
    void parse_sinusoid(
      const std::vector<std::string> &tokens, std::vector<double> &functionOfT, 
      const Input &iObj, const string_o &subckt); 
    void parse_custom(
      const std::vector<std::string> &tokens, std::vector<double> &functionOfT, 
      const Input &iObj, const string_o &subckt);
    void parse_noise(
      const std::vector<std::string> &tokens, std::vector<double> &functionOfT, 
      const Input &iObj, const string_o &subckt);
    void parse_pws(
      const std::vector<std::string> &tokens, std::vector<double> &functionOfT, 
      const Input &iObj, const string_o &subckt);  
    void voltage_to_phase(std::vector<double> &source, const Input &iObj);
    void phase_to_voltage(std::vector<double> &source, const Input &iObj);
  }
} // namespace JoSIM

#endif // JOSIM_FUNCTION_HPP
