// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/Function.hpp"
#include "JoSIM/Misc.hpp"
#include "JoSIM/Parameters.hpp"

#include <fstream>
#include <cmath>
#include <cassert>

using namespace JoSIM;

void Function::parse_function(
  const std::string& str, const Input& iObj, const string_o& subckt) {
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
  /* NOISE(VA TD TSTEP) */
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
    parse_exp(t, iObj, subckt);
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

void Function::parse_pwl(
  const tokens_t& t, const Input& iObj, const string_o& s) {
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

void Function::parse_pulse(
  const tokens_t& t, const Input& iObj, const string_o& s) {
  /* PULSE(A1 A2 TD[0.0] TR[TSTEP] TF[TSTEP] PW[TSTOP] PER[TSTOP])*/
  if (t.size() < 2) {
    Errors::function_errors(
      FunctionErrors::PULSE_TOO_FEW_ARGUMENTS, std::to_string(t.size()));
  }
  // Step and stop time shorthand
  const double& tstop = iObj.transSim.tstop();
  const double& tstep = iObj.transSim.tstep();
  // Set the 2 amplitudes
  ampValues_.emplace_back(parse_param(t.at(0), iObj.parameters, s));
  ampValues_.emplace_back(parse_param(t.at(1), iObj.parameters, s));
  // Set default values for optional arguments
  timeValues_.emplace_back(0.0);
  timeValues_.emplace_back(tstep);
  timeValues_.emplace_back(tstep);
  timeValues_.emplace_back(tstop);
  timeValues_.emplace_back(tstop);
  // Test if values exist
  if (t.size() >= 3)
    timeValues_.at(0) = parse_param(t.at(2), iObj.parameters, s);
  if (t.size() >= 4)
    timeValues_.at(1) = parse_param(t.at(3), iObj.parameters, s);
  if (t.size() >= 5)
    timeValues_.at(2) = parse_param(t.at(4), iObj.parameters, s);
  if (t.size() >= 6)
    timeValues_.at(3) = parse_param(t.at(5), iObj.parameters, s);
  if (t.size() >= 7)
    timeValues_.at(4) = parse_param(t.at(6), iObj.parameters, s);
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

void Function::parse_sin(
  const tokens_t& t, const Input& iObj, const string_o& s) {
  /* SIN(VO VA FREQ[1/TSTOP] TD[0.0] THETA[0.0]) */
  if (t.size() < 2) {
    Errors::function_errors(
      FunctionErrors::SIN_TOO_FEW_ARGUMENTS, std::to_string(t.size()));
  }
  const double& tstop = iObj.transSim.tstop();
  // Set offset and peak amplitude
  ampValues_.emplace_back(parse_param(t.at(0), iObj.parameters, s));
  ampValues_.emplace_back(parse_param(t.at(1), iObj.parameters, s));
  if (iObj.argVerb) {
    if (ampValues_.at(1) == 0.0) {
      Errors::function_errors(FunctionErrors::SIN_VA_ZERO, t.at(1));
    }
  }
  // Set default optional values
  timeValues_.emplace_back((1.0 / tstop));
  timeValues_.emplace_back(0.0);
  timeValues_.emplace_back(0.0);
  // Test if optional values exist
  if (t.size() >= 3)
    timeValues_.at(0) = parse_param(t.at(2), iObj.parameters, s);
  if (t.size() >= 4)
    timeValues_.at(1) = parse_param(t.at(3), iObj.parameters, s);
  if (t.size() >= 5)
    timeValues_.at(2) = parse_param(t.at(4), iObj.parameters, s);
}

void Function::parse_cus(
  const tokens_t& t, const Input& iObj, const string_o& s) {
  /* CUS(WaveFile.dat TS[TSTEP] SF[1.0] IM[1] TD[0.0] PER[0]) */
  if (t.size() < 2) {
    Errors::function_errors(
      FunctionErrors::CUS_TOO_FEW_ARGUMENTS, std::to_string(t.size()));
  }
  // First argument is required and is the wave file name                     
  std::string WFline = t.at(0);
  std::vector<std::string> WF;
  const double& tstep = iObj.transSim.tstep();
  // Default values for the arguments
  timeValues_.emplace_back(tstep);
  timeValues_.emplace_back(1.0);
  timeValues_.emplace_back(1);
  timeValues_.emplace_back(0.0);
  timeValues_.emplace_back(0);
  // Test if optional values exist
  if (t.size() >= 3)
    timeValues_.at(0) = parse_param(t.at(2), iObj.parameters, s);
  if (t.size() >= 4)
    timeValues_.at(1) = parse_param(t.at(3), iObj.parameters, s);
  if (t.size() >= 5)
    timeValues_.at(2) = parse_param(t.at(4), iObj.parameters, s);
  if (t.size() >= 6)
    timeValues_.at(3) = parse_param(t.at(5), iObj.parameters, s);
  if (t.size() >= 7)
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
  timeValues_.emplace_back(iObj.transSim.tstop());
}

void Function::parse_noise(
  const tokens_t& t, const Input& iObj, const string_o& s) {
  /* NOISE(VA TD[0.0] TSTEP[TSTEP]) */
  if (t.size() < 2) {
    Errors::function_errors(
      FunctionErrors::NOISE_TOO_FEW_ARGUMENTS, std::to_string(t.size()));
  }
  const double& tstep = iObj.transSim.tstep();
  // Set amplitudes
  ampValues_.emplace_back(parse_param(t.at(0), iObj.parameters, s));
  // Default values for the arguments
  timeValues_.emplace_back(0.0);
  timeValues_.emplace_back(tstep);
  // Test if optional values exist
  if (t.size() >= 2)
    timeValues_.at(0) = parse_param(t.at(1), iObj.parameters, s);
  if (t.size() >= 3)
    timeValues_.at(1) = parse_param(t.at(2), iObj.parameters, s);
  if (iObj.argVerb) {
    if (ampValues_.at(0) == 0.0) {
      Errors::function_errors(FunctionErrors::NOISE_VA_ZERO, t.at(1));
    }
  }
}

void Function::parse_dc(
  const tokens_t& t, const Input& iObj, const string_o& s) {
  /* DC VALUE */
  // Set amplitudes
  ampValues_.emplace_back(parse_param(t.back(), iObj.parameters, s));
}

void Function::parse_exp(
  const tokens_t& t, const Input& iObj, const string_o& s) {
  /* EXP(V1 V2 TD1[0.0] TAU1[TSTEP] TD2[TD1+TSTEP] TAU2[TSTEP]) */
  if (t.size() < 2) {
    Errors::function_errors(
      FunctionErrors::EXP_TOO_FEW_ARGUMENTS, std::to_string(t.size()));
  }
  const double& tstep = iObj.transSim.tstep();
  // Set amplitudes
  ampValues_.emplace_back(parse_param(t.at(0), iObj.parameters, s));
  ampValues_.emplace_back(parse_param(t.at(1), iObj.parameters, s));
  // Default values for the arguments
  timeValues_.emplace_back(0.0);
  timeValues_.emplace_back(tstep);
  timeValues_.emplace_back(tstep);
  timeValues_.emplace_back(tstep);
  // Test if optional values exist
  if (t.size() >= 3)
    timeValues_.at(0) = parse_param(t.at(2), iObj.parameters, s);
  if (t.size() >= 4)
    timeValues_.at(1) = parse_param(t.at(3), iObj.parameters, s);
}

double Function::return_pwl(double& x) {
  // If x larger than the last timestep then assume last amplitude value
  if (x >= timeValues_.back()) {
    return ampValues_.back();
    // Else check within which range x falls
  } else {
    for (int i = 0; i < timeValues_.size() - 1; ++i) {
      if (x >= timeValues_.at(i) && x < timeValues_.at(i + 1)) {
        double& y2 = ampValues_.at(i + 1);
        double& y1 = ampValues_.at(i);
        double& x2 = timeValues_.at(i + 1);
        double& x1 = timeValues_.at(i);
        // Calculate function value and return it
        return y1 + ((y2 - y1) / (x2 - x1)) * (x - x1);
      }
    }
  }
  return 0.0;
}

double Function::return_pulse(double& x) {
  // Shorthand for the relevant time values
  double& td = timeValues_.at(0);
  double& tr = timeValues_.at(1);
  double& tf = timeValues_.at(2);
  double& pw = timeValues_.at(3);
  double& per = timeValues_.at(4);
  double& tstop = timeValues_.at(5);
  double& alow = ampValues_.at(0);
  double& ahigh = ampValues_.at(1);
  auto val = alow;
  if (x < td) return(val);
  int index = (x - td) / per;
  auto time = x - td - index * per;
  if (time < tr)  {
    val = alow + (ahigh - alow) * time / tr;
  } else if (time < tr + pw) {
    val = ahigh;
  } else if (time < tr + pw + tf) {
    val = ahigh + (alow - ahigh) * (time - tr - pw) / tf;
  } else { 
    val = alow;
  }

  return(val);
}

double Function::return_sin(double& x) {
  if (x >= timeValues_.at(1)) {
    return ampValues_.at(0) + ampValues_.at(1) *
      sin(2 * Constants::PI * timeValues_.at(0) * (x - timeValues_.at(1))) *
      exp(-timeValues_.at(2) * (x - timeValues_.at(1)));
  } else return ampValues_.at(0);
}

double Function::return_cus(double& x) {
  // Shorthand for the relevant time values
  double& ts = timeValues_.at(0);
  double& sf = timeValues_.at(1);
  double& im = timeValues_.at(2);
  double& td = timeValues_.at(3);
  double& per = timeValues_.at(4);
  double& tstop = timeValues_.at(5);
  int n = 1;
  double repTime = 0;
  if (static_cast<int>(per) != 1) {
    n = tstop / (ampValues_.size() * ts);
    repTime = tstop / (ampValues_.size());
  }
  for (int i = 1; i < n; ++i) {
    if (sf == 1.0) {
      if (x >= (i * repTime) && x < (i * repTime + td)) {
        return 0;
      } else if (x >= (i * repTime + td) && x < (i * repTime + td + ts)) {
        return ((ampValues_.at(0) * sf) / ts) * (x - (i * repTime + td));
      } else {
        for (int j = 1; j < ampValues_.size(); ++j) {
          if (x >= (i * repTime + td + j * ts) &&
            x < (i * repTime + td + (j + 1) * ts)) {
            return ((ampValues_.at(j - 1) * sf) +
              ((ampValues_.at(j) * sf) - (ampValues_.at(j - 1) * sf)) / ts) *
              (x - (i * repTime + td + j * ts));
          }
        }
      }
    } else if (sf == 2.0) {
      if (x >= (i * repTime) && x < (i * repTime + td)) {
        return 0;
      } else if (x >= (i * repTime + td) && x < (i * repTime + td + ts)) {
        return (ampValues_.at(0));
      } else {
        for (int j = 1; j < ampValues_.size(); ++j) {
          if (x >= (i * repTime + td + j * ts) &&
            x < (i * repTime + td + (j + 1) * ts)) {
            return (ampValues_.at(j));
          }
        }
      }
    }
  }
  return 0.0;
}

double Function::return_noise(double& x) {
  if (x < timeValues_.front())
    return 0.0;
  else
    return ampValues_.at(0) * Misc::grand() / sqrt(2.0 * timeValues_.back());
}

double Function::return_pws(double& x) {
  // If x larger than the last timestep then assume last amplitude value
  if (x >= timeValues_.back()) {
    return ampValues_.back();
    // Else check within which range x falls
  } else {
    for (int i = 0; i < timeValues_.size() - 1; ++i) {
      if (x >= timeValues_.at(i) && x < timeValues_.at(i + 1)) {
        double& y2 = ampValues_.at(i + 1);
        double& y1 = ampValues_.at(i);
        double& x2 = timeValues_.at(i + 1);
        double& x1 = timeValues_.at(i);
        double period = (x2 - x1) * 2;
        double ba;
        if (y1 < y2) {
          ba = (y2 - y1) / 2;
          return y1 + ba * sin((2 * Constants::PI *
            (x - (period / 4))) / period) + ba;
        } else if (y1 > y2) {
          ba = (y1 - y2) / 2;
          return y1 - ba * sin((2 * Constants::PI *
            (x + (period / 4))) / period) - ba;
        } else if (y1 == y2) {
          return y1;
        }
      }
    }
  }
  return 0.0;
}

double Function::return_dc() {
  return ampValues_.back();
}

double Function::return_exp(double& x) {
  // Shorthand for time values
  double& td1 = timeValues_.at(0);
  double& tau1 = timeValues_.at(1);
  double& td2 = timeValues_.at(2);
  double& tau2 = timeValues_.at(3);
  if (x < td1) {
    return ampValues_.at(0);
  } else if (x >= td1 && x < td2) {
    return ampValues_.at(0) + (ampValues_.at(1) - ampValues_.at(0)) *
      (1 - exp(-(x - td1) / tau1));
  } else if (x >= td2) {
    return ampValues_.at(0) + (ampValues_.at(1) - ampValues_.at(0)) *
      (1 - exp(-(x - td1) / tau1)) + (ampValues_.at(0) - ampValues_.at(1)) *
      (1 - exp(-(x - td2) / tau2));
  }
  return 0.0;
}

double Function::value(double x) {
  switch (fType_) {
  case FunctionType::PWL:
    return return_pwl(x);
  case FunctionType::PULSE:
    return return_pulse(x);
  case FunctionType::SINUSOID:
    return return_sin(x);
  case FunctionType::CUS:
    return return_cus(x);
  case FunctionType::NOISE:
    return return_noise(x);
  case FunctionType::PWS:
    return return_pws(x);
  case FunctionType::DC:
    return return_dc();
  case FunctionType::EXP:
    return return_exp(x);
  default:
    return 0.0;
  }
}
void Function::ampValues(std::vector<double> values) {
  ampValues_ = values;
}