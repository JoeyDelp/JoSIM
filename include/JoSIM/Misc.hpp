// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_MISC_HPP
#define JOSIM_MISC_HPP

#include <algorithm>
#include <iomanip>
#include <map>
#include <regex>
#include <sstream>
#include <unordered_map>

#include "JoSIM/Input.hpp"
#include "JoSIM/Parameters.hpp"
#include "JoSIM/TypeDefines.hpp"

namespace JoSIM {
class Input;
namespace Misc {
double string_constant(const std::string& s);

bool isclose(const double& a, const double& b);

std::string file_from_path(const std::string& path);

bool has_suffix(const std::string& str, const std::string& suffix);

bool starts_with(const std::string& input, char test);

// View vector of strings as a string with a default delimiter
std::string vector_to_string(const tokens_t& s, std::string d = " ");

// Split a string into tokens with a default delimiter
tokens_t tokenize(const std::string& c, std::string d = " \t",
                  bool trimEmpty = true, bool trimSpaces = false,
                  int64_t count = 0);

void ltrim(std::string& s);

void rtrim(std::string& s);

double modifier(const std::string& value);

void unique_push(std::vector<std::string>& vector, const std::string& string);

int64_t index_of(const std::vector<std::string>& vector,
                 const std::string& value);

std::string substring_after(const std::string& str,
                            const std::string& whatpart);

std::string substring_before(const std::string& str,
                             const std::string& whatpart);

bool findX(const std::vector<std::string>& segment, std::string& theLine,
           int64_t& linePos);

template <typename A, typename B>
std::pair<B, A> flip_pair(const std::pair<A, B>& p) {
  return std::pair<B, A>(p.second, p.first);
}

template <typename A, typename B>
std::map<B, A> flip_map(const std::unordered_map<A, B>& src) {
  std::map<B, A> dst;
  std::transform(src.begin(), src.end(), std::inserter(dst, dst.begin()),
                 flip_pair<A, B>);
  return dst;
}

template <typename T>
std::string precise_to_string(const T a_value) {
  std::ostringstream out;
  out << std::uppercase
      << std::setprecision(std::numeric_limits<long double>::digits10 + 1)
      << a_value;
  return out.str();
}

int64_t numDigits(int64_t number);

double grand();
}  // namespace Misc
}  // namespace JoSIM
#endif
