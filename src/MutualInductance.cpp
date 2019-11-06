// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/MutualInductance.hpp"
#include "JoSIM/Misc.hpp"
#include "JoSIM/Errors.hpp"
#include "JoSIM/Parameters.hpp"

#include <optional>
#include <cmath>

void MutualInductance::create_mutualinductance(const std::pair<std::string, std::string> &s,
    std::vector<Inductor> &inductors, const std::unordered_map<JoSIM::ParameterName, 
    Parameter> &p, const int &antyp, const double &timestep) {
  std::vector<std::string> tokens = Misc::tokenize_space(s.first);
  if(tokens.size() < 4) {
    Errors::invalid_component_errors(static_cast<int>(ComponentErrors::MUT_ERROR), s.first);
  }
  std::optional<int> ind1Index, ind2Index;
  for (int i = 0; i < inductors.size(); ++i) {
    if(inductors.at(i).get_label() == tokens.at(1)) {
      ind1Index = i;
    }
  }
  if(!ind1Index) {
    Errors::invalid_component_errors(static_cast<int>(ComponentErrors::MISSING_INDUCTOR), tokens.at(1));
  }

  for (int i = 0; i < inductors.size(); ++i) {
    if(inductors.at(i).get_label() == tokens.at(2)) {
      ind2Index = i;
    }
  }
  if(!ind2Index) {
    Errors::invalid_component_errors(static_cast<int>(ComponentErrors::MISSING_INDUCTOR), tokens.at(2));
  }

  double cf = Parameters::parse_param(tokens.at(3), p, s.second);
  double mutual = cf * std::sqrt(inductors.at(ind1Index.value()).get_inductance() * inductors.at(ind2Index.value()).get_inductance());

  inductors.at(ind1Index.value()).add_mutualinductance(mutual, antyp, timestep);
  inductors.at(ind2Index.value()).add_mutualinductance(mutual, antyp, timestep);
}