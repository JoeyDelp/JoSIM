// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_NOISE_HPP
#define JOSIM_NOISE_HPP

#include "JoSIM/Input.hpp"

namespace JoSIM {

  class Noise {
    private:
    static double determine_spectral_amplitude(const double& R,
      const double& T);
    static std::pair<tokens_t, string_o>
      create_resistive_current_noise(Input& iObj, const double& R,
        const double& T, const double& B,
        std::vector<std::pair<tokens_t, string_o>>::iterator& i);
    static void determine_global_temperature(Input& iObj);
    static void determine_noise_effective_bandwidth(Input& iObj);


    public:
    static void add_noise_sources(Input& iObj);
  };
}

#endif