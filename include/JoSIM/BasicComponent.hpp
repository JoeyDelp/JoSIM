// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_BASICCOMPONENT_H
#define JOSIM_BASICCOMPONENT_H

#include <cmath>
#include <unordered_map>
#include <vector>

#include "JoSIM/Errors.hpp"
#include "JoSIM/TypeDefines.hpp"

namespace JoSIM {
enum class NodeConfig { GND = 0, POSGND = 1, GNDNEG = 2, POSNEG = 3 };

using nodemap = std::unordered_map<std::string, int64_t>;
using nodeconnections = std::vector<std::vector<std::pair<double, int64_t>>>;

class NetlistInfo {
 public:
  std::string label_;
  double value_ = 0.0;
};

class IndexInfo {
 public:
  int_o posIndex_, negIndex_, currentIndex_;
  NodeConfig nodeConfig_ = NodeConfig::GND;
};

class MatrixInfo {
 public:
  std::vector<double> nonZeros_;
  std::vector<int64_t> columnIndex_;
  std::vector<int64_t> rowPointer_;
};

class BasicComponent {
 public:
  NetlistInfo netlistInfo;
  IndexInfo indexInfo;
  MatrixInfo matrixInfo;

  void set_node_indices(const tokens_t& t, const nodemap& nm,
                        nodeconnections& nc) {
    switch (indexInfo.nodeConfig_) {
      case NodeConfig::POSGND:
        indexInfo.posIndex_ = nm.at(t.at(0));
        nc.at(nm.at(t.at(0)))
            .emplace_back(std::make_pair(1, indexInfo.currentIndex_.value()));
        break;
      case NodeConfig::GNDNEG:
        indexInfo.negIndex_ = nm.at(t.at(1));
        nc.at(nm.at(t.at(1)))
            .emplace_back(std::make_pair(-1, indexInfo.currentIndex_.value()));
        break;
      case NodeConfig::POSNEG:
        indexInfo.posIndex_ = nm.at(t.at(0));
        indexInfo.negIndex_ = nm.at(t.at(1));
        nc.at(nm.at(t.at(0)))
            .emplace_back(std::make_pair(1, indexInfo.currentIndex_.value()));
        nc.at(nm.at(t.at(1)))
            .emplace_back(std::make_pair(-1, indexInfo.currentIndex_.value()));
        break;
      case NodeConfig::GND:
        break;
    }
  }

  void set_matrix_info() {
    switch (indexInfo.nodeConfig_) {
      case NodeConfig::POSGND:
        matrixInfo.nonZeros_.emplace_back(1);
        matrixInfo.columnIndex_.emplace_back(indexInfo.posIndex_.value());
        matrixInfo.rowPointer_.emplace_back(2);
        break;
      case NodeConfig::GNDNEG:
        matrixInfo.nonZeros_.emplace_back(-1);
        matrixInfo.columnIndex_.emplace_back(indexInfo.negIndex_.value());
        matrixInfo.rowPointer_.emplace_back(2);
        break;
      case NodeConfig::POSNEG:
        matrixInfo.nonZeros_.emplace_back(1);
        matrixInfo.nonZeros_.emplace_back(-1);
        matrixInfo.columnIndex_.emplace_back(indexInfo.posIndex_.value());
        matrixInfo.columnIndex_.emplace_back(indexInfo.negIndex_.value());
        matrixInfo.rowPointer_.emplace_back(3);
        break;
      case NodeConfig::GND:
        matrixInfo.rowPointer_.emplace_back(1);
        break;
    }
    matrixInfo.columnIndex_.emplace_back(indexInfo.currentIndex_.value());
    sanity_check();
  }

  void sanity_check() {
    for (auto& i : matrixInfo.nonZeros_) {
      if (i == 0 || std::isinf(i) || std::isnan(i)) {
        Errors::matrix_errors(MatrixErrors::SANITY_ERROR, netlistInfo.label_);
      }
    }
  }

  virtual void update_timestep(const double& factor){};

  virtual void step_back(){};

  virtual ~BasicComponent() {}

};  // class BasicComponent

}  // namespace JoSIM
#endif