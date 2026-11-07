// Copyright (c) 2025 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_INDUCTOR_HPP
#define JOSIM_INDUCTOR_HPP

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
  Llabel V+ V- L

  V - (3*L)/(2*h)Io = -(2*L)/(h)In-1 + (L)/(2*h)In-2

  [ 0  0            1 ] [ V+ ]   [                              0 ]
  | 0  0           -1 | | V- | = |                              0 |
  [ 1 -1 -(3*L)/(2*h) ] [ Io ]   [ -(2*L)/(h)In-1 + (L)/(2*h)In-2 ]

  (PHASE)
  phi - L(2e/hbar)Io = 0

  [ 0  0           1 ] [ phi+ ]   [ 0 ]
  | 0  0          -1 | | phi- | = | 0 |
  [ 1 -1 -L(2e/hbar) ] [  Io  ]   [ 0 ]
*/

using mutualinductors = std::vector<std::pair<int64_t, double>>;

class Inductor : public BasicComponent {
  private:
    JoSIM::AnalysisType at_;
    size_t              selfIndex_ = 0;

  public:
    double          In2_ = 0.0, In3_ = 0.0, In4_ = 0.0;
    double          alpha_ = 0.0;
    // Total linkage at n-2 before assembly, including mutual flux, in Wb.
    double          previousFlux_ = 0.0;
    // This step's BDF2 history contribution, in Wb.
    double          fluxHistory_ = 0.0;
    mutualinductors mutualInductances_;

    Inductor(const std::pair<tokens_t, string_o>& s,
             const NodeConfig&                    ncon,
             const nodemap&                       nm,
             std::unordered_set<std::string>&     lm,
             nodeconnections&                     nc,
             Input&                               iObj,
             Spread&                              spread,
             int64_t&                             bi);

    void set_mutualInductance(const std::pair<int64_t, double>& mut) { mutualInductances_.emplace_back(mut); }

    void add_mutualInductance(const double& m, const AnalysisType& at, const double& h, const int64_t& ci);

    const mutualinductors get_mutualInductance() const { return mutualInductances_; }

    double flux(double current) const;
    double differential_inductance(double current) const;
    bool linearize(double current, double timestep);

    void                  update_timestep(const double& factor) override;

    void                  step_back() override { In2_ = In4_; }
}; // class Inductor

} // namespace JoSIM
#endif // JOSIM_INDUCTOR_HPP
