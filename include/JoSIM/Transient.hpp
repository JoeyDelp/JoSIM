// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_TRANSIENT_HPP
#define JOSIM_TRANSIENT_HPP

#include <string>
#include <vector>

#include "JoSIM/TypeDefines.hpp"
#include "JoSIM/Parameters.hpp"

namespace JoSIM {
class Transient {
 private:
  double tstep_;
  double tstop_;
  double prstart_;
  double prstep_;
  bool startup_;

 public:
  Transient()
      : tstep_(0.25E-12),
        tstop_(0.0),
        prstart_(0.0),
        prstep_(1E-12),
        startup_(true){};

  double tstep() const { return tstep_; }
  double tstop() const { return tstop_; }
  double prstart() const { return prstart_; }
  double prstep() const { return prstep_; }
  int64_t simsize() const { return static_cast<int64_t>(tstop_ / tstep_); }
  bool startup() const { return startup_; }

  void tstep(double value) { tstep_ = value; }
  void tstop(double value) { tstop_ = value; }
  void prstart(double value) { prstart_ = value; }
  void prstep(double value) { prstep_ = value; }
  void startup(bool value) { startup_ = value; }

  static void identify_simulation(std::vector<tokens_t>& controls,
                                  Transient& tObj, param_map& params);
};
}  // namespace JoSIM

#endif