// Copyright (c) 2018 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef J_SIMULATION_H
#define J_SIMULATION_H
#include "j_std_include.hpp"

#define TRANSIENT 0
#define DC 1
#define AC 2
#define PHASE 3
#define NONE_SPECIFIED 4

/* RCSJ simulation object*/
class rcsj_sim_object
{
public:
	int vPositive, vNegative, bPhase;
	std::string label, positiveNodeRow, negativeNodeRow;
	int jjRtype, currentState, previousState, mptrPP, mptrPN, mptrNP, mptrNN;
	bool superconducting;
	double jjIcrit, jjCap, jjVg, jjRzero, jjRn, VB, VB_Prev, VB_dt, VB_dt_Prev, VB_Guess, Phase,
		Phase_Guess, Phase_Prev, Is, delV, transitionCurrent, gLarge, middle, upper, subCond, transCond, normalCond;
	rcsj_sim_object()
	{
		previousState = SUBGAP;
		currentState = SUBGAP;
		superconducting = true;
		vPositive = -1;
		vNegative = -1;
		bPhase = -1;
		VB_Prev = 0.0;
		VB_dt = 0.0;
		VB_dt_Prev = 0.0;
		VB_Guess = 0.0;
		Phase = 0.0;
		Phase_Guess = 0.0;
		Phase_Prev = 0.0;
		Is = 0.0;
		delV = 0.1E-3;
		transitionCurrent = 0.0;
		gLarge = 0.0;
		mptrPP = -1;
		mptrPN = -1;
		mptrNP = -1;
		mptrNN = -1;
	}
};

/*
  Identify simulation type. Be it transient, ac, dc or phase
*/
void
identify_simulation(InputFile& iFile);
/*
		Perform transient simulation
*/
void
transient_simulation(InputFile& iFile);
#endif