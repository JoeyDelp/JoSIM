// Copyright (c) 2020 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/JJ.hpp"
#include "JoSIM/IntegrationType.hpp"
#include "JoSIM/Misc.hpp"
#include "JoSIM/Errors.hpp"
#include "JoSIM/Constants.hpp"

#include <cmath>

using namespace JoSIM;

JJ::JJ(
    const std::pair<tokens_t, string_o> &s, const NodeConfig &ncon,
    const nodemap &nm, std::unordered_set<std::string> &lm, nodeconnections &nc,
    const param_map &pm, const vector_pair_t<Model, string_o> &models,
    const AnalysisType &at, const double &h, int &bi) {
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
  lm.emplace(s);
  // Junction line has potential to have up to 6 parts
  for(int i = 0; i < s.first.size(); ++i) {
    // If the part contains the AREA specifiere
    if(s.first.at(i).find("AREA=") != std::string::npos) {
      // Set the area
      area_ = parse_param(
        s.first.at(i).substr(s.first.at(i).find("AREA=") + 5), pm, s.second);
      t.erase(t.begin() + i);
    }
  }
  // Set the model for this JJ instance
  set_model(t, models, s.second);
  // Get and set the phase offset from the model
  pn1_ = model_.value().get_phaseOffset();
  // Set the transitional conductance value
  gLarge_ = model_.value().get_criticalCurrent() /
          (model_.value().get_criticalToNormalRatio() * 
            model_.value().get_deltaV()));
  // Set subgap impedance (1/R0) + (3C/2h)
  subImp_ = ((1/model_.value().get_subgapResistance()) + 
    ((3.0 * model_.value().get_capacitance()) / (2.0 * h)));
  // Set transitional impedance (GL) + (3C/2h)
  transImp_ = (gLarge_ + 
    ((3.0 * model_.value().get_capacitance()) / (2.0 * h)));
  // Set normal impedance (1/RN) + (3C/2h)
  normImp_ = ((1/model_.value().get_normalResistance()) + 
    ((3.0 * model_.value().get_capacitance()) / (2.0 * h)));
  // Set the phase constant
  if(at == AnalysisType::Voltage) {
    // If voltage mode set this to (3 * hbar) / (4 * h * eV)
    phaseConst_ = (3 * Constants::HBAR) / (4 * h * Constants::EV);
  } else if (at == AnalysisType::Phase) { 
    // If phase mode set this to (4 * h * eV) / (3 * hbar)
    phaseConst_ = (4 * h * Constants::EV) / (3 * Constants::HBAR);
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
  // Set te node indices, using token 2 and 3
  set_node_indices(tokens_t(s.first.begin()+1, s.first.begin()+2), nm);
  // Set variable index and increment it
  variableIndex_ = bi++;
  // Set current index and increment it
  indexInfo.currentIndex_ = bi++;
  // Set the non zero, column index and row pointer vectors
  set_matrix_info();
  // The non zero vector should have 2 subsets, the variable row and current row
  std::vector<double> tempNZ = matrixInfo.nonZeros_;
  // Add the phase constant to the end of the non zero vector
  matrixInfo.nonZeros_.emplace_back(-phaseConst_);
  // Add the temporary non zero vector to the end of the non zero vector
  matrixInfo.nonZeros_.insert(
    matrixInfo.nonZeros_.end(), tempNZ.begin(), tempNZ.end());
  // Append the RC conductance to the non zero vector
  matrixInfo.nonZeros_.emplace_back(-1/subImp_);
  // Now alter the column indices vector
  matrixInfo.columnIndex_.back() = variableIndex_;
  matrixInfo.columnIndex_.insert(matrixInfo.columnIndex_.end(), 
    matrixInfo.columnIndex_.begin(), matrixInfo.columnIndex_.end());
  matrixInfo.columnIndex_.back() = indexInfo.currentIndex_.value();
  // There are two rows of information for a JJ, duplicate the nz count
  matrixInfo.rowPointer_.emplace_back(matrixInfo.rowPointer_.back());
}

void JJ::set_model(
    const tokens_t &t, const vector_pair_t<Model, string_o> &models, 
    const string_o &subc) {
  // Loop through all models
  for(auto &i : models) {
    // If the model name matches that of an identified model
    if(i.first.get_modelName() == t.back() && 
        i.second == subc) {
      // Check that the subcircuit names match (if any)
      if((subc && subc.value() == i.second.value()) || (!subc)) {
        // Set the model
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
bool JJ::update_value(const double &v) {
  // Shorthand for the model
  const Model &m = model_.value();
  // If the absolute value of the voltage is less than lower bounds
  if(fabs(v) < lowerB_) {
    // Set the transition current to 0
    transitionCurrent_ = 0.0;
    // If the non zero vector does not end with the subgap conductance
    if(matrixInfo.nonZeros_.back() != -1/subImp_) {
      // Make it end with the subgap conductance
      matrixInfo.nonZeros_.back() = -1/subImp_;
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
    if(matrixInfo.nonZeros_.back() != -1/transImp_) {
      // Set it to the transition conductance
      matrixInfo.nonZeros_.back() = -1/transImp_;
      // Return that a value has changed
      return true;
    } else {
      // Return that nothing has changed
      return false;
    }
  // If neither of the above then it must be in normal operating region
  } else {
    // Set the transition current
    transitionCurrent_ = 
      (m.get_criticalCurrent() / m.get_criticalToNormalRatio() + 
        m.get_voltageGap() * (1 / m.get_subgapResistance()) - lowerB_ * 
        (1 / m.get_normalResistance()));
    // If the voltage is negative, current must be negative
    if (v < 0) transitionCurrent_ = -transitionCurrent_;
    // If the back of the non zero vector is not the normal conductance
    if(matrixInfo.nonZeros_.back() != -1/normImp_) {
      // Set it to the normal conductance
      matrixInfo.nonZeros_.back() = -1/normImp_;
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