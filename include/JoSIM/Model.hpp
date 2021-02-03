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
    float voltageGap_;
    float criticalCurrent_;
    int resistanceType_;
    float normalResistance_;
    float subgapResistance_;
    float capacitance_;
    float temperature_;
    float criticalTemperature_;
    float deltaV_;
    float transparency_;
    float criticalToNormalRatio_;
    float phaseOffset_;

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
    float get_voltageGap() const { return voltageGap_; }
    float get_criticalCurrent() const { return criticalCurrent_; }
    int get_resistanceType() const { return resistanceType_; }
    float get_normalResistance() const { return normalResistance_; }
    float get_subgapResistance() const { return subgapResistance_; }
    float get_capacitance() const { return capacitance_; }
    float get_temperature() const { return temperature_; }
    float get_criticalTemperature() const { return criticalTemperature_; }
    float get_deltaV() const { return deltaV_; }
    float get_transparency() const { return transparency_; }
    float get_criticalToNormalRatio() const { return criticalToNormalRatio_; }
    float get_phaseOffset() const { return phaseOffset_; }

    void set_modelName(const std::string &n) { modelName_ = n; }
    void set_voltageGap(const float &v) { voltageGap_ = v; }
    void set_criticalCurrent(const float &i) { criticalCurrent_ = i; }
    void set_resistanceType(const int &r) { resistanceType_ = r; }
    void set_normalResistance(const float &r) { normalResistance_ = r; }
    void set_subgapResistance(const float &r) { subgapResistance_ = r; }
    void set_capacitance(const float &c) { capacitance_ = c; }
    void set_temperature(const float &t) { temperature_ = t; }
    void set_criticalTemperature(const float &t) { criticalTemperature_ = t; }
    void set_deltaV(const float &d) { deltaV_ = d; }
    void set_transparency(const float &t) { transparency_ = t; }
    void set_criticalToNormalRatio(const float &r) { 
      criticalToNormalRatio_ = r; }
    void set_phaseOffset(const float &o) { phaseOffset_ = o; }

    static void parse_model(
      const std::pair<tokens_t, string_o> &s, 
      vector_pair_t<Model, string_o> &models, const param_map &p);
};
} // namespace JoSIM

#endif