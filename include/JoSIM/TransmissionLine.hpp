// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_TRANSMISSIONLINE_HPP
#define JOSIM_TRANSMISSIONLINE_HPP

#include "./ParameterName.hpp"
#include "./Parameters.hpp"
#include "./AnalysisType.hpp"

#include <vector>
#include <unordered_map>
#include <optional>

class TransmissionLine {
  private:
    std::string label_;
    std::vector<double> nonZeros_;
    std::vector<int> columnIndex_;
    std::vector<int> rowPointer_;
    std::optional<int> posIndex1_, negIndex1_;
    std::optional<int> posIndex2_, negIndex2_;
    int currentIndex1_;
    int currentIndex2_;
    double value_;
    double dv1n1_, dv1n2_, dv2n1_, dv2n2_;
    double dv1nk_, dv1nk1_, dv2nk_, dv2nk1_;
    int timestepDelay_;
  public:
    TransmissionLine() :
    dv1n1_(0), dv1n2_(0), dv2n1_(0), dv2n2_(0),
    dv1nk_(0), dv1nk1_(0), dv2nk_(0), dv2nk1_(0)
    {};
    
    static TransmissionLine create_transmissionline(
        const std::pair<std::string, std::string> &s,
        const std::unordered_map<std::string, int> &nm, 
        std::vector<std::vector<std::pair<int, int>>> &nc,
        const std::unordered_map<JoSIM::ParameterName, Parameter> &p,
        const JoSIM::AnalysisType &antyp,
        const double &timestep,
        int &branchIndex);
    void set_label(const std::string &l) { label_ = l; }
    void set_nonZeros_and_columnIndex(const std::pair<std::string, std::string> &n1, 
        const std::pair<std::string, std::string> &n2,
        const std::unordered_map<std::string, int> &nm, 
        const std::string &s, int &branchIndex);
    void set_indices(const std::pair<std::string, std::string> &n1, 
        const std::pair<std::string, std::string> &n2, 
        const std::unordered_map<std::string, int> &nm, std::vector<std::vector<std::pair<int, int>>> &nc, const int &branchIndex);
    void set_currentIndex1(const int &cc) { currentIndex1_ = cc; }
    void set_currentIndex2(const int &cc) { currentIndex2_ = cc; }
    void set_value(const std::pair<std::string, std::string> &s, 
        const std::unordered_map<JoSIM::ParameterName, Parameter> &p,
        const JoSIM::AnalysisType &antyp, const double &timestep);
    void set_timestepDelay(const std::pair<std::string, std::string> &s, 
        const std::unordered_map<JoSIM::ParameterName, Parameter> &p,
        const double &timestep);
    void set_dv1n1(const double &v) { dv1n1_ = v; }
    void set_dv1n2(const double &v) { dv1n2_ = v; }
    void set_dv2n1(const double &v) { dv2n1_ = v; }
    void set_dv2n2(const double &v) { dv2n2_ = v; }
    void set_dv1nk(const double &v) { dv1nk_ = v; }
    void set_dv1nk1(const double &v) { dv1nk1_ = v; }
    void set_dv2nk(const double &v) { dv2nk_ = v; }
    void set_dv2nk1(const double &v) { dv2nk1_ = v; }

    const std::string& get_label() const { return label_; }
    const std::vector<double>& get_nonZeros() const { return nonZeros_; }
    const std::vector<int>& get_columnIndex() const { return columnIndex_; }
    const std::vector<int>& get_rowPointer() const { return rowPointer_;}
    const std::optional<int>& get_posIndex() const { return posIndex1_; }
    const std::optional<int>& get_negIndex() const { return negIndex1_; }
    const std::optional<int>& get_posIndex2() const { return posIndex2_; }
    const std::optional<int>& get_negIndex2() const { return negIndex2_; }
    const int& get_currentIndex() const { return currentIndex1_; }
    const int& get_currentIndex2() const { return currentIndex2_; }
    const double& get_value() const { return value_; }
    const int& get_timestepDelay() const { return timestepDelay_; }
    const double& get_dv1n1() const { return dv1n1_; }
    const double& get_dv1n2() const { return dv1n2_; }
    const double& get_dv2n1() const { return dv2n1_; }
    const double& get_dv2n2() const { return dv2n2_; }
    const double& get_dv1nk() const { return dv1nk_; }
    const double& get_dv1nk1() const { return dv1nk1_; }
    const double& get_dv2nk() const { return dv2nk_; }
    const double& get_dv2nk1() const { return dv2nk1_; }
};

#endif