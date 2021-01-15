// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/Noise.hpp"
#include "JoSIM/Constants.hpp"
#include "JoSIM/Parameters.hpp"
#include "JoSIM/Misc.hpp"
#include "JoSIM/Errors.hpp"

using namespace JoSIM;

void JoSIM::Noise::determine_global_temperature(Input& iObj) {
  for (auto i : iObj.controls) {
    if (i.front() == "TEMP") {
      iObj.globalTemp = parse_param(i.back(), iObj.parameters);
    }
  }
}

void JoSIM::Noise::determine_noise_effective_bandwidth(Input& iObj) {
  for (auto i : iObj.controls) {
    if (i.front() == "NEB") {
      iObj.neB = parse_param(i.back(), iObj.parameters);
    }
  }
}

double JoSIM::Noise::determine_spectral_amplitude(const double& R, 
  const double& T) {
  // spectral amplitute = sqrt(4 * kB * T * B / R)
  double spAmp = sqrt((4 * Constants::BOLTZMANN * T ) / R);
  return spAmp;
}

std::pair<tokens_t, string_o>
JoSIM::Noise::create_resistive_current_noise(Input& iObj, const double& R,
  const double& T, const double& B, 
  std::vector<std::pair<tokens_t, string_o>>::iterator& i) {
  std::pair<tokens_t, string_o> noiseSource;
  noiseSource.first.emplace_back("INOISE_" + i->first.front());
  noiseSource.first.emplace_back(i->first.at(1));
  noiseSource.first.emplace_back(i->first.at(2));
  noiseSource.first.emplace_back("NOISE(" + Misc::precise_to_string(
    determine_spectral_amplitude(R, T)));
  noiseSource.first.emplace_back("0");
  noiseSource.first.emplace_back(
    Misc::precise_to_string(1 / B) + ")");
  noiseSource.second = i->second;
  return noiseSource;
}

void JoSIM::Noise::add_noise_sources(Input& iObj) {
  determine_global_temperature(iObj);
  determine_noise_effective_bandwidth(iObj);
  std::vector<std::pair<tokens_t, string_o>> noises;
  for (auto it = iObj.netlist.expNetlist.begin(); 
    it != iObj.netlist.expNetlist.end(); ++it) {
    if (it->first.front().at(0) == 'R') {
      std::optional<double> T;
      double B = 0, R = 0;
      if (!iObj.neB) {
        B = 1E12;
      }
      if (iObj.globalTemp) {
        T = iObj.globalTemp.value();
      }
      if (it->first.size() > 4) {
        tokens_t t(it->first.begin() + 4, it->first.end());
        std::string temp = Misc::vector_to_string(t, " ");
        t = Misc::tokenize(temp, " =");
        for (auto i = t.begin(); i < t.end(); ++i) {
          if (*i == "TEMP") {
            if (i + 1 == t.end() || *(i+1) == "NEB") {
              Errors::invalid_component_errors(ComponentErrors::RES_ERROR,
                Misc::vector_to_string(it->first));
            } else {
              T = parse_param(*(i + 1), iObj.parameters);
            }
          }
          if (*i == "NEB") {
            if (i + 1 == t.end() || *(i + 1) == "TEMP") {
              Errors::invalid_component_errors(ComponentErrors::RES_ERROR,
                Misc::vector_to_string(it->first));
            } else {
              B = parse_param(*(i + 1), iObj.parameters);
            }
          }
        }
        R = parse_param(*(it->first.begin() + 3), iObj.parameters);
      } else {
        R = parse_param(it->first.back(), iObj.parameters);
      }
      if (T) {
        noises.emplace_back(
          create_resistive_current_noise(iObj, R, T.value(), B, it));
      }
    }
  }
  if (noises.size() != 0) {
    iObj.netlist.expNetlist.insert(iObj.netlist.expNetlist.end(),
      noises.begin(), noises.end());
  }
}
