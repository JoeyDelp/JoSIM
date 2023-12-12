// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/Model.hpp"

#include "JoSIM/Errors.hpp"
#include "JoSIM/Misc.hpp"
#include "JoSIM/TypeDefines.hpp"

using namespace JoSIM;

void Model::parse_model(const std::pair<tokens_t, string_o>& s,
                        vector_pair_t<Model, string_o>& models,
                        const param_map& p) {
  // Ensure the model conforms to correct syntax
  if (s.first.size() < 3) {
    Errors::model_errors(ModelErrors::BAD_MODEL_DEFINITION,
                         Misc::vector_to_string(s.first));
  }
  // Create a model that will be stored
  Model temp;
  // The second token is the model label
  temp.modelName(s.first.at(1));
  // The third token needs to start with "JJ" for this to be valid
  if (s.first.at(2).compare(0, 2, "JJ") != 0) {
    Errors::model_errors(ModelErrors::UNKNOWN_MODEL_TYPE,
                         Misc::vector_to_string(s.first));
  }
  // Create a temporary tokens variable containing the model parameters
  tokens_t tokens(s.first.begin() + 2, s.first.end());
  tokens = Misc::tokenize(Misc::vector_to_string(tokens).substr(2), "=(), ");
  // Add any tokens surrounded by curly braces to the same token
  int par_open = -1;
  for (int i = 0; i < tokens.size(); ++i) {
    if (par_open != -1) {
      tokens.at(par_open) += "," + tokens.at(i);
      tokens.erase(tokens.begin() + i);
      --i;
      if (tokens.at(par_open).back() == '}') {
        tokens.at(par_open).pop_back();  
        par_open = -1;
      }
    }
    if (tokens.at(i).front() == '{') {
      par_open = i;
      tokens.at(i) = tokens.at(i).substr(1);
    }
  }
  if (par_open != -1) {
    Errors::model_errors(JoSIM::ModelErrors::PARAM_PARENTHESIS,
                         Misc::vector_to_string(s.first));
  }
  // Sanity check, there should be an even number of tokens (parameter=value)
  if (tokens.size() % 2 != 0) {
    Errors::model_errors(ModelErrors::BAD_MODEL_DEFINITION,
                         Misc::vector_to_string(s.first));
  }
  double value = 0.0;
  // Loop through the parameter tokens
  for (int64_t i = 0; i < tokens.size(); i += 2) {
    if (tokens.at(i) == "CPR") {
      std::vector<double> v;
      tokens_t t = Misc::tokenize(tokens.at(i + 1), ",");
      for (auto& val : t) v.emplace_back(parse_param(val, p, s.second));
      temp.cpr(v);
    } else {
      // Every even odd token should be a value (otherwise complain)
      value = parse_param(tokens.at(i + 1), p, s.second);
      if (std::isnan(value)) {
        Errors::model_errors(ModelErrors::BAD_MODEL_DEFINITION,
                             Misc::vector_to_string(s.first));
      }
      // Assign the relevant model parameters
      if (tokens.at(i) == "VG" || tokens.at(i) == "VGAP") {
        temp.vg(value);
      } else if (tokens.at(i) == "IC" || tokens.at(i) == "ICRIT") {
        temp.ic(value);
      } else if (tokens.at(i) == "RTYPE") {
        temp.rtype((int64_t)value);
      } else if (tokens.at(i) == "RN") {
        temp.rn(value);
      } else if (tokens.at(i) == "R0") {
        temp.r0(value);
      } else if (tokens.at(i) == "CAP" || tokens.at(i) == "C") {
        temp.c(value);
      } else if (tokens.at(i) == "T") {
        temp.t(value);
        temp.tDep(true);
      } else if (tokens.at(i) == "TC") {
        temp.tc(value);
        temp.tDep(true);
      } else if (tokens.at(i) == "DELV") {
        temp.deltaV(value);
      } else if (tokens.at(i) == "D") {
        temp.d(value);
        temp.tDep(true);
      } else if (tokens.at(i) == "ICFACT" || tokens.at(i) == "ICFCT") {
        temp.icFct(value);
      } else if (tokens.at(i) == "PHI") {
        temp.phiOff(value);
      } else {
        // Incompatible parameter
        Errors::model_errors(ModelErrors::PARAM_TYPE_ERROR,
                             Misc::vector_to_string(
                                 tokens_t{Misc::vector_to_string(s.first),
                                          "\nThe parameter: ", tokens.at(i)}));
      }
    }
  }

  models.emplace_back(std::make_pair(temp, s.second));
}