// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_CCVS_HPP
#define JOSIM_CCVS_HPP

#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "JoSIM/AnalysisType.hpp"
#include "JoSIM/BasicComponent.hpp"
#include "JoSIM/Input.hpp"
#include "JoSIM/ParameterName.hpp"
#include "JoSIM/Parameters.hpp"

namespace JoSIM {

/*
 Hlabel Vo⁺ Vo⁻ Vc⁺ Vc⁻ G

 Vo = GIc

 ⎡ 0  0  0  0  0  1⎤ ⎡Vo⁺⎤   ⎡ 0⎤
 ⎜ 0  0  0  0  0 -1⎟ ⎜Vo⁻⎟   ⎜ 0⎟
 ⎜ 0  0  0  0  1  0⎟ ⎜Vc⁺⎟ = ⎜ 0⎟
 ⎜ 0  0  0  0 -1  0⎟ ⎜Vc⁻⎟   ⎜ 0⎟
 ⎜ 1 -1  0  0 -G  0⎟ ⎜Ic ⎟   ⎜ 0⎟
 ⎣ 0  0  1 -1  0  0⎦ ⎣Io ⎦   ⎣ 0⎦

 (PHASE)
 φ - (2e/hbar)(2h/3G)Ic = (4/3)φn-1 - (1/3)φn-2

 ⎡ 0  0  0  0                 0  1⎤ ⎡φo⁺⎤   ⎡                     0⎤
 ⎜ 0  0  0  0                 0 -1⎟ ⎜φo⁻⎟   ⎜                     0⎟
 ⎜ 0  0  0  0                 1  0⎟ ⎜φc⁺⎟ = ⎜                     0⎟
 ⎜ 0  0  0  0                -1  0⎟ ⎜φc⁻⎟   ⎜                     0⎟
 ⎜ 1 -1  0  0 -(2e/hbar)(2h/3G)  0⎟ ⎜Ic ⎟   ⎜ (4/3)φn-1 - (1/3)φn-2⎟
 ⎣ 0  0  1 -1                 0  0⎦ ⎣Io ⎦   ⎣                     0⎦
*/

class CCVS : public BasicComponent {
 private:
  int64_t hDepPos_ = 0;
  JoSIM::AnalysisType at_;

 public:
  NodeConfig nodeConfig2_;
  int_o posIndex2_, negIndex2_;
  int64_t currentIndex2_ = 0;
  double pn1_ = 0.0, pn2_ = 0.0, pn3_ = 0.0, pn4_ = 0.0;

  CCVS(const std::pair<tokens_t, string_o>& s, const NodeConfig& ncon,
       const std::optional<NodeConfig>& ncon2, const nodemap& nm,
       std::unordered_set<std::string>& lm, nodeconnections& nc,
       const param_map& pm, int64_t& bi, const AnalysisType& at,
       const double& h);

  void set_node_indices(const tokens_t& t, const nodemap& nm,
                        nodeconnections& nc);
  void set_matrix_info(const AnalysisType& at, const double& h);

  void update_timestep(const double& factor) override;

  void step_back() override { pn2_ = pn4_; }

};  // class CCVS

}  // namespace JoSIM
#endif