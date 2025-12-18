// Copyright (c) 2025 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_CAPACITOR_HPP
#define JOSIM_CAPACITOR_HPP

#include "JoSIM/AnalysisType.hpp"
#include "JoSIM/BasicComponent.hpp"
#include "JoSIM/Input.hpp"
#include "JoSIM/ParameterName.hpp"
#include "JoSIM/Parameters.hpp"
#include "JoSIM/Spread.hpp"

#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace JoSIM {

/*
  Clabel V+ V- C

  V - (2*h)/(3*C)Io = (4/3)Vn-1 - (1/3)Vn-2

  [ 0  0            1 ] [ V+ ]   [                     0 ]
  | 0  0           -1 | | V- | = |                     0 |
  [ 1 -1 -(2*h)/(3*C) ] [ Io ]   [ (4/3)Vn-1 - (1/3)Vn-2 ]

  (PHASE)
  phi - (4*h*h*2*e)/(hbar*9*C)Io =
    (8/3)phin-1 - (22/9)phin-2 + (8/9)phin-3 - (1/9)phin-4

  [ 0  0                       1 ] [ phi+ ]   [   0 ]
  | 0  0                      -1 | | phi- | = |   0 |
  [ 1 -1 -(4*h*h*2*e)/(hbar*9*C) ] [  Io  ]   [ RHS ]

  RHS = (8/3)phin-1 - (22/9)phin-2 + (8/9)phin-3 - (1/9)phin-4
*/

class Capacitor : public BasicComponent {
  private:
    JoSIM::AnalysisType at_;

  public:
    double pn1_ = 0.0, pn2_ = 0.0, pn3_ = 0.0, pn4_ = 0.0, pn5_ = 0.0, pn6_ = 0.0, pn7_ = 0.0;
    Capacitor(const std::pair<tokens_t, string_o>& s,
              const NodeConfig&                    ncon,
              const nodemap&                       nm,
              std::unordered_set<std::string>&     lm,
              nodeconnections&                     nc,
              Input&                               iObj,
              Spread&                              spread,
              int64_t&                             bi);

    void update_timestep(const double& factor) override;

    void step_back() override {
        pn4_ = pn7_;
        pn3_ = pn6_;
        pn2_ = pn5_;
    }
}; // class Capacitor

} // namespace JoSIM
#endif // JOSIM_CAPACITOR_HPP
