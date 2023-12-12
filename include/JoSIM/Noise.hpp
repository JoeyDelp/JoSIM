// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_NOISE_HPP
#define JOSIM_NOISE_HPP

#include "JoSIM/Input.hpp"

namespace JoSIM {

class Noise {
 public:
  static void determine_global_temperature(Input& iObj);
  static void determine_noise_effective_bandwidth(Input& iObj);
  static double determine_spectral_amplitude(const double& R, const double& T);
};
}  // namespace JoSIM

#endif