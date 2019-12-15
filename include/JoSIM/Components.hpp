// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_COMPONENTS_H
#define JOSIM_COMPONENTS_H

#include "./Misc.hpp"
#include "./Parameters.hpp"
#include "./Constants.hpp"
#include "./Input.hpp"
#include "./Errors.hpp"
#include "./Resistor.hpp"
#include "./Inductor.hpp"
#include "./Capacitor.hpp"
#include "./JJ.hpp"
#include "./VoltageSource.hpp"
#include "./PhaseSource.hpp"
#include "./CurrentSource.hpp"
#include "./TransmissionLine.hpp"

#include <unordered_map>
#include <variant>

class Components {
  public:
    std::vector<std::variant<Resistor, 
                             Inductor, 
                             Capacitor,
                             JJ,
                             VoltageSource,
                             PhaseSource,
                             TransmissionLine>> devices; 
    std::vector<CurrentSource> currentsources;
    std::vector<int> junctionIndices, resistorIndices, inductorIndices, capacitorIndices, vsIndices, psIndices, txIndices;
    std::vector<std::pair<std::string, std::string>> mutualinductances;
};
#endif
