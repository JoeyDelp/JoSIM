// Copyright (c) 2020 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_CAPACITOR_HPP
#define JOSIM_CAPACITOR_HPP

#include "JoSIM/ParameterName.hpp"
#include "JoSIM/Parameters.hpp"
#include "JoSIM/AnalysisType.hpp"
#include "JoSIM/IntegrationType.hpp"

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <optional>

namespace JoSIM {

class Capacitor {
  private:
    std::string label_;
    std::vector<double> nonZeros_;
    std::vector<int> columnIndex_;
    std::vector<int> rowPointer_;
    std::optional<int> posIndex_, negIndex_;
    int currentIndex_;
    double value_;
    double capacitance_;
    double pn1_;
    double dpn1_;
  public:
    Capacitor() : 
      currentIndex_(-1),
      value_(0.0),
      capacitance_(0.0),
      pn1_(0.0),
      dpn1_(0.0)
      {};
    
    static Capacitor create_capacitor(const std::pair<std::string, std::string> &s,
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
    void set_nonZeros_and_columnIndex(const std::pair<std::string, std::string> &n, 
                                      const std::unordered_map<std::string, int> &nm, 
                                      const std::string &s, 
                                      int &branchIndex);
    void set_indices(const std::pair<std::string, std::string> &n, 
                      const std::unordered_map<std::string, int> &nm, 
                      std::vector<std::vector<std::pair<double, int>>> &nc, 
                      const int &branchIndex);
    void set_currentIndex(const int &cc) { currentIndex_ = cc; }
    void set_value(const std::pair<std::string, std::string> &s, 
                    const std::unordered_map<ParameterName, Parameter> &p,
                    const AnalysisType &antyp, 
                    const double &timestep);
    void set_value_gear(const std::pair<std::string, std::string> &s, 
                        const std::unordered_map<ParameterName, Parameter> &p,
                        const AnalysisType &antyp, 
                        const double &timestep);
    void set_pn1(const double &v) { pn1_ = v; }
    void set_dpn1(const double &v) { dpn1_ = v; }

    const std::string& get_label() const { return label_; }
    const std::vector<double>& get_nonZeros() const { return nonZeros_; }
    const std::vector<int>& get_columnIndex() const { return columnIndex_; }
    const std::vector<int>& get_rowPointer() const { return rowPointer_;}
    const std::optional<int>& get_posIndex() const { return posIndex_; }
    const std::optional<int>& get_negIndex() const { return negIndex_; }
    const int& get_currentIndex() const { return currentIndex_; }
    const double& get_value() const { return value_; }
    const double& get_capacitance() const { return capacitance_; }
    const double& get_pn1() const { return pn1_; }
    const double& get_dpn1() const { return dpn1_; }

};

} // namespace JoSIM
#endif