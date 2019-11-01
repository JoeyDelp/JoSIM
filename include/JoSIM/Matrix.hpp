// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_J_MATRIX_H
#define JOSIM_J_MATRIX_H

#include "./AnalysisType.hpp"
#include "./Components.hpp"
#include "./Input.hpp"
#include "./Errors.hpp"

#include <unordered_map>

struct target_less
{
    template<class It>
    bool operator()(It const &a, It const &b) const { return *a < *b; }
};
struct target_equal
{
    template<class It>
    bool operator()(It const &a, It const &b) const { return *a == *b; }
};
template<class It> It uniquify(It begin, It const end)
{
    std::vector<It> v;
    v.reserve(static_cast<size_t>(std::distance(begin, end)));
    for (It i = begin; i != end; ++i)
    { v.push_back(i); }
    std::sort(v.begin(), v.end(), target_less());
    v.erase(std::unique(v.begin(), v.end(), target_equal()), v.end());
    std::sort(v.begin(), v.end());
    size_t j = 0;
    for (It i = begin; i != end && j != v.size(); ++i)
    {
        if (i == v[j])
        {
            using std::iter_swap; iter_swap(i, begin);
            ++j;
            ++begin;
        }
    }
    return begin;
}

class matrix_element {
public:
  std::string label;
  int rowIndex;
  int colIndex;
  double value;
  matrix_element() {
    label = "NOTHING";
    rowIndex = -1;
    colIndex = -1;
    value = 0.0;
  }
};

struct RowDescriptor {
  enum class Type {
    VoltageNode,
    PhaseNode,
    VoltageResistor,
    PhaseResistor,
    VoltageCapacitor,
    PhaseCapacitor,
    VoltageInductor,
    PhaseInductor,
    VoltageJJ,
    PhaseJJ,
    VoltageVS,
    PhaseVS,
    VoltageCS,
    PhaseCS,
    VoltagePS,
    PhasePS,
    VoltageTX,
    VoltageTX1,
    VoltageTX2,
    PhaseTX,
    PhaseTX1,
    PhaseTX2
  } type;
  int index;

  RowDescriptor() {}
};

struct LabelNodes {
  std::string posNode;
  std::string negNode;
};

class Matrix {
public:
  JoSIM::AnalysisType analysisType;
  Components components;
  std::vector<NodeConnections> nodeConnections;
  std::vector<RowDescriptor> rowDesc;
  std::unordered_map<std::string, RowDescriptor> deviceLabelIndex;
  std::unordered_map<std::string, int> nodeMap, relToXMap;
  std::vector<matrix_element> mElements;
  std::vector<std::vector<double>> sources;
  std::vector<std::string> rowNames, columnNames, relevantToStore;
  std::vector<double> nzval;
  std::vector<int> colind, rowptr;
  std::vector<int> relXInd;
  std::unordered_map<int, int> XtoTraceMap;
  std::unordered_map<std::string, LabelNodes> labelNodes;
  int Nsize, Msize;

  Components_new components_new;
  std::unordered_map<std::string, int> nm;
  std::vector<int> nc;
  
  Matrix(){};
  void create_matrix(Input &iObj);

  void create_A_volt(Input &iObj);

  void create_A_phase(Input &iObj);

  void create_CSR();

  void find_relevant_x(Input &iObj);
};
#endif
