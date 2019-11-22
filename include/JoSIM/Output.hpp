// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_J_OUTPUT_H
#define JOSIM_J_OUTPUT_H

#include "./Matrix.hpp"
#include "./Misc.hpp"
#include "./Simulation.hpp"
#include "./Input.hpp"

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

  static std::vector<std::vector<std::string>> write_output(const Input &iObj, const Matrix &mObj, const Simulation &sObj);

  static void format_csv_or_dat(const std::string &filename, const std::vector<std::vector<std::string>> &output, const char &delimiter);

  static void format_raw(const std::string &filename, const std::vector<std::vector<std::string>> &output);

  static void format_cout(const std::vector<std::vector<std::string>> &output);
};

#endif
