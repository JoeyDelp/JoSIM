// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#ifndef JOSIM_MUTUALINDUCTANCE_HPP
#define JOSIM_MUTUALINDUCTANCE_HPP

#include "./Inductor.hpp"

class MutualInductance {
  public:
    static void create_mutualinductance(const std::pair<std::string, std::string> &s, 
        std::vector<Inductor> &inductors, const std::unordered_map<JoSIM::ParameterName, 
        Parameter> &p, const int &antyp, const double &timestep);
};

#endif