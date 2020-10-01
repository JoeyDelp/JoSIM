// Copyright (c) 2020 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_INDUCTOR_HPP
#define JOSIM_INDUCTOR_HPP

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
  Llabel V⁺ V⁻ L

  V - (3*L)/(2*h)Io = -(2*L)/(h)In-1 + (L)/(2*h)In-2

  ⎡ 0  0            1⎤ ⎡V⁺⎤   ⎡                              0⎤
  ⎜ 0  0           -1⎟ ⎜V⁻⎟ = ⎜                              0⎟
  ⎣ 1 -1 -(3*L)/(2*h)⎦ ⎣Io⎦   ⎣ -(2*L)/(h)In-1 + (L)/(2*h)In-2⎦

  (PHASE)
  φ - L(2e/hbar)Io = 0
  
  ⎡ 0  0           1⎤ ⎡φ⁺⎤   ⎡ 0⎤
  ⎜ 0  0          -1⎟ ⎜φ⁻⎟ = ⎜ 0⎟
  ⎣ 1 -1 -L(2e/hbar)⎦ ⎣Io⎦   ⎣ 0⎦
 */ 

using mutualinductors = std::vector<std::pair<int, double>>;

class Inductor : public BasicComponent {
  private:
  JoSIM::AnalysisType at_;
  public:
  double In2_ = 0.0;
  mutualinductors mutualInductances_;

  Inductor(
    const std::pair<tokens_t, string_o> &s, const NodeConfig &ncon, 
    const nodemap &nm, std::unordered_set<std::string> &lm, nodeconnections &nc,
    const param_map &pm, const AnalysisType &at, const double &h, int &bi);
    
  void set_mutualInductance(const std::pair<int, double> &mut) { 
    mutualInductances_.emplace_back(mut); }
  void add_mutualInductance(
    const double &m, const AnalysisType &at, const double &h, const int &ci);
  const mutualinductors get_mutualInductance() const { 
    return mutualInductances_; } 

  void update_timestep(const double &factor) override;
}; // class Inductor

} // namespace JoSIM
#endif