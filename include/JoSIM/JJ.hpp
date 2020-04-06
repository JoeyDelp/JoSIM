// Copyright (c) 2020 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_JJ_HPP
#define JOSIM_JJ_HPP

#include "JoSIM/ParameterName.hpp"
#include "JoSIM/Parameters.hpp"
#include "JoSIM/Model.hpp"
#include "JoSIM/AnalysisType.hpp"
#include "JoSIM/IntegrationType.hpp"

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <optional>

namespace JoSIM {
class JJ {
  private:
    std::string label_;
    std::vector<double> nonZeros_;
    std::vector<int> columnIndex_;
    std::vector<int> rowPointer_;
    std::optional<int> posIndex_, negIndex_;
    int currentIndex_;
    int variableIndex_;
    double area_;
    Model model_;
    double value_;
    double phaseConst_;

    double lowerB_, upperB_, subCond_, transCond_, normalCond_, gLarge_;
    double del0_, del_, rncalc_;
    double pn1_, phi0_;
    double vn1_, vn2_;
    double dvn1_, dvn2_;
    double transitionCurrent_;
    
  public:
    JJ() :
      currentIndex_(-1),
      variableIndex_(-1),
      area_(1),
      value_(0.0),
      phaseConst_(0),
      lowerB_(0), upperB_(0), subCond_(0), transCond_(0), normalCond_(0),
      del0_(0), del_(0), rncalc_(0),
      pn1_(0), vn1_(0), vn2_(0), dvn1_(0), dvn2_(0),
      transitionCurrent_(0)
      {};
    
    static JJ create_jj(const std::pair<std::string, std::string> &s,
                        const std::unordered_map<std::string, int> &nm, 
                        std::unordered_set<std::string> &lm,
                        std::vector<std::vector<std::pair<double, int>>> &nc,
                        const std::unordered_map<ParameterName, Parameter> &p,
                        const std::vector<std::pair<Model, std::string>> &models,
                        const AnalysisType &antyp,
                        const IntegrationType & inttyp,
                        const double &timestep,
                        int &branchIndex);
    void set_label(const std::string &s, 
            std::unordered_set<std::string> &lm);
    void set_nonZeros_and_columnIndex(const std::pair<std::string, std::string> &n, 
                                      const std::unordered_map<std::string, int> &nm, 
                                      const std::string &s, int &branchIndex,
                                      const AnalysisType &antyp, 
                                      const double &timestep);
    void set_indices(const std::pair<std::string, std::string> &n, 
                      const std::unordered_map<std::string, int> &nm, 
                      std::vector<std::vector<std::pair<double, int>>> &nc, 
                      const int &branchIndex);
    void set_currentIndex(const int &cc) { currentIndex_ = cc; }
    void set_variableIndex(const int &vc) { variableIndex_ = vc; }
    void set_area(const std::pair<std::string, std::string> &s, 
                  const std::unordered_map<ParameterName, Parameter> &p);
    void set_model(const std::pair<std::string, std::string> &s, 
                    const std::vector<std::pair<Model, std::string>> &models);
    void set_value(const double &v) { value_ = v; };
    void set_phaseConst(const double &timestep, 
                        const AnalysisType &antyp);
    void set_pn1(const double &v) { pn1_ = v; }
    void set_phi0(const double &v) { phi0_ = v; }
    void set_vn1(const double &v) { vn1_ = v; }
    void set_vn2(const double &v) { vn2_ = v; }
    void set_dvn1(const double &v) { dvn1_ = v; }
    void set_dvn2(const double &v) { dvn2_ = v; }
    void set_gLarge(const double &v) { gLarge_ = v; }
    void set_subCond(const double &v) { subCond_ = v; }
    void set_transCond(const double &v) { transCond_ = v; }
    void set_normalCond(const double &v) { normalCond_ = v; }
    void set_del0(const double &v) { del0_ = v; }
    void set_del(const double &v) { del_ = v; }
    void set_rncalc(const double &v) { rncalc_ = v; }
    bool update_value(const double &v);

    const std::string& get_label() const { return label_; }
    const std::vector<double>& get_nonZeros() const { return nonZeros_; }
    const std::vector<int>& get_columnIndex() const { return columnIndex_; }
    const std::vector<int>& get_rowPointer() const { return rowPointer_;}
    const std::optional<int>& get_posIndex() const { return posIndex_; }
    const std::optional<int>& get_negIndex() const { return negIndex_; }
    const int& get_currentIndex() const { return currentIndex_; }
    const int& get_variableIndex() const { return variableIndex_; }
    const double& get_area() const { return area_; }
    const double& get_value() const { return value_; }
    const double& get_pn1() const { return pn1_; }
    const double& get_phi0() const { return phi0_; }
    const double& get_vn1() const { return vn1_; }
    const double& get_vn2() const { return vn2_; }
    const double& get_dvn1() const { return dvn1_; }
    const double& get_dvn2() const { return dvn2_; }
    const Model& get_model() const { return model_; }
    const double& get_gLarge() const { return gLarge_; }
    const double& get_subCond() const { return subCond_; }
    const double& get_transCond() const { return transCond_; }
    const double& get_normalCond() const { return normalCond_; }
    const double& get_del0() const { return del0_; }
    const double& get_del() const { return del_; }
    const double& get_rncalc() const { return rncalc_; }
    const double& get_transitionCurrent() const { return transitionCurrent_; }
};

} // namespace JoSIM
#endif