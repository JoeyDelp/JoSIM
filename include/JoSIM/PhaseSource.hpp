// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_PHASESOURCE_HPP
#define JOSIM_PHASESOURCE_HPP

#include "JoSIM/BasicComponent.hpp"
#include "JoSIM/ParameterName.hpp"
#include "JoSIM/Parameters.hpp"

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <optional>


namespace JoSIM {

  /*
   Plabel φ⁺ φ⁻ sourcetype

   ⎡ 0  0  1⎤ ⎡φ⁺⎤   ⎡  0⎤
   ⎜ 0  0 -1⎟ ⎜φ⁻⎟ = ⎜  0⎟
   ⎣ 1 -1  0⎦ ⎣Io⎦   ⎣  0⎦
  */

  class PhaseSource : public BasicComponent {
    public:
    int sourceIndex_;
    PhaseSource(
      const std::pair<tokens_t, string_o>& s, const NodeConfig& ncon,
      const nodemap& nm, std::unordered_set<std::string>& lm,
      nodeconnections& nc, int& bi, const int& ci);
  }; // class PhaseSource

} // namespace JoSIM
#endif