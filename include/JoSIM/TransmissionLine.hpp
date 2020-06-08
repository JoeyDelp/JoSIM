// Copyright (c) 2020 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_TRANSMISSIONLINE_HPP
#define JOSIM_TRANSMISSIONLINE_HPP

#include "JoSIM/ParameterName.hpp"
#include "JoSIM/Parameters.hpp"
#include "JoSIM/AnalysisType.hpp"
#include "JoSIM/IntegrationType.hpp"

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <optional>

namespace JoSIM {
class TransmissionLine {
  private:
    std::string label_;
    std::vector<double> nonZeros_;
    std::vector<int> columnIndex_;
    std::vector<int> rowPointer_;
    int_o posIndex1_, negIndex1_;
    int_o posIndex2_, negIndex2_;
    int currentIndex1_;
    int currentIndex2_;
    double value_;
    double p1n2_, p2n2_, p1nk2_, p2nk2_;
    double dp1n1_, dp1n2_, dv2n1_, dp2n2_;
    double dp1nk_, dp1nk1_, dp2nk_, dp2nk1_;
    int timestepDelay_;
  public:
    TransmissionLine() :
    currentIndex1_(-1), currentIndex2_(-1), 
    value_(0),
    p1n2_(0), p2n2_(0), p1nk2_(0), p2nk2_(0),
    dp1n1_(0), dp1n2_(0), dv2n1_(0), dp2n2_(0),
    dp1nk_(0), dp1nk1_(0), dp2nk_(0), dp2nk1_(0),
    timestepDelay_(0)
    {};
    
    static TransmissionLine create_transmissionline(const std::pair<std::string, std::string> &s,
                                                    const std::unordered_map<std::string, int> &nm, 
                                                    std::unordered_set<std::string> &lm,
                                                    std::vector<std::vector<std::pair<double, int>>> &nc,
                                                    const std::unordered_map<ParameterName, Parameter> &p,
                                                    const AnalysisType &antyp,
                                                    const IntegrationType & inttyp,
                                                    const double &timestep,
                                                    int &branchIndex);
    void set_label(const std::string &s, 
                   std::unordered_set<std::string> &lm);
    void set_nonZeros_and_columnIndex(const std::pair<std::string, std::string> &n1, 
                                      const std::pair<std::string, std::string> &n2,
                                      const std::unordered_map<std::string, int> &nm, 
                                      const std::string &s, int &branchIndex);
    void set_indices(const std::pair<std::string, std::string> &n1, 
                     const std::pair<std::string, std::string> &n2, 
                     const std::unordered_map<std::string, int> &nm, 
                     std::vector<std::vector<std::pair<double, int>>> &nc, 
                     const int &branchIndex);
    void set_currentIndex1(const int &cc) { currentIndex1_ = cc; }
    void set_currentIndex2(const int &cc) { currentIndex2_ = cc; }
    void set_value(const std::pair<std::string, std::string> &s, 
                    const std::unordered_map<ParameterName, Parameter> &p,
                    const AnalysisType &antyp, const double &timestep);
    void set_value_gear(const std::pair<std::string, std::string> &s, 
                        const std::unordered_map<ParameterName, Parameter> &p,
                        const AnalysisType &antyp, const double &timestep);
    void set_timestepDelay(const std::pair<std::string, std::string> &s, 
                            const std::unordered_map<ParameterName, Parameter> &p,
                            const double &timestep);
    void set_p1n2(const double &v) { p1n2_ = v; }
    void set_p2n2(const double &v) { p2n2_ = v; }
    void set_p1nk2(const double &v) { p1nk2_ = v; }
    void set_p2nk2(const double &v) { p2nk2_ = v; }
    void set_dp1n1(const double &v) { dp1n1_ = v; }
    void set_dp1n2(const double &v) { dp1n2_ = v; }
    void set_dp2n1(const double &v) { dv2n1_ = v; }
    void set_dp2n2(const double &v) { dp2n2_ = v; }
    void set_dp1nk(const double &v) { dp1nk_ = v; }
    void set_dp1nk1(const double &v) { dp1nk1_ = v; }
    void set_dp2nk(const double &v) { dp2nk_ = v; }
    void set_dp2nk1(const double &v) { dp2nk1_ = v; }

    const std::string& get_label() const { return label_; }
    const std::vector<double>& get_nonZeros() const { return nonZeros_; }
    const std::vector<int>& get_columnIndex() const { return columnIndex_; }
    const std::vector<int>& get_rowPointer() const { return rowPointer_;}
    const int_o& get_posIndex() const { return posIndex1_; }
    const int_o& get_negIndex() const { return negIndex1_; }
    const int_o& get_posIndex2() const { return posIndex2_; }
    const int_o& get_negIndex2() const { return negIndex2_; }
    const int& get_currentIndex() const { return currentIndex1_; }
    const int& get_currentIndex2() const { return currentIndex2_; }
    const double& get_value() const { return value_; }
    const int& get_timestepDelay() const { return timestepDelay_; }
    const double& get_p1n2() const { return p1n2_; }
    const double& get_p2n2() const { return p2n2_; }
    const double& get_p1nk2() const { return p1nk2_; }
    const double& get_p2nk2() const { return p2nk2_; }
    const double& get_dp1n1() const { return dp1n1_; }
    const double& get_dp1n2() const { return dp1n2_; }
    const double& get_dp2n1() const { return dv2n1_; }
    const double& get_dp2n2() const { return dp2n2_; }
    const double& get_dp1nk() const { return dp1nk_; }
    const double& get_dp1nk1() const { return dp1nk1_; }
    const double& get_dp2nk() const { return dp2nk_; }
    const double& get_dp2nk1() const { return dp2nk1_; }
};
} // namespace JoSIM

#endif