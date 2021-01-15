// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_INPUTTYPE_HPP
#define JOSIM_INPUTTYPE_HPP

#include <stdexcept>

namespace JoSIM {

enum class InputType { Jsim = 0, WrSpice = 1 };

constexpr InputType input_type_from_int(InputType type) {
  switch (type) {
  case InputType::Jsim:
    return InputType::Jsim;
  case InputType::WrSpice:
    return InputType::WrSpice;
  default:
    throw std::runtime_error("Invalid input type");
  }
}

} // namespace JoSIM

#endif // JOSIM_INPUTTYPE_HPP
