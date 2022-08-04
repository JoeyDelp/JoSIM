// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/CCVS.hpp"

#include <utility>

#include "JoSIM/Constants.hpp"
#include "JoSIM/Errors.hpp"
#include "JoSIM/Misc.hpp"

using namespace JoSIM;

/*
 Hlabel Vo⁺ Vo⁻ Vc⁺ Vc⁻ G

 Vo = GIc

 ⎡ 0  0  0  0  0 -1⎤ ⎡Vo⁺⎤   ⎡ 0⎤
 ⎜ 0  0  0  0  0  1⎟ ⎜Vo⁻⎟   ⎜ 0⎟
 ⎜ 0  0  0  0  1  0⎟ ⎜Vc⁺⎟ = ⎜ 0⎟
 ⎜ 0  0  0  0 -1  0⎟ ⎜Vc⁻⎟   ⎜ 0⎟
 ⎜ 1 -1  0  0 -G  0⎟ ⎜Ic ⎟   ⎜ 0⎟
 ⎣ 0  0  1 -1  0  0⎦ ⎣Io ⎦   ⎣ 0⎦

 (PHASE)
 φ - (2e/hbar)(2Gh/3)Ic = (4/3)φn-1 - (1/3)φn-2

 ⎡ 0  0  0  0                 0 -1⎤ ⎡φo⁺⎤   ⎡                     0⎤
 ⎜ 0  0  0  0                 0  1⎟ ⎜φo⁻⎟   ⎜                     0⎟
 ⎜ 0  0  0  0                 1  0⎟ ⎜φc⁺⎟ = ⎜                     0⎟
 ⎜ 0  0  0  0                -1  0⎟ ⎜φc⁻⎟   ⎜                     0⎟
 ⎜ 1 -1  0  0 -(2e/hbar)(2Gh/3)  0⎟ ⎜Ic ⎟   ⎜ (4/3)φn-1 - (1/3)φn-2⎟
 ⎣ 0  0  1 -1                 0  0⎦ ⎣Io ⎦   ⎣                     0⎦
*/

CCVS::CCVS(const std::pair<tokens_t, string_o>& s, const NodeConfig& ncon,
           const std::optional<NodeConfig>& ncon2, const nodemap& nm,
           std::unordered_set<std::string>& lm, nodeconnections& nc,
           const param_map& pm, int64_t& bi, const AnalysisType& at,
           const double& h) {
  at_ = at;
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
  // Set secondary current index and increment it
  currentIndex2_ = bi++;
  // Set te node indices, using tokens 2 to 5
  set_node_indices(tokens_t(s.first.begin() + 1, s.first.begin() + 5), nm, nc);
  // Set the non zero, column index and row pointer vectors
  set_matrix_info(at, h);
}

void CCVS::set_node_indices(const tokens_t& t, const nodemap& nm,
                            nodeconnections& nc) {
  // Set the output node indices and column indices
  switch (indexInfo.nodeConfig_) {
    case NodeConfig::POSGND:
      indexInfo.posIndex_ = nm.at(t.at(0));
      nc.at(nm.at(t.at(0))).emplace_back(std::make_pair(-1, currentIndex2_));
      break;
    case NodeConfig::GNDNEG:
      indexInfo.negIndex_ = nm.at(t.at(1));
      nc.at(nm.at(t.at(1))).emplace_back(std::make_pair(1, currentIndex2_));
      break;
    case NodeConfig::POSNEG:
      indexInfo.posIndex_ = nm.at(t.at(0));
      indexInfo.negIndex_ = nm.at(t.at(1));
      nc.at(nm.at(t.at(0))).emplace_back(std::make_pair(-1, currentIndex2_));
      nc.at(nm.at(t.at(1))).emplace_back(std::make_pair(1, currentIndex2_));
      break;
    case NodeConfig::GND:
      break;
  }
  // Set the controlling node indices and column indices
  switch (nodeConfig2_) {
    case NodeConfig::POSGND:
      posIndex2_ = nm.at(t.at(2));
      nc.at(nm.at(t.at(2)))
          .emplace_back(std::make_pair(1, indexInfo.currentIndex_.value()));
      break;
    case NodeConfig::GNDNEG:
      negIndex2_ = nm.at(t.at(3));
      nc.at(nm.at(t.at(3)))
          .emplace_back(std::make_pair(-1, indexInfo.currentIndex_.value()));
      break;
    case NodeConfig::POSNEG:
      posIndex2_ = nm.at(t.at(2));
      negIndex2_ = nm.at(t.at(3));
      nc.at(nm.at(t.at(2)))
          .emplace_back(std::make_pair(1, indexInfo.currentIndex_.value()));
      nc.at(nm.at(t.at(3)))
          .emplace_back(std::make_pair(-1, indexInfo.currentIndex_.value()));
      break;
    case NodeConfig::GND:
      break;
  }
}

void CCVS::set_matrix_info(const AnalysisType& at, const double& h) {
  switch (indexInfo.nodeConfig_) {
    case NodeConfig::POSGND:
      matrixInfo.nonZeros_.emplace_back(1);
      matrixInfo.columnIndex_.emplace_back(indexInfo.posIndex_.value());
      matrixInfo.rowPointer_.emplace_back(2);
      break;
    case NodeConfig::GNDNEG:
      matrixInfo.nonZeros_.emplace_back(-1);
      matrixInfo.columnIndex_.emplace_back(indexInfo.negIndex_.value());
      matrixInfo.rowPointer_.emplace_back(2);
      break;
    case NodeConfig::POSNEG:
      matrixInfo.nonZeros_.emplace_back(1);
      matrixInfo.nonZeros_.emplace_back(-1);
      matrixInfo.columnIndex_.emplace_back(indexInfo.posIndex_.value());
      matrixInfo.columnIndex_.emplace_back(indexInfo.negIndex_.value());
      matrixInfo.rowPointer_.emplace_back(3);
      break;
    case NodeConfig::GND:
      matrixInfo.rowPointer_.emplace_back(1);
      break;
  }
  if (at == AnalysisType::Voltage) {
    matrixInfo.nonZeros_.emplace_back(-netlistInfo.value_);
  } else if (at == AnalysisType::Phase) {
    pn2_ = 0;
    matrixInfo.nonZeros_.emplace_back(
        -netlistInfo.value_ * (1.0 / Constants::SIGMA) * ((2 * h) / 3.0));
    hDepPos_ = matrixInfo.nonZeros_.size() - 1;
  }
  matrixInfo.columnIndex_.emplace_back(indexInfo.currentIndex_.value());
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

// Update timestep based on a scalar factor i.e 0.5 for half the timestep
void CCVS::update_timestep(const double& factor) {
  if (at_ == AnalysisType::Phase) {
    matrixInfo.nonZeros_.at(hDepPos_) =
        factor * matrixInfo.nonZeros_.at(hDepPos_);
  }
}