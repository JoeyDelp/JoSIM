// Copyright (c) 2018 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef J_SIMULATION_H_
#define J_SIMULATION_H_
#include "j_std_include.h"

#define TRANSIENT 0
#define DC 1
#define AC 2
#define PHASE 3
#define NONE_SPECIFIED 4

class Simulation
{
public:
	/*
		Identify simulation type. Be it transient, ac, dc or phase
	*/
	static
	void
	identify_simulation(InputFile& iFile);
	/*
		Perform transient simulation
	*/
	static
	void
	transient_voltage_simulation(InputFile& iFile);

	/*
		Perform transient simulation
	*/
	static
	void
	transient_phase_simulation(InputFile& iFile);
};
#endif