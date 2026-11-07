// Copyright (c) 2025 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/Inductor.hpp"

#include "JoSIM/Constants.hpp"
#include "JoSIM/Errors.hpp"
#include "JoSIM/Misc.hpp"

#include <algorithm>
#include <initializer_list>
#include <utility>

using namespace JoSIM;

namespace {
double scaled_product(std::initializer_list<double> values) {
    double mantissa = 1.0;
    int exponent = 0;
    for (double value : values) {
        if (value == 0) { return 0.0; }
        int part = 0;
        mantissa *= std::frexp(value, &part);
        exponent += part;
    }
    return std::ldexp(mantissa, exponent);
}
} // namespace

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

Inductor::Inductor(const std::pair<tokens_t, string_o>& s,
                   const NodeConfig&                    ncon,
                   const nodemap&                       nm,
                   std::unordered_set<std::string>&     lm,
                   nodeconnections&                     nc,
                   Input&                               iObj,
                   Spread&                              spread,
                   int64_t&                             bi) {
    double spr = 1.0;
    bool alphaSpecified = false;
    for (const auto& attribute : parameter_assignments(s.first, 4)) {
        if (attribute.first == "SPREAD") {
            spr = parse_param(attribute.second, iObj.parameters, s.second);
        } else if (attribute.first == "ALPHA") {
            alphaSpecified = true;
            alpha_ = parse_param(attribute.second, iObj.parameters, s.second);
            if (!std::isfinite(alpha_) || alpha_ < 0) {
                Errors::invalid_component_errors(ComponentErrors::INVALID_COMPONENT_DECLARATION,
                    s.first.front() + ": ALPHA must be finite and non-negative");
            }
        } else {
            Errors::invalid_component_errors(ComponentErrors::INVALID_COMPONENT_DECLARATION,
                s.first.front() + ": unknown inductor parameter " + attribute.first);
        }
    }
    at_  = iObj.argAnal;
    // Set previous current value
    In2_ = 0.0;
    // Check if the label has already been defined
    if (lm.count(s.first.at(0)) != 0) {
        Errors::invalid_component_errors(ComponentErrors::DUPLICATE_LABEL, s.first.at(0));
    }
    // Set the label
    netlistInfo.label_ = s.first.at(0);
    // Add the label to the known labels list
    lm.emplace(s.first.at(0));
    // Set the value (Inductance), this should be the 4th token
    netlistInfo.value_ = spread.spread_value(parse_param(s.first.at(3), iObj.parameters, s.second), Spread::IND, spr);
    if (alphaSpecified && (!std::isfinite(netlistInfo.value_) || netlistInfo.value_ <= 0)) {
        Errors::invalid_component_errors(ComponentErrors::INVALID_COMPONENT_DECLARATION,
            s.first.front() + ": ALPHA requires finite positive L0");
    }
    // Set the node configuration type
    indexInfo.nodeConfig_   = ncon;
    // Set current index and increment it
    indexInfo.currentIndex_ = bi++;
    // Set te node indices, using token 2 and 3
    set_node_indices(tokens_t(s.first.begin() + 1, s.first.begin() + 3), nm, nc);
    // Set the non zero, column index and row pointer vectors
    set_matrix_info();
    selfIndex_ = matrixInfo.nonZeros_.size();
    // Append the value to the non zero vector
    if (at_ == AnalysisType::Voltage) {
        // If voltage mode analysis then append -(3/2) * (L/h)
        matrixInfo.nonZeros_.emplace_back(-(3.0 / 2.0) * (netlistInfo.value_ / iObj.transSim.tstep()));
    } else if (at_ == AnalysisType::Phase) {
        // If phase mdoe analysis then append -(L/σ)
        matrixInfo.nonZeros_.emplace_back(-netlistInfo.value_ / Constants::SIGMA);
    }
}

double Inductor::flux(double current) const {
    if (alpha_ == 0) { return netlistInfo.value_ * current; }
    const double coefficient = netlistInfo.value_ * alpha_;
    if (!std::isfinite(coefficient) || coefficient == 0) {
        return netlistInfo.value_ * current
             + scaled_product({netlistInfo.value_, alpha_, current, current, current, 1.0 / 3.0});
    }
    return current * (netlistInfo.value_ + ((coefficient * current) * current) / 3.0);
}

double Inductor::differential_inductance(double current) const {
    if (alpha_ == 0) { return netlistInfo.value_; }
    const double coefficient = netlistInfo.value_ * alpha_;
    if (!std::isfinite(coefficient) || coefficient == 0) {
        return netlistInfo.value_ + scaled_product({netlistInfo.value_, alpha_, current, current});
    }
    return netlistInfo.value_ + (coefficient * current) * current;
}

bool Inductor::linearize(double current, double timestep) {
    const double scale = at_ == AnalysisType::Voltage ? 1.5 / timestep : 1.0 / Constants::SIGMA;
    const double value = -scale * differential_inductance(current);
    const bool changed = matrixInfo.nonZeros_.at(selfIndex_) != value;
    matrixInfo.nonZeros_.at(selfIndex_) = value;
    return changed;
}

void Inductor::add_mutualInductance(const double& m, const AnalysisType& at, const double& h, const int64_t& ci) {
    // Adds the mutual inductance to the non zero vector
    if (at == AnalysisType::Voltage) {
        // If voltage mode then add -(3/2) * (m/h)
        matrixInfo.nonZeros_.emplace_back(-(3.0 / 2.0) * (m / h));
    } else if (at == AnalysisType::Phase) {
        // If phase mode then add -(m/σ)
        matrixInfo.nonZeros_.emplace_back(-m / Constants::SIGMA);
    }
    // Append the column index of the current branch of the second inductor
    matrixInfo.columnIndex_.emplace_back(ci);
    // Increase the row pointer vector by 1
    matrixInfo.rowPointer_.back()++;
}

// Update timestep based on a scalar factor i.e 0.5 for half the timestep
void Inductor::update_timestep(const double& factor) {
    if (at_ == AnalysisType::Voltage) {
        const auto own = std::find(matrixInfo.columnIndex_.begin(), matrixInfo.columnIndex_.end(),
                                   indexInfo.currentIndex_.value());
        // The own-current coefficient is followed by every mutual-current term.
        for (size_t i = own - matrixInfo.columnIndex_.begin(); i < matrixInfo.nonZeros_.size(); ++i) {
            matrixInfo.nonZeros_.at(i) = (1.0 / factor) * matrixInfo.nonZeros_.at(i);
        }
    }
}
