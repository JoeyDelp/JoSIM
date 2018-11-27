#ifndef J_ARGS_H_
#define J_ARGS_H_
#include "j_std_include.h"

class Args {
    public:
        int analysisT, saveType, sbcktConv;
        bool plotRes, saveRes, verbose, dev;
        std::string inName;
        std::string outName;

        Args(){
            analysisT = VANALYSIS; 
            saveType = CSV;
            sbcktConv = LEFT;
            plotRes = false;
            saveRes = false;
            verbose = false;
            dev = false;
        };

        void parse_arguments(int argc, char* argv[]);
        void display_help();
        void version_info();
};

extern Args cArg;

#endif