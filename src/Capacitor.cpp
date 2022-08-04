// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/Capacitor.hpp"

#include <utility>

#include "JoSIM/Constants.hpp"
#include "JoSIM/Errors.hpp"
#include "JoSIM/Misc.hpp"

using namespace JoSIM;

/*
  Clabel V⁺ V⁻ C

  V - (2*h)/(3*C)Io = (4/3)Vn-1 - (1/3)Vn-2

  ⎡ 0  0            1⎤ ⎡ V⁺⎤   ⎡                     0⎤
  ⎜ 0  0           -1⎟ ⎜ V⁻⎟ = ⎜                     0⎟
  ⎣ 1 -1 -(2*h)/(3*C)⎦ ⎣Io ⎦   ⎣ (4/3)Vn-1 - (1/3)Vn-2⎦

  (PHASE)
  φ - (4*h*h*2*e)/(hbar*9*C)Io = (8/3)φn-1 - (22/9)φn-2 + (8/9)φn-3 - (1/9)φn-4

  ⎡ 0  0                       1⎤ ⎡ φ⁺⎤   ⎡   0⎤
  ⎜ 0  0                      -1⎟ ⎜ φ⁻⎟ = ⎜   0⎟
  ⎣ 1 -1 -(4*h*h*2*e)/(hbar*9*C)⎦ ⎣Io ⎦   ⎣ RHS⎦
  RHS = (8/3)φn-1 - (22/9)φn-2 + (8/9)φn-3 - (1/9)φn-4
*/

Capacitor::Capacitor(const std::pair<tokens_t, string_o>& s,
                     const NodeConfig& ncon, const nodemap& nm,
                     std::unordered_set<std::string>& lm, nodeconnections& nc,
                     Input& iObj, Spread& spread, int64_t& bi) {
  double spr = 1.0;
  for (auto i : s.first) {
    if (i.find("SPREAD=") != std::string::npos) {
      spr = parse_param(i.substr(i.find("SPREAD=") + 7), iObj.parameters,
                        s.second);
    }
  }
  at_ = iObj.argAnal;
  // Check if the label has already been defined
  if (lm.count(s.first.at(0)) != 0) {
    Errors::invalid_component_errors(ComponentErrors::DUPLICATE_LABEL,
                                     s.first.at(0));
  }
  // Set the label
  netlistInfo.label_ = s.first.at(0);
  // Add the label to the known labels list
  lm.emplace(s.first.at(0));
  // Set the value (Capacitance), this should be the 4th token
  netlistInfo.value_ = spread.spread_value(
      parse_param(s.first.at(3), iObj.parameters, s.second), Spread::CAP, spr);
  // Set the node configuration type
  indexInfo.nodeConfig_ = ncon;
  // Set current index and increment it
  indexInfo.currentIndex_ = bi++;
  // Set te node indices, using token 2 and 3
  set_node_indices(tokens_t(s.first.begin() + 1, s.first.begin() + 3), nm, nc);
  // Set the non zero, column index and row pointer vectors
  set_matrix_info();
  // Append the value to the non zero vector
  if (at_ == AnalysisType::Voltage) {
    // If voltage mode analysis then append -(2/3) * (h/C)
    matrixInfo.nonZeros_.emplace_back(
        -(2.0 / 3.0) * (iObj.transSim.tstep() / netlistInfo.value_));
  } else if (at_ == AnalysisType::Phase) {
    // If phase mdoe analysis then append -(4/9) * ((h*h)/C) * (1/σ)
    matrixInfo.nonZeros_.emplace_back(
        -(4.0 / 9.0) *
        ((iObj.transSim.tstep() * iObj.transSim.tstep()) / netlistInfo.value_) *
        (1 / Constants::SIGMA));
  }
}

// Update timestep based on a scalar factor i.e 0.5 for half the timestep
void Capacitor::update_timestep(const double& factor) {
  if (at_ == AnalysisType::Voltage) {
    matrixInfo.nonZeros_.back() = factor * matrixInfo.nonZeros_.back();
  } else if (at_ == AnalysisType::Phase) {
    matrixInfo.nonZeros_.back() = factor * factor * matrixInfo.nonZeros_.back();
  }
}