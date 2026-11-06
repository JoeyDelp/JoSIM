// Copyright (c) 2026 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#include "JoSIM/Function.hpp"
#include "JoSIM/Misc.hpp"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <stdexcept>

using namespace JoSIM;

static void near(double actual, double expected, const std::string& context, double tolerance = 1e-11) {
    if (!std::isfinite(actual) || std::abs(actual - expected) > tolerance) {
        throw std::runtime_error(context + ": expected " + std::to_string(expected) + ", got " + std::to_string(actual));
    }
}

int main() {
    const auto file = std::filesystem::current_path() / "function-regression-wave.dat";
    const auto name = file.filename().string();
    Input input(AnalysisType::Voltage, 0, true);
    input.transSim.tstep(0.25);
    input.transSim.tstop(12.0);
    input.fileParentPath = file.parent_path().string();
    auto write = [&](const std::string& values) { std::ofstream(file) << values << '\n'; };
    int failures = 0;
    auto check = [&](const std::string& label, const std::function<void()>& test) {
        try {
            write("1 3 5 7");
            test();
            std::cout << label << " passed\n";
        } catch (const std::exception& error) {
            std::cerr << label << ": " << error.what() << '\n';
            ++failures;
        }
    };
    check("relative file without input parent", [&] {
        Input bare = input;
        bare.fileParentPath.reset();
        Function source;
        source.parse_function("CUS(" + name + " 1 1 0)", bare, std::nullopt);
        near(source.value(0.0), 1.0, "first stored sample");
    });
    check("linear sample alignment", [&] {
        Function source;
        source.parse_function("CUS(" + name + " 1 1 1)", input, std::nullopt);
        for (double t : {0.0, 0.25, 0.5, 1.0, 1.5, 2.0, 2.75, 3.0}) {
            near(source.value(t), 1.0 + 2.0 * t, "linear interpolation between stored samples");
        }
        near(source.value(3.5), 7.0, "last sample held for its remaining interval");
        near(source.value(4.0), 0.0, "nonperiodic end");
    });
    check("delayed cubic alignment", [&] {
        Function source;
        source.parse_function("CUS(" + name + " 1 1 2 2.5)", input, std::nullopt);
        near(source.value(2.0), 0.0, "before delay");
        for (double t : {0.0, 0.25, 0.5, 1.25, 2.5, 3.0}) {
            near(source.value(2.5 + t), 1.0 + 2.0 * t, "cubic interpolation preserves a straight line after delay");
        }
    });
    check("period measured from delay", [&] {
        Function source;
        source.parse_function("CUS(" + name + " 1 1 0 5.5 1)", input, std::nullopt);
        for (double offset : {0.0, 4.0, 8.0}) {
            near(source.value(5.5 + offset), 1.0, "period start");
            near(source.value(6.75 + offset), 3.0, "period sample");
        }
    });
    check("natural cubic in physical time units", [&] {
        write("0 1 0");
        Input physical = input;
        physical.transSim.tstep(0.25e-12);
        Function source;
        source.parse_function("CUS(" + name + " 1E-12 1 2 2.5E-12)", physical, std::nullopt);
        // Natural cubic through (0,0),(1,1),(2,0): 1.5*x-0.5*x^3
        // on the first interval, reflected about x=1 on the second.
        near(source.value(3.0e-12), 0.6875, "first cubic midpoint");
        near(source.value(4.0e-12), 0.6875, "second cubic midpoint");
    });
    check("sample timing independent of simulation step", [&] {
        Input coarse = input;
        coarse.transSim.tstep(2.0);
        Function source;
        source.parse_function("CUS(" + name + " 1 1 0)", coarse, std::nullopt);
        near(source.value(1.5), 3.0, "requested sample interval must not be stretched");
    });
    check("rescaling and replacing a waveform", [&] {
        Function source;
        source.parse_function("CUS(" + name + " 1 2 1)", input, std::nullopt);
        near(source.value(0.5), 4.0, "scale factor");
        source.parse_function("CUS(" + name + " 1 -1 0)", input, std::nullopt);
        near(source.value(1.5), -3.0, "new waveform replaces old samples");
    });
    check("invalid CUS definitions", [&] {
        for (const std::string args : {"", "1 1 0", "0 1 1", "-1 1 1", "1 1 1.5", "1 1 1 -1", "1 1 1 0 0.5", "1 1 1 0 1 2"}) {
            bool rejected = false;
            if (args == "1 1 0") { write(""); }
            else { write("1 3 5 7"); }
            try {
                Function source;
                source.parse_function("CUS(" + name + " " + args + ")", input, std::nullopt);
            } catch (const std::runtime_error&) { rejected = true; }
            if (!rejected) { throw std::runtime_error("invalid or empty waveform accepted: " + args); }
        }
        write("1 2");
        bool rejected = false;
        try {
            Function source;
            source.parse_function("CUS(" + name + " 1 1 2)", input, std::nullopt);
        } catch (const std::runtime_error&) { rejected = true; }
        if (!rejected) { throw std::runtime_error("cubic interpolation accepted fewer than three samples"); }
        write("nan 1 2");
        rejected = false;
        try {
            Function source;
            source.parse_function("CUS(" + name + " 1 1 0)", input, std::nullopt);
        } catch (const std::runtime_error&) { rejected = true; }
        if (!rejected) { throw std::runtime_error("non-finite waveform samples accepted"); }
    });
    const std::vector<std::pair<std::string, double>> expressions = {
        {"1P", 1e-12}, {"1p", 1e-12}, {"1Meg", 1e6}, {"1m", 1e-3}, {"2mV", 2e-3},
        {"2.5E-3", 0.0025}, {"2.5e-3", 0.0025}, {"1.E-3", 0.001}, {".5e-3", 0.0005},
        {"-3.4e+2", -340.0}, {"3*-2", -6.0}, {"3/-2", -1.5}, {"-2^2", -4.0},
        {"(-2)^2", 4.0}, {"2^-3", 0.125}, {"2^3^2", 512.0},
        {"SIN(0.5)^2", std::sin(0.5) * std::sin(0.5)}, {"sin(0.5)^2", std::sin(0.5) * std::sin(0.5)},
        {"LOG10(100)", 2.0}, {"EXP(LOG(3))", 3.0}, {"SQRT(9)+CBRT(8)", 5.0},
    };
    for (const auto& expression : expressions) {
        check("numeric " + expression.first, [&] {
            near(parse_param(expression.first, {}), expression.second, expression.first,
                 std::max(1e-300, std::abs(expression.second) * 2e-13));
        });
    }
    check("scoped expression with global fallback", [&] {
        param_map parameters;
        Parameter local, global, other;
        local.set_value(2.0);
        global.set_value(10.0);
        other.set_value(3.0);
        parameters[ParameterName("R", "CELL")] = local;
        parameters[ParameterName("R", std::nullopt)] = global;
        parameters[ParameterName("G", std::nullopt)] = other;
        near(parse_param("R+G", parameters, "CELL"), 5.0, "local R plus global G");
        near(parse_param("G+R", parameters, "CELL"), 5.0, "global lookup must not depend on operand order");
        near(parse_param("R+G", parameters), 13.0, "global expression remains independent");
    });
    check("engineering suffixes in waveform file", [&] {
        write("1m 3m 5m 7m");
        Function source;
        source.parse_function("CUS(" + name + " 1 1 0)", input, std::nullopt);
        near(source.value(1.5), 0.003, "waveform file amplitudes use engineering suffixes");
    });
    check("invalid numeric domains and literals", [&] {
        for (const std::string expression : {"1/0", "SQRT(-1)", "LOG(0)", "EXP(1000)", "1E", "1.2.3"}) {
            bool rejected = false;
            try { parse_param(expression, {}); }
            catch (const std::runtime_error&) { rejected = true; }
            if (!rejected) { throw std::runtime_error("invalid numeric expression accepted: " + expression); }
        }
        for (const std::string literal : {"1e", "1.2.3", "1e+"}) {
            bool rejected = false;
            try { Misc::modifier(literal); }
            catch (const std::runtime_error&) { rejected = true; }
            if (!rejected) { throw std::runtime_error("malformed scalar accepted: " + literal); }
        }
    });
    std::error_code ignored;
    std::filesystem::remove(file, ignored);
    return failures ? 1 : 0;
}
