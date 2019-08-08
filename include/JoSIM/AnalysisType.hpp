#ifndef JOSIM_ANALYSISTYPE_HPP
#define JOSIM_ANALYSISTYPE_HPP

#include <stdexcept>

namespace JoSIM {

enum class AnalysisType : int { Voltage = 0, Phase = 1 };

constexpr AnalysisType analysis_type_from_int(int type) {
  switch (type) {
  case static_cast<int>(AnalysisType::Voltage):
    return AnalysisType::Voltage;
  case static_cast<int>(AnalysisType::Phase):
    return AnalysisType::Phase;
  default:
    throw std::runtime_error("Invalid analysis type");
  }
}

} // namespace JoSIM

#endif // JOSIM_ANALYSISTYPE_HPP
