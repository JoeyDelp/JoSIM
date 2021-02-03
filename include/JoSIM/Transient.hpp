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
  float tstep;
  float tstop;
  float prstart;
  float prstep;
  int simsize;
  public:
  Transient() :
    tstep(1E-12),
    tstop(0.0),
    prstart(0.0),
    prstep(1E-12),
    simsize(0)
  { };

  float get_tstep() const { return tstep; }
  float get_tstop() const { return tstop; }
  float get_prstart() const { return prstart; }
  float get_prstep() const { return prstep; }
  int get_simsize() const { return simsize; }

  void set_tstep(float value) { tstep = value; }
  void set_tstop(float value) { tstop = value; }
  void set_prstart(float value) { prstart = value; }
  void set_prstep(float value) { prstep = value; }
  void set_simsize() { simsize = (tstop / tstep); }

  static void identify_simulation(
    const std::vector<tokens_t> &controls, Transient &tObj);
};
}

#endif