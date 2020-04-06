// Copyright (c) 2020 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/Verbose.hpp"

#include <iostream>

using namespace JoSIM;

void Verbose::print_expanded_netlist(
    const std::vector<std::pair<std::string, std::string>> &expNetlist) {
  std::cout << "Printing expanded netlist:" << std::endl;
  for (const auto &i : expNetlist) {
    std::cout << i.first << std::endl;
  }
  std::cout << std::endl;
}
