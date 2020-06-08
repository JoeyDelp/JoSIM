// Copyright (c) 2020 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/Resistor.hpp"
#include "JoSIM/IntegrationType.hpp"
#include "JoSIM/Misc.hpp"
#include "JoSIM/Errors.hpp"
#include "JoSIM/Constants.hpp"

#include <utility>

using namespace JoSIM;

Resistor::Resistor(
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
  // Set the value (Resistance), this should be the 4th token
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
    // If voltage mode analysis then append -R
    matrixInfo.nonZeros_.emplace_back(-netlistInfo.value_);
  } else if (at == AnalysisType::Phase) { 
    // If phase mdoe analysis then append -((2*h)/3) * (R/σ)
    matrixInfo.nonZeros_.emplace_back(
      -((2.0 * h)/3.0) * (netlistInfo.value_ / Constants::SIGMA));
  }
}