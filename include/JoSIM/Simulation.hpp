// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_J_SIMULATION_H
#define JOSIM_J_SIMULATION_H

#include "./Errors.hpp"
#include "./Misc.hpp"
#include "./Matrix.hpp"

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
  void trans_sim_new(Input &iObj, Matrix &mObj);

  public:
  Results results;
  bool sOutput = true;

  Simulation(Input &iObj, Matrix &mObj) {
    trans_sim_new(iObj, mObj);
  }
};
} // namespace JoSIM
#endif
