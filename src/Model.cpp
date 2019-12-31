// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/Model.hpp"
#include "JoSIM/Misc.hpp"
#include "JoSIM/Errors.hpp"

void Model::parse_model(
    const std::pair<std::string, std::string> &s,
    std::vector<std::pair<Model,std::string>> &models,
    const std::unordered_map<JoSIM::ParameterName, Parameter> &p) {
  
  // Split keywords using spaces
  std::vector<std::string> tokens = Misc::tokenize_space(s.first);
  // Ensure the model conforms to correct syntax: .model modelname modeltype(parameters)
  if(tokens.size() < 3) {
    Errors::model_errors(ModelErrors::BAD_MODEL_DEFINITION, s.first);
  }
  Model temp;

  temp.set_modelName(tokens.at(1));
  
  auto startParam = s.first.find("(");
  auto endParam = s.first.find(")");
  if(tokens.at(2).find("JJ") != std::string::npos) {
    if(startParam == std::string::npos) {
      Errors::model_errors(ModelErrors::BAD_MODEL_DEFINITION, s.first);
    } else if(endParam == std::string::npos) {
      Errors::model_errors(ModelErrors::BAD_MODEL_DEFINITION, s.first);
    } else {
      tokens = Misc::tokenize_delimiter(s.first.substr(startParam+1, endParam), "();, \t");
    }
  } else {
    Errors::model_errors(ModelErrors::UNKNOWN_MODEL_TYPE, s.first);
  }
  
  for (int i = 0; i < tokens.size(); i++) {
    if(tokens.at(i) == "=" && (i-1) >= 0 && (i+1) < tokens.size()) {
      tokens.at(i-1) += tokens.at(i);
      tokens.at(i-1) += tokens.at(i+1);
      tokens.erase(tokens.begin() + i);
      tokens.erase(tokens.begin() + i);
    } else if(tokens.at(i).back() == '=' && (i+1) < tokens.size()) {
      tokens.at(i) += tokens.at(i+1);
      tokens.erase(tokens.begin() + i + 1);
    } else if(tokens.at(i).front() == '=' && (i-1) >= 0) {
      tokens.at(i-1) += tokens.at(i);
      tokens.erase(tokens.begin() + i);
    }
  }

  for (int i = 0; i < tokens.size(); ++i) {
    std::vector<std::string> itemToken = Misc::tokenize_delimiter(tokens.at(i), "=");
    if (itemToken.size() == 1) {
      Errors::model_errors(ModelErrors::BAD_MODEL_DEFINITION, s.first);
    }
    double value = Parameters::parse_param(itemToken.at(1), p,
                                s.second);
    if (itemToken.at(0) == "VG" || itemToken.at(0) == "VGAP")
      temp.set_voltageGap(value);
    else if (itemToken.at(0) == "IC" || itemToken.at(0) == "ICRIT")
      temp.set_criticalCurrent(value);
    else if (itemToken.at(0) == "RTYPE")
      temp.set_resistanceType((int)value);
    else if (itemToken.at(0) == "RN")
      temp.set_normalResistance(value);
    else if (itemToken.at(0) == "R0")
      temp.set_subgapResistance(value);
    else if (itemToken.at(0) == "CAP" || itemToken.at(0) == "C")
      temp.set_capacitance(value);
    else if (itemToken.at(0) == "T")
      temp.set_temperature(value);
    else if (itemToken.at(0) == "TC")
      temp.set_criticalTemperature(value);
    else if (itemToken.at(0) == "DELV")
      temp.set_deltaV(value);
    else if (itemToken.at(0) == "D")
      temp.set_transparency(value);
    else if (itemToken.at(0) == "ICFACT" || itemToken.at(0) == "ICFCT")
      temp.set_criticalToNormalRatio(value);
    else if (itemToken.at(0) == "PHI")
      temp.set_phaseOffset(value);
  }

  models.emplace_back(std::make_pair(temp, s.second));
}