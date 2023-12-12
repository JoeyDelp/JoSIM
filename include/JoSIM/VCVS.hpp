// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_VCVS_HPP
#define JOSIM_VCVS_HPP

#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "JoSIM/AnalysisType.hpp"
#include "JoSIM/BasicComponent.hpp"
#include "JoSIM/ParameterName.hpp"
#include "JoSIM/Parameters.hpp"

namespace JoSIM {

/*
 Elabel Vo⁺ Vo⁻ Vc⁺ Vc⁻ G

 Vo = GVc
 ⎡ 0  0  0  0   -1⎤ ⎡Vo⁺⎤   ⎡ 0⎤
 ⎜ 0  0  0  0    1⎟ ⎜Vo⁻⎟   ⎜ 0⎟
 ⎜ 0  0  0  0    0⎟ ⎜Vc⁺⎟ = ⎜ 0⎟
 ⎜ 0  0  0  0    0⎟ ⎜Vc⁻⎟   ⎜ 0⎟
 ⎣-1  1  G -G    0⎦ ⎣Io ⎦   ⎣ 0⎦
*/

class VCVS : public BasicComponent {
 public:
  NodeConfig nodeConfig2_;
  int_o posIndex2_, negIndex2_;

  VCVS(const std::pair<tokens_t, string_o>& s, const NodeConfig& ncon,
       const std::optional<NodeConfig>& ncon2, const nodemap& nm,
       std::unordered_set<std::string>& lm, nodeconnections& nc,
       const param_map& pm, int64_t& bi);

  void set_node_indices(const tokens_t& t, const nodemap& nm,
                        nodeconnections& nc);
  void set_matrix_info();
};  // class VCVS

}  // namespace JoSIM
#endif