// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_FUNCTION_HPP
#define JOSIM_FUNCTION_HPP

#include <vector>
#include "Input.hpp"

namespace JoSIM {

class Function {
  public:
    static std::vector<double> parse_function(const std::string &str, JoSIM::Input &iObj,
                                              const std::string &subckt);  
    static void parse_pwl(const std::vector<std::string>  &tokens, std::vector<double> &functionOfT, 
                          const JoSIM::Input &iObj, const std::string &subckt);
    static void parse_pulse(const std::vector<std::string> &tokens, std::vector<double> &functionOfT, 
                            const JoSIM::Input &iObj, const std::string &subckt);
    static void parse_sinusoid(const std::vector<std::string> &tokens, std::vector<double> &functionOfT, 
                                const JoSIM::Input &iObj, const std::string &subckt); 
    static void parse_custom(const std::vector<std::string> &tokens, std::vector<double> &functionOfT, 
                              const JoSIM::Input &iObj, const std::string &subckt);
    static void parse_noise(const std::vector<std::string> &tokens, std::vector<double> &functionOfT, 
                            const JoSIM::Input &iObj, const std::string &subckt);
    static void parse_pws(const std::vector<std::string> &tokens, std::vector<double> &functionOfT, 
                          const JoSIM::Input &iObj, const std::string &subckt);  
    static void voltage_to_phase(std::vector<double> &source, const JoSIM::Input &iObj);
    static void phase_to_voltage(std::vector<double> &source, const JoSIM::Input &iObj);
};

} // namespace JoSIM

#endif // JOSIM_FUNCTION_HPP
