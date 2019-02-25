#ifndef JOSIM_PARAMETERNAME_HPP
#define JOSIM_PARAMETERNAME_HPP

#include <string>
#include <utility>

namespace JoSIM {

class ParameterName {
  std::string name_;
  std::string subcircuit_;

public:
  ParameterName(std::string name, std::string subcircuit)
      : name_(std::move(name)), subcircuit_(std::move(subcircuit)) {
    // Empty
  }

  ParameterName(const ParameterName &other)
      : name_(other.name()), subcircuit_(other.subcircuit()) {
    // Empty
  }

  const std::string &name() const noexcept { return name_; }
  const std::string &subcircuit() const noexcept { return subcircuit_; }

  bool operator==(const ParameterName &other) const noexcept {
    return (name_ == other.name_ && subcircuit_ == other.subcircuit_);
  }
};

} // namespace JoSIM

namespace std {

template <> struct hash<JoSIM::ParameterName> {
  std::size_t operator()(const JoSIM::ParameterName &parameter_name) const {
    using std::hash;
    using std::string;

    auto hash_name = hash<string>()(parameter_name.name());
    auto hash_subcircuit = hash<string>()(parameter_name.subcircuit());

    return hash_name ^ (hash_subcircuit << 1);
  }
};

} // namespace std

#endif // JOSIM_PARAMETERNAME_HPP
