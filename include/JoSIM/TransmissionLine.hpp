// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_TRANSMISSIONLINE_HPP
#define JOSIM_TRANSMISSIONLINE_HPP

#include "./ParameterName.hpp"
#include "./Parameters.hpp"

#include <vector>
#include <unordered_map>
#include <optional>

class TransmissionLine {
  private:
    std::string label_;
    std::vector<double> nonZeros_;
    std::vector<int> columnIndex_;
    std::optional<int> posIndex1_, negIndex1_;
    std::optional<int> posIndex2_, negIndex2_;
    int currentIndex1_;
    int currentIndex2_;
    double value_;
    int timestepDelay_;
  public:
    TransmissionLine() {};
    
    static void create_transmissionline(
        const std::pair<std::string, std::string> &s,
        std::vector<TransmissionLine> &transmissionlines, 
        const std::unordered_map<std::string, int> &nm, 
        std::vector<int> &nc,
        const std::unordered_map<JoSIM::ParameterName, Parameter> &p,
        const int &antyp,
        const double &timestep);
    void set_label(const std::string &l) { label_ = l; }
    void set_nonZeros_and_columnIndex(const std::pair<std::string, std::string> &n1, 
        const std::pair<std::string, std::string> &n2,
        const std::unordered_map<std::string, int> &nm, 
        const std::string &s, std::vector<int> &nc);
    void set_indices(const std::pair<std::string, std::string> &n1, 
        const std::pair<std::string, std::string> &n2, 
        const std::unordered_map<std::string, int> &nm, std::vector<int> &nc);
    void set_currentIndex1(const int &cc) { currentIndex1_ = cc; }
    void set_currentIndex2(const int &cc) { currentIndex2_ = cc; }
    void set_value(const std::pair<std::string, std::string> &s, 
        const std::unordered_map<JoSIM::ParameterName, Parameter> &p,
        const int &antyp, const double &timestep);
    void set_timestepDelay(const std::pair<std::string, std::string> &s, 
        const std::unordered_map<JoSIM::ParameterName, Parameter> &p,
        const double &timestep);

    std::string get_label() const { return label_; }
    std::vector<double> get_nonZeros() const { return nonZeros_; }
    std::vector<int> get_columnIndex() const { return columnIndex_; }
    std::optional<int> get_posIndex1() const { return posIndex1_; }
    std::optional<int> get_negIndex1() const { return negIndex1_; }
    std::optional<int> get_posIndex2() const { return posIndex2_; }
    std::optional<int> get_negIndex2() const { return negIndex2_; }
    int get_currentIndex1() const { return currentIndex1_; }
    int get_currentIndex2() const { return currentIndex2_; }
    double get_value() const { return value_; }
    int get_timestepDelay() const { return timestepDelay_; }

};

#endif