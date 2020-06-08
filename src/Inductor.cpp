// Copyright (c) 2020 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/Inductor.hpp"
#include "JoSIM/IntegrationType.hpp"
#include "JoSIM/Misc.hpp"
#include "JoSIM/Errors.hpp"
#include "JoSIM/Constants.hpp"

#include <utility>

using namespace JoSIM;

Inductor::Inductor(
    const std::pair<tokens_t, string_o> &s, const NodeConfig &ncon, 
    const nodemap &nm, std::unordered_set<std::string> &lm, nodeconnections &nc,
    const param_map &pm, const AnalysisType &at, const double &h, int &bi) {
  // Check if the label has already been defined
  if(lm.count(s.first.at(0)) != 0) {
    Errors::invalid_component_errors(
      ComponentErrors::DUPLICATE_LABEL, s.first.at(0));
  }
  // Set the label
  netlistInfo.label_ = s.first.at(0);
  // Add the label to the known labels list
  lm.emplace(s);
  // Set the value (Inductance), this should be the 4th token
  netlistInfo.value_ = parse_param(s.first.at(3), pm, s.second);
  // Set the node configuration type
  indexInfo.nodeConfig_ = ncon;
  // Set te node indices, using token 2 and 3
  set_node_indices(tokens_t(s.first.begin()+1, s.first.begin()+2), nm);
  // Set current index and increment it
  indexInfo.currentIndex_ = bi++;
  // Set the non zero, column index and row pointer vectors
  set_matrix_info();
  // Append the value to the non zero vector
  if (at == AnalysisType::Voltage) { 
    // If voltage mode analysis then append -(3/2) * (L/h)
    matrixInfo.nonZeros_.emplace_back(
      -(3.0 / 2.0) * (netlistInfo.value_/h));
  } else if (at == AnalysisType::Phase) { 
    // If phase mdoe analysis then append -(L/σ)
    matrixInfo.nonZeros_.emplace_back(
      -netlistInfo.value_ / Constants::SIGMA);
  }
}

void Inductor::add_mutualInductance(
  const double &m, const AnalysisType &at, const double &h, const int &ci) {
  // Adds the mutual inductance to the non zero vector
  if(at == AnalysisType::Voltage) {
    // If voltage mode then add -(3/2) * (m/h)
    matrixInfo.nonZeros_.emplace_back(-(3 / 2) * (m / h));
  } else if(at == AnalysisType::Phase) {
    // If phase mode then add -(m/σ)
    matrixInfo.nonZeros_.emplace_back(-m / Constants::SIGMA);
  }
  // Append the column index of the current branch of the second inductor
  matrixInfo.columnIndex_.emplace_back(ci);
  // Increase the row pointer vector by 1
  matrixInfo.rowPointer_++;
}