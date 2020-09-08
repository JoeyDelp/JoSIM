// Copyright (c) 2020 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_J_SIMULATION_H
#define JOSIM_J_SIMULATION_H

#include "JoSIM/Errors.hpp"
#include "JoSIM/Misc.hpp"
#include "JoSIM/Matrix.hpp"

#include "suitesparse/klu.h"

#include <cassert>

namespace JoSIM{

#define TRANSIENT 0
#define DC 1
#define AC 2
#define PHASE 3
#define NONE_SPECIFIED 4


class Results {
  public:
  std::vector<std::optional<std::vector<double>>> xVector;
  std::vector<double> timeAxis;
};

class Simulation {
  private:
  std::vector<double> x_, b_;
  int simSize_;
  JoSIM::AnalysisType atyp_;
  bool minOut_;
  bool needsLU_;
  double stepSize_;
  int simOK_;
  klu_symbolic *Symbolic_;
  klu_common Common_;
  klu_numeric *Numeric_;

  void trans_sim(Matrix &mObj);
  void setup_b(Matrix &mObj, const int &i, const double factor = 1);
  void reduce_step(
    Matrix &mObj, const double &factor, 
    const int &stepCount, const double &currentStep);
  
  void handle_cs(const Matrix &mObj, const int &i);
  void handle_resistors(Matrix &mObj);
  void handle_inductors(Matrix &mObj, const double factor = 1);
  void handle_capacitors(Matrix &mObj);
  void handle_jj(Matrix &mObj, const int &i, const double factor = 1);
  void handle_vs(Matrix &mObj, const int &i, const double factor = 1);
  void handle_ps(Matrix &mObj, const int &i, const double factor = 1);
  void handle_ccvs(Matrix &mObj);
  void handle_vccs(Matrix &mObj);
  void handle_tx(Matrix &mObj, const int &i);

  public:
  Results results;

  Simulation(Input &iObj, Matrix &mObj);
};
} // namespace JoSIM
#endif