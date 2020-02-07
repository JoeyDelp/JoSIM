// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/Function.hpp"
#include "JoSIM/Misc.hpp"

#include <fstream>
#include <cmath>
#include <cassert>

std::vector<double> JoSIM::Function::parse_function(const std::string &str, JoSIM::Input &iObj,
                                         const std::string &subckt) {
  std::vector<double> functionOfT(iObj.transSim.get_simsize(), 0.0);
  std::vector<std::string> tokens;
  auto first = str.find('(') + 1;
  auto last = str.find(')');
  std::string params = str.substr(first, last - first);
  tokens = Misc::tokenize_delimiter(params, " ,");
  /* PWL(0 0 T1 V1 T2 V2 ... TN VN) */
  if (str.find("PWL") != std::string::npos) {
    parse_pwl(tokens, functionOfT, iObj, subckt);
  }
  /* PULSE */
  else if (str.find("PULSE") != std::string::npos) {
    parse_pulse(tokens, functionOfT, iObj, subckt);
  }
  /* SIN(VO VA <FREQ <TD <THETA>>>) */
  else if (str.find("SIN") != std::string::npos) {
    parse_sinusoid(tokens, functionOfT, iObj, subckt);
  }
  /* CUSTOM: CUS(WaveFile.dat TS SF IM <TD PER>) */
  else if (str.find("CUS") != std::string::npos) {
    parse_custom(tokens, functionOfT, iObj, subckt);
  }
  /* NOISE(VO VA TSTEP TD) */
  else if (str.find("NOISE") != std::string::npos) {
    parse_noise(tokens, functionOfT, iObj, subckt);
  }
  /* PWS(0 0 T1 V1 T2 V2 .. TN VN) */
  else if (str.find("PWS") != std::string::npos) {
    parse_pws(tokens, functionOfT, iObj, subckt);
  }
  return functionOfT;
}

void JoSIM::Function::parse_pwl(const std::vector<std::string> &tokens, std::vector<double> &functionOfT, 
                                const JoSIM::Input &iObj, const std::string &subckt) {
  std::vector<double> timesteps, values;
  if (std::stod(tokens.at(0)) != 0.0 || std::stod(tokens.at(1)) != 0.0) {
    Errors::function_errors(FunctionErrors::INITIAL_VALUES, tokens.at(0) + " & " + tokens.at(1));
  } else {
    timesteps.push_back(0.0);
    values.push_back(0.0);
  }
  for (int i = 0; i < tokens.size(); i = i + 2) {
    timesteps.push_back(Misc::modifier(tokens.at(i)));
  }
  for (int i = 1; i < tokens.size(); i = i + 2) {
    if (iObj.parameters.count(
            JoSIM::ParameterName(tokens.at(i), subckt)) != 0) {
      values.push_back(iObj.parameters.at(
          JoSIM::ParameterName(tokens.at(i), subckt)).get_value().value());
    } else if (iObj.parameters.count(
            JoSIM::ParameterName(tokens.at(i), "")) != 0) {
      values.push_back(iObj.parameters.at(
          JoSIM::ParameterName(tokens.at(i), "")).get_value().value());
    } else {
      values.push_back(JoSIM::Parameters::parse_param(
          tokens.at(i), iObj.parameters, subckt));
    }
  }
  if (timesteps.size() < values.size()) {
    Errors::function_errors(FunctionErrors::TOO_FEW_TIMESTEPS,
                            std::to_string(timesteps.size()) +
                                " timesteps & " +
                                std::to_string(timesteps.size()) + " values");
  }                                
  if (timesteps.size() > values.size()) {
    Errors::function_errors(
        FunctionErrors::TOO_FEW_VALUES, std::to_string(timesteps.size()) + " timesteps & " +
                            std::to_string(timesteps.size()) + " values");
  }
  if ((timesteps.back() > iObj.transSim.get_tstop()) &&
      (values.back() > values.at(values.size() - 2))) {
    values.at(values.size() - 1) = (iObj.transSim.get_tstop() / timesteps.back()) *
                                (values.back() - values.at(values.size() - 2));
    timesteps.at(timesteps.size() - 1) = iObj.transSim.get_tstop();
  } else if ((timesteps.back() > iObj.transSim.get_tstop()) &&
              (values.back() == values.at(values.size() - 2))) {
    timesteps.at(timesteps.size() - 1) = iObj.transSim.get_tstop();
  }
  if (values.at(values.size() - 1) != 0.0) {
    std::fill(functionOfT.begin() +
                  timesteps.at(timesteps.size() - 1) / iObj.transSim.get_prstep(),
              functionOfT.end(), values.at(values.size() - 1));
  }
  double value = 0.0;
  if((timesteps.back() / iObj.transSim.get_prstep()) > iObj.transSim.get_simsize()) functionOfT.resize(int(floor(timesteps.back() / iObj.transSim.get_prstep())), 0.0);
  for (int i = 1; i < timesteps.size(); ++i) {
    double startpoint = floor(timesteps.at(i - 1) / iObj.transSim.get_prstep());
    double endpoint = floor(timesteps.at(i) / iObj.transSim.get_prstep());
    functionOfT.at(startpoint) = values.at(i - 1);
    for (int j = (int)startpoint + 1; j < (int)endpoint; ++j) {
      if (values.at(i - 1) < values.at(i)) {
        value = values.at(i - 1) + (values.at(i) - (values.at(i - 1))) /
                                        (endpoint - startpoint) *
                                        (j - (int)startpoint);
      } else if (values.at(i - 1) > values.at(i)) {
        value = values.at(i - 1) -
                ((values.at(i - 1) - (values.at(i))) /
                  (endpoint - startpoint) * (j - (int)startpoint));
      } else if (values.at(i - 1) == values.at(i)) {
        value = values.at(i);
      }
      functionOfT.at(j) = value;
    }
  }
}

void JoSIM::Function::parse_pulse(const std::vector<std::string> &tokens, std::vector<double> &functionOfT, 
                                  const JoSIM::Input &iObj, const std::string &subckt) {
  if (std::stod(tokens.at(0)) != 0.0) {
      Errors::function_errors(FunctionErrors::INITIAL_PULSE_VALUE, tokens.at(0));
  }
  if (tokens.size() < 7) {
    Errors::function_errors(FunctionErrors::PULSE_TOO_FEW_ARGUMENTS, std::to_string(tokens.size()));
  }
  double vPeak, timeDelay, timeRise, timeFall, pulseWidth, pulseRepeat;
  vPeak = JoSIM::Parameters::parse_param(tokens.at(1), iObj.parameters, subckt);
  if (vPeak == 0.0) {
    if (iObj.argVerb) {
      Errors::function_errors(FunctionErrors::PULSE_VPEAK_ZERO, tokens.at(1));
    }
  }
  timeDelay = JoSIM::Parameters::parse_param(tokens.at(2), iObj.parameters, subckt);
  timeRise = JoSIM::Parameters::parse_param(tokens.at(3), iObj.parameters, subckt);
  timeFall = JoSIM::Parameters::parse_param(tokens.at(4), iObj.parameters, subckt);
  pulseWidth = JoSIM::Parameters::parse_param(tokens.at(5), iObj.parameters, subckt);
  if (pulseWidth == 0.0) {
    if (iObj.argVerb) {
      Errors::function_errors(FunctionErrors::PULSE_WIDTH_ZERO, tokens.at(5));
    }
  }
  pulseRepeat = Misc::modifier(tokens.at(6));
  if (pulseRepeat == 0.0) {
    if (iObj.argVerb) {
      Errors::function_errors(FunctionErrors::PULSE_REPEAT, tokens.at(6));
    }
  }
  int PR = pulseRepeat / iObj.transSim.get_prstep();
  int TD = timeDelay / iObj.transSim.get_prstep();
  std::vector<double> timesteps, values;
  for (int i = 0; i <= ((iObj.transSim.get_simsize() - TD) / PR); ++i) {
    double timestep = timeDelay + (pulseRepeat * i);
    if (timestep < iObj.transSim.get_tstop()) {
      timesteps.push_back(timestep);
    } else {
      break;
    }
    values.push_back(0.0);
    timestep = timeDelay + (pulseRepeat * i) + timeRise;
    if (timestep < iObj.transSim.get_tstop()) {
      timesteps.push_back(timestep);
    } else {
      break;
    }
    values.push_back(vPeak);
    timestep = timeDelay + (pulseRepeat * i) + timeRise + pulseWidth;
    if (timestep < iObj.transSim.get_tstop()) {
      timesteps.push_back(timestep);
    } else {
      break;
    }
    values.push_back(vPeak);
    timestep =
        timeDelay + (pulseRepeat * i) + timeRise + pulseWidth + timeFall;
    if (timestep < iObj.transSim.get_tstop()) {
      timesteps.push_back(timestep);
    } else {
      break;
    }
    values.push_back(0.0);
  }
  double value = 0;
  for (int i = 1; i < timesteps.size(); ++i) {
    double startpoint = floor(timesteps.at(i - 1) / iObj.transSim.get_prstep());
    double endpoint = floor(timesteps.at(i) / iObj.transSim.get_prstep());
    functionOfT.at(startpoint) = values.at(i - 1);
    for (int j = (int)startpoint + 1; j < (int)endpoint; ++j) {
      if (values.at(i - 1) < values.at(i)) {
        if (values.at(i - 1) < 0) {
          value = values.at(i - 1) + (values.at(i) - (values.at(i - 1))) /
                                          (endpoint - startpoint) *
                                          (j - (int)startpoint);
        } else {
          value = values.at(i) / (endpoint - startpoint) * (j - (int)startpoint);
        }
      } else if (values.at(i - 1) > values.at(i)) {
        value = values.at(i - 1) -
                ((values.at(i - 1) - (values.at(i))) /
                  (endpoint - startpoint) * (j - (int)startpoint));
      } else if (values.at(i - 1) == values.at(i)) {
        value = values.at(i);
      } else {
        assert(false);
      }
      functionOfT.at(j) = value;
    }
  }
}

void JoSIM::Function::parse_sinusoid(const std::vector<std::string> &tokens, std::vector<double> &functionOfT, 
                                      const JoSIM::Input &iObj, const std::string &subckt) {
  if (tokens.size() < 2) {
      Errors::function_errors(FunctionErrors::SIN_TOO_FEW_ARGUMENTS, std::to_string(tokens.size()));
  }
  if (tokens.size() > 5) {
    Errors::function_errors(FunctionErrors::SIN_TOO_MANY_ARGUMENTS, std::to_string(tokens.size()));
  }
  double VO = 0.0, VA = 0.0, TD = 0.0, FREQ = 1 / iObj.transSim.get_tstop(),
          THETA = 0.0;
  VO = JoSIM::Parameters::parse_param(tokens.at(0), iObj.parameters, subckt);
  VA = JoSIM::Parameters::parse_param(tokens.at(1), iObj.parameters, subckt);
  if (VA == 0.0) {
    if (iObj.argVerb) {
      Errors::function_errors(FunctionErrors::SIN_VA_ZERO, tokens.at(1));
    }
  }
  if (tokens.size() == 5) {
    FREQ = JoSIM::Parameters::parse_param(tokens.at(2), iObj.parameters, subckt);
    TD = JoSIM::Parameters::parse_param(tokens.at(3), iObj.parameters, subckt);
    THETA = JoSIM::Parameters::parse_param(tokens.at(4), iObj.parameters, subckt);
  } else if (tokens.size() == 4) {
    FREQ = JoSIM::Parameters::parse_param(tokens.at(2), iObj.parameters, subckt);
    TD = JoSIM::Parameters::parse_param(tokens.at(3), iObj.parameters, subckt);
  } else if (tokens.size() == 3) {
    FREQ = JoSIM::Parameters::parse_param(tokens.at(2), iObj.parameters, subckt);
  }
  int beginTime;
  beginTime = TD / iObj.transSim.get_prstep();

  assert(iObj.transSim.get_simsize() == functionOfT.size());
  for (int i = beginTime; i < iObj.transSim.get_simsize(); ++i) {
    double currentTimestep = i * iObj.transSim.get_prstep();
    double value = VO + VA * sin(2 * JoSIM::Constants::PI * FREQ * (currentTimestep - TD)) *
                      exp(-THETA * (currentTimestep - TD));
    functionOfT.at(i) = value;
  }
}

void JoSIM::Function::parse_custom(const std::vector<std::string> &tokens, std::vector<double> &functionOfT, 
                                    const JoSIM::Input &iObj, const std::string &subckt) {
  if (tokens.size() < 2){
    Errors::function_errors(FunctionErrors::CUS_TOO_FEW_ARGUMENTS, std::to_string(tokens.size()));
  }                            
  if (tokens.size() > 5) {
    Errors::function_errors(FunctionErrors::CUS_TOO_MANY_ARGUMENTS, std::to_string(tokens.size()));
  }
  std::string WFline = tokens.at(0);
  std::vector<std::string> WF;
  double TS = 0.0, SF = 0.0, TD = 0.0;
  int PER = 0;
  TS = JoSIM::Parameters::parse_param(tokens.at(1), iObj.parameters, subckt);
  SF = JoSIM::Parameters::parse_param(tokens.at(2), iObj.parameters, subckt);
  if (SF == 0.0) {
    if (iObj.argVerb) {
      Errors::function_errors(FunctionErrors::CUS_SF_ZERO, tokens.at(2)); 
    }
  }
  if (tokens.size() == 6) {
    TD = JoSIM::Parameters::parse_param(tokens.at(4), iObj.parameters, subckt);
    PER = stoi(tokens.at(5));
  } else if (tokens.size() == 5) {
    TD = JoSIM::Parameters::parse_param(tokens.at(4), iObj.parameters, subckt);
  }
  std::ifstream wffile(WFline);
  if (wffile.good()) {
    getline(wffile, WFline);
  } else {
    Errors::function_errors(FunctionErrors::CUS_WF_NOT_FOUND, WFline);
  }
  wffile.close();
  WF = Misc::tokenize_delimiter(WFline, " ,;");
  std::vector<double> timesteps, values;
  for (int i = 0; i < WF.size(); ++i) {
    values.push_back(Misc::modifier(WF.at(i)) * SF);
    timesteps.push_back(TD + i * TS);
  }
  if (TS < iObj.transSim.get_prstep()) {
    TS = iObj.transSim.get_prstep();
  }
  double functionSize = (iObj.transSim.get_tstop() - TD) / TS;
  if (PER == 1) {
    double repeats = functionSize / values.size();
    for (int j = 0; j < repeats; ++j) {
      double lastTimestep = timesteps.back() + TS;
      for (int i = 0; i < WF.size(); ++i) {
        values.push_back(Misc::modifier(WF.at(i)) * SF);
        timesteps.push_back(lastTimestep + i * TS);
      }
    }
  }
  double value = 0.0;
  for (int i = 1; i < timesteps.size(); ++i) {
    double startpoint = floor(timesteps.at(i - 1) / iObj.transSim.get_prstep());
    double endpoint = floor(timesteps.at(i) / iObj.transSim.get_prstep());
    functionOfT.at(startpoint) = values.at(i - 1);
    for (int j = (int)startpoint + 1; j < (int)endpoint; ++j) {
      if (values.at(i - 1) < values.at(i)) {
        if (values.at(i - 1) < 0) {
          value = values.at(i - 1) + (values.at(i) - (values.at(i - 1))) /
                                          (endpoint - startpoint) *
                                          (j - (int)startpoint);
        } else {
          value = values.at(i - 1) + (values.at(i) - (values.at(i - 1))) /
                                          (endpoint - startpoint) *
                                          (j - (int)startpoint);
        }
      } else if (values.at(i - 1) > values.at(i)) {
        value = values.at(i - 1) -
                ((values.at(i - 1) - (values.at(i))) /
                  (endpoint - startpoint) * (j - (int)startpoint));
      } else if (values.at(i - 1) == values.at(i)) {
        value = values.at(i);
      }
      functionOfT.at(j) = value;
    }
  }
}

void JoSIM::Function::parse_noise(const std::vector<std::string> &tokens, std::vector<double> &functionOfT, 
                                  const JoSIM::Input &iObj, const std::string &subckt) {
  if (tokens.size() < 2) {
      Errors::function_errors(FunctionErrors::NOISE_TOO_FEW_ARGUMENTS, std::to_string(tokens.size()));
  }
  if (tokens.size() > 4) {
    Errors::function_errors(FunctionErrors::NOISE_TOO_MANY_ARGUMENTS, std::to_string(tokens.size()));
  }
  double VO = 0.0, VA = 0.0, TD = 0.0, TSTEP = 0.0;
  VO = JoSIM::Parameters::parse_param(tokens.at(0), iObj.parameters, subckt);
  if(VO != 0.0 && tokens.size() == 4) {
    Errors::function_errors(FunctionErrors::NOISE_VO_ZERO, tokens.at(0));
    VA = JoSIM::Parameters::parse_param(tokens.at(1), iObj.parameters, subckt);
    if (VA == 0.0) {
      if (iObj.argVerb) {
        Errors::function_errors(FunctionErrors::NOISE_VA_ZERO, tokens.at(1));
      }
    }
    TD = JoSIM::Parameters::parse_param(tokens.at(3), iObj.parameters, subckt);
  } else {
    VA = JoSIM::Parameters::parse_param(tokens.at(0), iObj.parameters, subckt);
    if (VA == 0.0) {
      if (iObj.argVerb) {
        Errors::function_errors(FunctionErrors::NOISE_VA_ZERO, tokens.at(1));
      }
    }
    TD = JoSIM::Parameters::parse_param(tokens.at(2), iObj.parameters, subckt);
  }
  TSTEP = iObj.transSim.get_prstep();
  int beginTime;
  beginTime = TD / iObj.transSim.get_prstep();
  for (int i = beginTime; i < iObj.transSim.get_simsize(); ++i) {
    double currentTimestep = i * iObj.transSim.get_prstep();
    double value = VA * Misc::grand() / sqrt(2.0 * TSTEP);
    functionOfT.at(i) = value;
  }
}

void JoSIM::Function::parse_pws(const std::vector<std::string> &tokens, std::vector<double> &functionOfT, 
                                const JoSIM::Input &iObj, const std::string &subckt) {
  std::vector<double> timesteps, values;
  if (std::stod(tokens.at(0)) != 0.0 || std::stod(tokens.at(1)) != 0.0) {
    Errors::function_errors(FunctionErrors::INITIAL_VALUES, tokens.at(0) + " & " + tokens.at(1));
  } else {
    timesteps.push_back(0.0);
    values.push_back(0.0);
  }
  for (int i = 0; i < tokens.size(); i = i + 2) {
    timesteps.push_back(Misc::modifier(tokens.at(i)));
  }
  for (int i = 1; i < tokens.size(); i = i + 2) {
    if (iObj.parameters.count(
            JoSIM::ParameterName(tokens.at(i), subckt)) != 0) {
      values.push_back(iObj.parameters.at(
          JoSIM::ParameterName(tokens.at(i), subckt)).get_value().value());
    } else if (iObj.parameters.count(
            JoSIM::ParameterName(tokens.at(i), "")) != 0) {
      values.push_back(iObj.parameters.at(
          JoSIM::ParameterName(tokens.at(i), "")).get_value().value());
    } else {
      values.push_back(JoSIM::Parameters::parse_param(
          tokens.at(i), iObj.parameters, subckt));
    }
  }
  if (timesteps.size() < values.size()) {
    Errors::function_errors(FunctionErrors::TOO_FEW_TIMESTEPS, std::to_string(timesteps.size()) +
                                " timesteps & " + std::to_string(timesteps.size()) + " values");
  }
  if (timesteps.size() > values.size()) {
    Errors::function_errors(FunctionErrors::TOO_FEW_VALUES, std::to_string(timesteps.size()) + 
                                " timesteps & " + std::to_string(timesteps.size()) + " values");
  }
  if ((timesteps.back() > iObj.transSim.get_tstop()) &&
      (values.back() > values.at(values.size() - 2))) {
    values.at(values.size() - 1) = (iObj.transSim.get_tstop() / timesteps.back()) *
                                (values.back() - values.at(values.size() - 2));
    timesteps.at(timesteps.size() - 1) = iObj.transSim.get_tstop();
  } else if ((timesteps.back() > iObj.transSim.get_tstop()) &&
              (values.back() == values.at(values.size() - 2))) {
    timesteps.at(timesteps.size() - 1) = iObj.transSim.get_tstop();
  }
  if (values.at(values.size() - 1) != 0.0) {
    std::fill(functionOfT.begin() +
                  timesteps.at(timesteps.size() - 1) / iObj.transSim.get_prstep(),
              functionOfT.end(), values.at(values.size() - 1));
  }
  double value, ba;
  if((timesteps.back() / iObj.transSim.get_prstep()) > iObj.transSim.get_simsize()) functionOfT.resize(int(floor(timesteps.back() / iObj.transSim.get_prstep())), 0.0);
  for (int i = 1; i < timesteps.size(); ++i) {
    value = 0.0;
    double startpoint = floor(timesteps.at(i - 1) / iObj.transSim.get_prstep());
    double endpoint = floor(timesteps.at(i) / iObj.transSim.get_prstep());
    double period = (endpoint - startpoint) * 2;
    functionOfT.at(startpoint) = values.at(i - 1);
    for (int j = (int)startpoint + 1; j < (int)endpoint; ++j) {
      if (values.at(i - 1) < values.at(i)) {
        ba = (values.at(i) - values.at(i - 1)) / 2;
        value = values.at(i - 1) + ba * sin((2 * JoSIM::Constants::PI * (j - (period/4)))/period) + ba;
      } else if (values.at(i - 1) > values.at(i)) {
        ba = (values.at(i - 1) - values.at(i)) / 2;
        value = values.at(i - 1) - ba * sin((2 * JoSIM::Constants::PI * (j + (period/4)))/period) - ba;
      } else if (values.at(i - 1) == values.at(i)) {
        value = values.at(i);
      }
      functionOfT.at(j) = value;
    }
  }
}

void JoSIM::Function::voltage_to_phase(std::vector<double> &source, const JoSIM::Input &iObj) {
  double vn1, value;
  for(int i = 0; i < source.size(); i++) {
    if(i == 0) { 
      value = (iObj.transSim.get_prstep() / (2 * JoSIM::Constants::SIGMA)) * (source.at(i));
    } else { 
      value = (iObj.transSim.get_prstep() / (2 * JoSIM::Constants::SIGMA)) * (source.at(i) + vn1) + source.at(i - 1);
    }
    vn1 = source.at(i);
    source.at(i) = value;
  }
}

void JoSIM::Function::phase_to_voltage(std::vector<double> &source, const JoSIM::Input &iObj) {
  double pn1, value;
  for(int i = 0; i < source.size(); i++) {
    if(i == 0) {
      value = ((2 * JoSIM::Constants::SIGMA) / iObj.transSim.get_prstep()) * (source.at(i));
    } else {
      value = ((2 * JoSIM::Constants::SIGMA) / iObj.transSim.get_prstep()) * (source.at(i) - pn1) - source.at(i - 1);
    }
    pn1 = source.at(i);
    source.at(i) = value;
  }
}