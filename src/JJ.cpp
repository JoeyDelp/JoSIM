// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/JJ.hpp"
#include "JoSIM/Misc.hpp"
#include "JoSIM/Errors.hpp"
#include "JoSIM/Constants.hpp"

JJ JJ::create_jj(
    const std::pair<std::string, std::string> &s,
    const std::unordered_map<std::string, int> &nm, 
    std::unordered_set<std::string> &lm,
    std::vector<std::vector<std::pair<int, int>>> &nc,
    const std::unordered_map<JoSIM::ParameterName, Parameter> &p,
    const std::vector<std::pair<Model, std::string>> &models,
    const JoSIM::AnalysisType &antyp,
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
  temp.set_subCond((temp.get_model().get_subgapResistance() * timestep) / (timestep + 2 * temp.get_model().get_subgapResistance() * temp.get_model().get_capacitance()));
  temp.set_transCond((temp.get_model().get_criticalToNormalRatio() * temp.get_model().get_deltaV() * timestep) 
                     / (temp.get_model().get_criticalCurrent() * timestep 
                     + 2 * temp.get_model().get_criticalToNormalRatio() * temp.get_model().get_deltaV() * temp.get_model().get_capacitance()));
  temp.set_normalCond((temp.get_model().get_normalResistance() * timestep) / (timestep + 2 * temp.get_model().get_normalResistance() * temp.get_model().get_capacitance()));
  temp.set_value(temp.get_subCond());
  temp.set_del0(1.76 * JoSIM::Constants::BOLTZMANN * temp.get_model().get_criticalTemperature());
  temp.set_del(temp.get_del0() * sqrt(cos((JoSIM::Constants::PI / 2) 
               * (temp.get_model().get_temperature() / temp.get_model().get_criticalTemperature())
               * (temp.get_model().get_temperature() / temp.get_model().get_criticalTemperature()))));
  temp.set_rncalc(((JoSIM::Constants::PI * temp.get_del()) / (2 * JoSIM::Constants::EV * temp.get_model().get_criticalCurrent()))
                  * tanh(temp.get_del() / (2 * JoSIM::Constants::BOLTZMANN * temp.get_model().get_temperature())));
  temp.set_phaseConst(timestep, antyp);
  temp.set_nonZeros_and_columnIndex(std::make_pair(tokens.at(1), tokens.at(2)), nm, s.first, branchIndex, antyp, timestep);
  temp.set_indices(std::make_pair(tokens.at(1), tokens.at(2)), nm, nc, branchIndex);
  temp.set_variableIndex(branchIndex - 2);
  temp.set_currentIndex(branchIndex - 1);
  return temp;
}

void JJ::set_label(const std::string &s, std::unordered_set<std::string> &lm) {
  if(lm.count(s) != 0) {
    Errors::invalid_component_errors(ComponentErrors::DUPLICATE_LABEL, s);
  } else {
    label_ = s;
  }
}

void JJ::set_nonZeros_and_columnIndex(const std::pair<std::string, std::string> &n, 
    const std::unordered_map<std::string, int> &nm, const std::string &s, int &branchIndex, 
    const JoSIM::AnalysisType &antyp, const double &timestep) {
  nonZeros_.clear();
  columnIndex_.clear();
  if(n.second.find("GND") != std::string::npos || n.second == "0") {
    // 0 0
    if(n.first.find("GND") != std::string::npos || n.first == "0") {
      Errors::invalid_component_errors(ComponentErrors::BOTH_GROUND, s);
      nonZeros_.emplace_back(-phaseConst_);
      rowPointer_.emplace_back(1);
      if(antyp == JoSIM::AnalysisType::Voltage) { 
        nonZeros_.emplace_back(-value_); 
        rowPointer_.emplace_back(1);
        branchIndex++;
        columnIndex_.emplace_back(branchIndex - 2);
        columnIndex_.emplace_back(branchIndex - 1);
      } else if (antyp == JoSIM::AnalysisType::Phase) {
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
      if(antyp == JoSIM::AnalysisType::Voltage) { 
        columnIndex_.emplace_back(nm.at(n.first));
      } else if(antyp == JoSIM::AnalysisType::Phase) {
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
      if(antyp == JoSIM::AnalysisType::Voltage) { 
        nonZeros_.emplace_back(-1);
      } else if(antyp == JoSIM::AnalysisType::Phase) {
        nonZeros_.emplace_back(1);
      }
      nonZeros_.emplace_back(-value_);
      rowPointer_.emplace_back(2);
      branchIndex++;
      columnIndex_.emplace_back(nm.at(n.second));
      columnIndex_.emplace_back(branchIndex - 2);
      if(antyp == JoSIM::AnalysisType::Voltage) { 
        columnIndex_.emplace_back(nm.at(n.second));
      } else if(antyp == JoSIM::AnalysisType::Phase) {
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
    if(antyp == JoSIM::AnalysisType::Voltage) {
      nonZeros_.emplace_back(1);
      nonZeros_.emplace_back(-1);
    } else if(antyp == JoSIM::AnalysisType::Phase) {
      nonZeros_.emplace_back(1);
    }
    nonZeros_.emplace_back(-value_);
    rowPointer_.emplace_back(3);
    branchIndex++;
    columnIndex_.emplace_back(nm.at(n.first));
    columnIndex_.emplace_back(nm.at(n.second));
    columnIndex_.emplace_back(branchIndex - 2);
    if(antyp == JoSIM::AnalysisType::Voltage) {
      columnIndex_.emplace_back(nm.at(n.first));
      columnIndex_.emplace_back(nm.at(n.second));
    } else if(antyp == JoSIM::AnalysisType::Phase) {
      columnIndex_.emplace_back(branchIndex - 2);
    }
    columnIndex_.emplace_back(branchIndex - 1);
  }
}

void JJ::set_indices(const std::pair<std::string, std::string> &n, const std::unordered_map<std::string, int> &nm, std::vector<std::vector<std::pair<int, int>>> &nc, const int &branchIndex) {
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
        const std::unordered_map<JoSIM::ParameterName, Parameter> &p) {
          area_ = Parameters::parse_param(s.first, p, s.second);
}

void JJ::set_model(const std::pair<std::string, std::string> &s, const std::vector<std::pair<Model, std::string>> &models) {
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

void JJ::set_phaseConst(const double &timestep, const JoSIM::AnalysisType &antyp){
  if(antyp == JoSIM::AnalysisType::Voltage) phaseConst_ = JoSIM::Constants::HBAR / (timestep * JoSIM::Constants::EV);
  else if (antyp == JoSIM::AnalysisType::Phase) phaseConst_ = (timestep * JoSIM::Constants::EV) / JoSIM::Constants::HBAR;
}

// Update the value based on the matrix entry based on the current voltage value
bool JJ::update_value(const double &v) {
  if(abs(v) < lowerB_) {
    transitionCurrent_ = 0.0;
    if(nonZeros_.back() != -subCond_) {
      nonZeros_.back() = -subCond_;
      return true;
    } else {
      return false;
    }
  } else if (abs(v) < upperB_) {
    if (v < 0) {
      transitionCurrent_ = -lowerB_ * ((1 / model_.get_subgapResistance()) 
                           - (model_.get_criticalCurrent()/(model_.get_criticalToNormalRatio() * model_.get_deltaV())));
    } else {
      transitionCurrent_ = lowerB_ * ((1 / model_.get_subgapResistance()) 
                           - (model_.get_criticalCurrent()/(model_.get_criticalToNormalRatio() * model_.get_deltaV())));
    }
    if(nonZeros_.back() != -transCond_) {
      nonZeros_.back() = -transCond_;
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
    if(nonZeros_.back() != -normalCond_) {
      nonZeros_.back() = -normalCond_;
      return true;
    } else {
      return false;
    }
  }
  return false;
}