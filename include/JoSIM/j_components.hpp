// Copyright (c) 2018 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef J_COMPONENTS_H
#define J_COMPONENTS_H
#include "j_std_include.hpp"

/*
Return JJ parameters from tokens
*/
std::unordered_map<std::string, double>
jj_comp(std::vector<std::string> tokens,
        InputFile& iFile,
        int& jj_type);
#endif