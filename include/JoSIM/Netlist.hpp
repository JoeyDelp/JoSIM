// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_NETLIST_HPP
#define JOSIM_NETLIST_HPP

#include "./Parameters.hpp"
#include "./InputType.hpp"
#include "./Model.hpp"

#include <string>
#include <vector>
#include <unordered_map>

struct pair_hash {
  template <class T1, class T2>
  std::size_t operator()(const std::pair<T1, T2> &p) const noexcept {
    std::size_t h1 = std::hash<T1>{}(p.first);
    std::size_t h2 = std::hash<T2>{}(p.second);
    return h1 ^ (h2 << 1);
  }
};

class Subcircuit {
public:
  std::vector<std::string> io;
  std::vector<std::pair<std::string, std::string>> lines;
  std::vector<std::string> subckts;
  int jjCount, compCount, subcktCounter;
  bool containsSubckt;
  Subcircuit() :
    jjCount(0),
    compCount(0),
    subcktCounter(0),
    containsSubckt(false)
  {

  };
};

class Netlist {
public:
  std::unordered_map<std::pair<std::string, std::string>, std::string,
                     pair_hash>
      models;
  std::vector<std::pair<JoSIM::Model, std::string>> models_new;
  std::unordered_map<std::string, Subcircuit> subcircuits;
  std::unordered_map<std::string, int> subcktLookup;
  std::vector<std::string> maindesign;
  std::vector<std::string> subckts;
  std::vector<std::pair<std::string, std::string>> expNetlist;
  int jjCount, compCount, subcktCounter, nestedSubcktCount;
  bool containsSubckt;
  JoSIM::InputType argConv;
  Netlist() :
    jjCount(0),
    compCount(0),
    subcktCounter(0),
    nestedSubcktCount(0),
    containsSubckt(false),
    argConv(JoSIM::InputType::Jsim)
  {
  
  };
  void expand_subcircuits();
  void expand_maindesign();
};

#endif