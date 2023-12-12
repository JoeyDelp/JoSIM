// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/CurrentSource.hpp"

#include "JoSIM/Errors.hpp"
#include "JoSIM/Misc.hpp"

using namespace JoSIM;

/*
  Ilabel V⁺ V⁻ sourcetype

  ⎡ 0  0⎤ ⎡ V⁺⎤ = ⎡  Io⎤
  ⎣ 0  0⎦ ⎣ V⁻⎦   ⎣ -Io⎦
*/

CurrentSource::CurrentSource(const std::pair<tokens_t, string_o>& s,
                             const NodeConfig& ncon, const nodemap& nm,
                             std::unordered_set<std::string>& lm,
                             const int64_t& si) {
  // Check if the label has already been defined
  if (lm.count(s.first.at(0)) != 0) {
    Errors::invalid_component_errors(ComponentErrors::DUPLICATE_LABEL,
                                     s.first.at(0));
  }
  // Set the label
  netlistInfo.label_ = s.first.at(0);
  // Add the label to the known labels list
  lm.emplace(s.first.at(0));
  // Set the node configuration type
  indexInfo.nodeConfig_ = ncon;
  // Set te node indices, using token 2 and 3
  set_node_indices(tokens_t(s.first.begin() + 1, s.first.begin() + 3), nm);
  // Set the source index
  sourceIndex_ = si;
}

void CurrentSource::set_node_indices(const tokens_t& t, const nodemap& nm) {
  switch (indexInfo.nodeConfig_) {
    case NodeConfig::POSGND:
      indexInfo.posIndex_ = nm.at(t.at(0));
      break;
    case NodeConfig::GNDNEG:
      indexInfo.negIndex_ = nm.at(t.at(1));
      break;
    case NodeConfig::POSNEG:
      indexInfo.posIndex_ = nm.at(t.at(0));
      indexInfo.negIndex_ = nm.at(t.at(1));
      break;
    case NodeConfig::GND:
      break;
  }
}
