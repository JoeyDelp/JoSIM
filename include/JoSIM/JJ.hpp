// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_JJ_HPP
#define JOSIM_JJ_HPP

#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "JoSIM/AnalysisType.hpp"
#include "JoSIM/BasicComponent.hpp"
#include "JoSIM/Function.hpp"
#include "JoSIM/Input.hpp"
#include "JoSIM/Model.hpp"
#include "JoSIM/ParameterName.hpp"
#include "JoSIM/Parameters.hpp"
#include "JoSIM/Spread.hpp"

namespace JoSIM {

/*
  Blabel V⁺ V⁻ jjmodel area=value

  V - (hbar/2e)(3/2h)φ = (hbar/2e)(-(2/h)φn-1 + (1/2h)φn-2)
  V - (2Rh/(3RC + 2h))Io =
    -(2Rh/(3RC + 2h))(Icsinφ0 - (2C/h)Vn-1 + (C/2h)Vn-2)
  φ0 = (4/3)φn-1 - (1/3)φn-2 + (2e/hbar)(2h/3)v0
  v0 = (5/2)Vn-1 - 2Vn-2 + (1/2)Vn-3

  ⎡ 0  0                0                 1⎤ ⎡ V⁺⎤   ⎡    0⎤
  ⎜ 0  0                0                -1⎟ ⎜ V⁻⎟ = ⎜    0⎟
  ⎜ 1 -1 -(hbar/2e)(3/2h)                 0⎟ ⎜ φ ⎟   ⎜ RHS1⎟
  ⎣ 1 -1                0 -(2Rh/(3RC + 2h))⎦ ⎣ Io⎦   ⎣ RHS2⎦

  RHS1 = (hbar/2e)(-(2/h)φn-1 + (1/2h)φn-2)
  RHS2 = -(2Rh/(3RC + 2h))(Icsinφ0 - (2C/h)Vn-1 + (C/2h)Vn-2)

  (PHASE)
  φ - (2e/hbar)(2h/3)V = (4/3)φn-1 - (1/3)φn-2

  ⎡ 0  0                0                 1⎤ ⎡ φ⁺⎤   ⎡    0⎤
  ⎜ 0  0                0                -1⎟ ⎜ φ⁻⎟ = ⎜    0⎟
  ⎜ 1 -1 -(2e/hbar)(2h/3)                 0⎟ ⎜ V ⎟   ⎜ RHS1⎟
  ⎣ 1 -1                0 -(2Rh/(3RC + 2h))⎦ ⎣ Io⎦   ⎣ RHS2⎦

  RHS1 = (4/3)φn-1 - (1/3)φn-2
*/

class JJ : public BasicComponent {
 private:
  int64_t hDepPos_ = 0;
  int64_t state_ = 0;
  double h_ = 0.0;

 public:
  int64_t variableIndex_ = 0;
  double area_ = 1.0;
  std::optional<double> Ic_, temp_, neb_, spAmp_;
  Model model_;
  double phaseConst_ = 0.0;
  double lowerB_ = 0.0, upperB_ = 0.0, gLarge_ = 0.0;
  double del0_ = 0.0, del_ = 0.0;
  double pn1_ = 0.0, pn2_ = pn1_, pn3_ = pn2_, pn4_ = pn3_, phi0_ = 0.0;
  double vn1_ = 0.0, vn2_ = vn1_, vn3_ = vn2_, vn4_ = vn3_, vn5_ = vn4_,
         vn6_ = vn5_;
  double it_ = 0.0;
  JoSIM::AnalysisType at_;
  std::optional<Function> thermalNoise;

  JJ(const std::pair<tokens_t, string_o>& s, const NodeConfig& ncon,
     const nodemap& nm, std::unordered_set<std::string>& lm,
     nodeconnections& nc, Input& iObj, Spread& spread, int64_t& bi);

  double subgap_impedance();
  double transient_impedance();
  double normal_impedance();

  void set_matrix_info();

  void set_model(const tokens_t& t,
                 const vector_pair_t<Model, string_o>& models,
                 const string_o& subc);

  bool update_value(const double& v);

  void step_back() override {
    pn2_ = pn4_;
    vn3_ = vn6_;
    vn2_ = vn5_;
  }
};  // class JJ

}  // namespace JoSIM
#endif