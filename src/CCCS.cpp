// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/CCCS.hpp"

#include <utility>

#include "JoSIM/Constants.hpp"
#include "JoSIM/Errors.hpp"
#include "JoSIM/Misc.hpp"

using namespace JoSIM;

/*
 Flabel Vo⁺ Vo⁻ Vc⁺ Vc⁻ G

 Io = GIc

 ⎡ 0  0  0  0    1⎤ ⎡Vo⁺⎤   ⎡ 0⎤
 ⎜ 0  0  0  0   -1⎟ ⎜Vo⁻⎟   ⎜ 0⎟
 ⎜ 0  0  0  0  1/G⎟ ⎜Vc⁺⎟ = ⎜ 0⎟
 ⎜ 0  0  0  0 -1/G⎟ ⎜Vc⁻⎟   ⎜ 0⎟
 ⎣ 0  0  1 -1    0⎦ ⎣Io ⎦   ⎣ 0⎦
*/

CCCS::CCCS(const std::pair<tokens_t, string_o>& s, const NodeConfig& ncon,
           const std::optional<NodeConfig>& ncon2, const nodemap& nm,
           std::unordered_set<std::string>& lm, nodeconnections& nc,
           const param_map& pm, int64_t& bi) {
  // Check if the label has already been defined
  if (lm.count(s.first.at(0)) != 0) {
    Errors::invalid_component_errors(ComponentErrors::DUPLICATE_LABEL,
                                     s.first.at(0));
  }
  // Set the label
  netlistInfo.label_ = s.first.at(0);
  // Add the label to the known labels list
  lm.emplace(s.first.at(0));
  // Set the value, this should be the 6th token
  netlistInfo.value_ = parse_param(s.first.at(5), pm, s.second);
  // Set the node configuration type
  indexInfo.nodeConfig_ = ncon;
  // Set the secondary node configuration typpe
  nodeConfig2_ = ncon2.value();
  // Set current index and increment it
  indexInfo.currentIndex_ = bi++;
  // Set te node indices, using tokens 2 to 5
  set_node_indices(tokens_t(s.first.begin() + 1, s.first.begin() + 5), nm, nc);
  // Set the non zero, column index and row pointer vectors
  set_matrix_info();
}

void CCCS::set_node_indices(const tokens_t& t, const nodemap& nm,
                            nodeconnections& nc) {
  // Set the node indices for the controlling nodes and add column indices
  switch (indexInfo.nodeConfig_) {
    case NodeConfig::POSGND:
      indexInfo.posIndex_ = nm.at(t.at(0));
      nc.at(nm.at(t.at(0)))
          .emplace_back(std::make_pair(1, indexInfo.currentIndex_.value()));
      break;
    case NodeConfig::GNDNEG:
      indexInfo.negIndex_ = nm.at(t.at(1));
      nc.at(nm.at(t.at(1)))
          .emplace_back(std::make_pair(-1, indexInfo.currentIndex_.value()));
      break;
    case NodeConfig::POSNEG:
      indexInfo.posIndex_ = nm.at(t.at(0));
      indexInfo.negIndex_ = nm.at(t.at(1));
      nc.at(nm.at(t.at(0)))
          .emplace_back(std::make_pair(1, indexInfo.currentIndex_.value()));
      nc.at(nm.at(t.at(1)))
          .emplace_back(std::make_pair(-1, indexInfo.currentIndex_.value()));
      break;
    case NodeConfig::GND:
      break;
  }
  // Set the node indices for the controlled nodes and add column index info
  switch (nodeConfig2_) {
    case NodeConfig::POSGND:
      posIndex2_ = nm.at(t.at(2));
      nc.at(nm.at(t.at(2)))
          .emplace_back(std::make_pair((1 / netlistInfo.value_),
                                       indexInfo.currentIndex_.value()));
      break;
    case NodeConfig::GNDNEG:
      negIndex2_ = nm.at(t.at(3));
      nc.at(nm.at(t.at(3)))
          .emplace_back(std::make_pair(-(1 / netlistInfo.value_),
                                       indexInfo.currentIndex_.value()));
      break;
    case NodeConfig::POSNEG:
      posIndex2_ = nm.at(t.at(2));
      negIndex2_ = nm.at(t.at(3));
      nc.at(nm.at(t.at(2)))
          .emplace_back(std::make_pair((1 / netlistInfo.value_),
                                       indexInfo.currentIndex_.value()));
      nc.at(nm.at(t.at(3)))
          .emplace_back(std::make_pair(-(1 / netlistInfo.value_),
                                       indexInfo.currentIndex_.value()));
      break;
    case NodeConfig::GND:
      break;
  }
}

void CCCS::set_matrix_info() {
  switch (nodeConfig2_) {
    case NodeConfig::POSGND:
      matrixInfo.nonZeros_.emplace_back(1);
      matrixInfo.columnIndex_.emplace_back(posIndex2_.value());
      matrixInfo.rowPointer_.emplace_back(1);
      break;
    case NodeConfig::GNDNEG:
      matrixInfo.nonZeros_.emplace_back(-1);
      matrixInfo.columnIndex_.emplace_back(negIndex2_.value());
      matrixInfo.rowPointer_.emplace_back(1);
      break;
    case NodeConfig::POSNEG:
      matrixInfo.nonZeros_.emplace_back(1);
      matrixInfo.nonZeros_.emplace_back(-1);
      matrixInfo.columnIndex_.emplace_back(posIndex2_.value());
      matrixInfo.columnIndex_.emplace_back(negIndex2_.value());
      matrixInfo.rowPointer_.emplace_back(2);
      break;
    case NodeConfig::GND:
      break;
  }
}