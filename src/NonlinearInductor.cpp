// Copyright (c) 2026 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/Simulation.hpp"
#include "JoSIM/Constants.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <unordered_map>
#include <unordered_set>

using namespace JoSIM;

void Matrix::validate_inductor_coupling() const {
    const auto& devices = components.devices;
    const auto& indices = components.inductorIndices;
    if (std::none_of(indices.begin(), indices.end(), [&](int64_t index) {
        return std::get<Inductor>(devices[index]).alpha_ > 0;
    })) { return; }

    std::unordered_set<int64_t> visited;
    for (auto start : indices) {
        if (visited.count(start)) { continue; }
        std::vector<int64_t> group, pending{start};
        bool nonlinear = false;
        while (!pending.empty()) {
            const auto index = pending.back();
            pending.pop_back();
            if (!visited.insert(index).second) { continue; }
            group.push_back(index);
            const auto& device = std::get<Inductor>(devices[index]);
            nonlinear = nonlinear || device.alpha_ > 0;
            for (const auto& mutual : device.mutualInductances_) { pending.push_back(mutual.first); }
        }
        if (!nonlinear || (group.size() == 1 && std::get<Inductor>(devices[start]).mutualInductances_.empty())) {
            continue;
        }
        const auto fail = [&] {
            Errors::invalid_component_errors(ComponentErrors::INVALID_COMPONENT_DECLARATION,
                std::get<Inductor>(devices[start]).netlistInfo.label_
                + ": nonlinear coupling requires a finite positive-definite base inductance matrix without self-coupling");
        };
        std::sort(group.begin(), group.end());
        std::unordered_map<int64_t, size_t> position;
        for (size_t i = 0; i < group.size(); ++i) { position[group[i]] = i; }
        // Sparse LDL elimination on D^-1/2 L D^-1/2. Positive base pivots and
        // ALPHA>=0 keep the differential inductance matrix positive definite.
        std::vector<std::map<size_t, double>> upper(group.size());
        for (size_t i = 0; i < group.size(); ++i) {
            const auto& device = std::get<Inductor>(devices[group[i]]);
            if (!std::isfinite(device.netlistInfo.value_) || device.netlistInfo.value_ <= 0) { fail(); }
            upper[i][i] = 1.0;
            for (const auto& mutual : device.mutualInductances_) {
                const auto j = position.at(mutual.first);
                const double other = std::get<Inductor>(devices[mutual.first]).netlistInfo.value_;
                if (i == j || !std::isfinite(other) || other <= 0 || !std::isfinite(mutual.second)) { fail(); }
                if (j > i) {
                    upper[i][j] += mutual.second / (std::sqrt(device.netlistInfo.value_) * std::sqrt(other));
                }
            }
        }
        for (size_t k = 0; k < upper.size(); ++k) {
            const double pivot = upper[k].at(k);
            if (!std::isfinite(pivot) || pivot <= 64 * std::numeric_limits<double>::epsilon() * group.size()) { fail(); }
            for (auto i = upper[k].upper_bound(k); i != upper[k].end(); ++i) {
                for (auto j = i; j != upper[k].end(); ++j) {
                    upper[i->first][j->first] -= (i->second / pivot) * j->second;
                }
            }
        }
    }
}

bool Simulation::solve_step(Matrix& matrix) {
    const auto solve = [&] {
        auto solution = b_;
        if (!klu_l_tsolve(Symbolic_, Numeric_, matrix.rp.size() - 1, 1, solution.data(), &Common_)) {
            Errors::simulation_errors(SimulationErrors::MATRIX_SINGULAR);
        }
        return solution;
    };
    auto trial = solve();
    if (nonlinearInductors_.empty()) {
        if (!std::all_of(trial.begin(), trial.end(), [](double x) { return std::isfinite(x); })) {
            Errors::simulation_errors(SimulationErrors::SOLVER_FAILURE,
                "Non-finite solution; model or numerical range exceeded");
        }
        x_ = std::move(trial);
        return true;
    }
    const auto failed = [&](const std::string& message) {
        nonlinearFailure_ = message;
        needsTR_ = true;
        return false;
    };
    const double scale = atyp_ == AnalysisType::Voltage ? 1.5 / stepSize_ : 1.0 / Constants::SIGMA;
    const auto evaluate = [&](const std::vector<double>& values, std::vector<double>& residuals,
                              std::vector<double>* tolerances) {
        if (!std::all_of(values.begin(), values.end(), [](double x) { return std::isfinite(x); })) { return false; }
        residuals.clear();
        if (tolerances) { tolerances->clear(); }
        for (auto index : nonlinearInductors_) {
            const auto& device = std::get<Inductor>(matrix.components.devices[index]);
            const double current = values[*device.indexInfo.currentIndex_];
            const double self = device.flux(current), inductance = device.differential_inductance(current);
            const double node = ((device.indexInfo.posIndex_ ? values[*device.indexInfo.posIndex_] : 0.0)
                               - (device.indexInfo.negIndex_ ? values[*device.indexInfo.negIndex_] : 0.0)) / scale;
            double mutualFlux = 0.0, magnitude = std::abs(node) + std::abs(self) + std::abs(device.fluxHistory_);
            for (const auto& mutual : device.mutualInductances_) {
                const auto& other = std::get<Inductor>(matrix.components.devices[mutual.first]);
                const double flux = mutual.second * values[*other.indexInfo.currentIndex_];
                mutualFlux += flux;
                magnitude += std::abs(flux);
            }
            const double residual = node - self - mutualFlux - device.fluxHistory_;
            if (!std::isfinite(residual) || !std::isfinite(magnitude) || !std::isfinite(scale * inductance)) {
                return false;
            }
            residuals.push_back(residual);
            if (tolerances) { tolerances->push_back(1e-26 + 1e-12 * magnitude); }
        }
        return true;
    };
    const auto norm = [](const std::vector<double>& residuals, const std::vector<double>& tolerances) {
        double maximum = 0.0;
        for (size_t i = 0; i < residuals.size(); ++i) {
            maximum = std::max(maximum, std::abs(residuals[i]) / tolerances[i]);
        }
        return maximum;
    };
    // setup_b samples sources/predictors and shifts histories once. Inner
    // iterations change only the nonlinear inductor rows and the trial solution.
    for (int iteration = 0; iteration <= 40; ++iteration) {
        std::vector<double> residuals, tolerances;
        if (!evaluate(trial, residuals, &tolerances)) {
            return failed("non-finite nonlinear flux or differential inductance");
        }
        const double previous = norm(residuals, tolerances);
        if (previous <= 1.0) { x_ = std::move(trial); return true; }
        if (iteration == 40) { return failed("nonlinear inductor Newton iteration limit reached"); }
        for (auto index : nonlinearInductors_) {
            auto& device = std::get<Inductor>(matrix.components.devices[index]);
            const auto row = *device.indexInfo.currentIndex_;
            const double current = trial[row];
            b_[row] = scale * (device.fluxHistory_ + device.flux(current)
                              - device.differential_inductance(current) * current);
            if (!std::isfinite(b_[row])) { return failed("non-finite nonlinear inductor linearization"); }
            needsLU_ = device.linearize(current, stepSize_) || needsLU_;
        }
        if (needsLU_) { factorize(matrix); }
        const auto candidate = solve();
        bool accepted = false;
        double damping = 1.0;
        for (int backtrack = 0; backtrack <= 16; ++backtrack, damping *= 0.5) {
            std::vector<double> next(trial.size()), nextResiduals;
            for (size_t i = 0; i < next.size(); ++i) {
                next[i] = (1.0 - damping) * trial[i] + damping * candidate[i];
            }
            if (evaluate(next, nextResiduals, nullptr)) {
                const double nextNorm = norm(nextResiduals, tolerances);
                if (nextNorm <= 1.0 || nextNorm < previous * (1.0 - 1e-4 * damping)) {
                    trial = std::move(next);
                    accepted = true;
                    break;
                }
            }
        }
        if (!accepted) { return failed("nonlinear inductor Newton damping did not reduce the flux residual"); }
    }
    return failed("nonlinear inductor solve did not converge");
}
