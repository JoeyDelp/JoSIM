// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_SPREAD_HPP
#define JOSIM_SPREAD_HPP

#include "JoSIM/Input.hpp"

namespace JoSIM {

class Spread {
 private:
  double gspread_ = 1.0, lspread_ = 1.0, rspread_ = 1.0, cspread_ = 1.0,
         bspread_ = 1.0;

 public:
  Spread() {}
  void get_spreads(Input& iObj);
  double spread_value(double value, int64_t type = GLB, double spread = 1.0);
  enum { GLB, RES, IND, CAP, JJ };
};

}  // namespace JoSIM

#endif  // JOSIM_SPREAD_HPP