// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/VoltageSource.hpp"
#include "JoSIM/Misc.hpp"
#include "JoSIM/Errors.hpp"
#include "JoSIM/Constants.hpp"

#include <utility>

void VoltageSource::create_voltagesource(
    const std::pair<std::string, std::string> &s,
    std::vector<VoltageSource> &voltagesources, 
    const std::unordered_map<std::string, int> &nm, 
    std::vector<int> &nc,
    int &branchIndex) {
  std::vector<std::string> tokens = Misc::tokenize_space(s.first);
  // Ensure no device duplication occurs
  for(auto &i : voltagesources) {
    if(i.get_label() == tokens.at(0)) {
      Errors::invalid_component_errors(static_cast<int>(ComponentErrors::DUPLICATE_LABEL), tokens.at(0));
    }
  }
  VoltageSource temp;
  temp.set_label(tokens.at(0));
  temp.set_nonZeros_and_columnIndex(std::make_pair(tokens.at(1), tokens.at(2)), nm, s.first, branchIndex);
  temp.set_indices(std::make_pair(tokens.at(1), tokens.at(2)), nm, nc);
  temp.set_currentIndex(branchIndex);
  voltagesources.emplace_back(temp);
}

void VoltageSource::set_nonZeros_and_columnIndex(const std::pair<std::string, std::string> &n, const std::unordered_map<std::string, int> &nm, const std::string &s, int &branchIndex) {
  nonZeros_.clear();
  columnIndex_.clear();
  if(n.second.find("GND") != std::string::npos || n.second == "0") {
    // 0 0
    if(n.first.find("GND") != std::string::npos || n.first == "0") {
      Errors::invalid_component_errors(static_cast<int>(ComponentErrors::GROUNDED_VOLTAGE_SOURCE), s);
    // 1 0
    } else {
      nonZeros_.emplace_back(1);
      nonZeros_.emplace_back(1);
      rowPointer_.emplace_back(1);
      branchIndex++;
      columnIndex_.emplace_back(branchIndex - 1);
      columnIndex_.emplace_back(nm.at(n.first));
    }
  // 0 1
  } else if(n.first.find("GND") != std::string::npos || n.first == "0") {
      nonZeros_.emplace_back(-1);
      nonZeros_.emplace_back(-1);
      rowPointer_.emplace_back(1);
      branchIndex++;
      columnIndex_.emplace_back(branchIndex - 1);
      columnIndex_.emplace_back(nm.at(n.second));
  // 1 1
  } else {
    nonZeros_.emplace_back(1);
    nonZeros_.emplace_back(-1);
    nonZeros_.emplace_back(1);
    nonZeros_.emplace_back(-1);
    rowPointer_.emplace_back(2);
    branchIndex++;
    columnIndex_.emplace_back(branchIndex - 1);
    columnIndex_.emplace_back(branchIndex - 1);
    columnIndex_.emplace_back(nm.at(n.first));
    columnIndex_.emplace_back(nm.at(n.second));
  }
}

void VoltageSource::set_indices(const std::pair<std::string, std::string> &n, const std::unordered_map<std::string, int> &nm, std::vector<int> &nc) {
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