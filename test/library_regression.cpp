// Copyright (c) 2026 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#include <JoSIM/IV.hpp>
#include <JoSIM/Input.hpp>
#include <JoSIM/Matrix.hpp>
#include <JoSIM/Output.hpp>
#include <JoSIM/Rng.hpp>
#include <JoSIM/Simulation.hpp>

#include <cmath>
#include <functional>
#include <iostream>
#include <limits>
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

void cli_values() {
    const char* argv[] = {"josim", "--minimal=1", "--analysis=0", "--verbose=2", "example.cir"};
    auto options = JoSIM::CliOptions::parse(5, argv);
    if (!options.minimal || options.analysis_type != JoSIM::AnalysisType::Voltage || options.verbose != 2
        || options.cir_file_name.value_or("") != "example.cir") {
        throw std::runtime_error("CLI long-option values were lost");
    }
}

void invalid_transient() {
    for (double step : {0.0, -1.0, std::numeric_limits<double>::infinity(),
                        std::numeric_limits<double>::quiet_NaN()}) {
        auto run = input();
        run.transSim.tstep(step);
        must_throw([&] { volatile auto count = run.transSim.simsize(); (void)count; }, "Invalid transient analysis");
    }
    for (double stop : {0.0, -1.0, std::numeric_limits<double>::infinity(),
                        std::numeric_limits<double>::quiet_NaN(), 1e200}) {
        auto run = input();
        run.transSim.tstop(stop);
        must_throw([&] { volatile auto count = run.transSim.simsize(); (void)count; }, "Invalid transient analysis");
    }
    auto run = input();
    run.transSim.prstart(4.9e-12);
    must_throw([&] { run.transSim.validate_output(); }, "Print start is after the last simulation sample");
    run.transSim.prstart(0.0);
    run.transSim.prstep(0.0);
    must_throw([&] { run.transSim.validate_output(); }, "Print step");
    run.netlist.expNetlist = {{{"R1", "N", "0", "1K"}, std::nullopt}};
    run.transSim.tstep(0.0);
    JoSIM::Matrix matrix;
    must_throw([&] { matrix.create_matrix(run); }, "Invalid transient analysis");
}

void unsupported_capacitor_ic() {
    auto run = input();
    run.netlist.expNetlist = {{{"R1", "N", "0", "10"}, std::nullopt},
                             {{"C1", "N", "0", "1P", "IC=1M"}, std::nullopt}};
    must_throw([&] { JoSIM::Matrix matrix; matrix.create_matrix(run); }, "Capacitor IC is not supported");
}

void invalid_models() {
    const auto rejects = [](JoSIM::Model model, const std::string& diagnostic, const JoSIM::tokens_t& attributes = {}) {
        auto run = input();
        model.modelName("JM");
        run.netlist.models_new.emplace_back(model, std::nullopt);
        JoSIM::tokens_t junction{"B1", "N", "0", "JM"};
        junction.insert(junction.end(), attributes.begin(), attributes.end());
        run.netlist.expNetlist = {{{"V1", "N", "0", "DC", "0"}, std::nullopt}, {junction, std::nullopt}};
        must_throw([&] { JoSIM::Matrix matrix; matrix.create_matrix(run); }, diagnostic);
    };
    JoSIM::Model model;
    model.rtype(2); rejects(model, "RTYPE");
    model = {}; model.d(2); rejects(model, "D must");
    model = {}; model.c(-1e-12); rejects(model, "CAP");
    model = {}; model.rn(std::numeric_limits<double>::infinity()); rejects(model, "RN");
    model = {}; model.cpr({std::numeric_limits<double>::quiet_NaN()}); rejects(model, "CPR");
    model = {}; rejects(model, "AREA", {"AREA=0"});
    model = {}; model.ic(0); rejects(model, "IC override", {"IC=1U"});
    model = {}; model.ic(1e308); rejects(model, "IC must be finite", {"AREA=1E308"});
}

void unrequested_source_history() {
    auto run = input();
    run.netlist.expNetlist = {{{"I1", "0", "N", "NOISE(1P", "0", "1P)"}, std::nullopt},
                             {{"R1", "N", "0", "10"}, std::nullopt}};
    JoSIM::Matrix matrix;
    matrix.create_matrix(run);
    JoSIM::Simulation simulation(run, matrix);
    if (!simulation.results.sourceCurrents.empty()) {
        throw std::runtime_error("Unrequested source histories were retained");
    }
}

void default_phase_history_for_postprocessing() {
    auto run = input();
    run.argAnal = JoSIM::AnalysisType::Phase;
    run.transSim.startup(true);
    run.netlist.expNetlist = {{{"V1", "N", "0", "DC", "1"}, std::nullopt},
                             {{"R1", "N", "0", "10"}, std::nullopt}};
    JoSIM::Matrix matrix;
    matrix.create_matrix(run);
    JoSIM::Simulation simulation(run, matrix);
    run.controls = {{"PRINT", "V(N)"}};
    JoSIM::find_relevant_traces(run, matrix);
    JoSIM::Output output;
    output.write_output(run, matrix, simulation);
    for (double voltage : output.traces.at(1).data_) {
        if (std::abs(voltage - 1.0) > 1e-10) {
            throw std::runtime_error("Default results lost the phase history required for postprocessing");
        }
    }
}

void default_voltage_history_for_postprocessing() {
    auto run = input();
    run.transSim.startup(true);
    run.netlist.expNetlist = {{{"V1", "N", "0", "DC", "1"}, std::nullopt},
                             {{"R1", "N", "0", "10"}, std::nullopt}};
    JoSIM::Matrix matrix;
    matrix.create_matrix(run);
    JoSIM::Simulation simulation(run, matrix);
    run.controls = {{"PRINT", "P(N)"}};
    JoSIM::find_relevant_traces(run, matrix);
    JoSIM::Output output;
    output.write_output(run, matrix, simulation);
    run.argAnal = JoSIM::AnalysisType::Phase;
    JoSIM::Matrix phase_matrix;
    phase_matrix.create_matrix(run);
    JoSIM::Simulation phase_simulation(run, phase_matrix);
    const auto& reference = *phase_simulation.results.xVector.at(phase_matrix.nm.at("N"));
    if (output.traces.at(1).data_.size() != reference.size()) {
        throw std::runtime_error("Deferred phase output changed its time grid");
    }
    for (size_t i = 0; i < reference.size(); ++i) {
        if (std::abs(output.traces.at(1).data_[i] - reference[i]) > 1e-10 * std::abs(reference[i])) {
            throw std::runtime_error("Default voltage results lost startup phase for deferred output");
        }
    }
    auto selected = input();
    selected.netlist.expNetlist = run.netlist.expNetlist;
    selected.controls = {{"PRINT", "V(N)"}};
    JoSIM::Matrix selected_matrix;
    selected_matrix.create_matrix(selected);
    JoSIM::find_relevant_traces(selected, selected_matrix);
    JoSIM::Simulation selected_simulation(selected, selected_matrix);
    if (!selected_simulation.results.phaseBefore.empty()) {
        throw std::runtime_error("Unrequested converted phase history was retained");
    }
}

void scoped_and_legacy_expansion() {
    for (bool scoped : {false, true}) {
        auto run = input();
        run.netlist.argMin = true;
        run.netlist.maindesign = {{"X1", "CELL", "N", "0"}, {"I1", "0", "N", "DC", "1"}};
        auto& cell = run.netlist.subcircuits["CELL"];
        cell.io = {"IN", "RET"};
        cell.lines = {{{"R1", "IN", "RET", "RES"}, "CELL"}};
        JoSIM::create_parameter({".PARAM", scoped ? "RES=5" : "RES=12"}, run.parameters, "CELL");
        if (scoped) {
            JoSIM::create_parameter({".PARAM", "RES=6"}, run.parameters);
            run.netlist.maindesign[0].push_back("RES={RES*2}");
            run.netlist.expand_maindesign(run.parameters);
            JoSIM::update_parameters(run.parameters);
        }
        else {
            JoSIM::parse_parameters(run.parameters);
            run.netlist.expand_subcircuits();
            run.netlist.expand_maindesign();
        }
        JoSIM::Matrix matrix;
        matrix.create_matrix(run);
        JoSIM::Simulation simulation(run, matrix);
        for (double voltage : *simulation.results.xVector.at(matrix.nm.at("N"))) {
            if (std::abs(voltage - 12) > 1e-11) { throw std::runtime_error("Library expansion lost local parameters"); }
        }
    }
    auto run = input();
    run.netlist.argMin = true;
    run.netlist.maindesign = {{"X1", "CELL", "N", "0"}, {"I1", "0", "N", "DC", "1"}};
    auto& cell = run.netlist.subcircuits["CELL"];
    cell.io = {"IN", "RET"};
    cell.lines = {{{"R1", "IN", "RET", "RES"}, "CELL"}};
    JoSIM::create_parameter({".PARAM", "RES=UNKNOWN"}, run.parameters, "CELL");
    must_throw([&] { run.netlist.expand_maindesign(run.parameters); }, "RES |X1");
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
            if (selected == "all" || selected == "cli") { cli_values(); }
            if (selected == "all" || selected == "transient") { invalid_transient(); }
            if (selected == "all" || selected == "transient") { unsupported_capacitor_ic(); }
            if (selected == "all" || selected == "model") { invalid_models(); }
            if (selected == "all" || selected == "source") { unrequested_source_history(); }
            if (selected == "all" || selected == "parameters") { scoped_and_legacy_expansion(); }
            if (selected == "all" || selected == "phase") {
                default_phase_history_for_postprocessing();
                default_voltage_history_for_postprocessing();
            }
            valid_simulation();
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
