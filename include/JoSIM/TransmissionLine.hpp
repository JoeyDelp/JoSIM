// Copyright (c) 2025 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_TRANSMISSIONLINE_HPP
#define JOSIM_TRANSMISSIONLINE_HPP

#include "JoSIM/AnalysisType.hpp"
#include "JoSIM/BasicComponent.hpp"
#include "JoSIM/ParameterName.hpp"
#include "JoSIM/Parameters.hpp"

#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace JoSIM {

/*
  Tlabel Vi+ Vi- Vo+ Vo- td=value z0=value

  V1 - Z I1 = Z I2n-k + V2n-k
  V2 - Z I2 = Z I1n-k + V1n-k

  k = td/h

  [ 0  0  0  0  1  0 ] [ V1+ ]   [              0 ]
  | 0  0  0  0 -1  0 | | V1- |   |              0 |
  | 0  0  0  0  0  1 | | V2+ | = |              0 |
  | 0  0  0  0  0 -1 | | V2- |   |              0 |
  | 1 -1  0  0 -Z  0 | |  I1 |   | ZI2n-k + V2n-k |
  [ 0  0  1 -1  0 -Z ] [  I2 ]   [ ZI1n-k + V1n-k ]

  (PHASE)
  phi1 - Z(2e/hbar)(2h/3)I1 = Z(2e/hbar)(2h/3)I2n-k + (4/3)phi1n-1 - (1/3)phi1n-2 +
                              phi2n-k - (4/3)phi2n-k-1 + (1/3)phi2n-k-2
  phi2 - Z(2e/hbar)(2h/3)I2 = Z(2e/hbar)(2h/3)I1n-k + (4/3)phi2n-1 - (1/3)phi2n-2 +
                              phi1n-k - (4/3)phi1n-k-1 + (1/3)phi1n-k-2

  [ 0  0  0  0                -1                  0 ] [ phio+ ]   [    0 ]
  | 0  0  0  0                 1                  0 | | phio- |   |    0 |
  | 0  0  0  0                 0                  1 | | phic+ | = |    0 |
  | 0  0  0  0                 0                 -1 | | phic- |   |    0 |
  | 1 -1  0  0 -Z(2e/hbar)(2h/3)                  0 | |  Ic   |   |  RHS1|
  [ 0  0  1 -1                 0  -Z(2e/hbar)(2h/3) ] [  Io   ]   [  RHS2]

  RHS1 = Z(2e/hbar)(2h/3)I2n-k + (4/3)phi1n-1 - (1/3)phi1n-2 +
         phi2n-k - (4/3)phi2n-k-1 + (1/3)phi2n-k-2
  RHS2 = Z(2e/hbar)(2h/3)I1n-k + (4/3)phi2n-1 - (1/3)phi2n-2 +
         phi1n-k - (4/3)phi1n-k-1 + (1/3)phi1n-k-2
*/

class TransmissionLine : public BasicComponent {
  private:
    int64_t             hDepPos_ = 0;
    JoSIM::AnalysisType at_;

  public:
    NodeConfig nodeConfig2_;
    int_o      posIndex2_, negIndex2_;
    int64_t    currentIndex2_ = 0;
    double     n1_1_ = 0.0, n1_2_ = 0.0, n2_1_ = 0.0, n2_2_ = 0.0;
    double     nk_1_ = 0.0, nk_2_ = 0.0, nk1_1_ = 0.0, nk1_2_ = 0.0, nk2_1_ = 0.0, nk2_2_ = 0.0;
    int64_t    timestepDelay_ = 0;

    TransmissionLine(const std::pair<tokens_t, string_o>& s,
                     const NodeConfig&                    ncon,
                     const std::optional<NodeConfig>&     ncon2,
                     const nodemap&                       nm,
                     std::unordered_set<std::string>&     lm,
                     nodeconnections&                     nc,
                     const param_map&                     pm,
                     const AnalysisType&                  at,
                     const double&                        h,
                     int64_t&                             bi);

    void set_secondary_node_indices(const tokens_t& t, const nodemap& nm, nodeconnections& nc);
    void set_secondary_matrix_info();

    void update_timestep(const double& factor) override;
}; // class TransmissionLine

} // namespace JoSIM

#endif // JOSIM_TRANSMISSIONLINE_HPP
