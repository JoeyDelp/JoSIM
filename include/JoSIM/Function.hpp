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

  enum class FunctionType { 
    PWL = 0, 
    PULSE = 1, 
    SINUSOID = 2, 
    CUS = 3 ,
    NOISE = 4,
    PWS = 5,
    DC = 6,
    EXP = 7
  };

  class FunctObj {
    private:
      FunctionType fType_;
      std::vector<double> timeValues_;
      std::vector<double> ampValues_;
      void parse_pwl(const tokens_t &t, const Input &iObj, const string_o &s);
      void parse_pulse(const tokens_t &t, const Input &iObj, const string_o &s);
      void parse_sin(const tokens_t &t, const Input &iObj, const string_o &s); 
      void parse_cus(const tokens_t &t, const Input &iObj, const string_o &s);
      void parse_noise(const tokens_t &t, const Input &iObj, const string_o &s);
      void parse_dc(const tokens_t &t, const Input &iObj, const string_o &s);
      void parse_exp(const tokens_t &t, const Input &iObj, const string_o &s);
      double return_pwl(const double &x);
      double return_pulse(const double &x);
      double return_sin(const double &x);
      double return_cus(const double &x);
      double return_noise(const double &x);
      double return_pws(const double &x);
      double return_dc();
      double return_exp(const double &x);
    public:
      FunctObj();
      void parse_function(const std::string &str, const Input &iObj, 
        const string_o &subckt);
      double value(const double &x);  

  }; // class Function

} // namespace JoSIM

#endif // JOSIM_FUNCTION_HPP
