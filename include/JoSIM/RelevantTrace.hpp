// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_RELEVANTTRACES_HPP
#define JOSIM_RELEVANTTRACES_HPP

#include "./Misc.hpp"
#include "./Errors.hpp"


namespace JoSIM {
  class Matrix;
  enum class StorageType : int { Voltage = 0, Phase = 1, Current = 2};
}

struct target_less
{
    template<class It> bool operator()(It const &a, It const &b) const { return *a < *b; }
};
struct target_equal
{
    template<class It> bool operator()(It const &a, It const &b) const { return *a == *b; }
};
template<class It> It uniquify(It begin, It const end)
{
    std::vector<It> v;
    v.reserve(static_cast<size_t>(std::distance(begin, end)));
    for (It i = begin; i != end; ++i)
    { v.push_back(i); }
    std::sort(v.begin(), v.end(), target_less());
    v.erase(std::unique(v.begin(), v.end(), target_equal()), v.end());
    std::sort(v.begin(), v.end());
    size_t j = 0;
    for (It i = begin; i != end && j != v.size(); ++i)
    {
        if (i == v[j])
        {
            using std::iter_swap; iter_swap(i, begin);
            ++j;
            ++begin;
        }
    }
    return begin;
}

namespace JoSIM {
class RelevantTrace {
  public:
    bool device;
    std::optional<std::string> deviceLabel;
    JoSIM::StorageType storageType;
    std::optional<int> index1;
    std::optional<int> index2;
    std::optional<int> sourceIndex;

    RelevantTrace() :
      device(false)
    {};

    static void find_relevant_traces(const std::vector<std::string> &c, JoSIM::Matrix &mObj);
    static void handle_current(const std::string &s, JoSIM::Matrix &mObj);
    static void handle_voltage_or_phase(const std::string &s, bool voltage, JoSIM::Matrix &mObj);

};
}

#endif