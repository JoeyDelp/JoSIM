// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_J_OUTPUT_H
#define JOSIM_J_OUTPUT_H
#include "j_input.h"
#include "j_matrix.h"
#include "j_misc.h"
#include "j_simulation.h"
#include "j_std_include.h"


class Trace {
public:
  std::string name;
  char type;
  bool pointer;
  std::vector<double> *traceData = nullptr;
  std::vector<double> calcData;
  Trace() { pointer = false; };
  ~Trace(){};
};

class Output {
public:
  std::vector<Trace> traces;
  std::vector<double> *timesteps = nullptr;
  Output(){};

  void relevant_traces(Input &iObj, Matrix &mObj, Simulation &sObj);

  void write_data(std::string &outname, const Matrix &mObj, const Simulation &sObj);

  void write_legacy_data(std::string &outname, const Matrix &mObj, const Simulation &sObj);

  void write_wr_data(std::string &outname);

  void write_cout(const Matrix &mObj, const Simulation &sObj);

  void handle_voltage(const std::vector<std::string> &devToHandle, Trace &result, const Input &iObj, const Matrix &mObj, const Simulation &sObj);

  void handle_current(const std::vector<std::string> &devToHandle, Trace &result, const Input &iObj, const Matrix &mObj, const Simulation &sObj);

  void handle_phase(const std::vector<std::string> &devToHandle, Trace &result, const Input &iObj, const Matrix &mObj, const Simulation &sObj);
};

#endif
