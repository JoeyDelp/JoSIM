// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_PARAMETERNAME_HPP
#define JOSIM_PARAMETERNAME_HPP

#include <optional>
#include <string>
#include <utility>

namespace JoSIM {

class ParameterName {
  std::string name_;
  std::optional<std::string> subcircuit_;

 public:
  ParameterName(std::string name, std::optional<std::string> subcircuit)
      : name_(std::move(name)), subcircuit_(std::move(subcircuit)) {
    // Empty
  }

  /// Get parameter name
  const std::string& name() const noexcept { return name_; }

  /// Get subcircuit name of parameter, nullopt if it is in global scope
  const std::optional<std::string>& subcircuit() const noexcept {
    return subcircuit_;
  }

  /// Equality comparison
  bool operator==(const ParameterName& other) const noexcept {
    return (name_ == other.name_ && subcircuit_ == other.subcircuit_);
  }
};

}  // namespace JoSIM

namespace std {

// Hash function for ParameterName
template <>
struct hash<JoSIM::ParameterName> {
  std::size_t operator()(const JoSIM::ParameterName& parameter_name) const {
    using std::hash;
    using std::string;

    auto hash_name = hash<string>()(parameter_name.name());
    auto hash_subcircuit =
        hash<std::optional<string>>()(parameter_name.subcircuit());

    return hash_name ^ (hash_subcircuit << 1);
  }
};

}  // namespace std

#endif  // JOSIM_PARAMETERNAME_HPP
