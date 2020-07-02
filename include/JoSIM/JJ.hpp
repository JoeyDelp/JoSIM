// Copyright (c) 2020 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_JJ_HPP
#define JOSIM_JJ_HPP

#include "JoSIM/BasicComponent.hpp"
#include "JoSIM/ParameterName.hpp"
#include "JoSIM/Parameters.hpp"
#include "JoSIM/Model.hpp"
#include "JoSIM/AnalysisType.hpp"

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <optional>

namespace JoSIM {

/*
  Blabel V⁺ V⁻ jjmodel area=value

  V - (hbar/2e)(3/2h)φ = (hbar/2e)(-(2/h)φn-1 + (1/2h)φn-2)
  V - (2Rh/(3RC + 2h))Io = -(2Rh/(3RC + 2h))(Icsinφ0 - (2C/h)Vn-1 + (C/2h)Vn-2)
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
  public:
  int variableIndex_;
  double area_;
  std::optional<Model> model_;
  double phaseConst_;
  double lowerB_, upperB_, subImp_, transImp_, normImp_, gLarge_;
  double del0_, del_, rncalc_;
  double pn1_, pn2_, phi0_;
  double vn1_, vn2_, vn3_;
  double transitionCurrent_;

  JJ(
    const std::pair<tokens_t, string_o> &s, const NodeConfig &ncon,
    const nodemap &nm, std::unordered_set<std::string> &lm, nodeconnections &nc,
    const param_map &pm, const vector_pair_t<Model, string_o> &models,
    const AnalysisType &at, const double &h, int &bi);

  void set_matrix_info(const AnalysisType &at);

  void set_model(
    const tokens_t &t, const vector_pair_t<Model, string_o> &models, 
    const string_o &subc);

  bool update_value(const double &v);
}; // class JJ

} // namespace JoSIM
#endif