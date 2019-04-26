// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_J_SIMULATION_H
#define JOSIM_J_SIMULATION_H
#include "j_errors.h"
#include "j_matrix.h"
#include "j_misc.h"
#include "j_std_include.h"


#define TRANSIENT 0
#define DC 1
#define AC 2
#define PHASE 3
#define NONE_SPECIFIED 4

class Results {
public:
  std::vector<std::vector<double>> xVect;
  std::vector<double> timeAxis;
  std::unordered_map<std::string, std::vector<double>> junctionCurrents;
};

class Simulation {
public:
  Results results;
  bool sOutput = true;

  void identify_simulation(const std::vector<std::string> &controls,
                           double &prstep, double &tstop, double &tstart,
                           double &maxtstep);

  void transient_voltage_simulation(Input &iObj, Matrix &mObj);

  void transient_phase_simulation(Input &iObj, Matrix &mObj);
};
#endif
