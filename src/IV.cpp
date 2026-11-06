// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/IV.hpp"

#include "JoSIM/Errors.hpp"
#include "JoSIM/Misc.hpp"
#include "JoSIM/Parameters.hpp"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

using namespace JoSIM;

IV::IV(const Input iObj) {
    for (const auto& i : iObj.controls) {
        if (i.front() == "IV" || i.front() == ".IV") { setup_iv(i, iObj); }
    }
}

void IV::setup_iv(const tokens_t& i, const Input& iObj) {
    if (i.size() < 4) { Errors::control_errors(ControlErrors::INVALID_IV_COMMAND, Misc::vector_to_string(i)); }
    // Model can be part of a subcircuit so find subcircuit and model name
    tokens_t t     = Misc::tokenize(i.at(1), ".|");
    auto&    model = t.at(0);
    string_o subc;
    if (t.size() > 1) { subc = t.at(1); }
    // Check to see if model exists
    if (iObj.netlist.models.count(std::make_pair(model, subc)) == 0) {
        Errors::control_errors(ControlErrors::IV_MODEL_NOT_FOUND, model + ((subc) ? "|" + subc.value() : ""));
    }
    // Determine the maximum current to which to draw the IV curve
    double maxC = parse_param(i.at(2), iObj.parameters);
    if (!std::isfinite(maxC) || maxC <= 0.0) {
        Errors::control_errors(ControlErrors::INVALID_IV_COMMAND, "IV Imax must be > 0: " + Misc::vector_to_string(i));
    }
    std::optional<double> gtjj;
    std::optional<double> step;

    for (size_t k = 4; k < i.size(); ++k) {
        const std::string& opt = i.at(k);

        if (opt.rfind("GTJJ=", 0) == 0) {
            gtjj = parse_param(opt.substr(5), iObj.parameters);
        } else if (opt.rfind("STEP=", 0) == 0) {
            step = parse_param(opt.substr(5), iObj.parameters);
        } else if (opt.rfind("ISTEP=", 0) == 0) {
            step = parse_param(opt.substr(6), iObj.parameters);
        } else {
            Errors::control_errors(ControlErrors::INVALID_IV_COMMAND, "Unknown IV option: " + opt);
        }
    }

    if (gtjj && !std::isfinite(gtjj.value())) {
        Errors::control_errors(ControlErrors::INVALID_IV_COMMAND, "GTJJ must be a finite voltage");
    }
    double currentIncrement = step.value_or(25E-7); // default 2.5e-6 A
    if (!std::isfinite(currentIncrement) || currentIncrement <= 0.0) {
        Errors::control_errors(ControlErrors::INVALID_IV_COMMAND, "IV STEP must be > 0: " + Misc::vector_to_string(i));
    }
    // Bare IV output names are relative to the main input file.
    auto path = std::filesystem::path(i.at(3));
    if (!path.has_parent_path() && iObj.fileParentPath) {
        path = std::filesystem::path(iObj.fileParentPath.value()).append(i.at(3));
    }
    iObj.ensure_safe_output(path.string());
    // Create an input object for this simulation
    Input ivInp = iObj;
    ivInp.controls.clear();
    // A type-level model IV uses that definition's defaults, independently of
    // any instantiated copies and their overrides. Resolve only this scope.
    std::vector<ParameterName> defaultNames;
    if (subc) {
        for (const auto& parameter : ivInp.parameters) {
            if (parameter.first.subcircuit() == subc) { defaultNames.push_back(parameter.first); }
        }
        parse_parameters(ivInp.parameters, defaultNames);
    }
    if (!Model::find_model(model, ivInp.netlist.models_new, subc) || subc) {
        Model::parse_model({ivInp.netlist.models.at({model, subc}), subc},
                           ivInp.netlist.models_new, ivInp.parameters);
    }

    if (gtjj) {
        tokens_t          jj = {"B01", "1", "0", model, "AREA=1", "GC=GTVG"};
        // tokens_t ib = {"IB01", "0", "1", "DC", "2.5U"};
        tokens_t          ib = {"IB01", "0", "1", "PWL(0", "0", "10P", "0", "50P", "2.5U)"};
        std::stringstream gtjj_value;
        gtjj_value << std::scientific << std::uppercase << gtjj.value();
        tokens_t vgc = {"VGTC", "GTVG", "0", "DC", gtjj_value.str()};
        ivInp.netlist.expNetlist
                = {std::make_pair(jj, subc), std::make_pair(vgc, std::nullopt), std::make_pair(ib, std::nullopt)};
    } else {
        tokens_t jj              = {"B01", "1", "0", model, "AREA=1"};
        // tokens_t ib = {"IB01", "0", "1", "DC", "2.5U"};
        tokens_t ib              = {"IB01", "0", "1", "PWL(0", "0", "10P", "0", "50P", "2.5U)"};
        ivInp.netlist.expNetlist = {std::make_pair(jj, subc), std::make_pair(ib, std::nullopt)};
    }
    ivInp.transSim.tstep(0.05E-12);
    ivInp.transSim.tstop(500E-12);
    ivInp.argAnal = JoSIM::AnalysisType::Voltage;
    ivInp.argMin  = true;
    std::vector<std::pair<double, double>> iv_data;
    iv_data   = generate_iv(maxC, currentIncrement, ivInp);
    write_iv(iv_data, path.string());
}

std::vector<std::pair<double, double>> IV::generate_iv(double maxC, double currentIncrement, Input ivInp) {
    std::vector<std::pair<double, double>> iv_data;
    double                                 currentCurr = 0.0;
    Matrix                                 ivMat;
    ivMat.create_matrix(ivInp);
    const auto set_bias = [&](double initial, double target) {
        // A timestep retry reconstructs the matrix from Input, so keep the
        // generated three-knot PWL declaration and its active function in sync.
        auto& bias = ivInp.netlist.expNetlist.back().first;
        bias.at(6) = Misc::precise_to_string(initial);
        bias.at(8) = Misc::precise_to_string(target) + ")";
        ivMat.sourcegen.back().ampValues({0, initial, target});
    };
    auto step_towards = [](double cur, double target, double step) {
        return (cur < target) ? std::min(cur + step, target) : std::max(cur - step, target);
    };

    while (currentCurr < maxC) {
        currentCurr = step_towards(currentCurr, maxC, currentIncrement);
        set_bias(0, currentCurr);
        iv_data.emplace_back(do_simulate(ivInp, ivMat));
    }

    while (currentCurr > 0.0) {
        currentCurr = step_towards(currentCurr, 0.0, currentIncrement);
        set_bias(maxC, currentCurr);
        iv_data.emplace_back(do_simulate(ivInp, ivMat));
    }

    while (currentCurr > -maxC) {
        currentCurr = step_towards(currentCurr, -maxC, currentIncrement);
        set_bias(0, currentCurr);
        iv_data.emplace_back(do_simulate(ivInp, ivMat));
    }

    while (currentCurr < 0.0) {
        currentCurr = step_towards(currentCurr, 0.0, currentIncrement);
        set_bias(-maxC, currentCurr);
        iv_data.emplace_back(do_simulate(ivInp, ivMat));
    }
    return iv_data;
}

std::pair<double, double> IV::do_simulate(Input& ivInp, Matrix& ivMat) {
    // Simulate the iv curve
    Simulation   ivSim(ivInp, ivMat);
    // Add the results to the stack
    double       current  = ivMat.sourcegen.back().ampValues().back();
    auto&        voltVect = ivSim.results.xVector.front().value();
    const size_t start    = voltVect.size() / 2;
    const size_t count    = voltVect.size() - start;
    double       voltage  = 0;
    if (count == 0) {
        // handle gracefully (error or 0)
        Errors::control_errors(ControlErrors::INVALID_IV_COMMAND, "IV simulation produced empty voltage vector");
    }
    for (size_t idx = start; idx < voltVect.size(); ++idx) { voltage += voltVect.at(idx); }
    voltage /= static_cast<double>(count);
    return std::make_pair(voltage, current);
}

void IV::write_iv(std::vector<std::pair<double, double>>& iv_data, const std::string& output_path) {
    auto          path = std::filesystem::path(output_path);
    std::ofstream outfile(path.string());
    outfile << std::setprecision(6);
    outfile << std::scientific;
    if (outfile.is_open()) {
        outfile << "V,I\n";
        for (auto i : iv_data) { outfile << i.first << "," << i.second << "\n"; }
        outfile.close();
        if (!outfile) { Errors::output_errors(OutputErrors::WRITE_FAILED, path.string()); }
    } else {
        Errors::output_errors(OutputErrors::CANNOT_OPEN_FILE, path.string());
    }
}
