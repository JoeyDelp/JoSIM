#pragma once
#include "j_std_include.hpp"

class model_rcsj {
public:
	std::string modelname;
	int rtype;
	int cct;
	double vg;
	double delv;
	double icon;
	double rzero;
	double rnormal;
	double cap;
	double icrit;

	/* 
	  Default RCSJ model constructor 
	*/
	model_rcsj() {
		rtype = 0;
		cct = 0;
		vg = 2.8E-3;
		delv = 0.1E-3;
		icon = 1E-3;
		rzero = 30;
		rnormal = 5;
		cap = 2.5E-12;
		icrit = 1E-3;
	}
};

class model_rcsj_functions {
public:
	/*
	  Identify the models in both the subcircuits as well as the main design
	*/
	static void identify_models(InputFile &iFile, std::map<std::string, model_rcsj> &models);
};

extern std::map<std::string, model_rcsj> models;