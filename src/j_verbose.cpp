// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#include "j_verbose.h"

void 
Verbose::print_expanded_netlist(
    std::vector<std::pair<std::string, std::string>> expNetlist){
        std::cout << "Printing expanded netlist:" << std::endl;
        for(const auto& i : expNetlist) {
            std::cout << i.first << std::endl;
        }
        std::cout << std::endl;
}
