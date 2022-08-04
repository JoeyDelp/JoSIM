// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_TYPEDEFINES_HPP
#define JOSIM_TYPEDEFINES_HPP

#include <optional>
#include <string>
#include <vector>

namespace JoSIM {

// Define a type that specifies tokens
using tokens_t = std::vector<std::string>;

// Define types to shorthand optional types
using string_o = std::optional<std::string>;
using char_o = std::optional<char>;
using double_o = std::optional<double>;
using int_o = std::optional<int64_t>;

// Define a type that is a vector of pairs of type T1 and T2
template <typename T1, typename T2>
using vector_pair_t = std::vector<std::pair<T1, T2>>;

}  // namespace JoSIM

#endif  // JOSIM_TYPEDEFINES_HPP
