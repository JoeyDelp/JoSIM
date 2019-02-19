// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef J_VERBOSE_H_
#define J_VERBOSE_H_
#include "j_std_include.h"

class Verbose {
    public:
        static
        void 
        print_expanded_netlist(std::vector<std::pair<std::string, std::string>> expNetlist);

};

#endif
