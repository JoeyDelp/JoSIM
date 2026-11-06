// Copyright (c) 2026 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#include "JoSIM/Constants.hpp"
#include "JoSIM/Function.hpp"
#include "JoSIM/Input.hpp"
#include "JoSIM/Matrix.hpp"
#include "JoSIM/Model.hpp"
#include "JoSIM/Rng.hpp"
#include "JoSIM/Simulation.hpp"

#include <cmath>
#include <complex>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace JoSIM;

static void require(bool condition, const std::string& message) {
    if (!condition) { throw std::runtime_error(message); }
}

static Input source_input() {
    Input input(AnalysisType::Phase, 0, true);
    input.transSim.tstep(1.0 / 512.0);
    input.transSim.tstop(128.0);
    return input;
}

static void white_noise() {
    auto input = source_input();
    Rng::start_run(12345);
    Function noise;
    noise.parse_function("NOISE(1 0 0.01)", input, std::nullopt);
    const int count = 100000;
    double    mean = 0.0, squares = 0.0;
    for (int i = 0; i < count; ++i) {
        const double value = noise.value(i * 0.01);
        require(std::isfinite(value), "White noise contains a non-finite sample");
        mean    += value;
        squares += value * value;
    }
    mean                  /= count;
    const double variance  = squares / count - mean * mean;
    require(std::abs(variance / 50.0 - 1.0) < 0.03, "White noise grid variance differs from A^2/(2*dt)");
    require(std::abs(mean) < 0.15, "White noise mean is inconsistent with zero");
    std::cout << "white variance=" << variance << " expected=50\n";
}

static double spectral_density(const std::vector<double>& values, int bin) {
    const int    window = 2048;
    const int    hop    = window / 2;
    const double dt     = 1.0 / 512.0;
    double       power = 0.0, weight = 0.0;
    int          segments = 0;
    for (int start = 0; start + window <= values.size(); start += hop) {
        std::complex<double> sum;
        double               mean = 0.0;
        for (int j = 0; j < window; ++j) { mean += values[start + j]; }
        mean        /= window;
        double norm  = 0.0;
        for (int j = 0; j < window; ++j) {
            const double w     = 0.5 - 0.5 * std::cos(2.0 * Constants::PI * j / window);
            const double angle = -2.0 * Constants::PI * bin * j / window;
            sum  += w * (values[start + j] - mean) * std::complex<double>(std::cos(angle), std::sin(angle));
            norm += w * w;
        }
        power  += std::norm(sum);
        weight += norm;
        ++segments;
    }
    require(segments > 20, "Insufficient Welch segments");
    return 2.0 * dt * power / weight;
}

static void power_law_spectrum() {
    auto input = source_input();
    Rng::start_run(24680);
    Function noise;
    noise.parse_function("PNOISE(1 1 0 0.001953125 100 64)", input, std::nullopt);
    std::vector<double> values;
    for (int i = 0; i < 65536; ++i) { values.push_back(noise.value(i / 512.0)); }
    for (int bin : {8, 16, 32, 64}) {
        const double frequency = bin / 4.0;
        const double ratio     = spectral_density(values, bin) * frequency;
        std::cout << "PNOISE PSD at " << frequency << "Hz / (1/f)=" << ratio << '\n';
        require(ratio > 0.65 && ratio < 1.35, "Sampled PNOISE spectrum differs from its 1/f normalization");
    }
}

static void flux_units_and_delay() {
    auto     input = source_input();
    Function flux, phase;
    Rng::start_run(13579);
    flux.parse_function("FNOISE(0.001 1 0.25 0.1 0.001953125 100 64)", input, std::nullopt);
    std::vector<double> values;
    for (int i = 0; i < 1024; ++i) { values.push_back(flux.value(i / 512.0)); }
    Rng::start_run(13579);
    phase.parse_function("PNOISE(0.001 1 0.1 0.001953125 100 64)", input, std::nullopt);
    for (int i = 0; i < values.size(); ++i) {
        const double expected = 2.0 * Constants::PI * (0.25 + phase.value(i / 512.0));
        require(std::abs(values[i] - expected) < 1e-11, "FNOISE flux-to-phase mapping differs from 2*pi");
    }
}

static void normalization_outside_band() {
    auto     input = source_input();
    Function slow, fast;
    Rng::start_run(9876);
    slow.parse_function("PNOISE(1 1 0 0.001953125 1 64)", input, std::nullopt);
    std::vector<double> expected;
    for (int i = 0; i < 1024; ++i) { expected.push_back(slow.value(i / 512.0)); }
    input.transSim.tstep(input.transSim.tstep() * 1e-6);
    input.transSim.tstop(input.transSim.tstop() * 1e-6);
    Rng::start_run(9876);
    fast.parse_function("PNOISE(1 1 0 1.953125E-9 1E-6 64)", input, std::nullopt);
    // Shifting a 1/f band preserves variance and the normalized realization.
    for (int i = 0; i < expected.size(); ++i) {
        require(std::abs(fast.value(i / 512.0 * 1e-6) - expected[i]) < 1e-8,
                "1 Hz normalization changed when the entire noise band moved above 1 Hz");
    }
}

static void fractional_critical_current() {
    Input input(AnalysisType::Phase, 0, true);
    input.transSim.tstep(1.0 / 512.0);
    input.transSim.tstop(1.0);
    input.transSim.startup(false);
    Model model;
    model.modelName("JM");
    model.rtype(0);
    model.gwidth(0.01);
    input.netlist.models_new.emplace_back(model, std::nullopt);
    input.netlist.expNetlist
            = {{{"P1", "N", "0", "DC", "1.5707963267948966"}, std::nullopt},
               {{"VG", "G", "0", "DC", "0.005"}, std::nullopt},
               {{"B1", "N", "0", "JM", "GC=G", "ICNOISE=PNOISE(0.001,1,0,0.001953125,100,64)"}, std::nullopt}};
    Rng::start_run(445566);
    Matrix matrix;
    matrix.create_matrix(input);
    Simulation  simulation(input, matrix);
    const auto& junction = std::get<JJ>(matrix.components.devices.at(matrix.components.junctionIndices.at(0)));
    const auto& currents = simulation.results.xVector.at(junction.indexInfo.currentIndex_.value()).value();
    Rng::start_run(445566);
    Function reference;
    reference.parse_function("PNOISE(0.001 1 0 0.001953125 100 64)", input, std::nullopt);
    for (int i = 0; i < currents.size(); ++i) {
        const double fraction = reference.value(i / 512.0);
        if (i < 20) { continue; }
        const double expected = 0.5 * model.ic() * (1.0 + fraction);
        require(std::abs(currents[i] - expected) < 1e-10,
                "ICNOISE does not scale the gated Josephson current fractionally");
    }
}

static void invalid_noise_parameters() {
    const auto input = source_input();
    for (const std::string expression : {"NOISE(1 0 0)",
                                         "NOISE(1 -1 0.01)",
                                         "PNOISE(1 3 0 0.01 100 64)",
                                         "PNOISE(1 1 0 0.01 -1 64)",
                                         "PNOISE(1 1 0 0.01 100 4.5)",
                                         "FNOISE(1 1 0 0 0.01 100 1000)"}) {
        bool rejected = false;
        try {
            Function noise;
            noise.parse_function(expression, input, std::nullopt);
        } catch (const std::runtime_error&) { rejected = true; }
        require(rejected, "Invalid noise source accepted: " + expression);
    }
}

static void flux_through_inductor() {
    Input input(AnalysisType::Phase, 0, true);
    input.transSim.tstep(1.0 / 512.0);
    input.transSim.tstop(1.0);
    input.transSim.startup(false);
    input.netlist.expNetlist = {{{"P1", "N", "0", "FNOISE(0.001,1,0.25,0,0.001953125,100,64)"}, std::nullopt},
                                {{"L1", "N", "0", "1N"}, std::nullopt}};
    Rng::start_run(112233);
    Matrix matrix;
    matrix.create_matrix(input);
    Simulation  simulation(input, matrix);
    const auto& inductor = std::get<Inductor>(matrix.components.devices.at(matrix.components.inductorIndices.at(0)));
    const auto& currents = simulation.results.xVector.at(inductor.indexInfo.currentIndex_.value()).value();
    Rng::start_run(112233);
    Function reference;
    reference.parse_function("FNOISE(0.001 1 0.25 0 0.001953125 100 64)", input, std::nullopt);
    for (int i = 0; i < currents.size(); ++i) {
        const double expected = Constants::SIGMA / 1e-9 * reference.value(i / 512.0);
        require(std::abs(currents[i] - expected) < 1e-14, "Flux-source inductor current violates flux = L*I");
    }
}

static void full_cycle_step_guard() {
    Input input(AnalysisType::Voltage, 0, true);
    input.transSim.tstep(2.0 * Constants::PI * Constants::SIGMA / 1e-3);
    input.transSim.prstep(input.transSim.tstep());
    input.transSim.tstop(1000.0 * input.transSim.tstep());
    input.transSim.startup(false);
    Model model;
    model.modelName("JM");
    model.rtype(0);
    input.netlist.models_new.emplace_back(model, std::nullopt);
    input.netlist.expNetlist = {{{"V1", "N", "0", "DC", "1M"}, std::nullopt}, {{"B1", "N", "0", "JM"}, std::nullopt}};
    const double requested   = input.transSim.tstep();
    Matrix       matrix;
    matrix.create_matrix(input);
    Simulation simulation(input, matrix);
    require(input.transSim.tstep() < requested / 2.0, "A full Josephson cycle bypassed timestep reduction");
}

int main() {
    try {
        white_noise();
        power_law_spectrum();
        flux_units_and_delay();
        normalization_outside_band();
        fractional_critical_current();
        invalid_noise_parameters();
        flux_through_inductor();
        full_cycle_step_guard();
        std::cout << "Noise and phase-step regressions passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
