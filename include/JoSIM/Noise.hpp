// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_NOISE_HPP
#define JOSIM_NOISE_HPP

#include "JoSIM/Input.hpp"

namespace JoSIM {
  
  class Noise {
    private:
    static float determine_spectral_amplitude(const float& R, 
      const float& T);
    static std::pair<tokens_t, string_o>
      create_resistive_current_noise(Input& iObj, const float &R, 
        const float &T, const float &B,
        std::vector<std::pair<tokens_t, string_o>>::iterator& i);
    static void determine_global_temperature(Input& iObj);
    static void determine_noise_effective_bandwidth(Input& iObj);


    public:
    static void add_noise_sources(Input& iObj);
  };
}

#endif