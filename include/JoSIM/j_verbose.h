// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_J_VERBOSE_H
#define JOSIM_J_VERBOSE_H
#include "j_std_include.h"

class Verbose {
public:
  static void print_expanded_netlist(
      const std::vector<std::pair<std::string, std::string>> &expNetlist);
};

#endif
