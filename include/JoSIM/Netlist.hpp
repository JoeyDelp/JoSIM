// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_NETLIST_HPP
#define JOSIM_NETLIST_HPP

#include <string>
#include <unordered_map>
#include <vector>

#include "JoSIM/Model.hpp"
#include "JoSIM/Parameters.hpp"
#include "JoSIM/TypeDefines.hpp"

typedef std::unordered_map<std::string, std::string> s_map;

namespace JoSIM {
struct pair_hash {
  template <class T1, class T2>
  std::size_t operator()(const std::pair<T1, T2>& p) const noexcept {
    std::size_t h1 = std::hash<T1>{}(p.first);
    std::size_t h2 = std::hash<T2>{}(p.second);
    return h1 ^ (h2 << 1);
  }
};

class Subcircuit {
 public:
  tokens_t io;
  std::vector<std::pair<tokens_t, string_o>> lines;
  tokens_t subckts;
  int64_t jjCount, compCount, subcktCounter;
  bool containsSubckt;
  Subcircuit()
      : jjCount(0),
        compCount(0),
        subcktCounter(0),
        containsSubckt(false){

        };
};

class Input;

class Netlist {
 private:
  void id_io_subc_label(
      const tokens_t& lineTokens, tokens_t& io, s_map& params,
      std::string& subcktName, std::string& label,
      const std::unordered_map<std::string, Subcircuit>& subcircuits);
  bool rename_io_nodes(std::string& node, const tokens_t& subIO,
                       const tokens_t& parentIO);
  void expand_io(Subcircuit& subc, s_map& params, const tokens_t& io,
                 const std::string& label);
  void insert_parameter(tokens_t& t, s_map& params);

 public:
  std::unordered_map<std::pair<std::string, string_o>, tokens_t, pair_hash>
      models;
  std::vector<std::pair<Model, string_o>> models_new;
  std::unordered_map<std::string, Subcircuit> subcircuits;
  std::unordered_map<std::string, int64_t> subcktLookup;
  std::vector<tokens_t> maindesign;
  tokens_t subckts;
  std::vector<std::pair<tokens_t, string_o>> expNetlist;
  int64_t jjCount, compCount, subcktCounter, nestedSubcktCount, subcktTotal = 0;
  bool containsSubckt, argMin = false;
  Netlist()
      : jjCount(0),
        compCount(0),
        subcktCounter(0),
        nestedSubcktCount(0),
        containsSubckt(false){};
  void expand_subcircuits();
  void expand_maindesign();
};

}  // namespace JoSIM
#endif