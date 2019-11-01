// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_CURRENTSOURCE_HPP
#define JOSIM_CURRENTSOURCE_HPP

#include <optional>
#include <unordered_map>
#include <string>

class CurrentSource {
  private:
    std::string label_;
    std::optional<int> posIndex_, negIndex_;
    int sourceIndex_;
  
  public:
    static void create_currentsource(
        const std::pair<std::string, std::string> &s,
        std::vector<CurrentSource> &currentsource, 
        const std::unordered_map<std::string, int> &nm);
    void set_label(const std::string &l) { label_ = l; }
    void set_indices(const std::pair<std::string, std::string> &n, const std::unordered_map<std::string, int> &nm);
    void set_sourceIndex(const int &i) { sourceIndex_ = i; }

    std::string get_label() const { return label_; }
    std::optional<int> get_posIndex() const { return posIndex_; }
    std::optional<int> get_negIndex() const { return negIndex_; }
    int get_sourceIndex() const { return sourceIndex_; }
};

#endif