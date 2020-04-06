// Copyright (c) 2020 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_CCCS_HPP
#define JOSIM_CCCS_HPP

#include "JoSIM/ParameterName.hpp"
#include "JoSIM/Parameters.hpp"
#include "JoSIM/AnalysisType.hpp"

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <optional>

namespace JoSIM {
class CCCS {
  private:
    std::string label_;
    std::vector<double> nonZeros_;
    std::vector<int> columnIndex_;
    std::vector<int> rowPointer_;
    std::optional<int> posIndex1_, negIndex1_;
    std::optional<int> posIndex2_, negIndex2_;
    int currentIndex_;
    double value_;
  public:
    CCCS() :
      currentIndex_(-1),
      value_(1)
      {};
    
    static CCCS create_CCCS(const std::pair<std::string, std::string> &s,
                            const std::unordered_map<std::string, int> &nm, 
                            std::unordered_set<std::string> &lm,
                            std::vector<std::vector<std::pair<double, int>>> &nc,
                            const std::unordered_map<ParameterName, Parameter> &p,
                            int &branchIndex);
    void set_label(const std::string &s, 
                    std::unordered_set<std::string> &lm);
    void set_nonZeros_and_columnIndex(const std::pair<std::string, std::string> &n1, 
                                      const std::pair<std::string, std::string> &n2, 
                                      const std::unordered_map<std::string, int> &nm, 
                                      const std::string &s, 
                                      int &branchIndex);
    void set_indices(const std::pair<std::string, std::string> &n1, 
                      const std::pair<std::string, std::string> &n2, 
                      const std::unordered_map<std::string, int> &nm, 
                      std::vector<std::vector<std::pair<double, int>>> &nc, 
                      const int &branchIndex);
    void set_currentIndex(const int &cc) { currentIndex_ = cc; }
    void set_value(const std::pair<std::string, std::string> &s, 
                    const std::unordered_map<ParameterName, Parameter> &p);

    const std::string& get_label() const { return label_; }
    const std::vector<double>& get_nonZeros() const { return nonZeros_; }
    const std::vector<int>& get_columnIndex() const { return columnIndex_; }
    const std::vector<int>& get_rowPointer() const { return rowPointer_;}
    const std::optional<int>& get_posIndex() const { return posIndex1_; }
    const std::optional<int>& get_negIndex() const { return negIndex1_; }
    const std::optional<int>& get_posIndex2() const { return posIndex2_; }
    const std::optional<int>& get_negIndex2() const { return negIndex2_; }
    const int& get_currentIndex() const { return currentIndex_; }
    const double& get_value() const { return value_; }

};

} // namespace JoSIM
#endif