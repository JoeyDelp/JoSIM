// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_TRANSIENT_HPP
#define JOSIM_TRANSIENT_HPP

#include <string>
#include <vector>

namespace JoSIM {
class Transient {
private:
  double prstep;
  double tstop;
  double tstart;
  double maxtstep;
  int simsize;
public:
  Transient() :
    prstep(1E-12),
    tstop(0.0),
    tstart(0.0),
    maxtstep(1E-12),
    simsize(0)
  { };

  double get_prstep() const { return prstep; }
  double get_tstop() const { return tstop; }
  double get_tstart() const { return tstart; }
  double get_maxtstep() const { return maxtstep; }
  int get_simsize() const { return simsize; }

  void set_prstep(double value) { prstep = value; }
  void set_tstop(double value) { tstop = value; }
  void set_tstart(double value) { tstart = value; }
  void set_maxtstep(double value) { maxtstep = value; }
  void set_simsize() { simsize = ((tstop - tstart) / prstep); }

  static void identify_simulation(const std::vector<std::string> &controls, Transient &tObj);
};
}

#endif