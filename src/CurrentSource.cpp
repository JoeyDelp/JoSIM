// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/CurrentSource.hpp"
#include "JoSIM/Misc.hpp"
#include "JoSIM/Errors.hpp"

void CurrentSource::create_currentsource(
    const std::pair<std::string, std::string> &s,
    std::vector<CurrentSource> &currentsources, 
    const std::unordered_map<std::string, int> &nm) {
  std::vector<std::string> tokens = Misc::tokenize_space(s.first);
  // Ensure no device duplication occurs
  for(auto &i : currentsources) {
    if(i.get_label() == tokens.at(0)) {
      Errors::invalid_component_errors(static_cast<int>(ComponentErrors::DUPLICATE_LABEL), tokens.at(0));
    }
  }
  CurrentSource temp;
  temp.set_label(tokens.at(0));
  temp.set_indices(std::make_pair(tokens.at(1), tokens.at(2)), nm);
  currentsources.emplace_back(temp);
}

void CurrentSource::set_indices(const std::pair<std::string, std::string> &n, const std::unordered_map<std::string, int> &nm) {
  if(n.second.find("GND") != std::string::npos || n.second == "0") {
    posIndex_ = nm.at(n.first);
  } else if(n.first.find("GND") != std::string::npos || n.first == "0") {
    negIndex_ = nm.at(n.second);
  } else {
    posIndex_ = nm.at(n.first);
    negIndex_ = nm.at(n.second);
  }
}
