// Copyright (c) 2020 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/JJ.hpp"
#include "JoSIM/IntegrationType.hpp"
#include "JoSIM/Misc.hpp"
#include "JoSIM/Errors.hpp"
#include "JoSIM/Constants.hpp"

#include <cmath>

using namespace JoSIM;

JJ JJ::create_jj(const std::pair<std::string, std::string> &s,
                  const std::unordered_map<std::string, int> &nm, 
                  std::unordered_set<std::string> &lm,
                  std::vector<std::vector<std::pair<double, int>>> &nc,
                  const std::unordered_map<ParameterName, Parameter> &p,
                  const std::vector<std::pair<Model, std::string>> &models,
                  const AnalysisType &antyp,
                  const IntegrationType & inttyp,
                  const double &timestep,
                  int &branchIndex) {
  std::vector<std::string> tokens = Misc::tokenize_space(s.first);

  JJ temp;
  temp.set_label(tokens.at(0), lm);
  // Junction line has potential to have up to 6 parts, identifying the last 3 can be tricky.
  for(int i = 0; i < tokens.size(); ++i) {
    if(tokens.at(i).find("AREA=") != std::string::npos) {
      temp.set_area(std::make_pair(tokens.at(i).substr(tokens.at(i).find("AREA=") + 5), s.second), p);
      tokens.erase(tokens.begin() + i);
    }
  }
  temp.set_model(std::make_pair(tokens.back(), s.second), models);
  temp.set_pn1(temp.get_model().get_phaseOffset());
  temp.set_gLarge(temp.get_model().get_criticalCurrent() /
          (temp.get_model().get_criticalToNormalRatio() * temp.get_model().get_deltaV()));
  if(inttyp == IntegrationType::Trapezoidal) {
    // (1/R0) + (2C/h)
    temp.set_subCond((1/temp.get_model().get_subgapResistance()) + ((2.0 * temp.get_model().get_capacitance()) / timestep));
    // (GL) + (2C/h)
    temp.set_transCond(temp.get_gLarge() + ((2.0 * temp.get_model().get_capacitance()) / timestep));
    // (1/RN) + (2C/h)
    temp.set_normalCond((1/temp.get_model().get_normalResistance()) + ((2.0 * temp.get_model().get_capacitance()) / timestep));
    temp.set_phaseConst(timestep, antyp);
  } else {
    // (1/R0) + (3C/2h)
    temp.set_subCond((1/temp.get_model().get_subgapResistance()) + ((3.0 * temp.get_model().get_capacitance()) / (2.0 * timestep)));
    // (GL) + (3C/2h)
    temp.set_transCond(temp.get_gLarge() + ((3.0 * temp.get_model().get_capacitance()) / (2.0 * timestep)));
    // (1/RN) + (3C/2h)
    temp.set_normalCond((1/temp.get_model().get_normalResistance()) + ((3.0 * temp.get_model().get_capacitance()) / (2.0 * timestep)));
    temp.set_phaseConst_gear(timestep, antyp);
  }
  temp.set_value(1/temp.get_subCond());
  temp.set_del0(1.76 * Constants::BOLTZMANN * temp.get_model().get_criticalTemperature());
  temp.set_del(temp.get_del0() * sqrt(cos((Constants::PI / 2) 
               * (temp.get_model().get_temperature() / temp.get_model().get_criticalTemperature())
               * (temp.get_model().get_temperature() / temp.get_model().get_criticalTemperature()))));
  temp.set_rncalc(((Constants::PI * temp.get_del()) / (2 * Constants::EV * temp.get_model().get_criticalCurrent()))
                  * tanh(temp.get_del() / (2 * Constants::BOLTZMANN * temp.get_model().get_temperature())));
  temp.set_nonZeros_and_columnIndex(std::make_pair(tokens.at(1), tokens.at(2)), nm, s.first, branchIndex, antyp, timestep);
  temp.set_indices(std::make_pair(tokens.at(1), tokens.at(2)), nm, nc, branchIndex);
  temp.set_variableIndex(branchIndex - 2);
  temp.set_currentIndex(branchIndex - 1);
  return temp;
}

void JJ::set_label(const std::string &s, 
                    std::unordered_set<std::string> &lm) {
  if(lm.count(s) != 0) {
    Errors::invalid_component_errors(ComponentErrors::DUPLICATE_LABEL, s);
  } else {
    label_ = s;
    lm.emplace(s);
  }
}

void JJ::set_nonZeros_and_columnIndex(const std::pair<std::string, std::string> &n, 
                                      const std::unordered_map<std::string, int> &nm, 
                                      const std::string &s, 
                                      int &branchIndex, 
                                      const AnalysisType &antyp, 
                                      const double &timestep) {
  nonZeros_.clear();
  columnIndex_.clear();
  if(n.first != "0" && n.first.find("GND") == std::string::npos) {
    if(nm.count(n.first) == 0) Errors::netlist_errors(NetlistErrors::NO_SUCH_NODE, n.first);
  }
  if(n.second != "0" && n.second.find("GND") == std::string::npos) {
    if(nm.count(n.second) == 0) Errors::netlist_errors(NetlistErrors::NO_SUCH_NODE, n.second);
  }
  if(n.second.find("GND") != std::string::npos || n.second == "0") {
    // 0 0
    if(n.first.find("GND") != std::string::npos || n.first == "0") {
      Errors::invalid_component_errors(ComponentErrors::BOTH_GROUND, s);
      nonZeros_.emplace_back(-phaseConst_);
      rowPointer_.emplace_back(1);
      if(antyp == AnalysisType::Voltage) { 
        nonZeros_.emplace_back(-value_); 
        rowPointer_.emplace_back(1);
        branchIndex++;
        columnIndex_.emplace_back(branchIndex - 2);
        columnIndex_.emplace_back(branchIndex - 1);
      } else if (antyp == AnalysisType::Phase) {
        nonZeros_.emplace_back(1);
        nonZeros_.emplace_back(-value_); 
        rowPointer_.emplace_back(2);
        branchIndex++;
        columnIndex_.emplace_back(branchIndex - 2);
        columnIndex_.emplace_back(branchIndex - 2);
        columnIndex_.emplace_back(branchIndex - 1);
      }
    // 1 0
    } else {
      nonZeros_.emplace_back(1);
      nonZeros_.emplace_back(-phaseConst_);
      rowPointer_.emplace_back(2);
      branchIndex++;
      nonZeros_.emplace_back(1);
      nonZeros_.emplace_back(-value_);
      rowPointer_.emplace_back(2);
      branchIndex++;
      columnIndex_.emplace_back(nm.at(n.first));
      columnIndex_.emplace_back(branchIndex - 2);
      if(antyp == AnalysisType::Voltage) { 
        columnIndex_.emplace_back(nm.at(n.first));
      } else if(antyp == AnalysisType::Phase) {
        columnIndex_.emplace_back(branchIndex - 2);
      }
      columnIndex_.emplace_back(branchIndex - 1);
    }
  // 0 1
  } else if(n.first.find("GND") != std::string::npos || n.first == "0") {
      nonZeros_.emplace_back(-1);
      nonZeros_.emplace_back(-phaseConst_); 
      rowPointer_.emplace_back(2);
      branchIndex++;
      if(antyp == AnalysisType::Voltage) { 
        nonZeros_.emplace_back(-1);
      } else if(antyp == AnalysisType::Phase) {
        nonZeros_.emplace_back(1);
      }
      nonZeros_.emplace_back(-value_);
      rowPointer_.emplace_back(2);
      branchIndex++;
      columnIndex_.emplace_back(nm.at(n.second));
      columnIndex_.emplace_back(branchIndex - 2);
      if(antyp == AnalysisType::Voltage) { 
        columnIndex_.emplace_back(nm.at(n.second));
      } else if(antyp == AnalysisType::Phase) {
        columnIndex_.emplace_back(branchIndex - 2);
      }
      columnIndex_.emplace_back(branchIndex - 1);
  // 1 1
  } else {
    nonZeros_.emplace_back(1);
    nonZeros_.emplace_back(-1);
    nonZeros_.emplace_back(-phaseConst_);
    rowPointer_.emplace_back(3);
    branchIndex++;
    if(antyp == AnalysisType::Voltage) {
      nonZeros_.emplace_back(1);
      nonZeros_.emplace_back(-1);
      nonZeros_.emplace_back(-value_);
      rowPointer_.emplace_back(3);
    } else if(antyp == AnalysisType::Phase) {
      nonZeros_.emplace_back(1);
      nonZeros_.emplace_back(-value_);
      rowPointer_.emplace_back(2);
    }
    branchIndex++;
    columnIndex_.emplace_back(nm.at(n.first));
    columnIndex_.emplace_back(nm.at(n.second));
    columnIndex_.emplace_back(branchIndex - 2);
    if(antyp == AnalysisType::Voltage) {
      columnIndex_.emplace_back(nm.at(n.first));
      columnIndex_.emplace_back(nm.at(n.second));
    } else if(antyp == AnalysisType::Phase) {
      columnIndex_.emplace_back(branchIndex - 2);
    }
    columnIndex_.emplace_back(branchIndex - 1);
  }
}

void JJ::set_indices(const std::pair<std::string, std::string> &n, 
                      const std::unordered_map<std::string, int> &nm, 
                      std::vector<std::vector<std::pair<double, int>>> &nc, 
                      const int &branchIndex) {
  if(n.second.find("GND") != std::string::npos || n.second == "0") {
    posIndex_ = nm.at(n.first);
    nc.at(nm.at(n.first)).emplace_back(std::make_pair(1, branchIndex - 1));
  } else if(n.first.find("GND") != std::string::npos || n.first == "0") {
    negIndex_ = nm.at(n.second);
    nc.at(nm.at(n.second)).emplace_back(std::make_pair(-1, branchIndex - 1));
  } else {
    posIndex_ = nm.at(n.first);
    negIndex_ = nm.at(n.second);
    nc.at(nm.at(n.first)).emplace_back(std::make_pair(1, branchIndex - 1));
    nc.at(nm.at(n.second)).emplace_back(std::make_pair(-1, branchIndex - 1));
  }
}

void JJ::set_area(const std::pair<std::string, std::string> &s, 
                  const std::unordered_map<ParameterName, Parameter> &p) {
          area_ = parse_param(s.first, p, s.second);
}

void JJ::set_model(const std::pair<std::string, std::string> &s, 
                    const std::vector<std::pair<Model, std::string>> &models) {
  bool found = false;
  for(auto &i : models) {
    if(i.first.get_modelName() == s.first && i.second == s.second) {
      model_ = i.first;
      found = true;
      break;
    }
  }
  if(!found) {
    for(auto &i : models) {
      if(i.first.get_modelName() == s.first && i.second == "") {
        model_ = i.first;
        found = true;
      }
    }
  }
  if(!found) {
    Errors::invalid_component_errors(ComponentErrors::MODEL_NOT_DEFINED, s.first);
  }

  model_.set_capacitance(model_.get_capacitance() * area_);
  model_.set_normalResistance(model_.get_normalResistance() / area_);
  model_.set_subgapResistance(model_.get_subgapResistance() / area_);
  model_.set_criticalCurrent(model_.get_criticalCurrent() * area_);

  lowerB_ = model_.get_voltageGap() - 0.5 * model_.get_deltaV();
  upperB_ = model_.get_voltageGap() + 0.5 * model_.get_deltaV();
}

void JJ::set_phaseConst(const double &timestep, 
                        const AnalysisType &antyp){
  if(antyp == AnalysisType::Voltage) phaseConst_ = Constants::HBAR / (timestep * Constants::EV);
  else if (antyp == AnalysisType::Phase) phaseConst_ = (timestep * Constants::EV) / Constants::HBAR;
}

void JJ::set_phaseConst_gear(const double &timestep, 
                        const AnalysisType &antyp){
  if(antyp == AnalysisType::Voltage) phaseConst_ = Constants::SIGMA * (3.0 / (2.0 * timestep));
  else if (antyp == AnalysisType::Phase) phaseConst_ = ((2.0 * timestep) / 3.0) * (1.0 / Constants::SIGMA);
}

// Update the value based on the matrix entry based on the current voltage value
bool JJ::update_value(const double &v) {
  if(fabs(v) < lowerB_) {
    transitionCurrent_ = 0.0;
    if(nonZeros_.back() != -1/subCond_) {
      nonZeros_.back() = -1/subCond_;
      return true;
    } else {
      return false;
    }
  } else if (fabs(v) < upperB_) {
    if (v < 0) {
      transitionCurrent_ = -lowerB_ * ((1 / model_.get_subgapResistance()) 
                           - gLarge_);
    } else {
      transitionCurrent_ = lowerB_ * ((1 / model_.get_subgapResistance()) 
                           - gLarge_);
    }
    if(nonZeros_.back() != -1/transCond_) {
      nonZeros_.back() = -1/transCond_;
      return true;
    } else {
      return false;
    }
  } else {
    if (v < 0) {
      transitionCurrent_ = -(model_.get_criticalCurrent() / model_.get_criticalToNormalRatio() 
                           + model_.get_voltageGap() * (1 / model_.get_subgapResistance()) 
                           - lowerB_ * (1 / model_.get_normalResistance()));
    } else { 
      transitionCurrent_ = (model_.get_criticalCurrent() / model_.get_criticalToNormalRatio() 
                           + model_.get_voltageGap() * (1 / model_.get_subgapResistance()) 
                           - lowerB_ * (1 / model_.get_normalResistance()));
    }
    if(nonZeros_.back() != -1/normalCond_) {
      nonZeros_.back() = -1/normalCond_;
      return true;
    } else {
      return false;
    }
  }
  return false;
}