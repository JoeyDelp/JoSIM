// Copyright (c) 2020 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_CAPACITOR_HPP
#define JOSIM_CAPACITOR_HPP

#include "JoSIM/BasicComponent.hpp"
#include "JoSIM/ParameterName.hpp"
#include "JoSIM/Parameters.hpp"
#include "JoSIM/AnalysisType.hpp"

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <optional>

namespace JoSIM {

/*
  Clabel V⁺ V⁻ C

  V - (2*h)/(3*C)Io = (4/3)Vn-1 - (1/3)Vn-2

  ⎡ 0  0            1⎤ ⎡ V⁺⎤   ⎡                     0⎤
  ⎜ 0  0           -1⎟ ⎜ V⁻⎟ = ⎜                     0⎟
  ⎣ 1 -1 -(2*h)/(3*C)⎦ ⎣Io ⎦   ⎣ (4/3)Vn-1 - (1/3)Vn-2⎦

  (PHASE)
  φ - (4*h*h*2*e)/(hbar*9*C)Io = (8/3)φn-1 + (10/9)φn-2 - (1/9)φn-4

  ⎡ 0  0                       1⎤ ⎡ φ⁺⎤   ⎡                                  0⎤
  ⎜ 0  0                      -1⎟ ⎜ φ⁻⎟ = ⎜                                  0⎟
  ⎣ 1 -1 -(4*h*h*2*e)/(hbar*9*C)⎦ ⎣Io ⎦   ⎣ (8/3)φn-1 + (10/9)φn-2 - (1/9)φn-4⎦
*/ 

class Capacitor : public BasicComponent {
  private:
  JoSIM::AnalysisType at_;
  public:
  double pn1_ = 0.0;
  double dpn1_ = 0.0;
  double pn2_ = 0.0;
  double pn3_ = 0.0;
  Capacitor(
    const std::pair<tokens_t, string_o> &s, const NodeConfig &ncon,
    const nodemap &nm, std::unordered_set<std::string> &lm, nodeconnections &nc,
    const param_map &pm, const AnalysisType &at, const double &h, int &bi);
  
  void update_timestep(const double &factor) override;
}; // class Capacitor

} // namespace JoSIM
#endif