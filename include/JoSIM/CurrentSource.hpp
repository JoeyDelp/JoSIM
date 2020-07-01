// Copyright (c) 2020 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_CURRENTSOURCE_HPP
#define JOSIM_CURRENTSOURCE_HPP

#include "JoSIM/BasicComponent.hpp"

#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <string>

namespace JoSIM {
class CurrentSource : public BasicComponent {
  public:
  int sourceIndex_;

  CurrentSource(
    const std::pair<tokens_t, string_o> &s, const NodeConfig &ncon,
    const nodemap &nm, std::unordered_set<std::string> &lm, const int &si);

  void set_node_indices(const tokens_t &t, const nodemap &nm);
}; // class CurrentSource

} // namespace JoSIM
#endif