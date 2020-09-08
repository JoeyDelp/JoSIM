// Copyright (c) 2020 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/Function.hpp"
#include "JoSIM/Misc.hpp"
#include "JoSIM/Parameters.hpp"

#include <fstream>
#include <cmath>
#include <cassert>

using namespace JoSIM;

std::vector<double> Function::parse_function(
      const std::string &str, Input &iObj, const string_o &subckt) {
  std::vector<double> functionOfT(iObj.transSim.get_simsize(), 0.0);
  std::vector<std::string> tokens;
  auto first = str.find('(') + 1;
  auto last = str.find(')');
  std::string params = str.substr(first, last - first);
  tokens = Misc::tokenize(params, " ,");
  /* PWL(0 0 T1 V1 T2 V2 ... TN VN) */
  if (str.find("PWL") != std::string::npos) {
    parse_pwl(tokens, functionOfT, iObj, subckt);
  }
  /* PULSE(A1 A2 <TD <TR <TF <PW <PER>>>>>)*/
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
  /* DC VALUE */
  else if (str.find("DC") != std::string::npos) {
    if (!std::isnan(parse_param(
      Misc::tokenize(str).back(), iObj.parameters, subckt))) {
      tokens = {"0", "0", Misc::precise_to_string(iObj.transSim.get_prstep()), 
        Misc::tokenize(str).back()};
      parse_pwl(tokens, functionOfT, iObj, subckt);
    }
  }
  /* Only a value was provided? Assume user is trying to make a DC source */
  else if (!std::isnan(parse_param(
      Misc::tokenize(str).back(), iObj.parameters, subckt))) {
    tokens = {"0", "0", Misc::precise_to_string(iObj.transSim.get_prstep()), 
      Misc::tokenize(str).back()};
    parse_pwl(tokens, functionOfT, iObj, subckt);
  }
  return functionOfT;
}

void Function::parse_pwl(
      const std::vector<std::string> &tokens, std::vector<double> &functionOfT, 
      const Input &iObj, const string_o &subckt) {
  std::vector<double> timesteps, values;
  if (std::stod(tokens.at(0)) != 0.0 || std::stod(tokens.at(1)) != 0.0) {
    Errors::function_errors(
      FunctionErrors::INITIAL_VALUES, tokens.at(0) + " & " + tokens.at(1));
    timesteps.push_back(0.0);
    values.push_back(0.0);
  }
  for (int i = 0; i < tokens.size(); i = i + 2) {
    timesteps.push_back(Misc::modifier(tokens.at(i)));
  }
  for (int i = 1; i < tokens.size(); i = i + 2) {
    values.push_back(parse_param(tokens.at(i), iObj.parameters, subckt));
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

void Function::parse_pulse(
      const std::vector<std::string> &tokens, std::vector<double> &functionOfT, 
      const Input &iObj, const string_o &subckt) {
  if (std::stod(tokens.at(0)) != 0.0) {
      Errors::function_errors(FunctionErrors::INITIAL_PULSE_VALUE, tokens.at(0));
  }
  if (tokens.size() < 7) {
    Errors::function_errors(FunctionErrors::PULSE_TOO_FEW_ARGUMENTS, std::to_string(tokens.size()));
  }
  double vPeak, timeDelay, timeRise, timeFall, pulseWidth, pulseRepeat;
  vPeak = parse_param(tokens.at(1), iObj.parameters, subckt);
  if (vPeak == 0.0) {
    if (iObj.argVerb) {
      Errors::function_errors(FunctionErrors::PULSE_VPEAK_ZERO, tokens.at(1));
    }
  }
  timeDelay = parse_param(tokens.at(2), iObj.parameters, subckt);
  timeRise = parse_param(tokens.at(3), iObj.parameters, subckt);
  timeFall = parse_param(tokens.at(4), iObj.parameters, subckt);
  pulseWidth = parse_param(tokens.at(5), iObj.parameters, subckt);
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

void Function::parse_sinusoid(
      const std::vector<std::string> &tokens, std::vector<double> &functionOfT, 
      const Input &iObj, const string_o &subckt) {
  if (tokens.size() < 2) {
      Errors::function_errors(FunctionErrors::SIN_TOO_FEW_ARGUMENTS, std::to_string(tokens.size()));
  }
  if (tokens.size() > 5) {
    Errors::function_errors(FunctionErrors::SIN_TOO_MANY_ARGUMENTS, std::to_string(tokens.size()));
  }
  double VO = 0.0, VA = 0.0, TD = 0.0, FREQ = 1 / iObj.transSim.get_tstop(),
          THETA = 0.0;
  VO = parse_param(tokens.at(0), iObj.parameters, subckt);
  VA = parse_param(tokens.at(1), iObj.parameters, subckt);
  if (VA == 0.0) {
    if (iObj.argVerb) {
      Errors::function_errors(FunctionErrors::SIN_VA_ZERO, tokens.at(1));
    }
  }
  if (tokens.size() == 5) {
    FREQ = parse_param(tokens.at(2), iObj.parameters, subckt);
    TD = parse_param(tokens.at(3), iObj.parameters, subckt);
    THETA = parse_param(tokens.at(4), iObj.parameters, subckt);
  } else if (tokens.size() == 4) {
    FREQ = parse_param(tokens.at(2), iObj.parameters, subckt);
    TD = parse_param(tokens.at(3), iObj.parameters, subckt);
  } else if (tokens.size() == 3) {
    FREQ = parse_param(tokens.at(2), iObj.parameters, subckt);
  }
  int beginTime;
  beginTime = TD / iObj.transSim.get_prstep();

  assert(iObj.transSim.get_simsize() == functionOfT.size());
  for (int i = beginTime; i < iObj.transSim.get_simsize(); ++i) {
    double currentTimestep = i * iObj.transSim.get_prstep();
    double value = VO + VA * sin(2 * Constants::PI * FREQ * (currentTimestep - TD)) *
                      exp(-THETA * (currentTimestep - TD));
    functionOfT.at(i) = value;
  }
}

void Function::parse_custom(
      const std::vector<std::string> &tokens, std::vector<double> &functionOfT, 
      const Input &iObj, const string_o &subckt) {
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
  TS = parse_param(tokens.at(1), iObj.parameters, subckt);
  SF = parse_param(tokens.at(2), iObj.parameters, subckt);
  if (SF == 0.0) {
    if (iObj.argVerb) {
      Errors::function_errors(FunctionErrors::CUS_SF_ZERO, tokens.at(2)); 
    }
  }
  if (tokens.size() == 6) {
    TD = parse_param(tokens.at(4), iObj.parameters, subckt);
    PER = stoi(tokens.at(5));
  } else if (tokens.size() == 5) {
    TD = parse_param(tokens.at(4), iObj.parameters, subckt);
  }
  std::ifstream wffile(WFline);
  if (wffile.good()) {
    getline(wffile, WFline);
  } else {
    Errors::function_errors(FunctionErrors::CUS_WF_NOT_FOUND, WFline);
  }
  wffile.close();
  WF = Misc::tokenize(WFline, " ,;");
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

void Function::parse_noise(
      const std::vector<std::string> &tokens, std::vector<double> &functionOfT, 
      const Input &iObj, const string_o &subckt) {
  if (tokens.size() < 2) {
      Errors::function_errors(FunctionErrors::NOISE_TOO_FEW_ARGUMENTS, std::to_string(tokens.size()));
  }
  if (tokens.size() > 4) {
    Errors::function_errors(FunctionErrors::NOISE_TOO_MANY_ARGUMENTS, std::to_string(tokens.size()));
  }
  double VO = 0.0, VA = 0.0, TD = 0.0, TSTEP = 0.0;
  VO = parse_param(tokens.at(0), iObj.parameters, subckt);
  if(VO != 0.0 && tokens.size() == 4) {
    Errors::function_errors(FunctionErrors::NOISE_VO_ZERO, tokens.at(0));
    VA = parse_param(tokens.at(1), iObj.parameters, subckt);
    if (VA == 0.0) {
      if (iObj.argVerb) {
        Errors::function_errors(FunctionErrors::NOISE_VA_ZERO, tokens.at(1));
      }
    }
    TD = parse_param(tokens.at(3), iObj.parameters, subckt);
  } else {
    VA = parse_param(tokens.at(0), iObj.parameters, subckt);
    if (VA == 0.0) {
      if (iObj.argVerb) {
        Errors::function_errors(FunctionErrors::NOISE_VA_ZERO, tokens.at(1));
      }
    }
    TD = parse_param(tokens.at(2), iObj.parameters, subckt);
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

void Function::parse_pws(
      const std::vector<std::string> &tokens, std::vector<double> &functionOfT, 
      const Input &iObj, const string_o &subckt) {
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
            ParameterName(tokens.at(i), subckt)) != 0) {
      values.push_back(iObj.parameters.at(
          ParameterName(tokens.at(i), subckt)).get_value().value());
    } else if (iObj.parameters.count(
            ParameterName(tokens.at(i), "")) != 0) {
      values.push_back(iObj.parameters.at(
          ParameterName(tokens.at(i), "")).get_value().value());
    } else {
      values.push_back(parse_param(
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
        value = values.at(i - 1) + ba * sin((2 * Constants::PI * (j - (period/4)))/period) + ba;
      } else if (values.at(i - 1) > values.at(i)) {
        ba = (values.at(i - 1) - values.at(i)) / 2;
        value = values.at(i - 1) - ba * sin((2 * Constants::PI * (j + (period/4)))/period) - ba;
      } else if (values.at(i - 1) == values.at(i)) {
        value = values.at(i);
      }
      functionOfT.at(j) = value;
    }
  }
}

void Function::voltage_to_phase(
  std::vector<double> &source, const Input &iObj) {
  double vn1, value;
  for(int i = 0; i < source.size(); ++i) {
    if(i == 0) { 
      value = (iObj.transSim.get_prstep() / (2 * Constants::SIGMA)) * (source.at(i));
    } else { 
      value = (iObj.transSim.get_prstep() / (2 * Constants::SIGMA)) * (source.at(i) + vn1) + source.at(i - 1);
    }
    vn1 = source.at(i);
    source.at(i) = value;
  }
}

void Function::phase_to_voltage(
  std::vector<double> &source, const Input &iObj) {
  double pn1, value;
  for(int i = 0; i < source.size(); ++i) {
    if(i == 0) {
      value = ((2 * Constants::SIGMA) / iObj.transSim.get_prstep()) * (source.at(i));
    } else {
      value = ((2 * Constants::SIGMA) / iObj.transSim.get_prstep()) * (source.at(i) - pn1) - source.at(i - 1);
    }
    pn1 = source.at(i);
    source.at(i) = value;
  }
}

void FunctObj::parse_function(
  const std::string &str, const Input &iObj, const string_o &subckt) {
  auto first = str.find('(') + 1;
  auto last = str.find(')');
  std::string p = str.substr(first, last - first);
  tokens_t t = Misc::tokenize(p, " ,");
  /* PWL(0 V0 T1 V1 T2 V2 ... TN VN) */
  if (str.find("PWL") != std::string::npos) {
    fType_ = FunctionType::PWL;
    parse_pwl(t, iObj, subckt);
  }
  /* PULSE(A1 A2 TD[0.0] TR[TSTEP] TF[TSTEP] PW[TSTOP] PER[TSTOP])*/
  else if (str.find("PULSE") != std::string::npos) {
    fType_ = FunctionType::PULSE;
    parse_pulse(t, iObj, subckt);
  }
  /* SIN(VO VA FREQ[1/TSTOP] TD[0.0] THETA[0.0]) */
  else if (str.find("SIN") != std::string::npos) {
    fType_ = FunctionType::SINUSOID;
    parse_sin(t, iObj, subckt);
  }
  /* CUS(WaveFile.dat TS[TSTEP] SF[1.0] IM[1] TD[0.0] PER[0]) */
  else if (str.find("CUS") != std::string::npos) {
    fType_ = FunctionType::CUS;
    parse_cus(t, iObj, subckt);
  }
  /* NOISE(VO VA TSTEP TD) */
  else if (str.find("NOISE") != std::string::npos) {
    fType_ = FunctionType::NOISE;
    parse_noise(t, iObj, subckt);
  }
  /* PWS(0 0 T1 V1 T2 V2 .. TN VN) */
  else if (str.find("PWS") != std::string::npos) {
    fType_ = FunctionType::PWS;
    parse_pwl(t, iObj, subckt);
  }
  /* EXP(V1 V2 TD1[0.0] TAU1[TSTEP] TD2[TD1+TSTEP] TAU2[TSTEP]) */
  else if (str.find("EXP") != std::string::npos) {
    fType_ = FunctionType::EXP;
    parse_exp(t,iObj, subckt);
  }
  /* DC VALUE */
  else if (str.find("DC") != std::string::npos) {
    if (!std::isnan(parse_param(
      Misc::tokenize(str).back(), iObj.parameters, subckt))) {
      fType_ = FunctionType::DC;
      parse_dc(t, iObj, subckt);
    }
  }
  /* Only a value was provided? Assume user is trying to make a DC source */
  else if (!std::isnan(parse_param(
      Misc::tokenize(str).back(), iObj.parameters, subckt))) {
    fType_ = FunctionType::DC;
    parse_dc(t, iObj, subckt);
  }
}

void FunctObj::parse_pwl(
  const tokens_t &t, const Input &iObj, const string_o &s) {
  /* PWL(0 V0 T1 V1 T2 V2 ... TN VN) */
  std::vector<double> timesteps, values;
  if (std::stod(t.at(0)) != 0.0) {
    Errors::function_errors(
      FunctionErrors::INITIAL_VALUES, t.at(0) + " & " + t.at(1));
    timesteps.push_back(0.0);
    values.push_back(0.0);
  }
  for (int i = 0; i < t.size(); i = i + 2) {
    timesteps.push_back(Misc::modifier(t.at(i)));
  }
  for (int i = 1; i < t.size(); i = i + 2) {
    values.push_back(parse_param(t.at(i), iObj.parameters, s));
  }
  if (timesteps.size() < values.size()) {
    Errors::function_errors(FunctionErrors::TOO_FEW_TIMESTEPS,
      std::to_string(timesteps.size()) + " timesteps & " + std::to_string(
        timesteps.size()) + " values");
  }                                
  if (timesteps.size() > values.size()) {
    Errors::function_errors(
      FunctionErrors::TOO_FEW_VALUES, std::to_string(
        timesteps.size()) + " timesteps & " + std::to_string(
          timesteps.size()) + " values");
  }
  timeValues_ = timesteps;
  ampValues_ = values;
}

void FunctObj::parse_pulse(
  const tokens_t &t, const Input &iObj, const string_o &s) {
  /* PULSE(A1 A2 TD[0.0] TR[TSTEP] TF[TSTEP] PW[TSTOP] PER[TSTOP])*/
  if (t.size() < 2) {
    Errors::function_errors(
      FunctionErrors::PULSE_TOO_FEW_ARGUMENTS, std::to_string(t.size()));
  }
  // Step and stop time shorthand
  const double &tstop = iObj.transSim.get_tstop();
  const double &tstep = iObj.transSim.get_prstep();
  // Set the 2 amplitudes
  ampValues_.emplace_back(parse_param(t.at(0), iObj.parameters, s));
  ampValues_.emplace_back(parse_param(t.at(1), iObj.parameters, s));
  // Set default values for optional arguments
  timeValues_.emplace_back(
    std::vector<double>{0.0, tstep, tstep, tstop, tstop});
  // Test if values exist
  if(t.size() >= 3)
    timeValues_.at(0) = parse_param(t.at(2), iObj.parameters, s);
  if(t.size() >= 4)
    timeValues_.at(1) = parse_param(t.at(3), iObj.parameters, s);
  if(t.size() >= 5)
    timeValues_.at(2) = parse_param(t.at(4), iObj.parameters, s);
  if(t.size() >= 6)
    timeValues_.at(3) = parse_param(t.at(5), iObj.parameters, s);
  if(t.size() >= 4)
    timeValues_.at(4) = parse_param(t.at(3), iObj.parameters, s);
  if (iObj.argVerb) {
    // If pulse width is 0.0 then there is no pulse, then useless so warn
    if (timeValues_.at(3) == 0.0) {
      Errors::function_errors(FunctionErrors::PULSE_WIDTH_ZERO, t.at(5));
    }
    // If repeat time is 0.0 then impossible, so warn
    if (timeValues_.at(4) == 0.0) {
      Errors::function_errors(FunctionErrors::PULSE_REPEAT, t.at(6));
    }
  }
  // Store the stop time
  timeValues_.emplace_back(tstop);
}

void FunctObj::parse_sin(
  const tokens_t &t, const Input &iObj, const string_o &s) {
  /* SIN(VO VA FREQ[1/TSTOP] TD[0.0] THETA[0.0]) */
  if (t.size() < 2) {
      Errors::function_errors(
        FunctionErrors::SIN_TOO_FEW_ARGUMENTS, std::to_string(t.size()));
  }
  const double &tstop = iObj.transSim.get_tstop();
  // Set offset and peak amplitude
  ampValues_.emplace_back(parse_param(t.at(0), iObj.parameters, s));
  ampValues_.emplace_back(parse_param(t.at(1), iObj.parameters, s));
  if (iObj.argVerb) {
    if (ampValues_.at(1) == 0.0) {
      Errors::function_errors(FunctionErrors::SIN_VA_ZERO, t.at(1));
    }
  }
  // Set default optional values
  timeValues_.emplace_back(std::vector<double>{(1.0/tstop), 0.0, 0.0});
  // Test if optional values exist
  if(t.size() >= 3)
    timeValues_.at(0) = parse_param(t.at(2), iObj.parameters, s);
  if(t.size() >= 4)
    timeValues_.at(1) = parse_param(t.at(3), iObj.parameters, s);
  if(t.size() >= 5)
    timeValues_.at(2) = parse_param(t.at(4), iObj.parameters, s);
}

void FunctObj::parse_cus(
  const tokens_t &t, const Input &iObj, const string_o &s) {
  /* CUS(WaveFile.dat TS[TSTEP] SF[1.0] IM[1] TD[0.0] PER[0]) */
  if (t.size() < 2){
    Errors::function_errors(
      FunctionErrors::CUS_TOO_FEW_ARGUMENTS, std::to_string(t.size()));
  }       
  // First argument is required and is the wave file name                     
  std::string WFline = t.at(0);
  std::vector<std::string> WF;
  const double &tstep = iObj.transSim.get_prstep();
  // Default values for the arguments
  timeValues_.emplace_back(std::vector<double>{tstep, 1.0, 1, 0.0, 0});
  // Test if optional values exist
  if(t.size() >= 3)
    timeValues_.at(0) = parse_param(t.at(2), iObj.parameters, s);
  if(t.size() >= 4)
    timeValues_.at(1) = parse_param(t.at(3), iObj.parameters, s);
  if(t.size() >= 5)
    timeValues_.at(2) = parse_param(t.at(4), iObj.parameters, s);
  if(t.size() >= 6)
    timeValues_.at(3) = parse_param(t.at(5), iObj.parameters, s);
  if(t.size() >= 7)
    timeValues_.at(4) = parse_param(t.at(6), iObj.parameters, s);

  if (iObj.argVerb) {
    if (timeValues_.at(1) == 0.0) {
      Errors::function_errors(FunctionErrors::CUS_SF_ZERO, t.at(2)); 
    }
  }
  std::ifstream wffile(WFline);
  if (wffile.good()) {
    getline(wffile, WFline);
  } else {
    Errors::function_errors(FunctionErrors::CUS_WF_NOT_FOUND, WFline);
  }
  wffile.close();
  WF = Misc::tokenize(WFline, " ,;");
  for (int i = 0; i < WF.size(); ++i) {
    ampValues_.emplace_back(Misc::modifier(WF.at(i)));
  }
  if (timeValues_.at(0) < tstep) {
    timeValues_.at(0) = tstep;
  }
  // Store the stop time
  timeValues_.emplace_back(iObj.transSim.get_tstop());
}

void FunctObj::parse_noise(
  const tokens_t &t, const Input &iObj, const string_o &s) {
  /* NOISE(VO=0.0 VA TSTEP[TSTEP] TD[0.0]) */
  if (t.size() < 2) {
      Errors::function_errors(
        FunctionErrors::NOISE_TOO_FEW_ARGUMENTS, std::to_string(t.size()));
  }
  const double &tstep = iObj.transSim.get_prstep();
  // Set amplitudes
  ampValues_.emplace_back(parse_param(t.at(0), iObj.parameters, s));
  ampValues_.emplace_back(parse_param(t.at(1), iObj.parameters, s));
  // Default values for the arguments
  timeValues_.emplace_back(std::vector<double>{tstep, 0.0});
  // Test if optional values exist
  if(t.size() >= 3)
    timeValues_.at(0) = parse_param(t.at(2), iObj.parameters, s);
  if(t.size() >= 4)
    timeValues_.at(1) = parse_param(t.at(3), iObj.parameters, s);
  if (iObj.argVerb) {
    if(ampValues_.at(0) != 0.0) {
      Errors::function_errors(FunctionErrors::NOISE_VO_ZERO, t.at(0));
      ampValues_.at(0) = 0.0;
    }
    if (ampValues_.at(1) == 0.0) {
      Errors::function_errors(FunctionErrors::NOISE_VA_ZERO, t.at(1));
    }
  }
}

void FunctObj::parse_dc(
  const tokens_t &t, const Input &iObj, const string_o &s) {
  /* DC VALUE */
  // Set amplitudes
  ampValues_.emplace_back(parse_param(t.at(0), iObj.parameters, s));
}

void FunctObj::parse_exp(
  const tokens_t &t, const Input &iObj, const string_o &s) {
  /* EXP(V1 V2 TD1[0.0] TAU1[TSTEP] TD2[TD1+TSTEP] TAU2[TSTEP]) */
  if (t.size() < 2) {
      Errors::function_errors(
        FunctionErrors::EXP_TOO_FEW_ARGUMENTS, std::to_string(t.size()));
  }
  const double &tstep = iObj.transSim.get_prstep();
  // Set amplitudes
  ampValues_.emplace_back(parse_param(t.at(0), iObj.parameters, s));
  ampValues_.emplace_back(parse_param(t.at(1), iObj.parameters, s));
  // Default values for the arguments
  timeValues_.emplace_back(std::vector<double>{0.0, tstep, tstep, tstep});
  // Test if optional values exist
  if(t.size() >= 3)
    timeValues_.at(0) = parse_param(t.at(2), iObj.parameters, s);
  if(t.size() >= 4)
    timeValues_.at(1) = parse_param(t.at(3), iObj.parameters, s);
}

double FunctObj::return_pwl(const double &x) {
  // If x larger than the last timestep then assume last amplitude value
  if(x >= timeValues_.back()) {
    return ampValues_.back();
  // Else check within which range x falls
  } else {
    for(int i = 0; i < timeValues_.size() - 1; ++i) {
      if(x >= timeValues_.at(i) && x < timeValues_.at(i+1)) {
        double &y2 = ampValues_.at(i+1);
        double &y1 = ampValues_.at(i);
        double &x2 = timeValues_.at(i+1);
        double &x1 = timeValues_.at(i);
        // Calculate function value and return it
        return y1 + ((y2 - y1) / (x2 - x1)) * x;
      }
    }
  }
}

double FunctObj::return_pulse(const double &x) {
  // Shorthand for the relevant time values
  double &td = timeValues_.at(0);
  double &tr = timeValues_.at(1);
  double &tf = timeValues_.at(2);
  double &pw = timeValues_.at(3);
  double &per = timeValues_.at(4);
  double &tstop = timeValues_.at(5);
  // Calculate how many times the pulse repeats
  int n = static_cast<int>(tstop / per);
  for(int i = 1; i < n; ++i){
    // Within a multiple of the pulse
    if(x >= (i * per) && x < ((i + 1) * per)) {
      // Within the rise time
      if(x >= (i * per) && x < (i * per + tr)) {
        // Calculate function value and return it
        return ampValues_.at(0) + 
          ((ampValues_.at(1) - ampValues_.at(0)) / 
          ((i * per + tr) - (i * per))) * x;
      // Within pulse width
      } else if(x >= (i * per + tr) && x < (i * per + tr + pw)) {
        return ampValues_.at(1);
      // Within fall time
      } else if(x >= (i * per + tr + pw) && x < (i * per + tr + pw + tf)) {
        // Calculate function value and return it
        return ampValues_.at(1) + 
          ((ampValues_.at(0) - ampValues_.at(1)) / 
          ((i * per + tr + pw) - (i * per + tr + pw + tf))) * x;
      // Between two pulses
      } else {
        return ampValues_.at(0);
      }
    }
  }
}

double FunctObj::return_sin(const double &x) {
  return ampValues_.at(0) + ampValues_.at(1) * 
    sin(2 * Constants::PI * timeValues_.at(0) * (x - timeValues_.at(1))) *
                      exp(-timeValues_.at(2) * (x - timeValues_.at(1)));
}

double FunctObj::return_cus(const double &x) {
  // Shorthand for the relevant time values
  double &ts = timeValues_.at(0);
  double &sf = timeValues_.at(1);
  double &im = timeValues_.at(2);
  double &td = timeValues_.at(3);
  double &per = timeValues_.at(4);
  double &tstop = timeValues_.at(5);
  int n = 1;
  double repTime = 0;
  if(static_cast<int>(per) != 1) {
    n = tstop / (ampValues_.size() * ts);
    repTime = tstop / (ampValues_.size());
  }
  for(int i = 1; i < n; ++i) {
    if(x >= (i * repTime) && x < (i * repTime + td)) {
      return 0;
    } else if (x >= (i * repTime + td) && x < (i * repTime + td + ts)) {
      return ((ampValues_.at(0) * sf) / ts) * x;
    } else {
      for (int j = 1; j < ampValues_.size(); ++j) {
        if (x >= (i * repTime + td + j * ts) && 
          x < (i * repTime + td + (j + 1) * ts)) {
            return ((ampValues_.at(j - 1) * sf) + 
              ((ampValues_.at(j) * sf) - (ampValues_.at(j-1) * sf)) / ts) * x;
        }
      }
    } 
  }
}

double FunctObj::return_noise(const double &x) {
  if(x < timeValues_.back()) 
    return 0.0;
  else 
    return ampValues_.at(1) * Misc::grand() / sqrt(2.0 * timeValues_.front());
}

double FunctObj::return_pws(const double &x) {
  // If x larger than the last timestep then assume last amplitude value
  if(x >= timeValues_.back()) {
    return ampValues_.back();
  // Else check within which range x falls
  } else {
    for(int i = 0; i < timeValues_.size() - 1; ++i) {
      if(x >= timeValues_.at(i) && x < timeValues_.at(i+1)) {
        double &y2 = ampValues_.at(i+1);
        double &y1 = ampValues_.at(i);
        double &x2 = timeValues_.at(i+1);
        double &x1 = timeValues_.at(i);
        double period = (x2 - x1) * 2;
        double ba;
        if (y1 < y2) {
          ba = (y2 - y1) / 2;
          return y1 + ba * sin((2 * Constants::PI * (x - (period/4)))/period) + ba;
        } else if (y1 > y2) {
          ba = (y1 - y2) / 2;
          return y1 - ba * sin((2 * Constants::PI * (x + (period/4)))/period) - ba;
        } else if (y1 == y2) {
          return y1;
        }
      }
    }
  }
}

double FunctObj::return_dc() {
  return ampValues_.back();
}

double FunctObj::return_exp(const double &x) {

}