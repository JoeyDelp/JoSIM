// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/VCVS.hpp"
#include "JoSIM/Misc.hpp"
#include "JoSIM/Errors.hpp"
#include "JoSIM/Constants.hpp"

#include <utility>

VCVS VCVS::create_VCVS(
    const std::pair<std::string, std::string> &s,
    const std::unordered_map<std::string, int> &nm, 
    std::unordered_set<std::string> &lm,
    std::vector<std::vector<std::pair<double, int>>> &nc,
    const std::unordered_map<JoSIM::ParameterName, Parameter> &p,
    int &branchIndex) {
  std::vector<std::string> tokens = Misc::tokenize_space(s.first);
  
  VCVS temp;
  temp.set_label(tokens.at(0), lm);
  if(s.first.find("{") != std::string::npos) {
    if(s.first.find("}") != std::string::npos) {
      tokens.at(5) = s.first.substr(s.first.find("{")+1, s.first.find("}") - s.first.find("{"));
    } else {
      Errors::invalid_component_errors(ComponentErrors::INVALID_EXPR, s.first);
    }
  }
  temp.set_value(std::make_pair(tokens.at(5), s.second), p);
  temp.set_nonZeros_and_columnIndex(std::make_pair(tokens.at(1), tokens.at(2)), std::make_pair(tokens.at(3), tokens.at(4)), nm, s.first, branchIndex);
  temp.set_indices(std::make_pair(tokens.at(1), tokens.at(2)), std::make_pair(tokens.at(3), tokens.at(4)), nm, nc, branchIndex);
  temp.set_currentIndex(branchIndex - 1);
  return temp;
}

void VCVS::set_label(const std::string &s, std::unordered_set<std::string> &lm) {
  if(lm.count(s) != 0) {
    Errors::invalid_component_errors(ComponentErrors::DUPLICATE_LABEL, s);
  } else {
    label_ = s;
    lm.emplace(s);
  }
}

void VCVS::set_nonZeros_and_columnIndex(const std::pair<std::string, std::string> &n1, const std::pair<std::string, std::string> &n2, 
  const std::unordered_map<std::string, int> &nm, const std::string &s, int &branchIndex) {
  nonZeros_.clear();
  columnIndex_.clear();
  if(n1.first != "0" && n1.first.find("GND") == std::string::npos) {
    if(nm.count(n1.first) == 0) Errors::netlist_errors(NetlistErrors::NO_SUCH_NODE, n1.first);
  }
  if(n1.second != "0" && n1.second.find("GND") == std::string::npos) {
    if(nm.count(n1.second) == 0) Errors::netlist_errors(NetlistErrors::NO_SUCH_NODE, n1.second);
  }
  if(n2.first != "0" && n2.first.find("GND") == std::string::npos) {
    if(nm.count(n2.first) == 0) Errors::netlist_errors(NetlistErrors::NO_SUCH_NODE, n2.first);
  }
  if(n2.second != "0" && n2.second.find("GND") == std::string::npos) {
    if(nm.count(n2.second) == 0) Errors::netlist_errors(NetlistErrors::NO_SUCH_NODE, n2.second);
  }
  // 0
  if(n1.first.find("GND") != std::string::npos || n1.first == "0")  {
    // 0 0
    if(n1.second.find("GND") != std::string::npos || n1.second == "0")  {
      // 0 0 0
      if(n2.first.find("GND") != std::string::npos || n2.first == "0")  {
        // 0 0 0 0  
        if(n2.second.find("GND") != std::string::npos || n2.second == "0")  {
        // 0 0 0 1
        } else {
          Errors::invalid_component_errors(ComponentErrors::BOTH_GROUND, s);
          nonZeros_.emplace_back(-value_);
          rowPointer_.emplace_back(1);
          branchIndex++;
          columnIndex_.emplace_back(nm.at(n2.second));
        }
      // 0 0 1 0  
      } else if(n2.second.find("GND") != std::string::npos || n2.second == "0")  {
        Errors::invalid_component_errors(ComponentErrors::BOTH_GROUND, s);
        nonZeros_.emplace_back(value_);
        rowPointer_.emplace_back(1);
        branchIndex++;
        columnIndex_.emplace_back(nm.at(n2.first));
      // 0 0 1 1
      } else {
        Errors::invalid_component_errors(ComponentErrors::BOTH_GROUND, s);
        nonZeros_.emplace_back(value_);
        nonZeros_.emplace_back(-value_);
        rowPointer_.emplace_back(2);
        branchIndex++;
        columnIndex_.emplace_back(nm.at(n2.first));
        columnIndex_.emplace_back(nm.at(n2.second));
      }
    // 0 1  
    } else if(n2.first.find("GND") != std::string::npos || n2.first == "0")  {
      // 0 1 0
      if(n2.first.find("GND") != std::string::npos)  {
        // 0 1 0 0  
        if(n2.second.find("GND") != std::string::npos || n2.second == "0")  {
          Errors::invalid_component_errors(ComponentErrors::BOTH_GROUND, s);
          nonZeros_.emplace_back(1);
          rowPointer_.emplace_back(1);
          branchIndex++;
          columnIndex_.emplace_back(nm.at(n1.second));
        // 0 1 0 1
        } else {
          nonZeros_.emplace_back(1);
          nonZeros_.emplace_back(-value_);
          rowPointer_.emplace_back(2);
          branchIndex++;
          columnIndex_.emplace_back(nm.at(n1.second));
          columnIndex_.emplace_back(nm.at(n2.second));
        }
      // 0 1 1 0  
      } else if(n2.second.find("GND") != std::string::npos || n2.second == "0")  {
        nonZeros_.emplace_back(1);
        nonZeros_.emplace_back(value_);
        rowPointer_.emplace_back(2);
        branchIndex++;
        columnIndex_.emplace_back(nm.at(n1.second));
        columnIndex_.emplace_back(nm.at(n2.first));
      // 0 1 1 1
      } else {
        nonZeros_.emplace_back(1);
        nonZeros_.emplace_back(value_);
        nonZeros_.emplace_back(-value_);
        rowPointer_.emplace_back(3);
        branchIndex++;
        columnIndex_.emplace_back(nm.at(n1.second));
        columnIndex_.emplace_back(nm.at(n2.first));
        columnIndex_.emplace_back(nm.at(n2.second));
      }
    }
  // 1
  } else {
    // 1 0
    if(n1.second.find("GND") != std::string::npos || n1.second == "0")  {
      // 1 0 0
      if(n2.first.find("GND") != std::string::npos || n2.first == "0")  {
        // 1 0 0 0  
        if(n2.second.find("GND") != std::string::npos || n2.second == "0")  {
          Errors::invalid_component_errors(ComponentErrors::BOTH_GROUND, s);
        // 1 0 0 1
        } else {
          nonZeros_.emplace_back(-1);
          nonZeros_.emplace_back(-value_);
          rowPointer_.emplace_back(2);
          branchIndex++;
          columnIndex_.emplace_back(nm.at(n1.first));
          columnIndex_.emplace_back(nm.at(n2.second));
        }
      // 1 0 1 0  
      } else if(n2.second.find("GND") != std::string::npos || n2.second == "0")  {
        nonZeros_.emplace_back(-1);
        nonZeros_.emplace_back(value_);
        rowPointer_.emplace_back(2);
        branchIndex++;
        columnIndex_.emplace_back(nm.at(n1.first));
        columnIndex_.emplace_back(nm.at(n2.first));
      // 1 0 1 1
      } else {
        nonZeros_.emplace_back(-1);
        nonZeros_.emplace_back(value_);
        nonZeros_.emplace_back(-value_);
        rowPointer_.emplace_back(3);
        branchIndex++;
        columnIndex_.emplace_back(nm.at(n1.first));
        columnIndex_.emplace_back(nm.at(n2.first));
        columnIndex_.emplace_back(nm.at(n2.second));
      }
    // 1 1  
    } else if(n2.first.find("GND") != std::string::npos || n2.first == "0")  {
      // 1 1 0
      if(n2.first.find("GND") != std::string::npos)  {
        // 1 1 0 0  
        if(n2.second.find("GND") != std::string::npos || n2.second == "0")  {
          Errors::invalid_component_errors(ComponentErrors::BOTH_GROUND, s);
          nonZeros_.emplace_back(-1);
          nonZeros_.emplace_back(1);
          rowPointer_.emplace_back(2);
          branchIndex++;
          columnIndex_.emplace_back(nm.at(n1.first));
          columnIndex_.emplace_back(nm.at(n1.second));
        // 1 1 0 1
        } else {
          nonZeros_.emplace_back(-1);
          nonZeros_.emplace_back(1);
          nonZeros_.emplace_back(-value_);
          rowPointer_.emplace_back(3);
          branchIndex++;
          columnIndex_.emplace_back(nm.at(n1.first));
          columnIndex_.emplace_back(nm.at(n1.second));
          columnIndex_.emplace_back(nm.at(n2.second));
        }
      // 1 1 1 0  
      } else if(n2.second.find("GND") != std::string::npos || n2.second == "0")  {
        nonZeros_.emplace_back(-1);
        nonZeros_.emplace_back(1);
        nonZeros_.emplace_back(value_);
        rowPointer_.emplace_back(3);
        branchIndex++;
        columnIndex_.emplace_back(nm.at(n1.first));
        columnIndex_.emplace_back(nm.at(n1.second));
        columnIndex_.emplace_back(nm.at(n2.first));
      // 1 1 1 1
      } else {
        nonZeros_.emplace_back(-1);
        nonZeros_.emplace_back(1);
        nonZeros_.emplace_back(value_);
        nonZeros_.emplace_back(-value_);
        rowPointer_.emplace_back(4);
        branchIndex++;
        columnIndex_.emplace_back(nm.at(n1.first));
        columnIndex_.emplace_back(nm.at(n1.second));
        columnIndex_.emplace_back(nm.at(n2.first));
        columnIndex_.emplace_back(nm.at(n2.second));
      }
    }
  }
}

void VCVS::set_indices(const std::pair<std::string, std::string> &n1, const std::pair<std::string, std::string> &n2, 
  const std::unordered_map<std::string, int> &nm, std::vector<std::vector<std::pair<double, int>>> &nc, const int &branchIndex) {
  if(n1.second.find("GND") != std::string::npos || n1.second == "0") {
    posIndex1_ = nm.at(n1.first);
    nc.at(nm.at(n1.first)).emplace_back(std::make_pair(1, branchIndex - 1));
  } else if(n1.first.find("GND") != std::string::npos || n1.first == "0") {
    negIndex1_ = nm.at(n1.second);
    nc.at(nm.at(n1.second)).emplace_back(std::make_pair(-1, branchIndex - 1));
  } else {
    posIndex1_ = nm.at(n1.first);
    negIndex1_ = nm.at(n1.second);
    nc.at(nm.at(n1.first)).emplace_back(std::make_pair(1, branchIndex - 1));
    nc.at(nm.at(n1.second)).emplace_back(std::make_pair(-1, branchIndex - 1));
  }
  if(n2.second.find("GND") != std::string::npos || n2.second == "0") {
    posIndex2_ = nm.at(n2.first);
  } else if(n2.first.find("GND") != std::string::npos || n2.first == "0") {
    negIndex2_ = nm.at(n2.second);
  } else {
    posIndex2_ = nm.at(n2.first);
    negIndex2_ = nm.at(n2.second);
  }
}

void VCVS::set_value(const std::pair<std::string, std::string> &s, 
  const std::unordered_map<JoSIM::ParameterName, Parameter> &p) {
  value_ = JoSIM::Parameters::parse_param(s.first, p, s.second);
}