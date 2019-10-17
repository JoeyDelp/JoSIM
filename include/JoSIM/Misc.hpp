// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_J_MISC_H
#define JOSIM_J_MISC_H

#include "./Parser.hpp"
#include "./Input.hpp"

#include <unordered_map>
#include <map>
#include <sstream>
#include <iomanip>

class Input;

class Misc {
public:
  static std::string file_from_path(const std::string &path);

  static bool has_suffix(const std::string &str, const std::string &suffix);

  static bool starts_with(const std::string &input, char test);

  static std::vector<std::string> tokenize_space(const std::string &c);

  static std::vector<std::string> tokenize_space_once(const std::string &c);

  static std::vector<std::string> tokenize_delimeter(const std::string &c,
                                                     const std::string &d);

  static void ltrim(std::string &s);

  static void rtrim(std::string &s);

  static int map_value_count(const std::unordered_map<std::string, int> &map,
                             int value);

  static double modifier(const std::string &value);

  static void unique_push(std::vector<std::string> &vector,
                          const std::string &string);

  static int index_of(const std::vector<std::string> &vector,
                      const std::string &value);

  static std::string substring_after(const std::string &str,
                                     const std::string &whatpart);

  static std::string substring_before(const std::string &str,
                                      const std::string &whatpart);

  static std::vector<double> parse_function(std::string &str, Input &iObj,
                                            const std::string &subckt = "");

  static bool findX(const std::vector<std::string> &segment,
                    std::string &theLine, int &linePos);

  template <typename A, typename B>
  static std::pair<B, A> flip_pair(const std::pair<A, B> &p) {
    return std::pair<B, A>(p.second, p.first);
  }

  template <typename A, typename B>
  static std::map<B, A> flip_map(const std::unordered_map<A, B> &src) {
    std::map<B, A> dst;
    std::transform(src.begin(), src.end(), std::inserter(dst, dst.begin()),
                   flip_pair<A, B>);
    return dst;
  }

  template <typename T>
  static std::string precise_to_string(const T a_value, const int n = 50) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(n) << a_value;
    return out.str();
  }

  static int numDigits(int number);

  static double grand();
};
#endif
