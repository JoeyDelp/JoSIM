// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_CONSTANTS_HPP
#define JOSIM_CONSTANTS_HPP

#include <string>

namespace JoSIM {
  namespace Constants {
    static constexpr float PI = 3.141592653589793238463;
    static constexpr float PHI_ZERO = 2.067833831170082E-15;
    static constexpr float BOLTZMANN = 1.38064852E-23;
    static constexpr float EV = 1.6021766208e-19;
    static constexpr float HBAR = 1.0545718001391127e-34;
    static constexpr float C = 299792458;
    static constexpr float MU0 = 12.566370614E-7;
    static constexpr float EPS0 = 8.854187817E-12;
    // HBAR / 2 * EV
    static constexpr float SIGMA = 3.291059757e-16;
  }
}

#endif