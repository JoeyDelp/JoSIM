// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/TransmissionLine.hpp"
#include "JoSIM/Misc.hpp"
#include "JoSIM/Errors.hpp"
#include "JoSIM/Constants.hpp"

#include <iostream>
#include <string>
#include <algorithm>
#include <locale>
#include <functional>

using namespace JoSIM;

TransmissionLine TransmissionLine::create_transmissionline(
    const std::pair<std::string, std::string> &s,
    const std::unordered_map<std::string, int> &nm, 
    std::unordered_set<std::string> &lm,
    std::vector<std::vector<std::pair<double, int>>> &nc,
    const std::unordered_map<ParameterName, Parameter> &p,
    const AnalysisType &antyp,
    const double &timestep,
    int &branchIndex) {
  std::vector<std::string> tokens = Misc::tokenize_space(s.first);
  // Ensure the device has at least 6 parts: LABEL PNODE1 NNODE1 PNODE2 NNODE2 VALUE
  if(tokens.size() < 6) {
    Errors::invalid_component_errors(ComponentErrors::INVALID_COMPONENT_DECLARATION, s.first);
  }

  TransmissionLine temp;
  temp.set_label(tokens.at(0), lm);
  std::string strippedLine = s.first;
  strippedLine.erase(std::remove_if(strippedLine.begin(), strippedLine.end(), std::bind(std::isspace<char>,
									std::placeholders::_1,
									std::locale::classic()
							)), strippedLine.end());
  auto impedance = strippedLine.find("Z0=");
  if(impedance == std::string::npos) {
    Errors::invalid_component_errors(ComponentErrors::INVALID_TX_DEFINED, s.first);
  }
  auto timeDelay = strippedLine.find("TD=");
  if(timeDelay == std::string::npos) {
    Errors::invalid_component_errors(ComponentErrors::INVALID_TX_DEFINED, s.first);
  }
  std::string impedanceValue;
  std::string timeDelayValue;
  if(timeDelay > impedance) {
    impedanceValue = strippedLine;
    impedanceValue.erase(impedanceValue.begin() + timeDelay, impedanceValue.end());
    impedanceValue.erase(impedanceValue.begin(), impedanceValue.begin() + impedance + 3);
    timeDelayValue = strippedLine;
    timeDelayValue.erase(timeDelayValue.begin(), timeDelayValue.begin() + timeDelay + 3);
  } else {
    timeDelayValue = strippedLine;
    timeDelayValue.erase(timeDelayValue.begin() + impedance, timeDelayValue.end());
    timeDelayValue.erase(timeDelayValue.begin(), timeDelayValue.begin() + timeDelay + 3);
    impedanceValue = strippedLine;
    impedanceValue.erase(impedanceValue.begin(), impedanceValue.begin() + impedance + 3);
  }
  if(impedanceValue.find("{") != std::string::npos) {
    if(impedanceValue.find("}") != std::string::npos) {
      impedanceValue = impedanceValue.substr(impedanceValue.find("{")+1, impedanceValue.find("}") - impedanceValue.find("{"));
    } else {
      Errors::invalid_component_errors(ComponentErrors::INVALID_EXPR, s.first);
    }
  }
  if(timeDelayValue.find("{") != std::string::npos) {
    if(timeDelayValue.find("}") != std::string::npos) {
      timeDelayValue = timeDelayValue.substr(timeDelayValue.find("{")+1, timeDelayValue.find("}") - timeDelayValue.find("{"));
    } else {
      Errors::invalid_component_errors(ComponentErrors::INVALID_EXPR, s.first);
    }
  }
  temp.set_value(std::make_pair(impedanceValue, s.second), p, antyp, timestep);
  temp.set_timestepDelay(std::make_pair(timeDelayValue, s.second), p, timestep);
  temp.set_nonZeros_and_columnIndex(std::make_pair(tokens.at(1), tokens.at(2)), std::make_pair(tokens.at(3), tokens.at(4)), nm, s.first, branchIndex);
  temp.set_indices(std::make_pair(tokens.at(1), tokens.at(2)), std::make_pair(tokens.at(3), tokens.at(4)), nm, nc, branchIndex);
  temp.set_currentIndex1(branchIndex - 2);
  temp.set_currentIndex2(branchIndex - 1);
  return temp;
}

void TransmissionLine::set_label(const std::string &s, std::unordered_set<std::string> &lm) {
  if(lm.count(s) != 0) {
    Errors::invalid_component_errors(ComponentErrors::DUPLICATE_LABEL, s);
  } else {
    label_ = s;
    lm.emplace(s);
  }
}

void TransmissionLine::set_nonZeros_and_columnIndex(const std::pair<std::string, std::string> &n1, 
        const std::pair<std::string, std::string> &n2,
        const std::unordered_map<std::string, int> &nm, 
        const std::string &s, int &branchIndex) {
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
          Errors::invalid_component_errors(ComponentErrors::BOTH_GROUND, s);
          nonZeros_.emplace_back(-value_);
          rowPointer_.emplace_back(1);
          branchIndex++;
          nonZeros_.emplace_back(-value_);
          rowPointer_.emplace_back(1);
          branchIndex++;
          columnIndex_.emplace_back(branchIndex - 2);
          columnIndex_.emplace_back(branchIndex - 1);
        // 0 0 0 1
        } else {
          Errors::invalid_component_errors(ComponentErrors::BOTH_GROUND, s);
          nonZeros_.emplace_back(-value_);
          rowPointer_.emplace_back(1);
          branchIndex++;
          nonZeros_.emplace_back(-1);
          nonZeros_.emplace_back(-value_);
          rowPointer_.emplace_back(2);
          branchIndex++;
          columnIndex_.emplace_back(branchIndex - 2);
          columnIndex_.emplace_back(nm.at(n2.second));
          columnIndex_.emplace_back(branchIndex - 1);
        }
      // 0 0 1 0  
      } else if(n2.second.find("GND") != std::string::npos || n2.second == "0")  {
        Errors::invalid_component_errors(ComponentErrors::BOTH_GROUND, s);
        nonZeros_.emplace_back(-value_);
        rowPointer_.emplace_back(1);
        branchIndex++;
        nonZeros_.emplace_back(1);
        nonZeros_.emplace_back(-value_);
        rowPointer_.emplace_back(2);
        branchIndex++;
        columnIndex_.emplace_back(branchIndex - 2);
        columnIndex_.emplace_back(nm.at(n2.first));
        columnIndex_.emplace_back(branchIndex - 1);
      // 0 0 1 1
      } else {
        Errors::invalid_component_errors(ComponentErrors::BOTH_GROUND, s);
        nonZeros_.emplace_back(-value_);
        rowPointer_.emplace_back(1);
        branchIndex++;
        nonZeros_.emplace_back(1);
        nonZeros_.emplace_back(-1);
        nonZeros_.emplace_back(-value_);
        rowPointer_.emplace_back(3);
        branchIndex++;
        columnIndex_.emplace_back(branchIndex - 2);
        columnIndex_.emplace_back(nm.at(n2.first));
        columnIndex_.emplace_back(nm.at(n2.second));
        columnIndex_.emplace_back(branchIndex - 1);
      }
    // 0 1  
    } else if(n2.first.find("GND") != std::string::npos || n2.first == "0")  {
      // 0 1 0
      if(n2.first.find("GND") != std::string::npos)  {
        // 0 1 0 0  
        if(n2.second.find("GND") != std::string::npos || n2.second == "0")  {
          Errors::invalid_component_errors(ComponentErrors::BOTH_GROUND, s);
          nonZeros_.emplace_back(-1);
          nonZeros_.emplace_back(-value_);
          rowPointer_.emplace_back(2);
          branchIndex++;
          nonZeros_.emplace_back(-value_);
          rowPointer_.emplace_back(1);
          branchIndex++;
          columnIndex_.emplace_back(nm.at(n1.second));
          columnIndex_.emplace_back(branchIndex - 2);
          columnIndex_.emplace_back(branchIndex - 1);
        // 0 1 0 1
        } else {
          nonZeros_.emplace_back(-1);
          nonZeros_.emplace_back(-value_);
          rowPointer_.emplace_back(2);
          branchIndex++;
          nonZeros_.emplace_back(-1);
          nonZeros_.emplace_back(-value_);
          rowPointer_.emplace_back(2);
          branchIndex++;
          columnIndex_.emplace_back(nm.at(n1.second));
          columnIndex_.emplace_back(branchIndex - 2);
          columnIndex_.emplace_back(nm.at(n2.second));
          columnIndex_.emplace_back(branchIndex - 1);
        }
      // 0 1 1 0  
      } else if(n2.second.find("GND") != std::string::npos || n2.second == "0")  {
        nonZeros_.emplace_back(-1);
        nonZeros_.emplace_back(-value_);
        rowPointer_.emplace_back(2);
        branchIndex++;
        nonZeros_.emplace_back(1);
        nonZeros_.emplace_back(-value_);
        rowPointer_.emplace_back(2);
        branchIndex++;
        columnIndex_.emplace_back(nm.at(n1.second));
        columnIndex_.emplace_back(branchIndex - 2);
        columnIndex_.emplace_back(nm.at(n2.first));
        columnIndex_.emplace_back(branchIndex - 1);
      // 0 1 1 1
      } else {
        nonZeros_.emplace_back(-1);
        nonZeros_.emplace_back(-value_);
        rowPointer_.emplace_back(2);
        branchIndex++;
        nonZeros_.emplace_back(1);
        nonZeros_.emplace_back(-1);
        nonZeros_.emplace_back(-value_);
        rowPointer_.emplace_back(3);
        branchIndex++;
        columnIndex_.emplace_back(nm.at(n1.second));
        columnIndex_.emplace_back(branchIndex - 2);
        columnIndex_.emplace_back(nm.at(n2.first));
        columnIndex_.emplace_back(nm.at(n2.second));
        columnIndex_.emplace_back(branchIndex - 1);
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
          nonZeros_.emplace_back(1);
          nonZeros_.emplace_back(-value_);
          rowPointer_.emplace_back(2);
          branchIndex++;
          nonZeros_.emplace_back(-1);
          nonZeros_.emplace_back(-value_);
          rowPointer_.emplace_back(2);
          branchIndex++;
          columnIndex_.emplace_back(nm.at(n1.first));
          columnIndex_.emplace_back(branchIndex - 2);
          columnIndex_.emplace_back(nm.at(n2.second));
          columnIndex_.emplace_back(branchIndex - 1);
        }
      // 1 0 1 0  
      } else if(n2.second.find("GND") != std::string::npos || n2.second == "0")  {
        nonZeros_.emplace_back(1);
        nonZeros_.emplace_back(-value_);
        rowPointer_.emplace_back(2);
        branchIndex++;
        nonZeros_.emplace_back(1);
        nonZeros_.emplace_back(-value_);
        rowPointer_.emplace_back(2);
        branchIndex++;
        columnIndex_.emplace_back(nm.at(n1.first));
        columnIndex_.emplace_back(branchIndex - 2);
        columnIndex_.emplace_back(nm.at(n2.first));
        columnIndex_.emplace_back(branchIndex - 1);
      // 1 0 1 1
      } else {
        nonZeros_.emplace_back(1);
        nonZeros_.emplace_back(-value_);
        rowPointer_.emplace_back(2);
        branchIndex++;
        nonZeros_.emplace_back(1);
        nonZeros_.emplace_back(-1);
        nonZeros_.emplace_back(-value_);
        rowPointer_.emplace_back(3);
        branchIndex++;
        columnIndex_.emplace_back(nm.at(n1.first));
        columnIndex_.emplace_back(branchIndex - 2);
        columnIndex_.emplace_back(nm.at(n2.first));
        columnIndex_.emplace_back(nm.at(n2.second));
        columnIndex_.emplace_back(branchIndex - 1);
      }
    // 1 1  
    } else if(n2.first.find("GND") != std::string::npos || n2.first == "0")  {
      // 1 1 0
      if(n2.first.find("GND") != std::string::npos)  {
        // 1 1 0 0  
        if(n2.second.find("GND") != std::string::npos || n2.second == "0")  {
          Errors::invalid_component_errors(ComponentErrors::BOTH_GROUND, s);
          nonZeros_.emplace_back(1);
          nonZeros_.emplace_back(-1);
          nonZeros_.emplace_back(-value_);
          rowPointer_.emplace_back(3);
          branchIndex++;
          nonZeros_.emplace_back(-value_);
          rowPointer_.emplace_back(1);
          branchIndex++;
          columnIndex_.emplace_back(nm.at(n1.first));
          columnIndex_.emplace_back(nm.at(n1.second));
          columnIndex_.emplace_back(branchIndex - 2);
          columnIndex_.emplace_back(branchIndex - 1);
        // 1 1 0 1
        } else {
          nonZeros_.emplace_back(1);
          nonZeros_.emplace_back(-1);
          nonZeros_.emplace_back(-value_);
          rowPointer_.emplace_back(3);
          branchIndex++;
          nonZeros_.emplace_back(-1);
          nonZeros_.emplace_back(-value_);
          rowPointer_.emplace_back(2);
          branchIndex++;
          columnIndex_.emplace_back(nm.at(n1.first));
          columnIndex_.emplace_back(nm.at(n1.second));
          columnIndex_.emplace_back(branchIndex - 2);
          columnIndex_.emplace_back(nm.at(n2.second));
          columnIndex_.emplace_back(branchIndex - 1);
        }
      // 1 1 1 0  
      } else if(n2.second.find("GND") != std::string::npos || n2.second == "0")  {
        nonZeros_.emplace_back(1);
        nonZeros_.emplace_back(-1);
        nonZeros_.emplace_back(-value_);
        rowPointer_.emplace_back(3);
        branchIndex++;
        nonZeros_.emplace_back(1);
        nonZeros_.emplace_back(-value_);
        rowPointer_.emplace_back(3);
        branchIndex++;
        columnIndex_.emplace_back(nm.at(n1.first));
        columnIndex_.emplace_back(nm.at(n1.second));
        columnIndex_.emplace_back(branchIndex - 2);
        columnIndex_.emplace_back(nm.at(n2.first));
        columnIndex_.emplace_back(branchIndex - 1);
      // 1 1 1 1
      } else {
        nonZeros_.emplace_back(1);
        nonZeros_.emplace_back(-1);
        nonZeros_.emplace_back(-value_);
        rowPointer_.emplace_back(3);
        branchIndex++;
        nonZeros_.emplace_back(1);
        nonZeros_.emplace_back(-1);
        nonZeros_.emplace_back(-value_);
        rowPointer_.emplace_back(3);
        branchIndex++;
        columnIndex_.emplace_back(nm.at(n1.first));
        columnIndex_.emplace_back(nm.at(n1.second));
        columnIndex_.emplace_back(branchIndex - 2);
        columnIndex_.emplace_back(nm.at(n2.first));
        columnIndex_.emplace_back(nm.at(n2.second));
        columnIndex_.emplace_back(branchIndex - 1);
      }
    }
  }
}

void TransmissionLine::set_indices(const std::pair<std::string, std::string> &n1, 
        const std::pair<std::string, std::string> &n2, 
        const std::unordered_map<std::string, int> &nm, std::vector<std::vector<std::pair<double, int>>> &nc, const int &branchIndex) {
  if(n1.second.find("GND") != std::string::npos || n1.second == "0") {
    posIndex1_ = nm.at(n1.first);
    nc.at(nm.at(n1.first)).emplace_back(std::make_pair(1, branchIndex - 2));
  } else if(n1.first.find("GND") != std::string::npos || n1.first == "0") {
    negIndex1_ = nm.at(n1.second);
    nc.at(nm.at(n1.second)).emplace_back(std::make_pair(-1, branchIndex - 2));
  } else {
    posIndex1_ = nm.at(n1.first);
    negIndex1_ = nm.at(n1.second);
    nc.at(nm.at(n1.first)).emplace_back(std::make_pair(1, branchIndex - 2));
    nc.at(nm.at(n1.second)).emplace_back(std::make_pair(-1, branchIndex - 2));
  }
  if(n2.second.find("GND") != std::string::npos || n2.second == "0") {
    posIndex2_ = nm.at(n2.first);
    nc.at(nm.at(n2.first)).emplace_back(std::make_pair(1, branchIndex - 1));
  } else if(n2.first.find("GND") != std::string::npos || n2.first == "0") {
    negIndex2_ = nm.at(n2.second);
    nc.at(nm.at(n2.second)).emplace_back(std::make_pair(-1, branchIndex - 1));
  } else {
    posIndex2_ = nm.at(n2.first);
    negIndex2_ = nm.at(n2.second);
    nc.at(nm.at(n2.first)).emplace_back(std::make_pair(1, branchIndex - 1));
    nc.at(nm.at(n2.second)).emplace_back(std::make_pair(-1, branchIndex - 1));
  }
}

void TransmissionLine::set_value(const std::pair<std::string, std::string> &s, 
        const std::unordered_map<ParameterName, Parameter> &p,
        const AnalysisType &antyp, const double &timestep) {
  if (antyp == AnalysisType::Voltage) {
    value_ = parse_param(s.first, p, s.second);
  } else if (antyp == AnalysisType::Phase) {
    value_ = (timestep * parse_param(s.first, p, s.second)) / (2 * Constants::SIGMA);
  }
}

void TransmissionLine::set_timestepDelay(const std::pair<std::string, std::string> &s, 
        const std::unordered_map<ParameterName, Parameter> &p,
        const double &timestep) {
  timestepDelay_ = (int)(parse_param(s.first, p, s.second) / timestep);
}