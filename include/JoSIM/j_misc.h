// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef J_MISC_H_
#define J_MISC_H_
#include "j_std_include.h"
#include "j_input.h"
#include "j_parser.h"

class Input;

class Misc {
  public:

    static
    std::string
    file_from_path(const std::string& path);

    static
    bool
    has_suffix(const std::string& str, const std::string& suffix);

    static
    bool
    starts_with(std::string input, char test);

    static
    std::vector<std::string>
    tokenize_space(const std::string& c);

    static
    std::vector<std::string>
    tokenize_delimeter(const std::string& c, const std::string& d);

    static
    int
    map_value_count(std::unordered_map<std::string, int> map, int value);

    static
    double
    modifier(const std::string& value);

    static
    void
    unique_push(std::vector<std::string>& vector, const std::string& string);

    static
    int
    index_of(std::vector<std::string> vector, const std::string& value);

    static
    std::string
    substring_after(const std::string& str, const std::string& whatpart);

    static
    std::string
    substring_before(std::string str, const std::string& whatpart);

    static
    std::vector<double>
    parse_function(std::string &str, Input &iObj, std::string subckt = "");

    static
    bool
    findX(std::vector<std::string>& segment, std::string& theLine, int &linePos);

    template<typename A, typename B>
    static
    std::pair<B, A>
    flip_pair(const std::pair<A, B>& p)
    {
      return std::pair<B, A>(p.second, p.first);
    }

    template<typename A, typename B>
    static
    std::map<B, A>
    flip_map(const std::unordered_map<A, B>& src)
    {
      std::map<B, A> dst;
      std::transform(
        src.begin(), src.end(), std::inserter(dst, dst.begin()), flip_pair<A, B>);
      return dst;
    }

    template<typename T>
    static
    std::string
    precise_to_string(const T a_value, const int n = 50)
    {
      std::ostringstream out;
      out << std::fixed << std::setprecision(n) << a_value;
      return out.str();
    }

    static
    std::string
    stringSubtract(const std::string& src, const std::string& comp);

    static
    int numDigits(int number);
};
#endif
