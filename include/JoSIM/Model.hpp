// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_MODEL_H
#define JOSIM_MODEL_H

#include "./Constants.hpp"
#include "./Parameters.hpp"

namespace JoSIM {
class Model {
  private:
    std::string modelName_;
    double voltageGap_;
    double criticalCurrent_;
    int resistanceType_;
    double normalResistance_;
    double subgapResistance_;
    double capacitance_;
    double temperature_;
    double criticalTemperature_;
    double deltaV_;
    double transparency_;
    double criticalToNormalRatio_;
    double phaseOffset_;

  public:
    Model() : 
      voltageGap_(2.8E-3),
      criticalCurrent_(1E-3),
      resistanceType_(1),
      normalResistance_(5),
      subgapResistance_(30),
      capacitance_(2.5E-12),
      temperature_(4.2),
      criticalTemperature_(9.1),
      deltaV_(0.1E-3),
      transparency_(0),
      criticalToNormalRatio_(Constants::PI / 4),
      phaseOffset_(0)
      {};

    std::string get_modelName() const { return modelName_; }
    double get_voltageGap() const { return voltageGap_; }
    double get_criticalCurrent() const { return criticalCurrent_; }
    int get_resistanceType() const { return resistanceType_; }
    double get_normalResistance() const { return normalResistance_; }
    double get_subgapResistance() const { return subgapResistance_; }
    double get_capacitance() const { return capacitance_; }
    double get_temperature() const { return temperature_; }
    double get_criticalTemperature() const { return criticalTemperature_; }
    double get_deltaV() const { return deltaV_; }
    double get_transparency() const { return transparency_; }
    double get_criticalToNormalRatio() const { return criticalToNormalRatio_; }
    double get_phaseOffset() const { return phaseOffset_; }

    void set_modelName(const std::string &n) { modelName_ = n; }
    void set_voltageGap(const double &v) { voltageGap_ = v; }
    void set_criticalCurrent(const double &i) { criticalCurrent_ = i; }
    void set_resistanceType(const int &r) { resistanceType_ = r; }
    void set_normalResistance(const double &r) { normalResistance_ = r; }
    void set_subgapResistance(const double &r) { subgapResistance_ = r; }
    void set_capacitance(const double &c) { capacitance_ = c; }
    void set_temperature(const double &t) { temperature_ = t; }
    void set_criticalTemperature(const double &t) { criticalTemperature_ = t; }
    void set_deltaV(const double &d) { deltaV_ = d; }
    void set_transparency(const double &t) { transparency_ = t; }
    void set_criticalToNormalRatio(const double &r) { criticalToNormalRatio_ = r; }
    void set_phaseOffset(const double &o) { phaseOffset_ = o; }

    static void parse_model(
        const std::pair<std::string, std::string> &s,
        std::vector<std::pair<Model,std::string>> &models,
        const std::unordered_map<ParameterName, Parameter> &p);
};
} // namespace JoSIM

#endif