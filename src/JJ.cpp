// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/JJ.hpp"
#include "JoSIM/Misc.hpp"
#include "JoSIM/Errors.hpp"
#include "JoSIM/Constants.hpp"

#include <cmath>

using namespace JoSIM;

/*
  Blabel V⁺ V⁻ jjmodel area=value

  V - (hbar/2e)(3/2h)φ = (hbar/2e)(-(2/h)φn-1 + (1/2h)φn-2)
  V - (2Rh/(3RC + 2h))Io = -(2Rh/(3RC + 2h))(Icsinφ0 - (2C/h)Vn-1 + (C/2h)Vn-2)
  φ0 = (4/3)φn-1 - (1/3)φn-2 + (2e/hbar)(2h/3)v0
  v0 = (5/2)Vn-1 - 2Vn-2 + (1/2)Vn-3

  ⎡ 0  0                0                 1⎤ ⎡ V⁺⎤   ⎡    0⎤
  ⎜ 0  0                0                -1⎟ ⎜ V⁻⎟ = ⎜    0⎟
  ⎜ 1 -1 -(hbar/2e)(3/2h)                 0⎟ ⎜ φ ⎟   ⎜ RHS1⎟
  ⎣ 1 -1                0 -(2Rh/(3RC + 2h))⎦ ⎣ Io⎦   ⎣ RHS2⎦

  RHS1 = (hbar/2e)(-(2/h)φn-1 + (1/2h)φn-2)
  RHS2 = -(2Rh/(3RC + 2h))(Icsinφ0 - (2C/h)Vn-1 + (C/2h)Vn-2)

  (PHASE)
  φ - (2e/hbar)(2h/3)V = (4/3)φn-1 - (1/3)φn-2

  ⎡ 0  0                0                 1⎤ ⎡ φ⁺⎤   ⎡    0⎤
  ⎜ 0  0                0                -1⎟ ⎜ φ⁻⎟ = ⎜    0⎟
  ⎜ 1 -1 -(2e/hbar)(2h/3)                 0⎟ ⎜ V ⎟   ⎜ RHS1⎟
  ⎣ 1 -1                0 -(2Rh/(3RC + 2h))⎦ ⎣ Io⎦   ⎣ RHS2⎦

  RHS1 = (4/3)φn-1 - (1/3)φn-2
*/ 

JJ::JJ(
    const std::pair<tokens_t, string_o> &s, const NodeConfig &ncon,
    const nodemap &nm, std::unordered_set<std::string> &lm, nodeconnections &nc,
    const param_map &pm, const vector_pair_t<Model, string_o> &models,
    const AnalysisType &at, const float &h, int &bi) {
  // Set component timestep
  h_ = h;
  at_ = at;
  // Set state to 0 (subgap)
  state_ = 0;
  // Make a copy of the tokens so that they are mutable
  tokens_t t = s.first;
  // Check if the label has already been defined
  if(lm.count(s.first.at(0)) != 0) {
    Errors::invalid_component_errors(
      ComponentErrors::DUPLICATE_LABEL, s.first.at(0));
  }
  // Set the label
  netlistInfo.label_ = s.first.at(0);
  // Add the label to the known labels list
  lm.emplace(s.first.at(0));
  // Junction line has potential to have up to 6 parts
  for(int i = 0; i < s.first.size(); ++i) {
    // If the part contains the AREA specifier
    if(s.first.at(i).find("AREA=") != std::string::npos) {
      // Set the area
      area_ = parse_param(
        s.first.at(i).substr(s.first.at(i).find("AREA=") + 5), pm, s.second);
      t.erase(t.begin() + i);
    } else {
      // If no AREA specifier found, assume area as unity
      area_ = 1;
    }
  }
  // Set the model for this JJ instance
  set_model(t, models, s.second);
  // Get and set the phase offset from the model
  pn1_ = model_.value().get_phaseOffset();
  // Set the transitional conductance value
  gLarge_ = model_.value().get_criticalCurrent() /
          (model_.value().get_criticalToNormalRatio() * 
            model_.value().get_deltaV());
  // Set the phase constant
  if(at == AnalysisType::Voltage) {
    // If voltage mode set this to (3 * hbar) / (4 * h * eV)
    phaseConst_ = (3 * Constants::HBAR) / (4 * h_ * Constants::EV);
  } else if (at == AnalysisType::Phase) { 
    // If phase mode set this to (4 * h * eV) / (3 * hbar)
    phaseConst_ = (4 * h_ * Constants::EV) / (3 * Constants::HBAR);
  }
  // Set the Del0 parameter
  del0_ = 1.76 * Constants::BOLTZMANN * 
    model_.value().get_criticalTemperature();
  // Set the del parameter
  del_ = del0_ * sqrt(cos((Constants::PI / 2) * 
    (model_.value().get_temperature() /
       model_.value().get_criticalTemperature()) * 
    (model_.value().get_temperature() / 
      model_.value().get_criticalTemperature())));
  // Set the calculated Rn value
  rncalc_ = ((Constants::PI * del_) / 
    (2 * Constants::EV * model_.value().get_criticalCurrent())) * 
    tanh(del_ / (2 * Constants::BOLTZMANN * model_.value().get_temperature()));
  // Set the node configuration type
  indexInfo.nodeConfig_ = ncon;
  // Set variable index and increment it
  variableIndex_ = bi++;
  // Set current index and increment it
  indexInfo.currentIndex_ = bi++;
  // Set te node indices, using token 2 and 3
  set_node_indices(tokens_t(s.first.begin()+1, s.first.begin()+3), nm, nc);
  // Set the non zero, column index and row pointer vectors
  set_matrix_info();
}

float JJ::subgap_impedance() {
  // Set subgap impedance (1/R0) + (3C/2h)
  return ((1/model_.value().get_subgapResistance()) + 
    ((3.0 * model_.value().get_capacitance()) / (2.0 * h_)));
}

float JJ::transient_impedance() {
  // Set transitional impedance (GL) + (3C/2h)
  return (gLarge_ + 
    ((3.0 * model_.value().get_capacitance()) / (2.0 * h_)));
}

float JJ::normal_impedance() {
  // Set normal impedance (1/RN) + (3C/2h)
  return ((1/model_.value().get_normalResistance()) + 
    ((3.0 * model_.value().get_capacitance()) / (2.0 * h_)));
}

void JJ::set_matrix_info() {
  switch(indexInfo.nodeConfig_) {
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
  matrixInfo.nonZeros_.emplace_back(-phaseConst_);
  hDepPos_ = matrixInfo.nonZeros_.size() - 1;
  matrixInfo.columnIndex_.emplace_back(variableIndex_);
  if(at_ == AnalysisType::Voltage) {
    matrixInfo.nonZeros_.insert(
      matrixInfo.nonZeros_.end(), 
      matrixInfo.nonZeros_.begin(), matrixInfo.nonZeros_.end());
    matrixInfo.nonZeros_.back() = -1/subgap_impedance();
    matrixInfo.columnIndex_.insert(
      matrixInfo.columnIndex_.end(), 
      matrixInfo.columnIndex_.begin(), matrixInfo.columnIndex_.end());
    matrixInfo.columnIndex_.back() = indexInfo.currentIndex_.value();
    matrixInfo.rowPointer_.emplace_back(matrixInfo.rowPointer_.back());
  } else if (at_ == AnalysisType::Phase) {
    matrixInfo.nonZeros_.emplace_back(1);
    matrixInfo.columnIndex_.emplace_back(variableIndex_);
    matrixInfo.nonZeros_.emplace_back(-1/subgap_impedance());
    matrixInfo.columnIndex_.emplace_back(indexInfo.currentIndex_.value());
    matrixInfo.rowPointer_.emplace_back(2);
  }
}

void JJ::set_model(
    const tokens_t &t, const vector_pair_t<Model, string_o> &models, 
    const string_o &subc) {
  // Loop through all models
  for(auto &i : models) {
    // If the model name matches that of an identified model
    if(i.first.get_modelName() == t.back()) {
      // If both models belong to a subcircuit and the subcircuit names match
      if ((i.second && subc) && (subc.value() == i.second.value())){
        // Set the model to the exact identified model
        model_ = i.first;
        break;
      // JJ might be in a subcircuit but model in global scope
      } else if (!i.second) {
        // Set the model to the globally identified model
        model_ = i.first;
        break;
      }
    }
  }
  // If no model was found
  if(!model_) {
    // Complain about it
    Errors::invalid_component_errors(
      ComponentErrors::MODEL_NOT_DEFINED, Misc::vector_to_string(t));
  }
  // Set the model capacitance for this JJ instance
  model_.value().set_capacitance(
    model_.value().get_capacitance() * area_);
  // Set the model normal resistance for this JJ instance  
  model_.value().set_normalResistance(
    model_.value().get_normalResistance() / area_);
  // Set the model subgap resistance for this JJ instance  
  model_.value().set_subgapResistance(
    model_.value().get_subgapResistance() / area_);
  // Set the model critical current for this JJ instance  
  model_.value().set_criticalCurrent(
    model_.value().get_criticalCurrent() * area_);
  // Set the lower boundary for the transition region
  lowerB_ = model_.value().get_voltageGap() - 0.5 * model_.value().get_deltaV();
  // Set the upper boundary for the transition region
  upperB_ = model_.value().get_voltageGap() + 0.5 * model_.value().get_deltaV();
}

// Update the value based on the matrix entry based on the current voltage value
bool JJ::update_value(const float &v) {
  // Shorthand for the model
  const Model &m = model_.value();
  // If the absolute value of the voltage is less than lower bounds
  if(fabs(v) < lowerB_) {
    // Set the transition current to 0
    transitionCurrent_ = 0.0;
    // If the non zero vector does not end with the subgap conductance
    if(matrixInfo.nonZeros_.back() != -1/subgap_impedance()) {
      // Make it end with the subgap conductance
      matrixInfo.nonZeros_.back() = -1/subgap_impedance();
      // Set state to 0 (Subgap)
      state_ = 0;
      // Return that a value has been updated
      return true;
    } else {
      // Return that nothing has changed
      return false;
    }
  // If the absolute value of the voltage is less than the upperbounds
  } else if (fabs(v) < upperB_) {
    // Set the transition current
    transitionCurrent_ = lowerB_ * ((1 / m.get_subgapResistance()) - gLarge_);
    // If the voltage is negative, current must be negative
    if (v < 0) transitionCurrent_ = -transitionCurrent_;
    // If the back of the non zero vector is not the transition conductance
    if(matrixInfo.nonZeros_.back() != -1/transient_impedance()) {
      // Set it to the transition conductance
      matrixInfo.nonZeros_.back() = -1/transient_impedance();
      // Set state to 1 (Transition)
      state_ = 1;
      // Return that a value has changed
      return true;
    } else {
      // Return that nothing has changed
      return false;
    }
  // If neither of the above then it must be in normal operating region
  } else {
    // Reset the transition current, transition has passed.
    transitionCurrent_ = 0.0;
    // If the back of the non zero vector is not the normal conductance
    if(matrixInfo.nonZeros_.back() != -1/normal_impedance()) {
      // Set it to the normal conductance
      matrixInfo.nonZeros_.back() = -1/normal_impedance();
      // Set state to 2 (Normal)
      state_ = 2;
      // Return that a value has changed
      return true;
    } else {
      // Return that nothing has changed
      return false;
    }
  }
  // This should never happen, only here to appease the return type
  return false;
}

// Update timestep based on a scalar factor i.e 0.5 for half the timestep
void JJ::update_timestep(const float &factor) {
  h_ = h_ * factor;
  if (state_ == 0) {
    matrixInfo.nonZeros_.back() = -1/subgap_impedance();
  } else if (state_ == 1) {
    matrixInfo.nonZeros_.back() = -1/transient_impedance();
  } else if (state_ == 2) {
    matrixInfo.nonZeros_.back() = -1/normal_impedance();
  }
  if (at_ == AnalysisType::Voltage) {
    matrixInfo.nonZeros_.at(hDepPos_) = 
      (1 / factor) * matrixInfo.nonZeros_.at(hDepPos_);
  } else if (at_ == AnalysisType::Phase) {
    matrixInfo.nonZeros_.at(hDepPos_) = 
      factor * matrixInfo.nonZeros_.at(hDepPos_);
  }
}