// Copyright (c) 2020 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_TRANSMISSIONLINE_HPP
#define JOSIM_TRANSMISSIONLINE_HPP

#include "JoSIM/BasicComponent.hpp"
#include "JoSIM/ParameterName.hpp"
#include "JoSIM/Parameters.hpp"
#include "JoSIM/AnalysisType.hpp"

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <optional>

namespace JoSIM {

class TransmissionLine : public BasicComponent {
  public:
  NodeConfig nodeConfig2_;
  int_o posIndex2_, negIndex2_;
  int currentIndex2_;
  double p1n2_, p2n2_, p1nk2_, p2nk2_;
  double dp1n1_, dp1n2_, dv2n1_, dp2n2_;
  double dp1nk_, dp1nk1_, dp2nk_, dp2nk1_;
  int timestepDelay_;

  TransmissionLine(
    const std::pair<tokens_t, string_o> &s, const NodeConfig &ncon, 
    const std::optional<NodeConfig> &ncon2, const nodemap &nm, 
    std::unordered_set<std::string> &lm, nodeconnections &nc, 
    const param_map &pm, const AnalysisType &at, const double &h, int &bi);

  void set_secondary_node_indices(
    const tokens_t &t, const nodemap &nm, nodeconnections &nc);
  void set_secondary_matrix_info();
}; // class TransmissionLine

} // namespace JoSIM

#endif