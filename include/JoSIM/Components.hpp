// Copyright (c) 2020 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_COMPONENTS_H
#define JOSIM_COMPONENTS_H

#include "JoSIM/Misc.hpp"
#include "JoSIM/Parameters.hpp"
#include "JoSIM/Constants.hpp"
#include "JoSIM/Input.hpp"
#include "JoSIM/Errors.hpp"
#include "JoSIM/Resistor.hpp"
#include "JoSIM/Inductor.hpp"
#include "JoSIM/Capacitor.hpp"
#include "JoSIM/JJ.hpp"
#include "JoSIM/VoltageSource.hpp"
#include "JoSIM/PhaseSource.hpp"
#include "JoSIM/CurrentSource.hpp"
#include "JoSIM/TransmissionLine.hpp"
#include "JoSIM/VCCS.hpp"
#include "JoSIM/CCCS.hpp"
#include "JoSIM/VCVS.hpp"
#include "JoSIM/CCVS.hpp"

#include <unordered_map>
#include <variant>
namespace JoSIM {

enum class NodeConfig { GND = 0, POSGND = 1, GNDNEG = 2, POSNEG = 3 };

using nodemap = std::unordered_map<std::string, int>;
using nodeconnections = std::vector<std::vector<std::pair<double, int>>>;

class Components {
  public:
    std::vector<std::variant<Resistor, 
                             Inductor, 
                             Capacitor,
                             JJ,
                             VoltageSource,
                             PhaseSource,
                             TransmissionLine,
                             VCCS,
                             CCCS,
                             VCVS,
                             CCVS>> devices; 
    std::vector<CurrentSource> currentsources;
    std::vector<int> junctionIndices, 
                      resistorIndices, 
                      inductorIndices, 
                      capacitorIndices, 
                      vsIndices, 
                      psIndices, 
                      txIndices, 
                      vccsIndices, 
                      cccsIndices, 
                      vcvsIndices, 
                      ccvsIndices;
    std::vector<std::pair<std::string, std::string>> mutualinductances;
};

class NetlistInfo {
  public:
  std::string label_;
  double value_;
};

class IndexInfo {
  public:
  int_o posIndex_, negIndex_, currentIndex_;
  NodeConfig nodeConfig_;
};

class MatrixInfo {
  public:
  std::vector<double> nonZeros_;
  std::vector<int> columnIndex_;
  std::vector<int> rowPointer_;
};

class BasicComponent {
  public:
  NetlistInfo netlistInfo;
  IndexInfo indexInfo;
  MatrixInfo matrixInfo;
  

  void set_node_indices(const tokens_t &t, const nodemap &nm) {
    switch(indexInfo.nodeConfig_) {
    case NodeConfig::POSGND:
      indexInfo.posIndex_ = nm.at(t.at(0));
      break;
    case NodeConfig::GNDNEG:
      indexInfo.negIndex_ = nm.at(t.at(1));
      break;
    case NodeConfig::POSNEG:
      indexInfo.posIndex_ = nm.at(t.at(0));
      indexInfo.negIndex_ = nm.at(t.at(1));
      break;
    }
  }

  void set_matrix_info() {
    switch(indexInfo.nodeConfig_) {
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
    matrixInfo.columnIndex_.emplace_back(indexInfo.currentIndex_);
  }

};
} // namespace JoSIM
#endif
