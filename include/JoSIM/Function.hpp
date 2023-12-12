// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_FUNCTION_HPP
#define JOSIM_FUNCTION_HPP

#include <vector>

#include "Input.hpp"

namespace JoSIM {

enum class FunctionType {
  PWL = 0,
  PULSE = 1,
  SINUSOID = 2,
  CUS = 3,
  NOISE = 4,
  PWS = 5,
  DC = 6,
  EXP = 7
};

class Function {
 private:
  FunctionType fType_ = FunctionType::PWL;
  std::vector<double> timeValues_;
  std::vector<double> ampValues_;
  std::vector<double> miscValues_;
  void parse_pwl(const tokens_t& t, const Input& iObj, const string_o& s);
  void parse_pulse(const tokens_t& t, const Input& iObj, const string_o& s);
  void parse_sin(const tokens_t& t, const Input& iObj, const string_o& s);
  void parse_cus(const tokens_t& t, const Input& iObj, const string_o& s);
  void parse_noise(const tokens_t& t, const Input& iObj, const string_o& s);
  void parse_dc(const tokens_t& t, const Input& iObj, const string_o& s);
  void parse_exp(const tokens_t& t, const Input& iObj, const string_o& s);
  double return_pwl(double& x);
  double return_pulse(double& x);
  double return_sin(double& x);
  double return_cus(double& x);
  double return_noise(double& x);
  double return_pws(double& x);
  double return_dc();
  double return_exp(double& x);

 public:
  Function(){};
  void parse_function(const std::string& str, const Input& iObj,
                      const string_o& subckt);
  double value(double x);
  void ampValues(std::vector<double> values);
  std::vector<double> ampValues() { return ampValues_; }
  void clearMisc() { miscValues_.clear(); }

};  // class Function

}  // namespace JoSIM

#endif  // JOSIM_FUNCTION_HPP
