// Copyright (c) 2020 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_INTEGRATIONTYPE_HPP
#define JOSIM_INTEGRATIONTYPE_HPP

#include <stdexcept>

namespace JoSIM {

enum class IntegrationType : int { Trapezoidal = 0, Gear = 1 };

constexpr IntegrationType integration_type_from_int(IntegrationType type) {
  switch (type) {
  case IntegrationType::Trapezoidal:
    return IntegrationType::Trapezoidal;
  case IntegrationType::Gear:
    return IntegrationType::Gear;
  default:
    throw std::runtime_error("Invalid analysis type");
  }
}

} // namespace JoSIM

#endif // JOSIM_INTEGRATIONTYPE_HPP
