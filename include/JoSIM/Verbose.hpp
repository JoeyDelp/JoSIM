// Copyright (c) 2020 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_J_VERBOSE_H
#define JOSIM_J_VERBOSE_H

#include "JoSIM/TypeDefines.hpp"

#include <string>
#include <vector>

namespace JoSIM {
namespace Verbose {
  void print_expanded_netlist(
    const std::vector<std::pair<tokens_t, string_o>> &expNetlist);
};
} // namespace JoSIM

#endif
