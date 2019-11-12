// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_JJ_HPP
#define JOSIM_JJ_HPP

#include "./ParameterName.hpp"
#include "./Parameters.hpp"
#include "./Model.hpp"

#include <vector>
#include <unordered_map>
#include <optional>

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

    double gLarge_, lowerB_, upperB_, subCond_, transCond_, normalCond_;
    double pn1_;
    double vn1_;
    double dvn1_;
    
  public:
    JJ() :
      area_(1),
      vn1_(0),
      dvn1_(0),
      {};
    
    static JJ create_jj(
        const std::pair<std::string, std::string> &s,
        const std::unordered_map<std::string, int> &nm, 
        std::vector<std::vector<std::pair<int, int>>> &nc,
        const std::unordered_map<JoSIM::ParameterName, Parameter> &p,
        const std::vector<std::pair<Model, std::string>> &models,
        const int &antyp,
        const double &timestep,
        int &branchIndex);
    void set_label(const std::string &l) { label_ = l; }
    void set_nonZeros_and_columnIndex(const std::pair<std::string, std::string> &n, 
        const std::unordered_map<std::string, int> &nm, const std::string &s, int &branchIndex,
        const int &antyp, const double &timestep);
    void set_indices(const std::pair<std::string, std::string> &n, const std::unordered_map<std::string, int> &nm, std::vector<std::vector<std::pair<int, int>>> &nc, const int &branchIndex);
    void set_currentIndex(const int &cc) { currentIndex_ = cc; }
    void set_variableIndex(const int &vc) { variableIndex_ = vc; }
    void set_area(const std::pair<std::string, std::string> &s, 
        const std::unordered_map<JoSIM::ParameterName, Parameter> &p);
    void set_model(const std::pair<std::string, std::string> &s, const std::vector<std::pair<Model, std::string>> &models);
    void set_value(const double &timestep);
    void set_phaseConst(const double &timestep, const int &antyp);
    void set_pn1(const double &v) { pn1_ = v; }
    void set_vn1(const double &v) { vn1_ = v; }
    void set_dvn1(const double &v) { dvn1_ = v; }
    void update_value(const double &v) { value_ = v; }
    void set_gLarge(const double &v) { gLarge_ = v; }
    void set_lowerB(const double &v) { lowerB_ = v; }
    void set_upperB(const double &v) { upperB_ = v; }
    void set_subCond(const double &v) { subCond_ = v; }
    void set_transCond(const double &v) { transCond_ = v; }
    void set_normalCond(const double &v) { normalCond_ = v; }

    const std::string& get_label() const { return label_; }
    const std::vector<double>& get_nonZeros() const { return nonZeros_; }
    const std::vector<int>& get_columnIndex() const { return columnIndex_; }
    const std::vector<int>& get_rowPointer() const { return rowPointer_;}
    const std::optional<int>& get_posIndex() const { return posIndex_; }
    const std::optional<int>& get_negIndex() const { return negIndex_; }
    const int& get_currentIndex() const { return currentIndex_; }
    const int& get_variableIndex() const { return variableIndex_; }
    const double& get_area() const { return area_; }
    const double& get_pn1() const { return pn1_; }
    const double& get_vn1() const { return vn1_; }
    const double& get_dvn1() const { return dvn1_; }
    const Model& get_model() const { return model_; }
};

#endif