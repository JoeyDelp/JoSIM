// Copyright (c) 2020 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_RESISTOR_HPP
#define JOSIM_RESISTOR_HPP

#include "JoSIM/Components.hpp"
#include "JoSIM/ParameterName.hpp"
#include "JoSIM/Parameters.hpp"
#include "JoSIM/AnalysisType.hpp"

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <optional>

namespace JoSIM {

class Resistor : public BasicComponent {
  public:
  double_o pn2_;

  Resistor(
    const std::pair<tokens_t, string_o> &s, const NodeConfig &ncon,
    const nodemap &nm, std::unordered_set<std::string> &lm, nodeconnections &nc,
    const param_map &pm, const AnalysisType &at, const double &h, int &bi);
}; // class Resistor

} // namespace JoSIM
#endif