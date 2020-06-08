// Copyright (c) 2020 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_JJ_HPP
#define JOSIM_JJ_HPP

#include "JoSIM/Components.hpp"
#include "JoSIM/ParameterName.hpp"
#include "JoSIM/Parameters.hpp"
#include "JoSIM/Model.hpp"
#include "JoSIM/AnalysisType.hpp"
#include "JoSIM/IntegrationType.hpp"

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <optional>

namespace JoSIM {
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
  double dvn1_, dvn2_;
  double transitionCurrent_;

  JJ(
    const std::pair<tokens_t, string_o> &s, const NodeConfig &ncon,
    const nodemap &nm, std::unordered_set<std::string> &lm, nodeconnections &nc,
    const param_map &pm, const vector_pair_t<Model, string_o> &models,
    const AnalysisType &at, const double &h, int &bi);

  void set_model(
    const tokens_t &t, const vector_pair_t<Model, string_o> &models, 
    const string_o &subc);

  bool update_value(const double &v);
}; // class JJ

} // namespace JoSIM
#endif