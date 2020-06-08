// Copyright (c) 2020 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/Verbose.hpp"
#include "JoSIM/Misc.hpp"

#include <iostream>

using namespace JoSIM;

void Verbose::print_expanded_netlist(
  const std::vector<std::pair<tokens_t, string_o>> &expNetlist) {
  std::cout << "Printing expanded netlist:" << std::endl;
  // Print the entire expanded netlist to standard output
  for (const auto &i : expNetlist) {
    std::cout << Misc::vector_to_string(i.first) << std::endl;
  }
  std::cout << std::endl;
}
