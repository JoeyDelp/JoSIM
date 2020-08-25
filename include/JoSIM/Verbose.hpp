// Copyright (c) 2020 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_J_VERBOSE_H
#define JOSIM_J_VERBOSE_H

#include "JoSIM/TypeDefines.hpp"
#include "JoSIM/Input.hpp"
#include "JoSIM/Matrix.hpp"

#include <string>
#include <vector>

namespace JoSIM {

namespace Verbose {
  void handle_verbosity(
    const int &vl, const Input &iObj, const Matrix &mObj);

  void print_circuit_stats(const Input &iObj, const Matrix &mObj);

  void print_parameters(const Input &iObj);

  void print_expanded_netlist(const Input &iObj);
}; // namespace Verbose

} // namespace JoSIM

#endif
