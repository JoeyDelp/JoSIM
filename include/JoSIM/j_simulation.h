// Copyright (c) 2018 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef J_SIMULATION_H_
#define J_SIMULATION_H_
#include "j_std_include.h"
#include "j_errors.h"
#include "j_misc.h"
#include "j_matrix.h"

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

		Results() {};

};

class Simulation {
	public:
		Results results;

		void 
		identify_simulation(std::vector<std::string> controls,
							double &prstep,
							double &tstop,
							double &tstart,
							double &maxtstep);

		Simulation() {};

		void
		transient_voltage_simulation(Input &iObj, Matrix &mObj);

		void
		transient_phase_simulation(Input &iObj, Matrix &mObj);
};
#endif