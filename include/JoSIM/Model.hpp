// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_MODEL_HPP
#define JOSIM_MODEL_HPP

#include "JoSIM/Constants.hpp"
#include "JoSIM/Parameters.hpp"
#include "JoSIM/TypeDefines.hpp"

namespace JoSIM {
class Model {
 private:
  std::string modelName_;
  double vg_;
  double ic_;
  std::vector<double> cpr_;
  int64_t rtype_;
  double rn_;
  double r0_;
  double c_;
  double t_;
  double tc_;
  double deltaV_;
  double d_;
  double icFct_;
  double phiOff_;
  bool tDep_;

 public:
  Model()
      : vg_(2.8E-3),
        ic_(1E-3),
        cpr_({1.0}),
        rtype_(1),
        rn_(5),
        r0_(30),
        c_(2.5E-12),
        t_(4.2),
        tc_(9.1),
        deltaV_(0.1E-3),
        d_(0),
        icFct_(Constants::PI / 4),
        phiOff_(0),
        tDep_(false){};

  std::string modelName() const { return modelName_; }
  void modelName(const std::string& n) { modelName_ = n; }
  double vg() const { return vg_; }
  void vg(const double& v) { vg_ = v; }
  double ic() const { return ic_; }
  void ic(const double& i) { ic_ = i; }
  std::vector<double> cpr() const { return cpr_; }
  void cpr(const std::vector<double>& i) { cpr_ = i; }
  int64_t rtype() const { return rtype_; }
  void rtype(const int64_t& r) { rtype_ = r; }
  double rn() const { return rn_; }
  void rn(const double& r) { rn_ = r; }
  double r0() const { return r0_; }
  void r0(const double& r) { r0_ = r; }
  double c() const { return c_; }
  void c(const double& c) { c_ = c; }
  double t() const { return t_; }
  void t(const double& t) { t_ = t; }
  double tc() const { return tc_; }
  void tc(const double& t) { tc_ = t; }
  double deltaV() const { return deltaV_; }
  void deltaV(const double& d) { deltaV_ = d; }
  double d() const { return d_; }
  void d(const double& t) { d_ = t; }
  double icFct() const { return icFct_; }
  void icFct(const double& r) { icFct_ = r; }
  double phiOff() const { return phiOff_; }
  void phiOff(const double& o) { phiOff_ = o; }
  bool tDep() { return tDep_; }
  void tDep(bool b) { tDep_ = b; }
  static void parse_model(const std::pair<tokens_t, string_o>& s,
                          vector_pair_t<Model, string_o>& models,
                          const param_map& p);
};
}  // namespace JoSIM

#endif