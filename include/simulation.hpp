#pragma once
#include "std_include.hpp"

#define TRANS 0
#define DC 1
#define AC 2
#define PHASE 3
#define NONE 4

class trans_sim {
public:
  double prstep;
  double tstop;
  double tstart;
  double maxtstep;
  trans_sim() {
	  tstart = 0.0;
	  maxtstep = 1E-12;
  }
	double simsize() { return (tstop - tstart) / maxtstep; }
};

extern trans_sim tsim;

/* 
  Identify simulation type. Be it transient, ac, dc or phase
*/
void identify_simulation(InputFile& iFile);
/*
	Perform transient simulation
*/
void transient_simulation();