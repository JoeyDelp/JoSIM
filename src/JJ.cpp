// Copyright (c) 2025 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/JJ.hpp"

#include "JoSIM/Constants.hpp"
#include "JoSIM/Errors.hpp"
#include "JoSIM/Misc.hpp"
#include "JoSIM/Noise.hpp"

#include <cmath>

using namespace JoSIM;

namespace {

// Collects a function-valued parameter that may be split across tokens,
// e.g. ICNOISE=PNOISE(1, 1, 0, 1E-4, 2, 64)
// might arrive as multiple whitespace-split tokens.
//
// On success:
//   - outExpr gets the RHS expression (e.g. "PNOISE(1, 1, ...)")
//   - idx is advanced to the last token consumed
static bool collect_kv_function_expr(const tokens_t&    tokens,
                                     size_t&            idx,
                                     const std::string& keyEq, // e.g. "ICNOISE="
                                     std::string&       outExpr) {
    if (idx >= tokens.size()) { return false; }
    const std::string& t0 = tokens[idx];
    if (t0.rfind(keyEq, 0) != 0) { return false; }

    outExpr = t0.substr(keyEq.size());

    // If the RHS begins in the *next* token (rare but possible): "ICNOISE=" "PNOISE(...)"
    if (outExpr.empty() && idx + 1 < tokens.size()) {
        ++idx;
        outExpr = tokens[idx];
    }

    // Count parentheses to know when we’re complete.
    auto update_depth = [](const std::string& s, int& depth) {
        for (char c : s) {
            if (c == '(') {
                ++depth;
            } else if (c == ')') {
                --depth;
            }
        }
    };

    int depth = 0;
    update_depth(outExpr, depth);

    // If we haven't even seen an opening paren yet, keep appending until we do.
    // (Allows cases like ICNOISE=PNOISE to still work if someone writes weird spacing.)
    while (depth <= 0 && outExpr.find('(') == std::string::npos && idx + 1 < tokens.size()) {
        ++idx;
        outExpr += " " + tokens[idx];
        update_depth(tokens[idx], depth);
    }

    // Now, if we’re inside parens, keep consuming tokens until balanced.
    while (depth > 0 && idx + 1 < tokens.size()) {
        ++idx;
        outExpr += " " + tokens[idx];
        update_depth(tokens[idx], depth);
    }

    // Balanced means depth == 0 (and we’ve seen at least one ')')
    if (depth != 0) { return false; }

    return true;
}

} // namespace


/*
  Blabel V+ V- jjmodel area=value

  V - (hbar/2e)(3/2h)phi = (hbar/2e)(-(2/h)phin-1 + (1/2h)phin-2)
  V - (2Rh/(3RC + 2h))Io =
    -(2Rh/(3RC + 2h))*( Ic*sin(phi0) - (2C/h)Vn-1 + (C/2h)Vn-2 )
  phi0 = (4/3)phin-1 - (1/3)phin-2 + (2e/hbar)(2h/3)v0
  v0   = (5/2)Vn-1 - 2Vn-2 + (1/2)Vn-3

  [ 0  0                0                 1 ] [ V+ ]   [    0 ]
  | 0  0                0                -1 | | V- | = |    0 |
  | 1 -1 -(hbar/2e)(3/2h)                 0 | |phi |   | RHS1 |
  [ 1 -1                0 -(2Rh/(3RC + 2h)) ] [ Io ]   [ RHS2 ]

  RHS1 = (hbar/2e)(-(2/h)phin-1 + (1/2h)phin-2)
  RHS2 = -(2Rh/(3RC + 2h))*( Ic*sin(phi0) - (2C/h)Vn-1 + (C/2h)Vn-2 )

  (PHASE)
  phi - (2e/hbar)(2h/3)V = (4/3)phin-1 - (1/3)phin-2

  [ 0  0                0                 1 ] [ phi+ ]   [    0 ]
  | 0  0                0                -1 | | phi- | = |    0 |
  | 1 -1 -(2e/hbar)(2h/3)                 0 | |  V   |   | RHS1 |
  [ 1 -1                0 -(2Rh/(3RC + 2h)) ] [  Io  ]   [ RHS2 ]

  RHS1 = (4/3)phin-1 - (1/3)phin-2
*/

JJ::JJ(const std::pair<tokens_t, string_o>& s,
       const NodeConfig&                    ncon,
       const nodemap&                       nm,
       std::unordered_set<std::string>&     lm,
       nodeconnections&                     nc,
       Input&                               iObj,
       Spread&                              spread,
       int64_t&                             bi) {
    double   spr = 1.0;
    tokens_t t;
    bool gateGiven = false;
    auto set_gate = [&](const std::string& node) {
        if (gateGiven) {
            Errors::invalid_component_errors(ComponentErrors::INVALID_COMPONENT_DECLARATION,
                                             s.first.front() + ": duplicate gate control");
        }
        gateGiven = true;
        if (node == "0" || node == "GND") { return; }
        auto found = nm.find(node);
        if (found == nm.end()) {
            Errors::invalid_component_errors(ComponentErrors::INVALID_COMPONENT_DECLARATION,
                                             s.first.front() + ": unknown gate node " + node);
        }
        gateIndex_ = found->second;
    };
    size_t firstAttribute = 3;
    // The experimental branch also accepted B+ B- gate model ordering.
    if (s.first.size() > 4 && s.first.at(3).find('=') == std::string::npos) {
        const auto* model = Model::find_model(s.first.at(4), iObj.netlist.models_new, s.second);
        if (model && model->gateWidthSpecified()) {
            // WRspice decks may contain an unused phase-node field here.
            if (nm.count(s.first.at(3)) != 0) { set_gate(s.first.at(3)); }
            firstAttribute = 4;
        }
    }
    for (size_t i = firstAttribute; i < s.first.size(); ++i) {
        auto& ti = s.first.at(i);
        if (ti.rfind("SPREAD=", 0) == 0) {
            spr = parse_param(ti.substr(7), iObj.parameters, s.second);
        } else if (ti.rfind("TEMP=", 0) == 0) {
            temp_ = parse_param(ti.substr(5), iObj.parameters, s.second);
        } else if (ti.rfind("NEB=", 0) == 0) {
            neb_ = parse_param(ti.substr(4), iObj.parameters, s.second);
        } else if (ti.rfind("AREA=", 0) == 0) {
            area_ = spread.spread_value(parse_param(ti.substr(5), iObj.parameters, s.second), Spread::JJ, spr);
        } else if (ti.rfind("GC=", 0) == 0) {
            set_gate(ti.substr(3));
        } else if (ti.rfind("IC=", 0) == 0) {
            Ic_ = spread.spread_value(parse_param(ti.substr(3), iObj.parameters, s.second), Spread::JJ, spr);
        } else if (ti.rfind("ICNOISE=", 0) == 0) {
            std::string expr;
            size_t      j = i;
            if (!collect_kv_function_expr(s.first, j, "ICNOISE=", expr)) {
                Errors::invalid_component_errors(ComponentErrors::INVALID_COMPONENT_DECLARATION);
            } else {
                Function f;
                f.parse_function(expr, iObj, s.second);
                icNoise = f; // std::optional<Function> icNoise; (in JJ.hpp)
                i       = j; // skip consumed tokens
            }

        } else {
            t.emplace_back(ti);
        }
    }
    if (!temp_ && iObj.globalTemp) { temp_ = iObj.globalTemp; }
    if (!neb_ && iObj.neB) { neb_ = iObj.neB; }
    // Set component timestep
    h_     = iObj.transSim.tstep();
    at_    = iObj.argAnal;
    // Set state to 0 (subgap)
    state_ = 0;
    // Check if the label has already been defined
    if (lm.count(s.first.at(0)) != 0) {
        Errors::invalid_component_errors(ComponentErrors::DUPLICATE_LABEL, s.first.at(0));
    }
    // Set the label
    netlistInfo.label_ = s.first.at(0);
    // Add the label to the known labels list
    lm.emplace(s.first.at(0));
    // Set the model for this JJ instance
    set_model(t, iObj.netlist.models_new, s.second);
    // Set the phase constant
    if (at_ == AnalysisType::Voltage) {
        // If voltage mode set this to (3 * hbar) / (4 * h * eV)
        phaseConst_ = (3 * Constants::HBAR) / (4 * h_ * Constants::EV);
    } else if (at_ == AnalysisType::Phase) {
        // If phase mode set this to (4 * h * eV) / (3 * hbar)
        phaseConst_ = (4 * h_ * Constants::EV) / (3 * Constants::HBAR);
    }
    // Set the node configuration type
    indexInfo.nodeConfig_   = ncon;
    // Set variable index and increment it
    variableIndex_          = bi++;
    // Set current index and increment it
    indexInfo.currentIndex_ = bi++;
    // Set te node indices, using token 2 and 3
    set_node_indices(tokens_t(s.first.begin() + 1, s.first.begin() + 3), nm, nc);
    // Set the non zero, column index and row pointer vectors
    set_matrix_info();
    if (temp_) {
        spAmp_ = Noise::determine_spectral_amplitude(this->model_.r0(), temp_.value());
        Function tnoise;
        tnoise.parse_function("NOISE(" + Misc::precise_to_string(spAmp_.value()) + ", 0.0, "
                                      + Misc::precise_to_string(1.0 / (2 * neb_.value())) + ")",
                              iObj,
                              s.second);
        thermalNoise = tnoise;
    }
}

double JJ::subgap_impedance() {
    // Set subgap impedance (1/R0) + (3C/2h)
    return ((1 / model_.r0()) + ((3.0 * model_.c()) / (2.0 * h_)));
}

double JJ::transient_impedance() {
    // Set transitional impedance (GL) + (3C/2h)
    return (gLarge_ + ((3.0 * model_.c()) / (2.0 * h_)));
}

double JJ::normal_impedance() {
    // Set normal impedance (1/RN) + (3C/2h)
    return ((1 / model_.rn()) + ((3.0 * model_.c()) / (2.0 * h_)));
}

void JJ::set_matrix_info() {
    switch (indexInfo.nodeConfig_) {
        case NodeConfig::POSGND:
            matrixInfo.nonZeros_.emplace_back(1);
            matrixInfo.columnIndex_.emplace_back(indexInfo.posIndex_.value());
            matrixInfo.rowPointer_.emplace_back(2);
            break;
        case NodeConfig::GNDNEG:
            matrixInfo.nonZeros_.emplace_back(-1);
            matrixInfo.columnIndex_.emplace_back(indexInfo.negIndex_.value());
            matrixInfo.rowPointer_.emplace_back(2);
            break;
        case NodeConfig::POSNEG:
            if (indexInfo.posIndex_.value() != indexInfo.negIndex_.value()) {
                matrixInfo.nonZeros_.emplace_back(1);
                matrixInfo.nonZeros_.emplace_back(-1);
                matrixInfo.columnIndex_.emplace_back(indexInfo.posIndex_.value());
                matrixInfo.columnIndex_.emplace_back(indexInfo.negIndex_.value());
                matrixInfo.rowPointer_.emplace_back(3);
            } else {
                matrixInfo.rowPointer_.emplace_back(1);
            }
            break;
        case NodeConfig::GND: matrixInfo.rowPointer_.emplace_back(1); break;
    }
    matrixInfo.nonZeros_.emplace_back(-phaseConst_);
    hDepPos_ = matrixInfo.nonZeros_.size() - 1;
    matrixInfo.columnIndex_.emplace_back(variableIndex_);
    if (at_ == AnalysisType::Voltage) {
        matrixInfo.nonZeros_.insert(
                matrixInfo.nonZeros_.end(), matrixInfo.nonZeros_.begin(), matrixInfo.nonZeros_.end());
        matrixInfo.nonZeros_.back() = -1 / subgap_impedance();
        matrixInfo.columnIndex_.insert(
                matrixInfo.columnIndex_.end(), matrixInfo.columnIndex_.begin(), matrixInfo.columnIndex_.end());
        matrixInfo.columnIndex_.back() = indexInfo.currentIndex_.value();
        matrixInfo.rowPointer_.emplace_back(matrixInfo.rowPointer_.back());
    } else if (at_ == AnalysisType::Phase) {
        matrixInfo.nonZeros_.emplace_back(1);
        matrixInfo.columnIndex_.emplace_back(variableIndex_);
        matrixInfo.nonZeros_.emplace_back(-1 / subgap_impedance());
        matrixInfo.columnIndex_.emplace_back(indexInfo.currentIndex_.value());
        matrixInfo.rowPointer_.emplace_back(2);
    }
}

void JJ::set_model(const tokens_t& t, const vector_pair_t<Model, string_o>& models, const string_o& subc) {
    const auto* model = t.empty() ? nullptr : Model::find_model(t.back(), models, subc);
    if (!model) {
        Errors::invalid_component_errors(ComponentErrors::MODEL_NOT_DEFINED, Misc::vector_to_string(t));
    }
    model_ = *model;
    // Change the area if ic was defined
    if (Ic_) { area_ = Ic_.value() / model_.ic(); }
    // Set the model critical current for this JJ instance
    model_.ic(model_.ic() * area_);
    if (model_.tDep()) {
        // Set the Del0 parameter
        del0_ = 1.76 * Constants::BOLTZMANN * model_.tc();
        // Set the del parameter
        del_  = del0_ * sqrt(cos((Constants::PI / 2) * (model_.t() / model_.tc()) * (model_.t() / model_.tc())));
        // Set the temperature dependent normal resistance
        model_.rn(((Constants::PI * del_) / (2 * Constants::EV * model_.ic()))
                  * tanh(del_ / (2 * Constants::BOLTZMANN * model_.t())));
    } else {
        // Set the model normal resistance for this JJ instance
        model_.rn(model_.rn() / area_);
    }
    // Set the model capacitance for this JJ instance
    model_.c(model_.c() * area_);
    // Set the model subgap resistance for this JJ instance
    model_.r0(model_.r0() / area_);
    // Set the lower boundary for the transition region
    lowerB_ = model_.vg() - 0.5 * model_.deltaV();
    // Set the upper boundary for the transition region
    upperB_ = model_.vg() + 0.5 * model_.deltaV();
    // Set the transitional conductance value
    gLarge_ = model_.ic() / (model_.icFct() * model_.deltaV());
    if (model_.rtype() == 0) { model_.r0(model_.rn()); }
}

// Update the value based on the matrix entry based on voltage value
bool JJ::update_value(const double& v) {
    // Shorthand for the model
    const Model& m = model_;
    // If the absolute value of the voltage is less than lower bounds
    if (fabs(v) < lowerB_) {
        // Set temperature resistance
        if (this->temp_) {
            thermalNoise.value().ampValuesRef().at(0)
                    = Noise::determine_spectral_amplitude(this->model_.r0(), temp_.value());
        }
        // Set the transition current to 0
        it_ = 0.0;
        // If the non zero vector does not end with the subgap conductance
        if (matrixInfo.nonZeros_.back() != -1 / subgap_impedance()) {
            // Make it end with the subgap conductance
            matrixInfo.nonZeros_.back() = -1 / subgap_impedance();
            // Set state to 0 (Subgap)
            state_                      = 0;
            // Return that a value has been updated
            return true;
        } else {
            // Return that nothing has changed
            return false;
        }
        // If the absolute value of the voltage is less than the upperbounds
    } else if (fabs(v) < upperB_) {
        // Set the transition current
        it_ = lowerB_ * ((1 / m.r0()) - gLarge_);
        // If the voltage is negative, current must be negative
        if (v < 0) { it_ = -it_; }
        // If the back of the non zero vector is not the transition conductance
        if (matrixInfo.nonZeros_.back() != -1 / transient_impedance()) {
            // Set it to the transition conductance
            matrixInfo.nonZeros_.back() = -1 / transient_impedance();
            // Set state to 1 (Transition)
            state_                      = 1;
            // Return that a value has changed
            return true;
        } else {
            // Return that nothing has changed
            return false;
        }
        // If neither of the above then it must be in normal operating region
    } else {
        // Set temperature resistance
        if (this->temp_) {
            thermalNoise.value().ampValuesRef().at(0)
                    = Noise::determine_spectral_amplitude(this->model_.rn(), temp_.value());
        }
        // Reset the transition current, transition has passed.
        it_ = 0.0;
        // If the back of the non zero vector is not the normal conductance
        if (matrixInfo.nonZeros_.back() != -1 / normal_impedance()) {
            // Set it to the normal conductance
            matrixInfo.nonZeros_.back() = -1 / normal_impedance();
            // Set state to 2 (Normal)
            state_                      = 2;
            // Return that a value has changed
            return true;
        } else {
            // Return that nothing has changed
            return false;
        }
    }
    // This should never happen, only here to appease the return type
    return false;
}
