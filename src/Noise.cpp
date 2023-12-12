// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/Noise.hpp"

#include "JoSIM/Constants.hpp"
#include "JoSIM/Errors.hpp"
#include "JoSIM/Misc.hpp"
#include "JoSIM/Parameters.hpp"

using namespace JoSIM;

void JoSIM::Noise::determine_global_temperature(Input& iObj) {
  for (auto& i : iObj.controls) {
    if (i.front() == "TEMP") {
      iObj.globalTemp = parse_param(i.back(), iObj.parameters);
    }
  }
}

void JoSIM::Noise::determine_noise_effective_bandwidth(Input& iObj) {
  for (auto& i : iObj.controls) {
    if (i.front() == "NEB") {
      iObj.neB = parse_param(i.back(), iObj.parameters);
    }
  }
}

double JoSIM::Noise::determine_spectral_amplitude(const double& R,
                                                  const double& T) {
  // spectral amplitute = sqrt(4 * kB * T / R)
  double spAmp = sqrt((4 * Constants::BOLTZMANN * T) / R);
  return spAmp;
}
