// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_J_OUTPUT_H
#define JOSIM_J_OUTPUT_H

#include "./Matrix.hpp"
#include "./Misc.hpp"
#include "./Simulation.hpp"
#include "./Input.hpp"

namespace JoSIM {
class Trace {
  public:
  std::string name_;
  char type_;
  std::vector<double> data_;
  Trace(const std::string &name) {
    name_ = name;
  }
  ~Trace(){};
};

class Output {
  public:
  std::vector<Trace> traces;
  std::vector<double> timesteps;
  Output(){};
  void write_output(const Input &iObj, 
    const Matrix &mObj, const Simulation &sObj);

  void format_csv_or_dat(const std::string &filename, const char &delimiter);

  void format_raw(const std::string &filename);

  void format_cout();
};
} // namespace JoSIM

#endif
