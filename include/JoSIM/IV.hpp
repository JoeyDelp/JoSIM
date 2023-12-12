// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_IV_HPP
#define JOSIM_IV_HPP

#include "Input.hpp"
#include "Matrix.hpp"
#include "Model.hpp"
#include "Simulation.hpp"

namespace JoSIM {

class IV {
 public:
  IV(const Input iObj);
  void setup_iv(const tokens_t& i, const Input& iObj);
  std::vector<std::pair<double, double>> generate_iv(double maxC, Input ivInp);
  std::pair<double, double> do_simulate(Input& ivInp, Matrix& ivMat);
  void write_iv(std::vector<std::pair<double, double>>& iv_data,
                const std::string& output_path);
};

}  // namespace JoSIM

#endif  // JOSIM_IV_HPP
