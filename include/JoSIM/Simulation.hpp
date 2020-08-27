// Copyright (c) 2020 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_J_SIMULATION_H
#define JOSIM_J_SIMULATION_H

#include "JoSIM/Errors.hpp"
#include "JoSIM/Misc.hpp"
#include "JoSIM/Matrix.hpp"

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
  std::vector<double> x_, x_prev_, b_;
  int simSize_;
  JoSIM::AnalysisType atyp_;
  bool minOut_;
  bool needsLU_;
  double stepSize_;

  void trans_sim(Matrix &mObj);
  void setup_rhs(Matrix &mObj);
  void reduce_step(Matrix &mObj, const int &factor);
  
  void handle_cs(const Matrix &mObj, const int &i);
  void handle_resistors(Matrix &mObj);
  void handle_inductors(Matrix &mObj);
  void handle_capacitors(Matrix &mObj);
  void handle_jj(Matrix &mObj, const int &i);
  void handle_vs(Matrix &mObj, const int &i);
  void handle_ps(Matrix &mObj, const int &i);
  void handle_ccvs(Matrix &mObj);
  void handle_vccs(Matrix &mObj);
  void handle_tx(Matrix &mObj, const int &i);

  public:
  Results results;

  Simulation(Input &iObj, Matrix &mObj) {
    // Simulation setup
    simSize_ = iObj.transSim.get_simsize();
    atyp_ = iObj.argAnal;
    minOut_ = iObj.argMin;
    needsLU_ = false;
    stepSize_ = iObj.transSim.get_prstep();
    x_prev_.resize(mObj.branchIndex, 0.0);
    if(!mObj.relevantTraces.empty()) {
      results.xVector.resize(mObj.branchIndex);
      for (const auto &i : mObj.relevantIndices) {
        results.xVector.at(i).emplace();
      }
    } else {
      results.xVector.resize(mObj.branchIndex, std::vector<double>(0));
    }
    // Run transient simulation
    trans_sim(mObj);
  }
};
} // namespace JoSIM
#endif
