// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_ANALYSISTYPE_HPP
#define JOSIM_ANALYSISTYPE_HPP

#include <stdexcept>

namespace JoSIM {

enum class AnalysisType : int { Voltage = 0, Phase = 1 };

constexpr AnalysisType analysis_type_from_int(AnalysisType type) {
  switch (type) {
  case AnalysisType::Voltage:
    return AnalysisType::Voltage;
  case AnalysisType::Phase:
    return AnalysisType::Phase;
  default:
    throw std::runtime_error("Invalid analysis type");
  }
}

} // namespace JoSIM

#endif // JOSIM_ANALYSISTYPE_HPP
