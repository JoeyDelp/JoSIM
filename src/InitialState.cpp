// Copyright (c) 2026 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/Simulation.hpp"
#include "JoSIM/Constants.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <map>
#include <optional>

using namespace JoSIM;

namespace {
[[noreturn]] void initial_error(const std::string& message) {
    Errors::simulation_errors(SimulationErrors::INVALID_MATRIX, "Capacitor IC: " + message);
}

// Initial voltages are algebraic unknowns; h*dV/dt supplies capacitor currents.
struct InitialSystem {
    std::vector<std::map<int64_t, double>> rows;
    std::vector<double> rhs;
    explicit InitialSystem(int64_t size) : rows(size), rhs(size, 0.0) {}
    void add(int64_t row, int64_t column, double value) { rows.at(row)[column] += value; }
    void difference(int64_t row, int_o positive, int_o negative, double factor, int64_t offset = 0) {
        if (positive) { add(row, offset + *positive, factor); }
        if (negative) { add(row, offset + *negative, -factor); }
    }
    std::vector<double> solve() const {
        std::vector<int64_t> rp{0}, ci;
        std::vector<double> nz;
        for (const auto& row : rows) {
            for (const auto& entry : row) {
                if (!std::isfinite(entry.second)) { initial_error("non-finite initial coefficient"); }
                if (entry.second != 0) { ci.push_back(entry.first); nz.push_back(entry.second); }
            }
            rp.push_back(static_cast<int64_t>(nz.size()));
        }
        auto solution = rhs;
        klu_l_common common{};
        klu_l_defaults(&common);
        auto* symbolic = klu_l_analyze(static_cast<int64_t>(rows.size()), rp.data(), ci.data(), &common);
        auto* numeric = symbolic ? klu_l_factor(rp.data(), ci.data(), nz.data(), symbolic, &common) : nullptr;
        const bool solved = numeric && klu_l_tsolve(symbolic, numeric, rows.size(), 1, solution.data(), &common);
        klu_l_free_numeric(&numeric, &common);
        klu_l_free_symbolic(&symbolic, &common);
        if (!solved) { initial_error("incompatible or underdetermined initial constraints"); }
        if (!std::all_of(solution.begin(), solution.end(), [](double x) { return std::isfinite(x); })) {
            initial_error("non-finite initial solution");
        }
        return solution;
    }
};

// Select independent constraints before sparse LU. Controlled-source feedback
// needs general linear relations, rather than only pairwise voltage offsets.
class InitialConstraints {
    using Row = std::map<int64_t, double>;
    InitialSystem selected_;
    std::vector<Row> basis_;
    std::vector<double> rhs_;
    int64_t rank_ = 0;
    static constexpr double roundoff = 64 * std::numeric_limits<double>::epsilon();

    static void subtract(Row& row, const Row& other, int64_t pivot, double factor) {
        row.erase(pivot);
        for (const auto& entry : other) {
            if (entry.first == pivot) { continue; }
            const double before = row[entry.first], product = factor * entry.second;
            const double after = before - product;
            if (!std::isfinite(after)) { initial_error("initial constraint exceeds the numerical range"); }
            if (std::abs(after) <= roundoff * (std::abs(before) + std::abs(product))) { row.erase(entry.first); }
            else { row[entry.first] = after; }
        }
    }

  public:
    explicit InitialConstraints(int64_t size) : selected_(size), basis_(size), rhs_(size, 0.0) {}
    std::optional<double> implied(Row row) const {
        double value = 0.0;
        for (auto it = row.begin(); it != row.end();) {
            if (it->second == 0) { it = row.erase(it); }
            else { ++it; }
        }
        while (!row.empty()) {
            const auto pivot = row.begin()->first;
            if (basis_[pivot].empty()) { return std::nullopt; }
            const double factor = row.begin()->second;
            value += factor * rhs_[pivot];
            subtract(row, basis_[pivot], pivot, factor);
        }
        if (!std::isfinite(value)) { initial_error("non-finite source curvature"); }
        return value;
    }
    void add(Row row, double rhs, const std::string& label, bool required = true) {
        double scale = 0.0;
        for (auto it = row.begin(); it != row.end();) {
            if (!std::isfinite(it->second)) { initial_error(label + ": non-finite initial coefficient"); }
            scale = std::max(scale, std::abs(it->second));
            if (it->second == 0) { it = row.erase(it); }
            else { ++it; }
        }
        if (!std::isfinite(rhs)) { initial_error(label + ": non-finite initial constraint"); }
        if (scale == 0) { scale = 1.0; }
        for (auto& entry : row) { entry.second /= scale; }
        rhs /= scale;
        if (!std::isfinite(rhs)) { initial_error(label + ": initial constraint exceeds the numerical range"); }
        const auto original = row;
        const double originalRhs = rhs;
        double magnitude = std::abs(rhs);
        while (!row.empty() && !basis_[row.begin()->first].empty()) {
            const auto pivot = row.begin()->first;
            const double factor = row.begin()->second;
            magnitude += std::abs(factor * rhs_[pivot]);
            rhs -= factor * rhs_[pivot];
            if (!std::isfinite(rhs) || !std::isfinite(magnitude)) {
                initial_error(label + ": initial constraint exceeds the numerical range");
            }
            subtract(row, basis_[pivot], pivot, factor);
        }
        if (row.empty()) {
            if (required && std::abs(rhs) > 1e-12 + 1e-10 * magnitude) {
                initial_error(label + ": inconsistent initial voltage or phase/source constraints");
            }
            return;
        }
        const auto pivot = row.begin()->first;
        const double divisor = row.begin()->second;
        for (auto& entry : row) {
            entry.second /= divisor;
            if (!std::isfinite(entry.second)) { initial_error(label + ": ill-conditioned initial constraints"); }
        }
        basis_[pivot] = std::move(row);
        rhs_[pivot] = rhs / divisor;
        if (!std::isfinite(rhs_[pivot])) { initial_error(label + ": ill-conditioned initial constraints"); }
        selected_.rows[rank_] = original;
        selected_.rhs[rank_++] = originalRhs;
    }

    std::vector<double> solve(const std::vector<bool>& required) {
        // Reduce only for uniqueness checks. Auxiliary capacitor-current
        // derivatives may be free; voltages, currents and their seeded histories
        // must not depend on those arbitrary derivatives.
        for (int64_t pivot = basis_.size(); pivot-- > 0;) {
            auto& row = basis_[pivot];
            if (row.empty()) {
                if (required[pivot]) { initial_error("incompatible or underdetermined initial constraints"); }
                continue;
            }
            auto it = row.upper_bound(pivot);
            while (it != row.end()) {
                const auto column = it->first;
                if (!basis_[column].empty()) { subtract(row, basis_[column], column, it->second); }
                it = row.upper_bound(column);
            }
            if (required[pivot] && row.size() != 1) {
                initial_error("incompatible or underdetermined initial constraints");
            }
        }
        for (size_t column = 0; column < basis_.size(); ++column) {
            if (basis_[column].empty()) { selected_.add(rank_, column, 1.0); ++rank_; }
        }
        return selected_.solve();
    }
};

double difference(const std::vector<double>& values, int_o positive, int_o negative) {
    return (positive ? values.at(*positive) : 0.0) - (negative ? values.at(*negative) : 0.0);
}
} // namespace

bool Simulation::initialize_capacitors(Matrix& matrix) {
    auto& components = matrix.components;
    const bool requested = std::any_of(components.capacitorIndices.begin(), components.capacitorIndices.end(),
        [&](int64_t index) { return std::get<Capacitor>(components.devices[index]).initialVoltage_.has_value(); });
    if (!requested) { return false; }
    if (startup_) { initial_error("explicit initial voltage requires .TRAN ... DST"); }
    const int64_t nodes = matrix.nm.size(), count = matrix.branchIndex;
    // State is (V, I, h*dV/dt, h*dI/dt). Capacitor-current derivatives
    // complete differentiated KCL but are not needed for transient histories.
    const int64_t total = 2 * count;
    InitialSystem constraints(total);
    std::vector<double> sources(matrix.sourcegen.size()), slopes(matrix.sourcegen.size());
    const double probeStep = stepSize_ * 1e-4;
    if (!(probeStep > 0)) { initial_error("timestep is too small to resolve source startup"); }
    const auto sample = [&](Function& function) {
        const double value = function.value(0.0);
        auto probe = function;
        const double slope = (probe.value(probeStep) - value) * (stepSize_ / probeStep);
        if (!std::isfinite(value) || !std::isfinite(slope)) { initial_error("non-finite source startup"); }
        return std::make_pair(value, slope);
    };
    for (size_t i = 0; i < sources.size(); ++i) {
        const auto value = sample(matrix.sourcegen[i]);
        sources[i] = value.first;
        slopes[i] = value.second;
    }

    const auto source_derivatives = [&](int64_t source) {
        const double value = sources[source], delta = stepSize_ / 8;
        auto probe = matrix.sourcegen[source];
        const double a = probe.value(delta) - value, b = probe.value(2 * delta) - value;
        const double c = probe.value(3 * delta) - value, d = probe.value(4 * delta) - value;
        // h*f', h^2*f'' and h^3*f''' from right-hand probes, at most h/2
        // from zero. Copies retain the actual source's cursor and random stream.
        return std::array<double, 3>{(48 * a - 36 * b + 16 * c - 3 * d) * (8.0 / 12),
            (-104 * a + 114 * b - 56 * c + 11 * d) * (64.0 / 12),
            (18 * a - 24 * b + 14 * c - 3 * d) * 256.0};
    };

    InitialConstraints phaseConstraints(nodes);
    InitialConstraints curvatureConstraints(nodes);
    const auto phase_constraint = [&](int_o positive, int_o negative, double phase,
                                      const std::string& label, bool required = true) {
        InitialSystem equation(1);
        equation.difference(0, positive, negative, 1.0);
        phaseConstraints.add(equation.rows[0], phase, label, required);
    };
    const auto source_curvature = [&](const auto& device, double curvature) {
        InitialSystem equation(1);
        equation.difference(0, device.indexInfo.posIndex_, device.indexInfo.negIndex_, 1.0);
        curvatureConstraints.add(equation.rows[0], curvature, device.netlistInfo.label_);
    };
    for (auto index : components.vsIndices) {
        const auto& device = std::get<VoltageSource>(components.devices[index]);
        source_curvature(device, source_derivatives(device.sourceIndex_)[1]);
    }
    for (auto index : components.psIndices) {
        const auto& device = std::get<PhaseSource>(components.devices[index]);
        const auto source = device.sourceIndex_;
        const double phase = sources[source];
        phase_constraint(device.indexInfo.posIndex_, device.indexInfo.negIndex_, phase, device.netlistInfo.label_);
        const auto derivatives = source_derivatives(source);
        sources[source] = (Constants::SIGMA / stepSize_) * derivatives[0];
        slopes[source] = (Constants::SIGMA / stepSize_) * derivatives[1];
        source_curvature(device, (Constants::SIGMA / stepSize_) * derivatives[2]);
        if (!std::isfinite(sources[source]) || !std::isfinite(slopes[source])) {
            initial_error(device.netlistInfo.label_ + ": non-finite phase-source startup derivative");
        }
    }
    for (auto index : components.vcvsIndices) {
        const auto& device = std::get<VCVS>(components.devices[index]);
        InitialSystem equation(1);
        equation.difference(0, device.indexInfo.posIndex_, device.indexInfo.negIndex_, 1.0);
        equation.difference(0, device.posIndex2_, device.negIndex2_, -device.netlistInfo.value_);
        phaseConstraints.add(equation.rows[0], 0.0, device.netlistInfo.label_);
        curvatureConstraints.add(equation.rows[0], 0.0, device.netlistInfo.label_);
    }
    for (auto index : components.cccsIndices) {
        const auto& device = std::get<CCCS>(components.devices[index]);
        phase_constraint(device.posIndex2_, device.negIndex2_, 0.0, device.netlistInfo.label_);
        InitialSystem equation(1);
        equation.difference(0, device.posIndex2_, device.negIndex2_, 1.0);
        curvatureConstraints.add(equation.rows[0], 0.0, device.netlistInfo.label_);
    }
    for (auto index : components.ccvsIndices) {
        const auto& device = std::get<CCVS>(components.devices[index]);
        phase_constraint(device.posIndex2_, device.negIndex2_, 0.0, device.netlistInfo.label_);
        InitialSystem equation(1);
        equation.difference(0, device.posIndex2_, device.negIndex2_, 1.0);
        curvatureConstraints.add(equation.rows[0], 0.0, device.netlistInfo.label_);
    }
    for (auto index : components.inductorIndices) {
        const auto& device = std::get<Inductor>(components.devices[index]);
        phase_constraint(device.indexInfo.posIndex_, device.indexInfo.negIndex_, 0.0,
                         device.netlistInfo.label_ + " (zero initial inductor current)");
    }
    for (auto index : components.junctionIndices) {
        const auto& device = std::get<JJ>(components.devices[index]);
        phase_constraint(device.indexInfo.posIndex_, device.indexInfo.negIndex_, 0.0, device.netlistInfo.label_, false);
    }
    for (int64_t node = 0; node < nodes; ++node) {
        phase_constraint(node, std::nullopt, 0.0, "initial node phase", false);
    }
    const auto phases = phaseConstraints.solve(std::vector<bool>(nodes, true));
    const auto voltage_curvature = [&](int_o positive, int_o negative) {
        InitialSystem equation(1);
        equation.difference(0, positive, negative, 1.0);
        return curvatureConstraints.implied(equation.rows[0]);
    };

    for (int64_t row = 0; row < nodes; ++row) {
        for (const auto& entry : matrix.nc[row]) {
            constraints.add(row, entry.second, entry.first);
            constraints.add(count + row, count + entry.second, entry.first);
        }
    }
    const auto inject = [&](const BasicComponent& device, double value, double slope) {
        if (device.indexInfo.posIndex_) {
            constraints.rhs[*device.indexInfo.posIndex_] -= value;
            constraints.rhs[count + *device.indexInfo.posIndex_] -= slope;
        }
        if (device.indexInfo.negIndex_) {
            constraints.rhs[*device.indexInfo.negIndex_] += value;
            constraints.rhs[count + *device.indexInfo.negIndex_] += slope;
        }
    };
    for (const auto& device : components.currentsources) {
        inject(device, sources[device.sourceIndex_], slopes[device.sourceIndex_]);
        if (!results.sourceCurrents.empty() && results.sourceCurrents[device.sourceIndex_]) {
            results.sourceCurrents[device.sourceIndex_]->push_back(sources[device.sourceIndex_]);
        }
    }
    const auto resistor = [&](double resistance, int64_t current, int_o positive, int_o negative) {
        for (int64_t offset : {int64_t(0), count}) {
            constraints.difference(offset + current, positive, negative, 1.0, offset);
            constraints.add(offset + current, offset + current, -resistance);
        }
    };
    for (auto index : components.resistorIndices) {
        auto& device = std::get<Resistor>(components.devices[index]);
        resistor(device.netlistInfo.value_, *device.indexInfo.currentIndex_,
                 device.indexInfo.posIndex_, device.indexInfo.negIndex_);
        if (device.thermalNoise) {
            const auto noise = sample(*device.thermalNoise);
            inject(device, noise.first, noise.second);
        }
    }
    for (auto index : components.txIndices) {
        const auto& device = std::get<TransmissionLine>(components.devices[index]);
        resistor(device.netlistInfo.value_, *device.indexInfo.currentIndex_,
                 device.indexInfo.posIndex_, device.indexInfo.negIndex_);
        resistor(device.netlistInfo.value_, device.currentIndex2_, device.posIndex2_, device.negIndex2_);
    }
    for (auto index : components.capacitorIndices) {
        const auto& device = std::get<Capacitor>(components.devices[index]);
        const auto row = *device.indexInfo.currentIndex_;
        constraints.add(row, row, 1.0);
        constraints.difference(row, device.indexInfo.posIndex_, device.indexInfo.negIndex_,
                               -device.netlistInfo.value_ / stepSize_, count);
        const auto curvature = voltage_curvature(device.indexInfo.posIndex_, device.indexInfo.negIndex_);
        if (curvature) {
            constraints.add(count + row, count + row, 1.0);
            constraints.rhs[count + row] = (device.netlistInfo.value_ / stepSize_) * *curvature;
        }
    }
    std::map<int64_t, int64_t> inductorSlopes;
    for (auto index : components.inductorIndices) {
        const auto& device = std::get<Inductor>(components.devices[index]);
        const auto current = *device.indexInfo.currentIndex_, row = count + current;
        inductorSlopes[index] = row;
        constraints.add(current, current, 1.0);
        constraints.add(row, row, device.netlistInfo.value_);
        constraints.difference(row, device.indexInfo.posIndex_, device.indexInfo.negIndex_, -stepSize_);
        for (const auto& mutual : device.mutualInductances_) {
            const auto& other = std::get<Inductor>(components.devices[mutual.first]);
            constraints.add(row, count + *other.indexInfo.currentIndex_, mutual.second);
        }
    }
    const auto voltage_source = [&](const auto& device) {
        const auto row = *device.indexInfo.currentIndex_;
        constraints.difference(row, device.indexInfo.posIndex_, device.indexInfo.negIndex_, 1.0);
        constraints.rhs[row] = sources[device.sourceIndex_];
        constraints.difference(count + row, device.indexInfo.posIndex_, device.indexInfo.negIndex_, 1.0, count);
        constraints.rhs[count + row] = slopes[device.sourceIndex_];
    };
    for (auto index : components.vsIndices) { voltage_source(std::get<VoltageSource>(components.devices[index])); }
    for (auto index : components.psIndices) { voltage_source(std::get<PhaseSource>(components.devices[index])); }
    for (auto index : components.vcvsIndices) {
        const auto& device = std::get<VCVS>(components.devices[index]);
        for (int64_t offset : {int64_t(0), count}) {
            const auto row = offset + *device.indexInfo.currentIndex_;
            constraints.difference(row, device.indexInfo.posIndex_, device.indexInfo.negIndex_, 1.0, offset);
            constraints.difference(row, device.posIndex2_, device.negIndex2_, -device.netlistInfo.value_, offset);
        }
    }
    for (auto index : components.vccsIndices) {
        const auto& device = std::get<VCCS>(components.devices[index]);
        for (int64_t offset : {int64_t(0), count}) {
            const auto row = offset + *device.indexInfo.currentIndex_;
            constraints.add(row, row, 1.0);
            constraints.difference(row, device.posIndex2_, device.negIndex2_, -device.netlistInfo.value_, offset);
        }
    }
    for (auto index : components.cccsIndices) {
        const auto& device = std::get<CCCS>(components.devices[index]);
        for (int64_t offset : {int64_t(0), count}) {
            constraints.difference(offset + *device.indexInfo.currentIndex_, device.posIndex2_, device.negIndex2_, 1.0, offset);
        }
    }
    for (auto index : components.ccvsIndices) {
        const auto& device = std::get<CCVS>(components.devices[index]);
        for (int64_t offset : {int64_t(0), count}) {
            const auto row = offset + *device.indexInfo.currentIndex_;
            constraints.difference(row, device.indexInfo.posIndex_, device.indexInfo.negIndex_, 1.0, offset);
            constraints.add(row, row, -device.netlistInfo.value_);
            constraints.difference(offset + device.currentIndex2_, device.posIndex2_, device.negIndex2_, 1.0, offset);
        }
    }

    std::vector<bool> required(total, false);
    std::fill(required.begin(), required.begin() + count + nodes, true);
    for (const auto& slope : inductorSlopes) { required[slope.second] = true; }
    std::vector<double> state(total, 0.0);
    bool converged = false;
    for (int iteration = 0; iteration < 32; ++iteration) {
        auto system = constraints;
        for (auto index : components.junctionIndices) {
            auto& device = std::get<JJ>(components.devices[index]);
            const auto positive = device.indexInfo.posIndex_, negative = device.indexInfo.negIndex_;
            const double voltage = difference(state, positive, negative);
            const double phase = difference(phases, positive, negative);
            if (device.model_.rtype() == 1) { device.update_value(voltage); }
            const double gateVoltage = device.gateIndex_ ? state[*device.gateIndex_] : 0.0;
            const double gate = std::clamp(1.0 - std::abs(gateVoltage) / device.model_.gwidth(), 0.0, 1.0);
            const auto criticalNoise = device.icNoise ? sample(*device.icNoise) : std::make_pair(0.0, 0.0);
            const double noise = std::max(-0.99, criticalNoise.first);
            const double scale = gate * (1.0 + noise);
            const auto row = *device.indexInfo.currentIndex_;
            system.add(device.variableIndex_, device.variableIndex_, 1.0);
            system.add(count + device.variableIndex_, count + device.variableIndex_, 1.0);
            system.add(row, row, -1.0);
            system.difference(row, positive, negative, device.conductance());
            system.difference(row, positive, negative, device.model_.c() / stepSize_, count);
            system.rhs[row] = -device.supercurrent(phase, scale) - device.it_;
            if (device.thermalNoise) {
                const auto thermal = sample(*device.thermalNoise);
                if (positive) { system.rhs[*positive] -= thermal.first; system.rhs[count + *positive] -= thermal.second; }
                if (negative) { system.rhs[*negative] += thermal.first; system.rhs[count + *negative] += thermal.second; }
            }
            const auto curvature = voltage_curvature(positive, negative);
            if (device.model_.c() == 0 || curvature) {
                const auto derivativeRow = count + row;
                system.add(derivativeRow, derivativeRow, -1.0);
                system.difference(derivativeRow, positive, negative, device.conductance(), count);
                const double derivative = (device.supercurrent(phase + 1e-6, scale)
                                         - device.supercurrent(phase - 1e-6, scale)) / 2e-6;
                system.difference(derivativeRow, positive, negative, stepSize_ * derivative / Constants::SIGMA);
                if (curvature) { system.rhs[derivativeRow] = -(device.model_.c() / stepSize_) * *curvature; }
                const double direction = gateVoltage != 0 ? gateVoltage
                    : (device.gateIndex_ ? state[count + *device.gateIndex_] : 0.0);
                if (device.gateIndex_ && gate > 0 && direction != 0) {
                    system.add(derivativeRow, count + *device.gateIndex_,
                        -device.supercurrent(phase, 1.0 + noise) * std::copysign(1.0, direction) / device.model_.gwidth());
                }
                if (criticalNoise.first > -0.99 || (criticalNoise.first == -0.99 && criticalNoise.second > 0)) {
                    system.rhs[derivativeRow] -= device.supercurrent(phase, gate) * criticalNoise.second;
                }
            }
        }
        InitialConstraints independent(total);
        for (int64_t row = 0; row < total; ++row) { independent.add(system.rows[row], system.rhs[row], "source/circuit"); }
        const auto initial_voltage = [&](const BasicComponent& device, double voltage, bool explicitIC) {
            InitialSystem equation(1);
            equation.difference(0, device.indexInfo.posIndex_, device.indexInfo.negIndex_, 1.0);
            independent.add(equation.rows[0], voltage, device.netlistInfo.label_, explicitIC);
        };
        for (auto index : components.capacitorIndices) {
            const auto& device = std::get<Capacitor>(components.devices[index]);
            if (device.initialVoltage_) { initial_voltage(device, *device.initialVoltage_, true); }
        }
        for (auto index : components.capacitorIndices) {
            const auto& device = std::get<Capacitor>(components.devices[index]);
            if (!device.initialVoltage_) { initial_voltage(device, 0.0, false); }
        }
        for (auto index : components.junctionIndices) {
            const auto& device = std::get<JJ>(components.devices[index]);
            if (device.model_.c() > 0) { initial_voltage(device, 0.0, false); }
        }
        auto solved = independent.solve(required);
        double change = 0.0;
        for (int64_t node = 0; node < nodes; ++node) {
            change = std::max(change, std::abs(solved[node] - state[node]) / (1e-12 + std::abs(solved[node])));
            change = std::max(change, std::abs(solved[count + node] - state[count + node])
                                     / (1e-12 + std::abs(solved[count + node])));
        }
        state = std::move(solved);
        if (change < 1e-10 || components.junctionIndices.empty()) { converged = true; break; }
    }
    if (!converged) { initial_error("junction initial-state selection did not converge"); }

    const auto phase_at = [&](int_o node, double steps) {
        return node ? phases[*node] + (steps * stepSize_ / Constants::SIGMA)
                          * (state[*node] + 0.5 * steps * state[count + *node]) : 0.0;
    };
    const auto phase_difference = [&](int_o positive, int_o negative, double steps) {
        return phase_at(positive, steps) - phase_at(negative, steps);
    };
    x_.assign(state.begin(), state.begin() + count);
    if (atyp_ == AnalysisType::Phase) { std::copy(phases.begin(), phases.end(), x_.begin()); }
    for (auto& history : results.phaseBefore) {
        history.second = {phase_at(history.first, -1), phase_at(history.first, -2)};
    }
    for (auto index : components.capacitorIndices) {
        auto& device = std::get<Capacitor>(components.devices[index]);
        const auto p = device.indexInfo.posIndex_, n = device.indexInfo.negIndex_;
        device.pn2_ = atyp_ == AnalysisType::Voltage
            ? difference(state, p, n) - difference(state, p ? int_o(count + *p) : std::nullopt,
                                                       n ? int_o(count + *n) : std::nullopt)
            : phase_difference(p, n, -1);
        if (atyp_ == AnalysisType::Phase) {
            device.pn3_ = phase_difference(p, n, -2);
            device.pn4_ = phase_difference(p, n, -3);
        }
    }
    for (auto index : components.inductorIndices) {
        auto& device = std::get<Inductor>(components.devices[index]);
        device.In2_ = -state[inductorSlopes.at(index)];
        device.previousFlux_ = Constants::SIGMA
            * phase_difference(device.indexInfo.posIndex_, device.indexInfo.negIndex_, -1);
    }
    for (auto index : components.junctionIndices) {
        auto& device = std::get<JJ>(components.devices[index]);
        const auto p = device.indexInfo.posIndex_, n = device.indexInfo.negIndex_;
        const double voltage = difference(state, p, n);
        const double slope = difference(state, p ? int_o(count + *p) : std::nullopt,
                                               n ? int_o(count + *n) : std::nullopt);
        device.pn2_ = phase_difference(p, n, -1);
        device.pn3_ = phase_difference(p, n, -2);
        device.vn2_ = voltage - slope;
        device.vn3_ = voltage - 2 * slope;
        x_[device.variableIndex_] = atyp_ == AnalysisType::Phase ? voltage : phase_difference(p, n, 0);
        device.gatePn1_ = phase_at(device.gateIndex_, -1);
        device.gatePn2_ = phase_at(device.gateIndex_, -2);
    }
    for (auto index : components.resistorIndices) {
        auto& device = std::get<Resistor>(components.devices[index]);
        device.pn2_ = phase_difference(device.indexInfo.posIndex_, device.indexInfo.negIndex_, -1);
    }
    for (auto index : components.vsIndices) {
        auto& device = std::get<VoltageSource>(components.devices[index]);
        device.pn2_ = phase_difference(device.indexInfo.posIndex_, device.indexInfo.negIndex_, -1);
    }
    for (auto index : components.psIndices) {
        auto& device = std::get<PhaseSource>(components.devices[index]);
        device.pn1_ = phase_difference(device.indexInfo.posIndex_, device.indexInfo.negIndex_, 0);
        device.pn2_ = phase_difference(device.indexInfo.posIndex_, device.indexInfo.negIndex_, -1);
    }
    for (auto index : components.ccvsIndices) {
        auto& device = std::get<CCVS>(components.devices[index]);
        device.pn2_ = phase_difference(device.indexInfo.posIndex_, device.indexInfo.negIndex_, -1);
    }
    for (auto index : components.vccsIndices) {
        auto& device = std::get<VCCS>(components.devices[index]);
        device.pn2_ = phase_difference(device.posIndex2_, device.negIndex2_, -1);
    }
    for (auto index : components.txIndices) {
        auto& device = std::get<TransmissionLine>(components.devices[index]);
        device.n1_1_ = phase_difference(device.indexInfo.posIndex_, device.indexInfo.negIndex_, -1);
        device.n1_2_ = phase_difference(device.posIndex2_, device.negIndex2_, -1);
        if (atyp_ == AnalysisType::Phase) {
            for (auto node : {device.indexInfo.posIndex_, device.indexInfo.negIndex_, device.posIndex2_, device.negIndex2_}) {
                if (node) { results.phaseBefore[*node] = {phase_at(node, -1), phase_at(node, -2)}; }
            }
        }
    }
    for (size_t i = 0; i < results.xVector.size(); ++i) {
        if (results.xVector[i]) { results.xVector[i]->push_back(x_[i]); }
    }
    results.timeAxis.push_back(0.0);
    needsLU_ = true;
    return true;
}
