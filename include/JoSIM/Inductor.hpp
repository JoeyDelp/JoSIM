// Copyright (c) 2020 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_INDUCTOR_HPP
#define JOSIM_INDUCTOR_HPP

#include "JoSIM/Components.hpp"
#include "JoSIM/ParameterName.hpp"
#include "JoSIM/Parameters.hpp"
#include "JoSIM/AnalysisType.hpp"

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <optional>


namespace JoSIM {

using mutualinductors = std::vector<std::pair<int, double>>;

class Inductor : public BasicComponent {
  public:
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
}; // class Inductor

} // namespace JoSIM
#endif