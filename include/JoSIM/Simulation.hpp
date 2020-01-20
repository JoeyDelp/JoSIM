// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_J_SIMULATION_H
#define JOSIM_J_SIMULATION_H

#include "./Errors.hpp"
#include "./Misc.hpp"
#include "./Matrix.hpp"


#define TRANSIENT 0
#define DC 1
#define AC 2
#define PHASE 3
#define NONE_SPECIFIED 4

class Results {
public:
  std::vector<std::optional<std::vector<double>>> xVector;
  std::vector<double> timeAxis;
  std::unordered_map<std::string, std::vector<double>> junctionCurrents;

};

namespace JoSIM{
class Simulation {
public:
  Results results;
  bool sOutput = true;

  void trans_sim_new(JoSIM::Input &iObj, JoSIM::Matrix &mObj);
};
}
#endif
