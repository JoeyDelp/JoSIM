// Copyright (c) 2026 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/Simulation.hpp"
#include "JoSIM/Constants.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <numeric>

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

// Offsets give V(node)-V(root), including a separate ground entry.
struct VoltageForest {
    std::vector<int64_t> parent, rank;
    std::vector<double> offset;
    explicit VoltageForest(int64_t size) : parent(size), rank(size, 0), offset(size, 0.0) {
        std::iota(parent.begin(), parent.end(), 0);
    }
    int64_t root(int64_t node) {
        if (parent[node] != node) {
            const auto previous = parent[node];
            parent[node] = root(previous);
            offset[node] += offset[previous];
        }
        return parent[node];
    }
    bool join(int64_t positive, int64_t negative, double voltage, const std::string& label, bool check = true) {
        auto p = root(positive), n = root(negative);
        const double delta = voltage - offset[positive] + offset[negative];
        if (!std::isfinite(delta)) { initial_error(label + ": initial voltage exceeds the numerical range"); }
        if (p == n) {
            if (check && std::abs(delta) > 1e-12 + 1e-10 * std::abs(voltage)) {
                initial_error(label + ": inconsistent initial voltage constraints");
            }
            return false;
        }
        if (rank[p] > rank[n]) { parent[n] = p; offset[n] = -delta; }
        else {
            parent[p] = n;
            offset[p] = delta;
            if (rank[p] == rank[n]) { ++rank[n]; }
        }
        return true;
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
    if (!components.psIndices.empty() || !components.vcvsIndices.empty() || !components.ccvsIndices.empty()
        || !components.vccsIndices.empty() || !components.cccsIndices.empty()) {
        initial_error("initialization with phase sources or controlled sources is not supported");
    }

    const int64_t nodes = matrix.nm.size(), ground = nodes, count = matrix.branchIndex;
    const int64_t total = count + nodes + components.inductorIndices.size();
    InitialSystem constraints(total);
    VoltageForest forest(nodes + 1);
    int64_t next = count;
    std::vector<double> sources(matrix.sourcegen.size()), slopes(matrix.sourcegen.size());
    const double probeStep = stepSize_ * 1e-4;
    if (!(probeStep > 0)) { initial_error("timestep is too small to resolve source startup"); }
    const auto sample = [&](Function& function) {
        const double value = function.value(0.0);
        // The probe stays inside the first noise interval (which is at least h).
        // Use a copy so waveform cursors and cached samples are not advanced.
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
    const auto connect = [&](const BasicComponent& device, double voltage, bool check) {
        return forest.join(device.indexInfo.posIndex_.value_or(ground),
                           device.indexInfo.negIndex_.value_or(ground), voltage, device.netlistInfo.label_, check);
    };
    for (auto index : components.vsIndices) {
        const auto& device = std::get<VoltageSource>(components.devices[index]);
        if (connect(device, sources[device.sourceIndex_], true)) {
            constraints.difference(next, device.indexInfo.posIndex_, device.indexInfo.negIndex_, 1.0, count);
            constraints.rhs[next++] = slopes[device.sourceIndex_];
        }
    }
    const auto constrain_voltage = [&](const BasicComponent& device, double voltage, bool check) {
        if (connect(device, voltage, check)) {
            constraints.difference(next, device.indexInfo.posIndex_, device.indexInfo.negIndex_, 1.0);
            constraints.rhs[next++] = voltage;
        }
    };
    for (auto index : components.capacitorIndices) {
        const auto& device = std::get<Capacitor>(components.devices[index]);
        if (device.initialVoltage_) { constrain_voltage(device, *device.initialVoltage_, true); }
    }
    for (auto index : components.capacitorIndices) {
        const auto& device = std::get<Capacitor>(components.devices[index]);
        if (!device.initialVoltage_) { constrain_voltage(device, 0.0, false); }
    }
    for (auto index : components.junctionIndices) {
        const auto& device = std::get<JJ>(components.devices[index]);
        if (device.model_.c() > 0) { constrain_voltage(device, 0.0, false); }
    }
    // Each floating capacitive island needs differentiated KCL to determine its
    // common-mode voltage derivative. This also handles purely algebraic nodes.
    std::map<int64_t, int64_t> islandRows;
    const auto groundRoot = forest.root(ground);
    for (int64_t node = 0; node < nodes; ++node) {
        const auto root = forest.root(node);
        if (root != groundRoot && !islandRows.count(root)) { islandRows[root] = next++; }
    }
    if (next != count + nodes) { initial_error("invalid initial constraint count"); }
    const auto island_sign = [&](int_o positive, int_o negative, int64_t island) {
        return (positive && forest.root(*positive) == island ? 1.0 : 0.0)
             - (negative && forest.root(*negative) == island ? 1.0 : 0.0);
    };
    for (int64_t row = 0; row < nodes; ++row) {
        for (const auto& entry : matrix.nc[row]) { constraints.add(row, entry.second, entry.first); }
    }
    const auto inject = [&](const BasicComponent& device, double value, double slope) {
        if (device.indexInfo.posIndex_) { constraints.rhs[*device.indexInfo.posIndex_] -= value; }
        if (device.indexInfo.negIndex_) { constraints.rhs[*device.indexInfo.negIndex_] += value; }
        for (const auto& island : islandRows) {
            constraints.rhs[island.second] -= slope
                * island_sign(device.indexInfo.posIndex_, device.indexInfo.negIndex_, island.first);
        }
    };
    for (const auto& device : components.currentsources) {
        inject(device, sources[device.sourceIndex_], slopes[device.sourceIndex_]);
        if (!results.sourceCurrents.empty() && results.sourceCurrents[device.sourceIndex_]) {
            results.sourceCurrents[device.sourceIndex_]->push_back(sources[device.sourceIndex_]);
        }
    }
    const auto resistor = [&](double resistance, int64_t current,
                               int_o positive, int_o negative) {
        constraints.difference(current, positive, negative, 1.0);
        constraints.add(current, current, -resistance);
        for (const auto& island : islandRows) {
            const double sign = island_sign(positive, negative, island.first);
            constraints.difference(island.second, positive, negative, sign / resistance, count);
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
    }
    std::map<int64_t, int64_t> inductorSlopes;
    for (auto index : components.inductorIndices) { inductorSlopes[index] = next++; }
    for (auto index : components.inductorIndices) {
        const auto& device = std::get<Inductor>(components.devices[index]);
        constraints.add(*device.indexInfo.currentIndex_, *device.indexInfo.currentIndex_, 1.0);
        const auto row = inductorSlopes.at(index);
        constraints.add(row, row, device.netlistInfo.value_);
        constraints.difference(row, device.indexInfo.posIndex_, device.indexInfo.negIndex_, -stepSize_);
        for (const auto& mutual : device.mutualInductances_) {
            constraints.add(row, inductorSlopes.at(mutual.first), mutual.second);
        }
        for (const auto& island : islandRows) {
            constraints.add(island.second, row,
                island_sign(device.indexInfo.posIndex_, device.indexInfo.negIndex_, island.first));
        }
    }
    for (auto index : components.vsIndices) {
        const auto& device = std::get<VoltageSource>(components.devices[index]);
        const auto row = *device.indexInfo.currentIndex_;
        constraints.difference(row, device.indexInfo.posIndex_, device.indexInfo.negIndex_, 1.0);
        constraints.rhs[row] = sources[device.sourceIndex_];
    }

    std::vector<double> state(total, 0.0);
    for (int64_t node = 0; node < nodes; ++node) { state[node] = forest.offset[node]; }
    // Junction capacitance shares the same node derivative as explicit C.
    // Only the initial piecewise resistance/gate selection may need correction.
    bool converged = false;
    for (int iteration = 0; iteration < 32; ++iteration) {
        auto system = constraints;
        for (auto index : components.junctionIndices) {
            auto& device = std::get<JJ>(components.devices[index]);
            const auto positive = device.indexInfo.posIndex_, negative = device.indexInfo.negIndex_;
            const double voltage = difference(state, positive, negative);
            if (device.model_.rtype() == 1) { device.update_value(voltage); }
            const double gateVoltage = device.gateIndex_ ? state[*device.gateIndex_] : 0.0;
            const double gate = std::clamp(1.0 - std::abs(gateVoltage) / device.model_.gwidth(), 0.0, 1.0);
            const auto criticalNoise = device.icNoise ? sample(*device.icNoise) : std::make_pair(0.0, 0.0);
            const double noise = std::max(-0.99, criticalNoise.first);
            const double scale = gate * (1.0 + noise);
            const auto row = *device.indexInfo.currentIndex_;
            system.add(device.variableIndex_, device.variableIndex_, 1.0);
            system.add(row, row, -1.0);
            system.difference(row, positive, negative, device.conductance());
            system.difference(row, positive, negative, device.model_.c() / stepSize_, count);
            system.rhs[row] = -device.supercurrent(0.0, scale) - device.it_;
            if (device.thermalNoise) {
                const auto thermal = sample(*device.thermalNoise);
                if (positive) { system.rhs[*positive] -= thermal.first; }
                if (negative) { system.rhs[*negative] += thermal.first; }
                for (const auto& island : islandRows) {
                    system.rhs[island.second] -= thermal.second * island_sign(positive, negative, island.first);
                }
            }
            for (const auto& island : islandRows) {
                const double sign = island_sign(positive, negative, island.first);
                if (sign == 0) { continue; }
                system.difference(island.second, positive, negative, sign * device.conductance(), count);
                const double derivative = (device.supercurrent(1e-6, scale) - device.supercurrent(-1e-6, scale)) / 2e-6;
                system.difference(island.second, positive, negative, sign * stepSize_ * derivative / Constants::SIGMA);
                const double gateDirection = gateVoltage != 0 ? gateVoltage
                    : (device.gateIndex_ ? state[count + *device.gateIndex_] : 0.0);
                if (device.gateIndex_ && gate > 0 && gateDirection != 0) {
                    system.add(island.second, count + *device.gateIndex_,
                        -sign * device.supercurrent(0.0, 1.0 + noise)
                        * std::copysign(1.0, gateDirection) / device.model_.gwidth());
                }
                if (criticalNoise.first > -0.99 || (criticalNoise.first == -0.99 && criticalNoise.second > 0)) {
                    system.rhs[island.second] -= sign * device.supercurrent(0.0, gate) * criticalNoise.second;
                }
            }
        }
        auto solved = system.solve();
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
        return node ? (steps * stepSize_ / Constants::SIGMA)
                          * (state[*node] + 0.5 * steps * state[count + *node]) : 0.0;
    };
    const auto phase_difference = [&](int_o positive, int_o negative, double steps) {
        return phase_at(positive, steps) - phase_at(negative, steps);
    };
    x_.assign(state.begin(), state.begin() + count);
    if (atyp_ == AnalysisType::Phase) { std::fill(x_.begin(), x_.begin() + nodes, 0.0); }
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
        x_[device.variableIndex_] = atyp_ == AnalysisType::Phase ? voltage : 0.0;
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
