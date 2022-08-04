// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_RELEVANTTRACES_HPP
#define JOSIM_RELEVANTTRACES_HPP

#include "JoSIM/Errors.hpp"
#include "JoSIM/Misc.hpp"
#include "JoSIM/TypeDefines.hpp"

namespace JoSIM {

class Matrix;
class Input;
enum class StorageType : int64_t { Voltage = 0, Phase = 1, Current = 2 };

struct target_less {
  template <class It>
  bool operator()(It const& a, It const& b) const {
    return *a < *b;
  }
};

struct target_equal {
  template <class It>
  bool operator()(It const& a, It const& b) const {
    return *a == *b;
  }
};

template <class It>
It uniquify(It begin, It const end) {
  std::vector<It> v;
  v.reserve(static_cast<size_t>(std::distance(begin, end)));
  for (It i = begin; i != end; ++i) {
    v.push_back(i);
  }
  std::sort(v.begin(), v.end(), target_less());
  v.erase(std::unique(v.begin(), v.end(), target_equal()), v.end());
  std::sort(v.begin(), v.end());
  size_t j = 0;
  for (It i = begin; i != end && j != v.size(); ++i) {
    if (i == v[j]) {
      using std::iter_swap;
      iter_swap(i, begin);
      ++j;
      ++begin;
    }
  }
  return begin;
}

class RelevantTrace {
 public:
  bool device;
  string_o deviceLabel;
  StorageType storageType = StorageType::Voltage;
  int_o index1;
  int_o index2;
  int_o sourceIndex;
  int_o variableIndex;
  int64_t fIndex = -1;

  RelevantTrace() : device(false){};
};  // class RelevantTrace

void find_relevant_traces(Input& iObj, Matrix& mObj);
void handle_current(const std::string& s, Matrix& mObj, int64_t fIndex);
void handle_voltage_or_phase(const std::string& s, bool voltage, Matrix& mObj,
                             int64_t fIndex);

}  // namespace JoSIM

#endif