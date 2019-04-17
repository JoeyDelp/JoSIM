// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef J_OUTPUT_H_
#define J_OUTPUT_H_
#include "j_std_include.h"
#include "j_input.h"
#include "j_simulation.h"
#include "j_matrix.h"
#include "j_misc.h"

class Trace {
    public:
        std::string name;
        char type;
        bool pointer;
        std::vector<double> *traceData = nullptr;
        std::vector<double> calcData;
        Trace() {
            pointer = true;
        };
        ~Trace() {
        };
};

class Output {
    public:
        std::vector<Trace> traces;
        std::vector<double> *timesteps = nullptr;
        Output() {};

        void
        relevant_traces(Input &iObj, Matrix &mObj, Simulation &sObj);

        void
        write_data(std::string &outname);

        void
        write_legacy_data(std::string &outname);

        void
        write_wr_data(std::string &outname);

        void
        write_cout(Matrix &mObj, Simulation &sObj);
};

#endif
