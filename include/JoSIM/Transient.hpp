// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_TRANSIENT_HPP
#define JOSIM_TRANSIENT_HPP

#include "JoSIM/TypeDefines.hpp"

#include <string>
#include <vector>

namespace JoSIM {
class Transient {
  private:
  double tstep;
  double tstop;
  double prstart;
  double prstep;
  int simsize;
  public:
  Transient() :
    tstep(1E-12),
    tstop(0.0),
    prstart(0.0),
    prstep(1E-12),
    simsize(0)
  { };

  double get_tstep() const { return tstep; }
  double get_tstop() const { return tstop; }
  double get_prstart() const { return prstart; }
  double get_prstep() const { return prstep; }
  int get_simsize() const { return simsize; }

  void set_tstep(double value) { tstep = value; }
  void set_tstop(double value) { tstop = value; }
  void set_prstart(double value) { prstart = value; }
  void set_prstep(double value) { prstep = value; }
  void set_simsize() { simsize = (tstop / tstep); }

  static void identify_simulation(
    const std::vector<tokens_t> &controls, Transient &tObj);
};
}

#endif