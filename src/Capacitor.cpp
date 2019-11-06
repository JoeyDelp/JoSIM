// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/Capacitor.hpp"
#include "JoSIM/Misc.hpp"
#include "JoSIM/Errors.hpp"
#include "JoSIM/Constants.hpp"

#include <utility>

void Capacitor::create_capacitor(
    const std::pair<std::string, std::string> &s,
    std::vector<Capacitor> &capacitors, 
    const std::unordered_map<std::string, int> &nm, 
    std::vector<int> &nc,
    const std::unordered_map<JoSIM::ParameterName, Parameter> &p,
    const int &antyp,
    const double &timestep,
    int &branchIndex) {
  std::vector<std::string> tokens = Misc::tokenize_space(s.first);
  // Ensure no device duplication occurs
  for(auto &i : capacitors) {
    if(i.get_label() == tokens.at(0)) {
      Errors::invalid_component_errors(static_cast<int>(ComponentErrors::DUPLICATE_LABEL), tokens.at(0));
    }
  }
  Capacitor temp;
  temp.set_label(tokens.at(0));
  if(s.first.find("{") != std::string::npos) {
    if(s.first.find("}") != std::string::npos) {
      tokens.at(3) = s.first.substr(s.first.find("{")+1, s.first.find("}") - s.first.find("{"));
    } else {
      Errors::invalid_component_errors(static_cast<int>(ComponentErrors::INVALID_EXPR), s.first);
    }
  }
  temp.set_value(std::make_pair(tokens.at(3), s.second), p, antyp, timestep);
  temp.set_nonZeros_and_columnIndex(std::make_pair(tokens.at(1), tokens.at(2)), nm, s.first, branchIndex);
  temp.set_indices(std::make_pair(tokens.at(1), tokens.at(2)), nm, nc);
  temp.set_currentIndex(branchIndex);
  capacitors.emplace_back(temp);
}

void Capacitor::set_nonZeros_and_columnIndex(const std::pair<std::string, std::string> &n, const std::unordered_map<std::string, int> &nm, const std::string &s, int &branchIndex) {
  nonZeros_.clear();
  columnIndex_.clear();
  if(n.second.find("GND") != std::string::npos || n.second == "0") {
    // 0 0
    if(n.first.find("GND") != std::string::npos || n.first == "0") {
      Errors::invalid_component_errors(static_cast<int>(ComponentErrors::BOTH_GROUND), s);
      nonZeros_.emplace_back(-value_);
      rowPointer_.emplace_back(1);
      branchIndex++;
      columnIndex_.emplace_back(2);
      columnIndex_.emplace_back(branchIndex - 1);
    // 1 0
    } else {
      nonZeros_.emplace_back(1);
      nonZeros_.emplace_back(1);
      nonZeros_.emplace_back(-value_);
      rowPointer_.emplace_back(2);
      branchIndex++;
      columnIndex_.emplace_back(branchIndex - 1);
      columnIndex_.emplace_back(nm.at(n.first));
      columnIndex_.emplace_back(branchIndex - 1);
    }
  // 0 1
  } else if(n.first.find("GND") != std::string::npos || n.first == "0") {
      nonZeros_.emplace_back(-1);
      nonZeros_.emplace_back(-1);
      nonZeros_.emplace_back(-value_);
      rowPointer_.emplace_back(2);
      branchIndex++;
      columnIndex_.emplace_back(branchIndex - 1);
      columnIndex_.emplace_back(nm.at(n.second));
      columnIndex_.emplace_back(branchIndex - 1);
  // 1 1
  } else {
    nonZeros_.emplace_back(1);
    nonZeros_.emplace_back(-1);
    nonZeros_.emplace_back(1);
    nonZeros_.emplace_back(-1);
    nonZeros_.emplace_back(-value_);
    rowPointer_.emplace_back(3);
    branchIndex++;
    columnIndex_.emplace_back(branchIndex - 1);
    columnIndex_.emplace_back(branchIndex - 1);
    columnIndex_.emplace_back(nm.at(n.first));
    columnIndex_.emplace_back(nm.at(n.second));
    columnIndex_.emplace_back(branchIndex - 1);
  }
}

void Capacitor::set_indices(const std::pair<std::string, std::string> &n, const std::unordered_map<std::string, int> &nm, std::vector<int> &nc) {
  if(n.second.find("GND") != std::string::npos || n.second == "0") {
    posIndex_ = nm.at(n.first);
    ++nc.at(nm.at(n.first));
  } else if(n.first.find("GND") != std::string::npos || n.first == "0") {
    negIndex_ = nm.at(n.second);
    ++nc.at(nm.at(n.second));
  } else {
    posIndex_ = nm.at(n.first);
    negIndex_ = nm.at(n.second);
    ++nc.at(nm.at(n.first));
    ++nc.at(nm.at(n.second));
  }
}

void Capacitor::set_value(const std::pair<std::string, std::string> &s, 
        const std::unordered_map<JoSIM::ParameterName, Parameter> &p,
        const int &antyp, const double &timestep) {
          if (antyp == 0) value_ = (timestep / 2) * (1 / Parameters::parse_param(s.first, p, s.second));
          else if (antyp == 1) value_ = (timestep * timestep) / (4 * JoSIM::Constants::SIGMA * Parameters::parse_param(s.first, p, s.second));
        }