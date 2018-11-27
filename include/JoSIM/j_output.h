// Copyright (c) 2018 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef J_OUTPUT_H_
#define J_OUTPUT_H_
#include "j_std_include.h"

class Output {
public:
	/*
		Function that writes the output file as requested by the user
	*/
 	static
	void 
	write_data(InputFile &iFile);
	/*
		Function that writes a legacy output file in JSIM_N format
	*/
 	static
	void 
	write_legacy_data(InputFile &iFile);
	/*
		Function that writes a wr output file for opening in WRspice
	*/
 	static
	void 
	write_wr_data(InputFile &iFile);
	/*
		Function that writes the output to cout as requested by the user
	*/
 	static
	void 
	write_cout(InputFile& iFile);
};
#endif