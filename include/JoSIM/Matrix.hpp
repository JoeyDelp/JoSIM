// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_MATRIX_HPP
#define JOSIM_MATRIX_HPP

#include <unordered_map>
#include <unordered_set>

#include "JoSIM/AnalysisType.hpp"
#include "JoSIM/Components.hpp"
#include "JoSIM/Errors.hpp"
#include "JoSIM/Input.hpp"
#include "JoSIM/RelevantTrace.hpp"
#include "JoSIM/Spread.hpp"

namespace JoSIM {

class Matrix {
 private:
  std::vector<NodeConfig> nodeConfig, nodeConfig2;
  bool needsTR_ = true;

 public:
  AnalysisType analysisType = AnalysisType::Phase;
  std::vector<Function> sourcegen;

  Components components;
  Spread spread;
  std::unordered_map<std::string, int64_t> nm;
  nodeconnections nc;
  std::unordered_set<std::string> lm;
  int64_t branchIndex = 0;
  std::vector<double> nz;
#ifdef SLU
  std::vector<long long> ci, rp;
#else
  std::vector<int64_t> ci, rp;
#endif
  std::vector<RelevantTrace> relevantTraces;
  std::vector<int64_t> relevantIndices;

  Matrix(){};
  void create_matrix(Input& iObj);
  void setup(Input& iObj);
  void create_components(Input& iObj);
  void handle_mutual_inductance(Input& iObj);
  void reduce_step(Input& iObj);
  void create_csr();
  void create_nz();
  void create_ci();
  void create_rp();
};
}  // namespace JoSIM
#endif
