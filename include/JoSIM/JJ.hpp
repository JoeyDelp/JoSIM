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

    double prevPhase_;
    double prevVolt_;
    double phaseGuess_;
    double voltGuess_;
    
  public:
    JJ() :
      area_(1)
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
    void set_prevPhase(const double &v) { prevPhase_ = v; }
    void set_prevVolt(const double &v) { prevVolt_ = v; }
    void set_phaseGuess(const double &v) { phaseGuess_ = v; }
    void set_voltGuess(const double &v) { voltGuess_ = v; }

    const std::string& get_label() const { return label_; }
    const std::vector<double>& get_nonZeros() const { return nonZeros_; }
    const std::vector<int>& get_columnIndex() const { return columnIndex_; }
    const std::vector<int>& get_rowPointer() const { return rowPointer_;}
    const std::optional<int>& get_posIndex() const { return posIndex_; }
    const std::optional<int>& get_negIndex() const { return negIndex_; }
    const int& get_currentIndex() const { return currentIndex_; }
    const int& get_variableIndex() const { return variableIndex_; }
    const double& get_area() const { return area_; }
    const double& get_prevPhase() const { return prevPhase_; }
    const double& get_prevVolt() const { return prevVolt_; }
    const double& get_phaseGuess() const { return phaseGuess_; }
    const double& get_voltGuess() const { return voltGuess_; }
};

#endif