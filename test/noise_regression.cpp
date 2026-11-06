// Copyright (c) 2026 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#include "JoSIM/Constants.hpp"
#include "JoSIM/Function.hpp"
#include "JoSIM/Input.hpp"
#include "JoSIM/Matrix.hpp"
#include "JoSIM/Model.hpp"
#include "JoSIM/Noise.hpp"
#include "JoSIM/Rng.hpp"
#include "JoSIM/Simulation.hpp"

#include <algorithm>
#include <cmath>
#include <complex>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

using namespace JoSIM;

static void require(bool condition, const std::string& message) {
    if (!condition) { throw std::runtime_error(message); }
}

static double spectral_density(const std::vector<double>& values, int bin,
                               double dt = 1.0 / 512.0, int window = 2048);

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

static void reseed_and_rewind() {
    for (int count : {1, 2, 3, 5}) {
        Rng::start_run(123456);
        std::vector<double> expected;
        for (int i = 0; i < count; ++i) { expected.push_back(Rng::normal01_noise()); }
        Rng::start_run(123456);
        for (double value : expected) {
            require(Rng::normal01_noise() == value, "Reseeding retained a cached Gaussian sample");
        }
        Rng::rewind();
        require(Rng::normal01_noise() == expected.front(), "Rewind did not restore the first Gaussian sample");
    }
}

static void effective_white_bandwidth() {
    auto input = source_input();
    const double h = 1.0 / 8192.0, bandwidth = 64;
    input.transSim.tstep(h);
    input.neB = bandwidth;
    const int count = 524288; // 64 s/seed; at least 8192 independent knots/seed.
    double sum = 0, squares = 0;
    const std::vector<int> bins{2, 4, 8, 64, 128, 256};
    std::vector<double> spectra(bins.size(), 0);
    for (uint64_t seed : {101, 202, 303, 404, 505, 606, 707, 808}) {
        Rng::start_run(seed);
        Function source;
        source.parse_function("NOISE(1)", input, std::nullopt);
        std::vector<double> values;
        values.reserve(count);
        for (int i = 0; i < count; ++i) {
            const double value = source.value(1.0 + i * h); // Discard the initial ramp.
            sum += value;
            squares += value * value;
            values.push_back(value);
        }
        for (size_t j = 0; j < bins.size(); ++j) { spectra[j] += spectral_density(values, bins[j], h, 16384); }
    }
    const double mean = sum / (8.0 * count), variance = squares / (8.0 * count) - mean * mean;
    std::cout << "NEB pooled variance=" << variance << " expected=" << bandwidth << '\n';
    // Linear interpolation has time-averaged variance ASD^2/(3*dt).
    // For n independent Gaussian knots, relative SE is about 1.5/sqrt(n).
    // Eight seeds give n>=65536: SE<0.006. A 5% limit is conservative;
    // h/dt<0.024 adds less than 0.03% sampling correction.
    require(std::abs(variance / bandwidth - 1) < .05, "NEB does not give the requested time-averaged white-noise power");
    require(std::abs(mean) < .2, "Pooled white-noise mean is inconsistent with zero");
    // 63 overlapping periodic-Hann segments/seed, eight independent seeds.
    // Adjacent segment power correlation is about 1/36; effective M~477,
    // giving relative SE~4.6%. A 30% bound exceeds six standard errors.
    const double dt = 1.0 / (3 * bandwidth);
    for (size_t j = 0; j < bins.size(); ++j) {
        const double frequency = bins[j] / 2.0;
        const double x = std::acos(-1.0) * frequency * dt;
        const double expected = std::pow(std::sin(x) / x, 4);
        const double ratio = spectra[j] / 8 / expected;
        std::cout << "white PSD " << frequency << " Hz / sinc^4=" << ratio << '\n';
        require(std::abs(ratio - 1) < .30, "White-noise PSD disagrees with ASD and interpolation transfer");
    }
}

static void applied_noise_amplitude() {
    auto input = source_input();
    Rng::start_run(4321);
    Function source;
    source.parse_function("NOISE(1 0 0.01)", input, std::nullopt);
    const double time = .0025, original = source.value(time);
    require(std::abs(original) > 1e-10, "Amplitude test sampled an accidental zero");
    source.ampValuesRef()[0] = 3;
    require(std::abs(source.value(time) - 3 * original) < 1e-12 * std::abs(original),
            "Changing white-noise amplitude did not scale the current interpolated sample");
    source.ampValuesRef()[0] = 0;
    require(source.value(time) == 0, "Zero noise amplitude retained an old scaled sample");
    source.ampValuesRef()[0] = 3;
    require(std::abs(source.value(time) - 3 * original) < 1e-12 * std::abs(original),
            "Restoring amplitude changed the underlying noise realization");
}

static void noise_sampling_variance() {
    auto input = source_input();
    input.transSim.tstep(1.0 / 16);
    for (int samples : {1, 2, 4, 16}) {
        double sum = 0, squares = 0;
        const int count = 8192 * samples;
        for (uint64_t seed : {101, 202, 303, 404, 505, 606, 707, 808}) {
            Rng::start_run(seed);
            Function source;
            source.parse_function("NOISE(1 0 1)", input, std::nullopt);
            for (int i = 0; i < count; ++i) {
                const double value = source.value(1 + double(i) / samples);
                sum += value;
                squares += value * value;
            }
        }
        const double mean = sum / (8.0 * count), variance = squares / (8.0 * count) - mean * mean;
        // Average of (1-u)^2+u^2 at u=j/K, j=0..K-1. Grid variance is 1/2.
        const double expected = .5 * (2.0 / 3 + 1.0 / (3 * samples * samples));
        std::cout << "white samples/interval=" << samples << " variance=" << variance << " expected=" << expected << '\n';
        require(std::abs(variance / expected - 1) < .05, "White-noise sampling-phase variance disagrees with interpolation");
        require(std::abs(mean) < .03, "Sampled white noise has a nonzero pooled mean");
    }
}

static void thermal_resistive_states() {
    Input input(AnalysisType::Voltage, 0, true);
    input.transSim.tstep(1e-13);
    input.transSim.tstop(1e-9);
    Model model;
    model.modelName("JM"); model.ic(1e-4); model.r0(30); model.rn(5); model.c(1e-13);
    input.netlist.models_new.emplace_back(model, std::nullopt);
    input.netlist.expNetlist = {{{"V1", "N", "0", "DC", "0"}, std::nullopt},
                               {{"B1", "N", "0", "JM", "TEMP=4.2", "NEB=1G"}, std::nullopt}};
    Rng::start_run(9876);
    Matrix matrix;
    matrix.create_matrix(input);
    auto& jj = std::get<JJ>(matrix.components.devices.at(matrix.components.junctionIndices.front()));
    auto& noise = *jj.thermalNoise;
    const double time = 1e-10;
    const double unit = noise.value(time) / noise.ampValues().front();
    const double transitionG = 1e-4 / ((std::acos(-1.0) / 4) * .1e-3);
    for (double voltage : {0., 2.8e-3, 5.6e-3, 2.8e-3, 0., -2.8e-3, -5.6e-3, -2.8e-3, 0.}) {
        jj.update_value(voltage);
        const double conductance = std::abs(voltage) < 2.75e-3 ? 1.0 / 30
                                   : std::abs(voltage) < 2.85e-3 ? transitionG : 1.0 / 5;
        // Classical local differential-conductance FDT approximation. This is
        // a test of that stated model, not a nonequilibrium quasiparticle theory.
        const double expected = std::sqrt(4 * 1.380649e-23 * 4.2 * conductance);
        require(std::abs(noise.ampValues().front() / expected - 1) < 1e-6,
                "Thermal amplitude depends on the previous state instead of active conductance");
        require(jj.spAmp_ && *jj.spAmp_ == noise.ampValues().front(), "Stored JJ amplitude differs from its applied source");
        require(std::abs(noise.value(time) - expected * unit) < 1e-6 * std::abs(expected * unit) + 1e-20,
                "Thermal source retained samples scaled by an earlier resistance");
    }
}

static double spectral_density(const std::vector<double>& values, int bin, double dt, int window) {
    const int    hop    = window / 2;
    double       power = 0.0, weight = 0.0;
    int          segments = 0;
    std::vector<std::complex<double>> factors;
    double norm = 0;
    for (int j = 0; j < window; ++j) {
        const double w = .5 - .5 * std::cos(2 * Constants::PI * j / window);
        const double angle = -2 * Constants::PI * bin * j / window;
        factors.emplace_back(w * std::cos(angle), w * std::sin(angle));
        norm += w * w;
    }
    for (int start = 0; start + window <= values.size(); start += hop) {
        std::complex<double> sum;
        double               mean = 0.0;
        for (int j = 0; j < window; ++j) { mean += values[start + j]; }
        mean        /= window;
        for (int j = 0; j < window; ++j) {
            sum += (values[start + j] - mean) * factors[j];
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
    const double infinity = std::numeric_limits<double>::infinity();
    const double nan = std::numeric_limits<double>::quiet_NaN();
    for (const auto values : {std::pair<double, double>{0, 1}, {-1, 1}, {infinity, 1},
                             {nan, 1}, {1, 0}, {1, -1}, {1, infinity}, {1, nan},
                             {std::numeric_limits<double>::denorm_min(), 1}}) {
        bool rejected = false;
        try { Noise::determine_sampling_interval(values.first, values.second); }
        catch (const std::runtime_error&) { rejected = true; }
        require(rejected, "Invalid or unrepresentable thermal sampling interval accepted");
    }
    require(Noise::determine_sampling_interval(1e12, 1e-12) == 1e-12,
            "Thermal sampling interval did not retain the simulation-step floor");
    require(Noise::determine_spectral_amplitude(-1, 0) == 0,
            "Zero temperature did not disable classical thermal noise");
    require(Noise::determine_spectral_amplitude(infinity, 4.2) == 0,
            "Zero conductance produced nonzero thermal noise");
    for (const auto values : {std::pair<double, double>{0, 4.2}, {-1, 4.2}, {nan, 0},
                             {1, -1}, {1, infinity}, {1, nan}}) {
        bool rejected = false;
        try { Noise::determine_spectral_amplitude(values.first, values.second); }
        catch (const std::runtime_error&) { rejected = true; }
        require(rejected, "Invalid thermal spectral amplitude accepted");
    }
    // A large requested NEB must use the same forward stream as explicit TSTEP=h.
    // Direct API callers historically use neB=0 for omitted-TSTEP fallback to h.
    for (double bandwidth : {0., 1e12}) {
        auto floored = source_input();
        floored.neB = bandwidth;
        Rng::start_run(8844);
        Function implicit;
        implicit.parse_function("NOISE(1)", floored, std::nullopt);
        std::vector<double> expected;
        for (int i = 0; i < 100; ++i) { expected.push_back(implicit.value(i / 512.0)); }
        Rng::start_run(8844);
        Function explicit_step;
        explicit_step.parse_function("NOISE(1 0 0.001953125)", floored, std::nullopt);
        for (int i = 0; i < 100; ++i) {
            require(explicit_step.value(i / 512.0) == expected[i], "Floored noise interval changed seeded samples");
        }
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

static void phase_source_samples() {
    for (const std::string waveform : {"SIN(0.2,0.1,50G,0,0,30)",
                                       "PWL(0,0.2,1P,0.3,3P,-0.1,20P,0.2)",
                                       "NOISE(1P,0,1P)", "PNOISE(0.001,1,0,1P,100P,16)",
                                       "FNOISE(0.001,1,0.05,0,1P,100P,16)"}) {
        for (bool startup : {false, true}) {
            for (auto mode : {AnalysisType::Voltage, AnalysisType::Phase}) {
                Input input(mode, 0, true);
                const double h = 1e-13;
                input.transSim.tstep(h);
                input.transSim.tstop(20e-12);
                input.transSim.startup(startup);
                input.netlist.expNetlist = {{{"P1", "N", "0", waveform}, std::nullopt},
                                           {{"R1", "N", "0", "10"}, std::nullopt}};
                Rng::start_run(123);
                Matrix matrix;
                matrix.create_matrix(input);
                Simulation simulation(input, matrix);
                require(input.transSim.tstep() == h, "Phase-source reference unexpectedly retried");
                const auto node = matrix.nm.at("N");
                const auto& resistor = std::get<Resistor>(matrix.components.devices.at(matrix.components.resistorIndices.front()));
                const auto& currents = *simulation.results.xVector.at(*resistor.indexInfo.currentIndex_);
                const auto& nodes = *simulation.results.xVector.at(node);
                Rng::start_run(123);
                Function reference;
                reference.parse_function(waveform, input, std::nullopt);
                double previous = 0, older = 0;
                double integrated = mode == AnalysisType::Voltage ? simulation.results.phaseBefore.at(node)[0] : 0;
                double integrated_older = mode == AnalysisType::Voltage ? simulation.results.phaseBefore.at(node)[1] : 0;
                // At 0.1 ps the bounded warmup has 1000 steps. Sample the
                // standalone generator only in time order, including warmup.
                for (int64_t i = startup ? -1000 : 0; i < static_cast<int64_t>(nodes.size()); ++i) {
                    const double phase = reference.value(i * h);
                    if (i >= 0) {
                        // Josephson voltage relation under the documented BDF2
                        // differentiation, compared against actual source samples.
                        const double voltage = Constants::SIGMA / h * (1.5 * phase - 2 * previous + .5 * older);
                        require(std::isfinite(currents[i]) && std::abs(currents[i] - voltage / 10) < 1e-15,
                                "Phase source did not apply its actual preceding generator samples: " + waveform);
                        double observed = nodes[i];
                        if (mode == AnalysisType::Voltage) {
                            observed = (2 * h / (3 * Constants::SIGMA)) * nodes[i]
                                       + (4.0 / 3.0) * integrated - (1.0 / 3.0) * integrated_older;
                            integrated_older = integrated;
                            integrated = observed;
                        }
                        require(std::isfinite(observed) && std::abs(observed - phase) < 1e-11,
                                "Phase source/integrated node lost prescribed phase: " + waveform);
                    }
                    older = previous;
                    previous = phase;
                }
            }
        }
    }
}

static void thermal_injection() {
    const std::vector<double> times{0, 10, 12, 20, 22, 30, 32, 40, 42, 50, 52, 60, 62, 70, 72, 80, 82, 100};
    const std::vector<double> volts{0, 0, .0028, .0028, .0056, .0056, .0028, .0028, 0, 0,
                                    -.0028, -.0028, -.0056, -.0056, -.0028, -.0028, 0, 0};
    const auto voltage = [&](double t) {
        const double ps = t * 1e12;
        if (ps <= 0) { return 0.0; }
        for (size_t i = 1; i < times.size(); ++i) {
            if (ps <= times[i]) {
                const double u = (ps - times[i - 1]) / (times[i] - times[i - 1]);
                return volts[i - 1] + u * (volts[i] - volts[i - 1]);
            }
        }
        return 0.0;
    };
    const std::string drive = "PWL(0,0,10P,0,12P,2.8M,20P,2.8M,22P,5.6M,30P,5.6M,32P,2.8M,40P,2.8M,42P,0,50P,0,52P,-2.8M,60P,-2.8M,62P,-5.6M,70P,-5.6M,72P,-2.8M,80P,-2.8M,82P,0,100P,0)";
    const double h = 1e-14, interval = 1.0 / (3 * 1e10);
    for (auto mode : {AnalysisType::Voltage, AnalysisType::Phase}) {
        for (int terminals : {0, 1, 2}) {
            Input input(mode, 0, true);
            input.transSim.tstep(h); input.transSim.tstop(1e-10);
            Model model;
            model.modelName("JM"); model.ic(1e-4); model.r0(30); model.rn(5); model.c(1e-13);
            input.netlist.models_new.emplace_back(model, std::nullopt);
            const std::string positive = terminals == 1 ? "0" : "A";
            const std::string negative = terminals == 0 ? "0" : "B";
            input.netlist.expNetlist = {{{"V1", positive, negative, drive}, std::nullopt},
                                       {{"B1", positive, negative, "JM", "TEMP=4.2", "NEB=10G"}, std::nullopt}};
            if (terminals == 2) { input.netlist.expNetlist.push_back({{"RREF", "B", "0", "1"}, std::nullopt}); }
            input.controls = {{"PRINT", "I(V1)", "I(B1)"}};
            Rng::start_run(1122);
            Matrix matrix;
            matrix.create_matrix(input);
            find_relevant_traces(input, matrix);
            Simulation simulation(input, matrix);
            require(input.transSim.tstep() == h, "Thermal protocol unexpectedly retried");
            const auto& jj = std::get<JJ>(matrix.components.devices.at(matrix.components.junctionIndices.front()));
            const auto& source = std::get<VoltageSource>(matrix.components.devices.at(matrix.components.vsIndices.front()));
            const auto& supplied = *simulation.results.xVector.at(*source.indexInfo.currentIndex_);
            const auto& branch = *simulation.results.xVector.at(*jj.indexInfo.currentIndex_);
            Rng::start_run(1122);
            Function unit;
            unit.parse_function("NOISE(1,0," + Misc::precise_to_string(interval) + ")", input, std::nullopt);
            double maximum = 0;
            for (size_t i = 0; i < supplied.size(); ++i) {
                const double t = simulation.results.timeAxis[i];
                // The documented explicit voltage predictor chooses this step's
                // linearized conductance. The source itself is prescribed above.
                const double predicted = 2.5 * voltage(t - h) - 2 * voltage(t - 2 * h) + .5 * voltage(t - 3 * h);
                const double conductance = std::abs(predicted) < .00275 ? 1.0 / 30
                    : std::abs(predicted) < .00285 ? 1e-4 / ((std::acos(-1.0) / 4) * .1e-3) : 1.0 / 5;
                const double expected = std::sqrt(4 * 1.380649e-23 * 4.2 * conductance) * unit.value(t);
                // The Norton source is a separate KCL injection; I(B1) is the
                // constitutive branch, so -(I(V1)+I(B1)) recovers its noise.
                const double observed = -(supplied[i] + branch[i]);
                maximum = std::max(maximum, std::abs(observed - expected));
                require(std::abs(observed - expected) < 1e-6 * std::abs(expected) + 1e-14,
                        "Applied JJ thermal sample does not match this step's conductance or terminal orientation");
            }
            std::cout << "thermal mode=" << int(mode) << " terminals=" << terminals << " max error=" << maximum << '\n';
        }
    }
}

static void full_cycle_step_guard(bool thermal) {
    Input input(AnalysisType::Voltage, 0, true);
    input.transSim.tstep(2.0 * Constants::PI * Constants::SIGMA / 1e-3);
    input.transSim.prstep(input.transSim.tstep());
    input.transSim.tstop(1000.0 * input.transSim.tstep());
    input.transSim.startup(false);
    Model model;
    model.modelName("JM");
    model.rtype(0);
    input.netlist.models_new.emplace_back(model, std::nullopt);
    input.netlist.expNetlist = {{{"V1", "N", "0", "DC", "1M"}, std::nullopt},
                               {{"B1", "N", "0", "JM"}, std::nullopt},
                               {{"I1", "0", "R", "NOISE(1P", "0", "1P)"}, std::nullopt},
                               {{"R1", "R", "0", "10"}, std::nullopt},
                               {{"P1", "PH", "0", "FNOISE(0.001,1,0.05,0,1P,100P,16)"}, std::nullopt},
                               {{"RP", "PH", "0", "10"}, std::nullopt}};
    if (thermal) {
        input.netlist.expNetlist.at(1).first.insert(input.netlist.expNetlist.at(1).first.end(), {"TEMP=4.2", "NEB=1T"});
    }
    input.controls = {{"PRINT", "I(I1)", "I(R1)", "P(PH)", "I(RP)"}};
    const double requested   = input.transSim.tstep();
    Rng::start_run(7654);
    Matrix       matrix;
    matrix.create_matrix(input);
    find_relevant_traces(input, matrix);
    Simulation simulation(input, matrix);
    require(input.transSim.tstep() < requested / 2.0, "A full Josephson cycle bypassed timestep reduction");
    const auto source_index = matrix.components.currentsources.front().sourceIndex_;
    const auto& captured = simulation.results.sourceCurrents.at(source_index).value();
    require(captured.size() == simulation.results.timeAxis.size(), "Retry retained samples from an abandoned run");
    const auto resistor_index = std::get<Resistor>(matrix.components.devices.at(matrix.components.resistorIndices.front()))
                                  .indexInfo.currentIndex_.value();
    const auto& resistor = simulation.results.xVector.at(resistor_index).value();
    for (size_t i = 0; i < captured.size(); ++i) {
        require(std::abs(captured[i] - resistor[i]) <= 1e-12 * std::abs(captured[i]) + 1e-20,
                "Captured retry source violates resistor KCL");
    }
    // Same-process explicit final-step run must replay the restarted stream.
    Input fine = input;
    Rng::start_run(7654);
    Matrix fine_matrix;
    fine_matrix.create_matrix(fine);
    find_relevant_traces(fine, fine_matrix);
    Simulation fine_simulation(fine, fine_matrix);
    require(simulation.results.timeAxis == fine_simulation.results.timeAxis, "Retry changed the final time grid");
    const auto fine_index = fine_matrix.components.currentsources.front().sourceIndex_;
    require(captured == fine_simulation.results.sourceCurrents.at(fine_index).value(),
            "Same-process final-step run did not replay the retry noise samples");
    require(simulation.results.xVector == fine_simulation.results.xVector
            && simulation.results.phaseBefore == fine_simulation.results.phaseBefore,
            "Phase-source history or noise realization changed after timestep retry");
}

int main(int argc, char** argv) {
    try {
        std::cout << std::setprecision(17);
        const std::string selection = argc == 1 ? "all" : argv[1];
        require(argc <= 2 && (selection == "all" || selection == "neb" || selection == "amplitude" || selection == "thermal"),
                "Unknown noise test selection");
        if (selection == "all" || selection == "neb") { effective_white_bandwidth(); noise_sampling_variance(); }
        if (selection == "all" || selection == "amplitude") { applied_noise_amplitude(); }
        if (selection == "all" || selection == "thermal") { thermal_resistive_states(); thermal_injection(); }
        if (selection != "all") { return 0; }
        white_noise();
        reseed_and_rewind();
        power_law_spectrum();
        flux_units_and_delay();
        normalization_outside_band();
        fractional_critical_current();
        invalid_noise_parameters();
        flux_through_inductor();
        phase_source_samples();
        full_cycle_step_guard(false);
        full_cycle_step_guard(true);
        std::cout << "Noise and phase-step regressions passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
