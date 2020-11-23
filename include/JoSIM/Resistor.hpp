// Copyright (c) 2020 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_RESISTOR_HPP
#define JOSIM_RESISTOR_HPP

#include "JoSIM/BasicComponent.hpp"
#include "JoSIM/ParameterName.hpp"
#include "JoSIM/Parameters.hpp"
#include "JoSIM/AnalysisType.hpp"

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <optional>

namespace JoSIM {

 /*
  Rlabel V⁺ V⁻ R

  V = RIo
  ⎡ 0  0  1⎤ ⎡V⁺⎤   ⎡ 0⎤
  ⎜ 0  0 -1⎟ ⎜V⁻⎟ = ⎜ 0⎟
  ⎣ 1 -1 -R⎦ ⎣Io⎦   ⎣ 0⎦

  (PHASE)
  φ - R(2e/hbar)(2h/3)Io = (4/3)φn-1 - (1/3)φn-2
  
  ⎡ 0  0                 1⎤ ⎡φ⁺⎤   ⎡                     0⎤
  ⎜ 0  0                -1⎟ ⎜φ⁻⎟ = ⎜                     0⎟
  ⎣ 1 -1 -R(2e/hbar)(2h/3)⎦ ⎣Io⎦   ⎣ (4/3)φn-1 - (1/3)φn-2⎦
 */ 

class Resistor : public BasicComponent {
  private:
  JoSIM::AnalysisType at_;
  public:
  double pn1_ = 0.0, pn2_ = 0.0, pn3_ = 0.0, pn4_ = 0.0;

  Resistor(
    const std::pair<tokens_t, string_o> &s, const NodeConfig &ncon,
    const nodemap &nm, std::unordered_set<std::string> &lm, nodeconnections &nc,
    const param_map &pm, const AnalysisType &at, const double &h, int &bi);

  void update_timestep(const double &factor) override;

  void step_back() override {
    pn2_ = pn4_;
  }
}; // class Resistor

} // namespace JoSIM
#endif