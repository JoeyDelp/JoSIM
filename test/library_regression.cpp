// Copyright (c) 2026 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#include <JoSIM/IV.hpp>
#include <JoSIM/Input.hpp>
#include <JoSIM/Matrix.hpp>
#include <JoSIM/Rng.hpp>
#include <JoSIM/Simulation.hpp>

#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
JoSIM::Input input() {
    JoSIM::Input result(JoSIM::AnalysisType::Voltage, 0, true);
    result.transSim.tstep(1e-12);
    result.transSim.tstop(5e-12);
    result.transSim.startup(false);
    return result;
}

void must_throw(const std::function<void()>& operation, const std::string& diagnostic) {
    try {
        operation();
    } catch (const std::runtime_error& error) {
        if (std::string(error.what()).find(diagnostic) != std::string::npos) { return; }
        throw std::runtime_error("Wrong diagnostic: " + std::string(error.what()));
    }
    throw std::runtime_error("Expected a recoverable error: " + diagnostic);
}

void valid_simulation() {
    auto run = input();
    run.netlist.expNetlist = {{{"V1", "N", "0", "DC", "1"}, std::nullopt},
                             {{"R1", "N", "0", "1K"}, std::nullopt}};
    JoSIM::Matrix matrix;
    matrix.create_matrix(run);
    JoSIM::Simulation simulation(run, matrix);
    const double voltage = simulation.results.xVector.at(matrix.nm.at("N")).value().back();
    if (!std::isfinite(voltage) || std::abs(voltage - 1.0) > 1e-12) {
        throw std::runtime_error("A simulation failed after recovering from an error");
    }
}

void missing_model() {
    auto run = input();
    run.controls = {{"IV", "MISSING", "100U", "unused-iv-output.csv"}};
    must_throw([&] { JoSIM::IV iv(run); }, "requested model was not found");
}

void singular_matrix() {
    auto run = input();
    // A resistor with neither terminal grounded has an undetermined common mode.
    run.netlist.expNetlist = {{{"R1", "A", "B", "1K"}, std::nullopt}};
    JoSIM::Matrix matrix;
    matrix.create_matrix(run);
    must_throw([&] { JoSIM::Simulation simulation(run, matrix); }, "Matrix is singular");
}

void invalid_matrix() {
    auto run = input();
    JoSIM::Matrix matrix;
    must_throw([&] { JoSIM::Simulation simulation(run, matrix); }, "Invalid sparse matrix");
    matrix.rp = {0, 2};
    matrix.ci = {0};
    matrix.nz = {1};
    matrix.branchIndex = 1;
    must_throw([&] { JoSIM::Simulation simulation(run, matrix); }, "Invalid sparse matrix");
}
} // namespace

int main(int argc, char** argv) {
    try {
        JoSIM::Rng::start_run(123);
        const std::string selected = argc > 1 ? argv[1] : "all";
        // Repeated failures exercise solver cleanup; sanitizers check allocations.
        for (int i = 0; i < 8; ++i) {
            if (selected == "all" || selected == "missing") { missing_model(); }
            if (selected == "all" || selected == "singular") { singular_matrix(); }
            if (selected == "all" || selected == "invalid") { invalid_matrix(); }
            valid_simulation();
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
