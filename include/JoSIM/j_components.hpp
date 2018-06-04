// Copyright (c) 2018 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef J_COMPONENTS_H
#define J_COMPONENTS_H
#include "j_std_include.hpp"

/*
Count the components
*/
void
count_component(std::string c, InputFile& iFile, std::string isSubCkt = "");
/*
Count the components in each subcircuit and add them to the respective
subcircuit component counts
*/
void
count_subcircuit_component(std::vector<std::string> c,
                           InputFile& iFile,
                           std::string isSubCkt = "");
/*
Return JJ parameters from tokens
*/
void
jj_comp(std::vector<std::string> tokens,
        double& jj_cap,
        double& jj_rn,
        double& jj_rzero,
        double& jj_icrit);
#endif