// Copyright (c) 2025 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/Noise.hpp"

#include "JoSIM/Constants.hpp"
#include "JoSIM/Errors.hpp"
#include "JoSIM/Misc.hpp"
#include "JoSIM/Parameters.hpp"

#include <algorithm>
#include <cmath>

using namespace JoSIM;

void JoSIM::Noise::determine_global_temperature(Input& iObj) {
    for (auto& i : iObj.controls) {
        if (i.front() == "TEMP") {
            iObj.globalTemp = parse_param(i.back(), iObj.parameters);
            if (!std::isfinite(*iObj.globalTemp) || *iObj.globalTemp < 0) {
                Errors::function_errors(FunctionErrors::INVALID_NOISE_PARAMETERS, "TEMP must be finite and non-negative");
            }
        }
    }
}

void JoSIM::Noise::determine_noise_effective_bandwidth(Input& iObj) {
    for (auto& i : iObj.controls) {
        if (i.front() == "NEB") {
            iObj.neB = parse_param(i.back(), iObj.parameters);
            if (!std::isfinite(iObj.neB) || iObj.neB <= 0) {
                Errors::function_errors(FunctionErrors::INVALID_NOISE_PARAMETERS, "NEB must be finite and positive");
            }
        }
    }
}

double JoSIM::Noise::determine_sampling_interval(const double& bandwidth, const double& simulation_step) {
    if (!std::isfinite(bandwidth) || bandwidth <= 0 || !std::isfinite(simulation_step) || simulation_step <= 0) {
        Errors::function_errors(FunctionErrors::INVALID_NOISE_PARAMETERS,
                                "NEB and simulation timestep must be finite and positive");
    }
    // The integral of one-sided ASD^2*sinc(f*dt)^4 is ASD^2/(3*dt).
    // Retain the documented simulation-step floor.
    const double interval = std::max(simulation_step, (1.0 / 3.0) / bandwidth);
    if (!std::isfinite(interval)) {
        Errors::function_errors(FunctionErrors::INVALID_NOISE_PARAMETERS, "NEB update interval is not representable");
    }
    return interval;
}

double JoSIM::Noise::determine_spectral_amplitude(const double& R, const double& T) {
    if (!std::isfinite(T) || T < 0 || std::isnan(R) || (T > 0 && R <= 0)) {
        Errors::function_errors(FunctionErrors::INVALID_NOISE_PARAMETERS,
                                "Thermal noise requires non-negative finite TEMP and positive resistance when TEMP>0");
    }
    if (T == 0) { return 0; }
    // Equivalent to sqrt(4*kB*T/R), without overflowing intermediate power.
    return 2 * std::sqrt(Constants::BOLTZMANN) * std::sqrt(T) / std::sqrt(R);
}
