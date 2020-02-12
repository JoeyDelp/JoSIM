// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_J_MATRIX_H
#define JOSIM_J_MATRIX_H

#include "./AnalysisType.hpp"
#include "./Components.hpp"
#include "./Input.hpp"
#include "./Errors.hpp"
#include "./RelevantTrace.hpp"

#include <unordered_map>
#include <unordered_set>

namespace JoSIM {
class Matrix {
  public:
  AnalysisType analysisType;
  std::vector<std::vector<double>> sources;

  Components components;
  std::unordered_map<std::string, int> nm;
  std::vector<std::vector<std::pair<double, int>>> nc;
  std::unordered_set<std::string> lm;
  int branchIndex;
  std::vector<double> nz;
  std::vector<int> ci, rp;
  std::vector<RelevantTrace> relevantTraces;
  std::vector<int> relevantIndices;
  
  Matrix(){};
  void create_matrix(Input &iObj);
  void create_csr();
  void create_nz();
  void create_ci();
  void create_rp();
};
} // namespace JoSIM
#endif
