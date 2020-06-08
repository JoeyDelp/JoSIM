// Copyright (c) 2020 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_J_MATRIX_H
#define JOSIM_J_MATRIX_H

#include "JoSIM/AnalysisType.hpp"
#include "JoSIM/Components.hpp"
#include "JoSIM/Input.hpp"
#include "JoSIM/Errors.hpp"
#include "JoSIM/RelevantTrace.hpp"

#include <unordered_map>
#include <unordered_set>

namespace JoSIM {

class Matrix {
  void print_progess(const int &i, const int &size);
  public:
  AnalysisType analysisType;
  std::vector<std::vector<double>> sources;

  Components components;
  std::unordered_map<std::string, int> nm;
  nodeconnections nc;
  std::unordered_set<std::string> lm;
  int branchIndex;
  std::vector<double> nz, nz05, nz2, nz4, nz8, nz16;
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
