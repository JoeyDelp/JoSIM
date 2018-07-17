// Copyright (c) 2018 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef J_COMPONENTS_H
#define J_COMPONENTS_H
#include "j_std_include.hpp"

/*
Return JJ parameters from tokens
*/
void
jj_comp(std::vector<std::string> tokens,
        InputFile& iFile,
        int& jj_type,
        double& jj_cap,
        double& jj_rn,
        double& jj_rzero,
        double& jj_icrit,
        double& jj_rtype,
        double& jj_vgap);
#endif