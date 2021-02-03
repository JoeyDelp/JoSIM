// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_FUNCTION_HPP
#define JOSIM_FUNCTION_HPP

#include "Input.hpp"

#include <vector>

namespace JoSIM {

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

  class Function {
    private:
      FunctionType fType_;
      std::vector<float> timeValues_;
      std::vector<float> ampValues_;
      void parse_pwl(const tokens_t &t, const Input &iObj, const string_o &s);
      void parse_pulse(const tokens_t &t, const Input &iObj, const string_o &s);
      void parse_sin(const tokens_t &t, const Input &iObj, const string_o &s); 
      void parse_cus(const tokens_t &t, const Input &iObj, const string_o &s);
      void parse_noise(const tokens_t &t, const Input &iObj, const string_o &s);
      void parse_dc(const tokens_t &t, const Input &iObj, const string_o &s);
      void parse_exp(const tokens_t &t, const Input &iObj, const string_o &s);
      float return_pwl(float &x);
      float return_pulse(float &x);
      float return_sin(float &x);
      float return_cus(float &x);
      float return_noise(float &x);
      float return_pws(float &x);
      float return_dc();
      float return_exp(float &x);
    public:
      Function() {};
      void parse_function(const std::string &str, const Input &iObj, 
        const string_o &subckt);
      float value(float x);  

  }; // class Function

} // namespace JoSIM

#endif // JOSIM_FUNCTION_HPP
