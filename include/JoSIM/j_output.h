// Copyright (c) 2018 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef J_OUTPUT_H_
#define J_OUTPUT_H_
#include "j_std_include.h"
#include "j_input.h"
#include "j_simulation.h"
#include "j_matrix.h"
#include "j_plot.h"
#include "j_misc.h"

class Trace {
    public:
        std::string name;
        char type;
        bool source;
        bool calcul;
        std::vector<double> *traceData = new std::vector<double>;
        std::vector<double> calcData;
        Trace() {
            source = false;
            calcul = false;
        };
};

class Output {
    public:
        std::vector<Trace> traces;
        Plot plot;
        Output() {};

        void
        relevant_traces(Input &iObj, Matrix &mObj, Simulation &sObj);

        void 
        write_data(std::vector<Trace> traces, Simulation &sObj);

        void 
        write_legacy_data(std::vector<Trace> traces, Simulation &sObj);

        void 
        write_wr_data(std::vector<Trace> traces, Simulation &sObj);

        void 
        write_cout(std::vector<Trace> traces, Simulation &sObj);
};

// class Output {
// public:
// 	/*
// 		Function that writes the output file as requested by the user
// 	*/
//  	static
// 	void 
// 	write_data(InputFile &iFile);
// 	/*
// 		Function that writes a legacy output file in JSIM_N format
// 	*/
//  	static
// 	void 
// 	write_legacy_data(InputFile &iFile);
// 	/*
// 		Function that writes a wr output file for opening in WRspice
// 	*/
//  	static
// 	void 
// 	write_wr_data(InputFile &iFile);
// 	/*
// 		Function that writes the output to cout as requested by the user
// 	*/
//  	static
// 	void 
// 	write_cout(InputFile& iFile);
// };
#endif