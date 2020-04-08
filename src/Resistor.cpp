// Copyright (c) 2020 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/Resistor.hpp"
#include "JoSIM/IntegrationType.hpp"
#include "JoSIM/Misc.hpp"
#include "JoSIM/Errors.hpp"
#include "JoSIM/Constants.hpp"

#include <utility>

using namespace JoSIM;

Resistor Resistor::create_resistor(const std::pair<std::string, std::string> &s,
                                    const std::unordered_map<std::string, int> &nm, 
                                    std::unordered_set<std::string> &lm,
                                    std::vector<std::vector<std::pair<double, int>>> &nc,
                                    const std::unordered_map<ParameterName, Parameter> &p,
                                    const AnalysisType &antyp,
                                    const IntegrationType & inttyp,
                                    const double &timestep,
                                    int &branchIndex) {
  std::vector<std::string> tokens = Misc::tokenize_space(s.first);
  
  Resistor temp;
  temp.set_label(tokens.at(0), lm);
  if(s.first.find("{") != std::string::npos) {
    if(s.first.find("}") != std::string::npos) {
      tokens.at(3) = s.first.substr(s.first.find("{")+1, s.first.find("}") - s.first.find("{"));
    } else {
      Errors::invalid_component_errors(ComponentErrors::INVALID_EXPR, s.first);
    }
  }
  if(inttyp == IntegrationType::Trapezoidal) {
    temp.set_value(std::make_pair(tokens.at(3), s.second), p, antyp, timestep);
  } else {
    temp.set_value_gear(std::make_pair(tokens.at(3), s.second), p, antyp, timestep);
  }
  temp.set_nonZeros_and_columnIndex(std::make_pair(tokens.at(1), tokens.at(2)), nm, s.first, branchIndex);
  temp.set_indices(std::make_pair(tokens.at(1), tokens.at(2)), nm, nc, branchIndex);
  temp.set_currentIndex(branchIndex - 1);
  return temp;
}

void Resistor::set_label(const std::string &s, std::unordered_set<std::string> &lm) {
  if(lm.count(s) != 0) {
    Errors::invalid_component_errors(ComponentErrors::DUPLICATE_LABEL, s);
  } else {
    label_ = s;
    lm.emplace(s);
  }
}

void Resistor::set_nonZeros_and_columnIndex(const std::pair<std::string, std::string> &n, 
                                            const std::unordered_map<std::string, int> &nm, 
                                            const std::string &s, 
                                            int &branchIndex) {
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
      nonZeros_.emplace_back(-value_);
      rowPointer_.emplace_back(1);
      branchIndex++;
      columnIndex_.emplace_back(branchIndex - 1);
    // 1 0
    } else {
      nonZeros_.emplace_back(1);
      nonZeros_.emplace_back(-value_);
      rowPointer_.emplace_back(2);
      branchIndex++;
      columnIndex_.emplace_back(nm.at(n.first));
      columnIndex_.emplace_back(branchIndex - 1);
    }
  // 0 1
  } else if(n.first.find("GND") != std::string::npos || n.first == "0") {
      nonZeros_.emplace_back(-1);
      nonZeros_.emplace_back(-value_);
      rowPointer_.emplace_back(2);
      branchIndex++;
      columnIndex_.emplace_back(nm.at(n.second));
      columnIndex_.emplace_back(branchIndex - 1);
  // 1 1
  } else {
    nonZeros_.emplace_back(1);
    nonZeros_.emplace_back(-1);
    nonZeros_.emplace_back(-value_);
    rowPointer_.emplace_back(3);
    branchIndex++;
    columnIndex_.emplace_back(nm.at(n.first));
    columnIndex_.emplace_back(nm.at(n.second));
    columnIndex_.emplace_back(branchIndex - 1);
  }
}

void Resistor::set_indices(const std::pair<std::string, std::string> &n, 
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

void Resistor::set_value(const std::pair<std::string, std::string> &s, 
                          const std::unordered_map<ParameterName, Parameter> &p,
                          const AnalysisType &antyp, 
                          const double &timestep) {
          if (antyp == AnalysisType::Voltage) value_ = parse_param(s.first, p, s.second);
          else if (antyp == AnalysisType::Phase) value_ = (timestep/(2.0 * Constants::SIGMA)) * parse_param(s.first, p, s.second);
        }

void Resistor::set_value_gear(const std::pair<std::string, std::string> &s, 
                              const std::unordered_map<ParameterName, Parameter> &p,
                              const AnalysisType &antyp, 
                              const double &timestep) {
          if (antyp == AnalysisType::Voltage) value_ = parse_param(s.first, p, s.second);
          else if (antyp == AnalysisType::Phase) value_ = ((2.0 * timestep)/(3.0 * Constants::SIGMA)) * parse_param(s.first, p, s.second);
        }