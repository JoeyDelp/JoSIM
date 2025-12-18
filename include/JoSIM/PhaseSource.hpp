// Copyright (c) 2025 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_PHASESOURCE_HPP
#define JOSIM_PHASESOURCE_HPP

#include "JoSIM/BasicComponent.hpp"
#include "JoSIM/ParameterName.hpp"
#include "JoSIM/Parameters.hpp"

#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace JoSIM {

/*
  Plabel phi+ phi- sourcetype

  [ 0  0  1 ] [ phi+ ]   [ 0 ]
  | 0  0 -1 | | phi- | = | 0 |
  [ 1 -1  0 ] [  Io  ]   [ 0 ]
*/

class PhaseSource : public BasicComponent {
  public:
    int64_t sourceIndex_;
    PhaseSource(const std::pair<tokens_t, string_o>& s,
                const NodeConfig&                    ncon,
                const nodemap&                       nm,
                std::unordered_set<std::string>&     lm,
                nodeconnections&                     nc,
                int64_t&                             bi,
                const int64_t&                       ci);
}; // class PhaseSource

} // namespace JoSIM
#endif // JOSIM_PHASESOURCE_HPP
