// Copyright (c) 2026 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#include <JoSIM/ProgressBar.hpp>
#include <JoSIM/Constants.hpp>
#include <JoSIM/IV.hpp>
#include <JoSIM/Input.hpp>
#include <JoSIM/Matrix.hpp>
#include <JoSIM/Model.hpp>
#include <JoSIM/Output.hpp>
#include <JoSIM/Rng.hpp>
#include <JoSIM/Simulation.hpp>

#include <cmath>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <streambuf>
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

void output_flush_failure() {
    // Accept every character, then fail only when the writer flushes it.
    struct FailedBuffer : std::streambuf {
        std::streamsize xsputn(const char*, std::streamsize count) override { return count; }
        int_type overflow(int_type value) override { return traits_type::not_eof(value); }
        int sync() override { return -1; }
    } buffer;
    JoSIM::Output output;
    output.traces.emplace_back("time");
    output.traces.back().data_ = {0.0};
    output.traces.emplace_back("V(N)");
    output.traces.back().data_ = {1.0};
    auto* original = std::cout.rdbuf(&buffer);
    try {
        must_throw([&] { output.format_cout(false); }, "Failed to write or close output: standard output");
    } catch (...) {
        std::cout.rdbuf(original);
        std::cout.clear();
        throw;
    }
    std::cout.rdbuf(original);
    std::cout.clear();
}

void nonfinite_derived_output() {
    for (bool phase : {false, true}) {
        auto run = input();
        const std::string amplitude = phase ? "1E305" : "1E308";
        run.netlist.expNetlist = {{{"V1", "NP", "0", "DC", amplitude}, std::nullopt},
                                 {{"V2", "NN", "0", "DC", "-" + amplitude}, std::nullopt},
                                 {{"R1", "NP", "0", "1K"}, std::nullopt},
                                 {{"R2", "NN", "0", "1K"}, std::nullopt}};
        JoSIM::Matrix matrix;
        matrix.create_matrix(run);
        JoSIM::Simulation simulation(run, matrix);
        run.controls = {{"PRINT", phase ? "P(NP)" : "V(NP,NN)"}};
        JoSIM::find_relevant_traces(run, matrix);
        JoSIM::Output output;
        must_throw([&] { output.write_output(run, matrix, simulation); }, "Non-finite output sample");
    }
}

void progress_worker_lifetime() {
    for (bool explicit_stop : {false, true}) {
        JoSIM::ProgressBar bar;
        bar.set_total(1);
        bar.set_bar_width(1);
        bar.create_thread();
        bar.create_thread();
        bar.update(1);
        if (explicit_stop) {
            bar.complete();
            bar.complete();
        }
        // Destruction must stop/join every worker in both cases.
    }
}

void inductor_timestep_stamp() {
    for (auto mode : {JoSIM::AnalysisType::Voltage, JoSIM::AnalysisType::Phase}) {
        for (auto nodes : {JoSIM::NodeConfig::GND, JoSIM::NodeConfig::POSGND,
                           JoSIM::NodeConfig::GNDNEG, JoSIM::NodeConfig::POSNEG}) {
            auto run = input();
            run.argAnal = mode;
            JoSIM::nodemap map{{"A", 0}, {"B", 1}};
            JoSIM::nodeconnections connections(2);
            std::unordered_set<std::string> labels;
            JoSIM::Spread spread;
            int64_t current = 2;
            JoSIM::Inductor inductor({{"L1", "A", "B", "1N"}, std::nullopt},
                nodes, map, labels, connections, run, spread, current);
            inductor.add_mutualInductance(2e-10, mode, 1e-12, 3);
            inductor.add_mutualInductance(-3e-10, mode, 1e-12, 4);
            const auto original = inductor.matrixInfo.nonZeros_;
            inductor.update_timestep(0.5);
            size_t coefficient = 0;
            for (size_t k = 0; k < original.size(); ++k) {
                double expected = original[k];
                if (inductor.matrixInfo.columnIndex_.at(k) >= 2) {
                    const double inductance[] = {1e-9, 2e-10, -3e-10};
                    expected = mode == JoSIM::AnalysisType::Voltage
                        ? -1.5 * inductance[coefficient] / 5e-13
                        : -inductance[coefficient] / JoSIM::Constants::SIGMA;
                    ++coefficient;
                }
                if (std::abs(inductor.matrixInfo.nonZeros_.at(k) - expected)
                    > 1e-12 * std::max(1.0, std::abs(expected))) {
                    throw std::runtime_error("Inductor timestep stamp differs from constitutive equation");
                }
            }
            inductor.update_timestep(2.0);
            if (inductor.matrixInfo.nonZeros_ != original) {
                throw std::runtime_error("Inductor timestep stamp did not reverse exactly");
            }
        }
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
    for (int corruption = 0; corruption < 6; ++corruption) {
        auto valid = input();
        valid.netlist.expNetlist = {{{"V1", "N", "0", "DC", "1"}, std::nullopt},
                                   {{"R1", "N", "0", "1K"}, std::nullopt}};
        JoSIM::Matrix damaged;
        damaged.create_matrix(valid);
        switch (corruption) {
            case 0: damaged.rp.at(1) = -1; break;
            case 1: damaged.rp.at(1) = damaged.rp.back() + 1; break;
            case 2: damaged.ci.front() = -1; break;
            case 3: damaged.ci.front() = damaged.branchIndex; break;
            case 4: damaged.nz.front() = std::numeric_limits<double>::quiet_NaN(); break;
            case 5: damaged.nz.front() = std::numeric_limits<double>::infinity(); break;
        }
        must_throw([&] { JoSIM::Simulation simulation(valid, damaged); }, "Invalid sparse matrix");
    }
}

void result_storage_limit() {
    auto run = input();
    run.transSim.tstep(1);
    run.transSim.tstop(8e18);
    run.netlist.expNetlist = {{{"V1", "N", "0", "DC", "1"}, std::nullopt},
                             {{"R1", "N", "0", "1K"}, std::nullopt}};
    JoSIM::Matrix matrix;
    matrix.create_matrix(run);
    // N fits int64 but exceeds any supported vector<double>::max_size().
    // This fails before allocation; it does not induce real memory pressure.
    must_throw([&] { JoSIM::Simulation simulation(run, matrix); }, "Cannot allocate transient result storage");
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

void capacitor_ic_requires_dst() {
    auto run = input();
    run.transSim.startup(true);
    run.netlist.expNetlist = {{{"R1", "N", "0", "10"}, std::nullopt},
                             {{"C1", "N", "0", "1P", "IC=1M"}, std::nullopt}};
    must_throw([&] { JoSIM::Matrix matrix; matrix.create_matrix(run); }, "capacitor IC requires .TRAN ... DST");
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

void beta_c_models() {
    const auto parse = [](const std::string& fields) {
        JoSIM::vector_pair_t<JoSIM::Model, JoSIM::string_o> models;
        JoSIM::Model::parse_model({{".MODEL", "JM", "JJ(" + fields + ")"}, std::nullopt}, models, {});
        return models.front().first;
    };
    const auto check_resistance = [](const JoSIM::Model& model, double expected) {
        if (!std::isfinite(model.rn()) || std::abs(model.rn() / expected - 1) > 1e-12) {
            throw std::runtime_error("BETA_C produced the wrong resistance");
        }
    };
    const double expected = std::sqrt(JoSIM::Constants::SIGMA / (1e-6 * 1e-13));
    check_resistance(parse("RTYPE=0 IC=1U CAP=0.1P BETA_C=1"), expected);
    check_resistance(parse("beta_c = 1, c = 0.1P, icrit = 1U, rtype = 0"), expected);
    check_resistance(parse("RTYPE=0 BETA_C=1"),
                     std::sqrt(JoSIM::Constants::SIGMA / (1e-3 * 2.5e-12)));
    // A finite result must survive intermediate products outside double range.
    check_resistance(parse("RTYPE=0 BETA_C=1E300 IC=1E150 CAP=1E150"),
                     std::sqrt(JoSIM::Constants::SIGMA));
    check_resistance(parse("RTYPE=0 BETA_C=1E-300 IC=1E-150 CAP=1E-150"),
                     std::sqrt(JoSIM::Constants::SIGMA));

    JoSIM::param_map params;
    JoSIM::create_parameter({".PARAM", "BETA=2", "CRIT=1U", "CAPVAL=0.1P"}, params, "CELL");
    JoSIM::parse_parameters(params);
    JoSIM::vector_pair_t<JoSIM::Model, JoSIM::string_o> models;
    JoSIM::Model::parse_model({{".MODEL", "JM", "JJ(RTYPE=0 IC=CRIT CAP=CAPVAL BETA_C={BETA/2})"}, "CELL"},
                             models, params);
    check_resistance(models.front().first, expected);

    const auto rejects = [&](const std::string& fields, const std::string& diagnostic) {
        must_throw([&] { parse(fields); }, diagnostic);
    };
    rejects("BETA_C=1", "BETA_C requires RTYPE=0");
    rejects("BETA_C=1 RTYPE=1", "BETA_C requires RTYPE=0");
    rejects("RN=10 BETA_C=1 RTYPE=0", "BETA_C and RN");
    rejects("RTYPE=0 BETA_C=1 RN=10", "BETA_C and RN");
    for (const auto& field : {"T=4.2", "TC=9.1", "D=0"}) {
        rejects("RTYPE=0 BETA_C=1 " + std::string(field), "temperature-dependent");
        rejects(std::string(field) + " BETA_C=1 RTYPE=0", "temperature-dependent");
    }
    for (const auto& value : {"0", "-1"}) {
        rejects("RTYPE=0 BETA_C=" + std::string(value), "BETA_C must be finite and positive");
        rejects("RTYPE=0 BETA_C=1 IC=" + std::string(value), "BETA_C requires finite positive IC");
        rejects("RTYPE=0 BETA_C=1 C=" + std::string(value), "BETA_C requires finite positive CAP");
    }
    rejects("RTYPE=0 BETA_C=1 BETA_C=2", "Duplicate BETA_C");
    rejects("RTYPE=0 BETA_C=1E308 IC=1E-300 CAP=1E-300", "unrepresentable RN");
    rejects("RTYPE=0 BETA_C=1E-300 IC=1E308 CAP=1E308", "unrepresentable RN");
    rejects("RTYPE=0 BETA_C={EXP(1000)}", "EXP");
    // Without BETA_C, existing signed/zero-Ic and zero-C models remain valid.
    check_resistance(parse("RTYPE=0 IC=0 CAP=0 RN=12"), 12);
    check_resistance(parse("RTYPE=0 IC=-1U CAP=0 RN=12"), 12);
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
        if (selected == "all" || selected == "model" || selected == "beta") { beta_c_models(); }
        // Repeated failures exercise solver cleanup; sanitizers check allocations.
        for (int i = 0; i < 8; ++i) {
            if (selected == "all" || selected == "missing") { missing_model(); }
            if (selected == "all" || selected == "singular") { singular_matrix(); }
            if (selected == "all" || selected == "invalid") { invalid_matrix(); }
            if (selected == "all" || selected == "storage") { result_storage_limit(); }
            if (selected == "all" || selected == "cli") { cli_values(); }
            if (selected == "all" || selected == "output") { output_flush_failure(); }
            if (selected == "all" || selected == "output") { nonfinite_derived_output(); }
            if (selected == "all" || selected == "progress") { progress_worker_lifetime(); }
            if (selected == "all" || selected == "inductor") { inductor_timestep_stamp(); }
            if (selected == "all" || selected == "transient") { invalid_transient(); }
            if (selected == "all" || selected == "transient") { capacitor_ic_requires_dst(); }
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
