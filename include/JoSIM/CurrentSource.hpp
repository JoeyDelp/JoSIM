// Copyright (c) 2020 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_CURRENTSOURCE_HPP
#define JOSIM_CURRENTSOURCE_HPP

#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <string>

namespace JoSIM {
class CurrentSource {
  private:
    std::string label_;
    int_o posIndex_, negIndex_;
    int sourceIndex_;
  
  public:
    CurrentSource() :
      sourceIndex_(-1)
      {};
      
    static CurrentSource create_currentsource(const std::pair<std::string, std::string> &s,
                                              const std::unordered_map<std::string, int> &nm,
                                              std::unordered_set<std::string> &lm);
    void set_label(const std::string &s, 
                    std::unordered_set<std::string> &lm);
    void set_indices(const std::pair<std::string, std::string> &n, 
                      const std::unordered_map<std::string, int> &nm);
    void set_sourceIndex(const int &i) { sourceIndex_ = i; }

    std::string get_label() const { return label_; }
    int_o get_posIndex() const { return posIndex_; }
    int_o get_negIndex() const { return negIndex_; }
    int get_sourceIndex() const { return sourceIndex_; }
};

} // namespace JoSIM
#endif