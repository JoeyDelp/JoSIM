// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_CURRENTSOURCE_HPP
#define JOSIM_CURRENTSOURCE_HPP

#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "JoSIM/BasicComponent.hpp"

namespace JoSIM {

/*
  Ilabel V⁺ V⁻ sourcetype

  ⎡ 0  0⎤ ⎡ V⁺⎤ = ⎡  Io⎤
  ⎣ 0  0⎦ ⎣ V⁻⎦   ⎣ -Io⎦
*/

class CurrentSource : public BasicComponent {
 public:
  int64_t sourceIndex_;

  CurrentSource(const std::pair<tokens_t, string_o>& s, const NodeConfig& ncon,
                const nodemap& nm, std::unordered_set<std::string>& lm,
                const int64_t& si);

  void set_node_indices(const tokens_t& t, const nodemap& nm);
};  // class CurrentSource

}  // namespace JoSIM
#endif