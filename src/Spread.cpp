// Copyright (c) 2025 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/Spread.hpp"

#include "JoSIM/Constants.hpp"
#include "JoSIM/Errors.hpp"
#include "JoSIM/Misc.hpp"
#include "JoSIM/Parameters.hpp"
#include "JoSIM/Rng.hpp"

using namespace JoSIM;

void Spread::get_spreads(Input& iObj) {
    gspread_ = lspread_ = rspread_ = cspread_ = bspread_ = 1.0;
    for (auto& i : iObj.controls) {
        if (i.front() == "SPREAD") {
            for (auto& j : tokens_t(i.begin() + 1, i.end())) {
                if (j.at(0) == 'R') {
                    auto t = Misc::tokenize(j, "=");
                    if (t.size() == 2) { rspread_ = parse_param(t.back(), iObj.parameters); }
                } else if (j.at(0) == 'L') {
                    auto t = Misc::tokenize(j, "=");
                    if (t.size() == 2) { lspread_ = parse_param(t.back(), iObj.parameters); }
                } else if (j.at(0) == 'C') {
                    auto t = Misc::tokenize(j, "=");
                    if (t.size() == 2) { cspread_ = parse_param(t.back(), iObj.parameters); }
                } else if (j.at(0) == 'B') {
                    auto t = Misc::tokenize(j, "=");
                    if (t.size() == 2) { bspread_ = parse_param(t.back(), iObj.parameters); }
                } else {
                    gspread_ = parse_param(j, iObj.parameters);
                }
            }
        }
    }
}

double Spread::spread_value(double value, int64_t type, double spread) const {
    if (spread == 1.0) {
        switch (type) {
            case GLB: spread = gspread_; break;
            case RES: spread = rspread_; break;
            case CAP: spread = cspread_; break;
            case IND: spread = lspread_; break;
            case JJ: spread = bspread_; break;
        }
    }
    if (spread == 1.0 && gspread_ != 1.0) { spread = gspread_; }
    if (spread == 1.0 && gspread_ == 1.0) {
        return value;
    } else {
        const double sigma = (std::abs(value) * spread) / 3.0; // (max-min)/6
        return Rng::normal_spread(value, sigma);
    }
    return value;
}
