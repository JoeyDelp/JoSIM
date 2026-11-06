// Copyright (c) 2026 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#include <JoSIM/Input.hpp>
#include <JoSIM/Matrix.hpp>
#include <JoSIM/Rng.hpp>
#include <JoSIM/Simulation.hpp>

#include <cmath>

int main() {
    JoSIM::Input input(JoSIM::AnalysisType::Voltage, 0, true);
    input.transSim.tstep(1e-12);
    input.transSim.tstop(5e-12);
    input.transSim.startup(false);
    input.netlist.expNetlist = {{{"V1", "N", "0", "DC", "1"}, std::nullopt},
                               {{"R1", "N", "0", "1K"}, std::nullopt}};
    JoSIM::Rng::start_run(123);
    JoSIM::Matrix matrix;
    matrix.create_matrix(input);
    JoSIM::Simulation simulation(input, matrix);
    const double voltage = simulation.results.xVector.at(matrix.nm.at("N")).value().back();
    return std::isfinite(voltage) && std::abs(voltage - 1.0) < 1e-12 ? 0 : 1;
}
