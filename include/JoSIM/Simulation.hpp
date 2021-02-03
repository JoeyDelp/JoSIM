// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_SIMULATION_H
#define JOSIM_SIMULATION_H

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
  std::vector<std::optional<std::vector<float>>> xVector;
  std::vector<float> timeAxis;
};

class Simulation {
  private:
  std::vector<float> x_, b_, xn2_, xn3_;
  int simSize_;
  JoSIM::AnalysisType atyp_;
  bool minOut_;
  bool needsLU_;
  bool needsTR_;
  float stepSize_, prstep_, prstart_;
  int simOK_;
  klu_symbolic *Symbolic_;
  klu_common Common_;
  klu_numeric *Numeric_;

  void trans_sim(Matrix &mObj);
  void setup_b(Matrix &mObj, int i, float step, float factor = 1);
  void reduce_step(Matrix &mObj, float factor, 
    int &stepCount, float currentStep);
  
  void handle_cs(Matrix &mObj, float &step, const int &i);
  void handle_resistors(Matrix &mObj);
  void handle_inductors(Matrix &mObj, float factor = 1);
  void handle_capacitors(Matrix &mObj);
  void handle_jj(Matrix &mObj, int &i, float &step, float factor = 1);
  void handle_vs(Matrix &mObj, const int &i, float &step, float factor = 1);
  void handle_ps(Matrix &mObj, const int &i, float &step, float factor = 1);
  void handle_ccvs(Matrix &mObj);
  void handle_vccs(Matrix &mObj);
  void handle_tx(Matrix &mObj, const int &i, float &step, float factor = 1);

  public:
  Results results;

  Simulation(Input &iObj, Matrix &mObj);
};
} // namespace JoSIM
#endif