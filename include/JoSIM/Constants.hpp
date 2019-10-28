// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_CONSTANTS_HPP
#define JOSIM_CONSTANTS_HPP

namespace JoSIM {
  namespace Constants {
    static constexpr double PI = 3.141592653589793238463;
    static constexpr double PHI_ZERO = 2.067833831170082E-15;
    static constexpr double BOLTZMANN = 1.38064852E-23;
    static constexpr double EV = 1.6021766208e-19;
    static constexpr double HBAR = 1.0545718001391127e-34;
    static constexpr double C = 299792458;
    static constexpr double MU0 = 12.566370614E-7;
    static constexpr double EPS0 = 8.854187817E-12;

    static double string_constant(std::string &s) {
      if (s == "PI") return PI;
      else if (s == "PHI_ZERO") return PHI_ZERO;
      else if (s == "BOLTZMANN") return BOLTZMANN;
      else if (s == "EV") return EV;
      else if (s == "HBAR") return HBAR;
      else if (s == "C") return C;
      else if (s == "MU0") return MU0;
      else if (s == "EPS0") return EPS0;
      return 0.0;
    };
  }
}

#endif