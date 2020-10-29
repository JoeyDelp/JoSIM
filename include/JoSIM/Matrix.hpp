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
  public:
  AnalysisType analysisType;
  std::vector<Function> sourcegen;

  Components components;
  std::unordered_map<std::string, int> nm;
  nodeconnections nc;
  std::unordered_set<std::string> lm;
  int branchIndex;
  std::vector<double> nz, nz_orig;
  std::vector<int> ci, rp;
  std::vector<RelevantTrace> relevantTraces;
  std::vector<int> relevantIndices;
  
  Matrix(){};
  void create_matrix(Input &iObj);
  void create_csr();
  void create_nz();
  void create_ci();
  void create_rp();
  void mod_timestep(const int factor);
};
} // namespace JoSIM
#endif
