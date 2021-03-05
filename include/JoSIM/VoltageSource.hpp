// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_VOLTAGESOURCE_HPP
#define JOSIM_VOLTAGESOURCE_HPP

#include "JoSIM/BasicComponent.hpp"
#include "JoSIM/ParameterName.hpp"
#include "JoSIM/Parameters.hpp"
#include "JoSIM/Function.hpp"

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <optional>

namespace JoSIM {

  /*
   Vlabel V⁺ V⁻ sourcetype

   ⎡ 0  0  1⎤ ⎡V⁺⎤   ⎡  0⎤
   ⎜ 0  0 -1⎟ ⎜V⁻⎟ = ⎜  0⎟
   ⎣ 1 -1  0⎦ ⎣Io⎦   ⎣  0⎦
  */

  class VoltageSource : public BasicComponent {
    public:
    int sourceIndex_;
    double pn1_ = 0.0, pn2_ = pn1_, pn3_ = pn2_, pn4_ = 0.0;
    VoltageSource(
      const std::pair<tokens_t, string_o>& s, const NodeConfig& ncon,
      const nodemap& nm, std::unordered_set<std::string>& lm,
      nodeconnections& nc, int& bi, const int& ci);

    void step_back() override {
      pn2_ = pn4_;
    }
  }; // class VoltageSource

} // namespace JoSIM
#endif