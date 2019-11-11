// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/JJ.hpp"
#include "JoSIM/Misc.hpp"
#include "JoSIM/Errors.hpp"
#include "JoSIM/Constants.hpp"

JJ JJ::create_jj(
    const std::pair<std::string, std::string> &s,
    const std::unordered_map<std::string, int> &nm, 
    std::vector<std::vector<std::pair<int, int>>> &nc,
    const std::unordered_map<JoSIM::ParameterName, Parameter> &p,
    const std::vector<std::pair<Model, std::string>> &models,
    const int &antyp,
    const double &timestep,
    int &branchIndex) {
  std::vector<std::string> tokens = Misc::tokenize_space(s.first);

  JJ temp;
  temp.set_label(tokens.at(0));
  // Junction line has potential to have up to 6 parts, identifying the last 3 can be tricky.
  for(int i = 0; i < tokens.size(); ++i) {
    if(tokens.at(i).find("AREA=") != std::string::npos) {
      temp.set_area(std::make_pair(tokens.at(i).substr(tokens.at(i).find("AREA=") + 5), s.second), p);
      tokens.erase(tokens.begin() + i);
    }
  }
  temp.set_model(std::make_pair(tokens.back(), s.second), models);
  temp.set_value(timestep);
  temp.set_phaseConst(timestep, antyp);

  temp.set_nonZeros_and_columnIndex(std::make_pair(tokens.at(1), tokens.at(2)), nm, s.first, branchIndex, antyp, timestep);
  temp.set_indices(std::make_pair(tokens.at(1), tokens.at(2)), nm, nc, branchIndex);
  temp.set_variableIndex(branchIndex - 2);
  temp.set_currentIndex(branchIndex - 1);
  return temp;
}

void JJ::set_nonZeros_and_columnIndex(const std::pair<std::string, std::string> &n, 
    const std::unordered_map<std::string, int> &nm, const std::string &s, int &branchIndex, 
    const int &antyp, const double &timestep) {
  nonZeros_.clear();
  columnIndex_.clear();
  if(n.second.find("GND") != std::string::npos || n.second == "0") {
    // 0 0
    if(n.first.find("GND") != std::string::npos || n.first == "0") {
      Errors::invalid_component_errors(static_cast<int>(ComponentErrors::BOTH_GROUND), s);
      nonZeros_.emplace_back(-phaseConst_);
      rowPointer_.emplace_back(1);
      if(antyp == 0) { 
        nonZeros_.emplace_back(-value_); 
        rowPointer_.emplace_back(1);
        branchIndex++;
        columnIndex_.emplace_back(branchIndex - 2);
        columnIndex_.emplace_back(branchIndex - 1);
      } else if (antyp == 1) {
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
      if(antyp == 0) { 
        columnIndex_.emplace_back(nm.at(n.first));
      } else if(antyp == 1) {
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
      if(antyp == 0) { 
        nonZeros_.emplace_back(-1);
      } else if(antyp == 1) {
        nonZeros_.emplace_back(1);
      }
      nonZeros_.emplace_back(-value_);
      rowPointer_.emplace_back(2);
      branchIndex++;
      columnIndex_.emplace_back(nm.at(n.second));
      columnIndex_.emplace_back(branchIndex - 2);
      if(antyp == 0) { 
        columnIndex_.emplace_back(nm.at(n.second));
      } else if(antyp == 1) {
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
    if(antyp == 0) {
      nonZeros_.emplace_back(1);
      nonZeros_.emplace_back(-1);
    } else if(antyp == 1) {
      nonZeros_.emplace_back(1);
    }
    nonZeros_.emplace_back(-value_);
    rowPointer_.emplace_back(3);
    branchIndex++;
    columnIndex_.emplace_back(nm.at(n.first));
    columnIndex_.emplace_back(nm.at(n.second));
    columnIndex_.emplace_back(branchIndex - 2);
    if(antyp == 0) {
      columnIndex_.emplace_back(nm.at(n.first));
      columnIndex_.emplace_back(nm.at(n.second));
    } else if(antyp == 1) {
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
    Errors::invalid_component_errors(static_cast<int>(ComponentErrors::MODEL_NOT_DEFINED), s.first);
  }

  model_.set_capacitance(model_.get_capacitance() * area_);
  model_.set_normalResistance(model_.get_normalResistance() / area_);
  model_.set_subgapResistance(model_.get_subgapResistance() / area_);
  model_.set_criticalCurrent(model_.get_criticalCurrent() * area_);
}

void JJ::set_value(const double &timestep) {
  value_ = (model_.get_subgapResistance() * timestep) / (timestep + 2 * model_.get_subgapResistance() * model_.get_capacitance());
}

void JJ::set_phaseConst(const double &timestep, const int &antyp){
  if(antyp == 0) phaseConst_ = JoSIM::Constants::HBAR / (timestep * JoSIM::Constants::EV);
  else if (antyp == 1) phaseConst_ = (timestep * JoSIM::Constants::EV) / JoSIM::Constants::HBAR;
}