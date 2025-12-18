// Copyright (c) 2025 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_RESISTOR_HPP
#define JOSIM_RESISTOR_HPP

#include "JoSIM/AnalysisType.hpp"
#include "JoSIM/BasicComponent.hpp"
#include "JoSIM/Function.hpp"
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
  Rlabel V+ V- R

  V = R Io

  [ 0  0  1 ] [ V+ ]   [ 0 ]
  | 0  0 -1 | | V- | = | 0 |
  [ 1 -1 -R ] [ Io ]   [ 0 ]

  (PHASE)
  phi - R(2e/hbar)(2h/3)Io = (4/3)phin-1 - (1/3)phin-2

  [ 0  0                 1 ] [ phi+ ]   [                       0 ]
  | 0  0                -1 | | phi- | = |                       0 |
  [ 1 -1 -R(2e/hbar)(2h/3) ] [  Io  ]   [ (4/3)phin-1-(1/3)phin-2 ]
*/

class Resistor : public BasicComponent {
  private:
    JoSIM::AnalysisType   at_;
    std::optional<double> spAmp_, temp_, neb_;

  public:
    double                  pn1_ = 0.0, pn2_ = 0.0, pn3_ = 0.0, pn4_ = 0.0;
    std::optional<Function> thermalNoise;

    Resistor(const std::pair<tokens_t, string_o>& s,
             const NodeConfig&                    ncon,
             const nodemap&                       nm,
             std::unordered_set<std::string>&     lm,
             nodeconnections&                     nc,
             Input&                               iObj,
             Spread&                              spread,
             int64_t&                             bi);

    void update_timestep(const double& factor) override;

    void step_back() override { pn2_ = pn4_; }
}; // class Resistor

} // namespace JoSIM
#endif // JOSIM_RESISTOR_HPP
