// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/Resistor.hpp"

#include <utility>

#include "JoSIM/Constants.hpp"
#include "JoSIM/Errors.hpp"
#include "JoSIM/Misc.hpp"
#include "JoSIM/Noise.hpp"

using namespace JoSIM;

/*
 Rlabel V⁺ V⁻ R

 V = RIo
 ⎡ 0  0  1⎤ ⎡V⁺⎤   ⎡ 0⎤
 ⎜ 0  0 -1⎟ ⎜V⁻⎟ = ⎜ 0⎟
 ⎣ 1 -1 -R⎦ ⎣Io⎦   ⎣ 0⎦

 (PHASE)
 φ - R(2e/hbar)(2h/3)Io = (4/3)φn-1 - (1/3)φn-2

 ⎡ 0  0                 1⎤ ⎡φ⁺⎤   ⎡                     0⎤
 ⎜ 0  0                -1⎟ ⎜φ⁻⎟ = ⎜                     0⎟
 ⎣ 1 -1 -R(2e/hbar)(2h/3)⎦ ⎣Io⎦   ⎣ (4/3)φn-1 - (1/3)φn-2⎦
*/

Resistor::Resistor(const std::pair<tokens_t, string_o>& s,
                   const NodeConfig& ncon, const nodemap& nm,
                   std::unordered_set<std::string>& lm, nodeconnections& nc,
                   Input& iObj, Spread& spread, int64_t& bi) {
  double spr = 1.0;
  for (int64_t i = 4; i < s.first.size(); ++i) {
    auto& t = s.first.at(i);
    if (t.rfind("SPREAD=", 0) == 0) {
      spr = parse_param(t.substr(7), iObj.parameters, s.second);
    } else if (t.rfind("TEMP=", 0) == 0) {
      temp_ = parse_param(t.substr(5), iObj.parameters, s.second);
    } else if (t.rfind("NEB=", 0) == 0) {
      neb_ = parse_param(t.substr(4), iObj.parameters, s.second);
    }
  }
  if (!temp_ && iObj.globalTemp) {
    temp_ = iObj.globalTemp;
  }
  if (!neb_ && iObj.neB) {
    neb_ = iObj.neB;
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
  // Set the value (Resistance), this should be the 4th token
  netlistInfo.value_ = spread.spread_value(
      parse_param(s.first.at(3), iObj.parameters, s.second), Spread::RES, spr);
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
    // If voltage mode analysis then append -R
    matrixInfo.nonZeros_.emplace_back(-netlistInfo.value_);
  } else if (at_ == AnalysisType::Phase) {
    // Set the value of node n-2 to 0
    pn2_ = 0;
    // If phase mdoe analysis then append -((2*h)/3) * (R/σ)
    matrixInfo.nonZeros_.emplace_back(-((2.0 * iObj.transSim.tstep()) / 3.0) *
                                      (netlistInfo.value_ / Constants::SIGMA));
  }
  if (temp_) {
    spAmp_ =
        Noise::determine_spectral_amplitude(netlistInfo.value_, temp_.value());
    Function tnoise;
    tnoise.parse_function("NOISE(" + Misc::precise_to_string(spAmp_.value()) +
                              ", 0.0, " +
                              Misc::precise_to_string(1.0 / neb_.value()) + ")",
                          iObj, s.second);
    thermalNoise = tnoise;
  }
}

// Update timestep based on a scalar factor i.e 0.5 for half the timestep
void Resistor::update_timestep(const double& factor) {
  if (at_ == AnalysisType::Phase) {
    matrixInfo.nonZeros_.back() = factor * factor * matrixInfo.nonZeros_.back();
  }
}