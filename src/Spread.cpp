// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/Spread.hpp"

#include <random>

#include "JoSIM/Constants.hpp"
#include "JoSIM/Errors.hpp"
#include "JoSIM/Misc.hpp"
#include "JoSIM/Parameters.hpp"

using namespace JoSIM;

void Spread::get_spreads(Input& iObj) {
  for (auto& i : iObj.controls) {
    if (i.front() == "SPREAD") {
      for (auto& j : tokens_t(i.begin() + 1, i.end())) {
        if (j.at(0) == 'R') {
          auto t = Misc::tokenize(j, "=");
          if (t.size() == 2) {
            rspread_ = parse_param(t.back(), iObj.parameters);
          }
        } else if (j.at(0) == 'L') {
          auto t = Misc::tokenize(j, "=");
          if (t.size() == 2) {
            lspread_ = parse_param(t.back(), iObj.parameters);
          }
        } else if (j.at(0) == 'C') {
          auto t = Misc::tokenize(j, "=");
          if (t.size() == 2) {
            cspread_ = parse_param(t.back(), iObj.parameters);
          }
        } else if (j.at(0) == 'B') {
          auto t = Misc::tokenize(j, "=");
          if (t.size() == 2) {
            bspread_ = parse_param(t.back(), iObj.parameters);
          }
        } else {
          gspread_ = parse_param(j, iObj.parameters);
        }
      }
    }
  }
}

double Spread::spread_value(double value, int64_t type, double spread) {
  if (spread == 1.0) {
    switch (type) {
      case GLB:
        spread = gspread_;
        break;
      case RES:
        spread = rspread_;
        break;
      case CAP:
        spread = cspread_;
        break;
      case IND:
        spread = lspread_;
        break;
      case JJ:
        spread = bspread_;
        break;
    }
  }
  if (spread == 1.0 && gspread_ != 1.0) {
    spread = gspread_;
  }
  if (spread == 1.0 && gspread_ == 1.0) {
    return value;
  } else {
    std::random_device rd{};
    std::mt19937 gen{rd()};
    auto min = value * (1 - spread);
    auto max = value * (1 + spread);
    std::normal_distribution<> d(value, (max - min) / 6);
    return d(gen);
  }
  return value;
}