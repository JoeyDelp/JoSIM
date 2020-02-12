// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/Misc.hpp"
#include "JoSIM/Constants.hpp"

#include <cassert>
#include <fstream>
#include <cmath>
#include <cctype>

using namespace JoSIM;

double Misc::string_constant(const std::string &s) {
  if (s == "PI") return Constants::PI;
  else if (s == "PHI_ZERO") return Constants::PHI_ZERO;
  else if (s == "BOLTZMANN") return Constants::BOLTZMANN;
  else if (s == "EV") return Constants::EV;
  else if (s == "HBAR") return Constants::HBAR;
  else if (s == "C") return Constants::C;
  else if (s == "MU0") return Constants::MU0;
  else if (s == "EPS0") return Constants::EPS0;
  else if (s == "SIGMA") return Constants::SIGMA;
  return 0.0;
}

std::string Misc::file_from_path(const std::string &path) {
  auto posLastSlash = path.find_last_of("/\\");
  if (posLastSlash == std::string::npos) {
    posLastSlash = 0;
    return path.substr(posLastSlash);
  } else
    return path.substr(posLastSlash + 1);
}

bool Misc::has_suffix(const std::string &str, const std::string &suffix) {
  return str.size() >= suffix.size() &&
         str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool Misc::starts_with(const std::string &input, char test) {
  for (const auto i : input) {
    if (i != ' ') {
      return i == test;
    }
  }
  return false;
}

std::vector<std::string> Misc::tokenize_space(const std::string &c) {
  std::string::size_type pos, lastPos = 0, length = c.length();
  std::string delimiters = " \t";
  std::vector<std::string> tokens;
  bool trimEmpty = true;

  using value_type = typename std::vector<std::string>::value_type;
  using size_type = typename std::vector<std::string>::size_type;

  while (lastPos < length + 1) {
    pos = c.find_first_of(delimiters, lastPos);
    if (pos == std::string::npos) {
      pos = length;
    }

    if (pos != lastPos || !trimEmpty)
      tokens.push_back(
          value_type(c.data() + lastPos, (size_type)pos - lastPos));

    lastPos = pos + 1;
  }
  return tokens;
}

std::vector<std::string> Misc::tokenize_space_once(const std::string &c) {
  std::vector<std::string> tokens (2, "");
  std::string::size_type pos = c.find_first_of(" \t");
  std::string::size_type length = c.length();
  if (pos == std::string::npos) {
      pos = length;
  }
  tokens.at(0) = c.substr(0, pos);
  if(pos != length) tokens.at(1) = c.substr(pos + 1, length);
  return tokens;
}

std::vector<std::string> Misc::tokenize_delimiter(const std::string &c,
                                                  const std::string &d) {
  std::vector<std::string> tokens;
  std::stringstream stringStream(c);
  std::string line;
  while (std::getline(stringStream, line)) {
    std::size_t prev = 0, pos;
    while ((pos = line.find_first_of(d, prev)) != std::string::npos) {
      if (pos > prev)
        tokens.push_back(line.substr(prev, pos - prev));
      prev = pos + 1;
    }
    if (prev < line.length())
      tokens.push_back(line.substr(prev, std::string::npos));
  }
  return tokens;
}

void Misc::ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
        return !std::isspace(ch);
    }));
}

void Misc::rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

double Misc::modifier(const std::string &value) {
  std::string::size_type sz;
  double number;
  try {
    number = std::stod(value, &sz);
  } catch (const std::invalid_argument &) {
    Errors::misc_errors(MiscErrors::STOD_ERROR, value);
  } catch (std::exception &e) {
    Errors::misc_errors(MiscErrors::STOD_ERROR, value);
  }
  switch (value.substr(sz)[0]) {
    /* mega */
  case 'X':
    return number * 1E6;
    /* mega or milli */
  case 'M':
    /* mega */
    if (value.substr(sz)[1] == 'E' && value.substr(sz)[2] == 'G')
      return number * 1E6;
    /* milli */
    else
      return number * 1E-3;
    /* micro */
  case 'U':
    return number * 1E-6;
    /* nano */
  case 'N':
    return number * 1E-9;
    /* pico */
  case 'P':
    return number * 1E-12;
    /* femto */
  case 'F':
    return number * 1E-15;
    /* kilo */
  case 'K':
    return number * 1E3;
    /* giga */
  case 'G':
    return number * 1E9;
    /* tera */
  case 'T':
    return number * 1E12;
    /* auto modifier */
  case 'E':
    return std::stod(value);
  default:
    return number;
  }
}

void Misc::unique_push(std::vector<std::string> &vector,
                       const std::string &string) {
  if (std::find(vector.begin(), vector.end(), string) == vector.end()) {
    vector.push_back(string);
  }
}

int Misc::index_of(const std::vector<std::string> &vector,
                   const std::string &value) {
  int counter = 0;
  for (const auto &i : vector) {
    /* Value found, return counter */
    if (value == vector.at(counter))
      return counter;
    counter++;
  }
  /* Value was not found, set counter equal to -1 and return */
  counter = -1;
  return counter;
}

std::string Misc::substring_after(const std::string &str,
                                  const std::string &whatpart) {
  std::size_t pos = 0;
  std::string substring;
  if (str.find(whatpart) != std::string::npos)
    pos = str.find(whatpart) + whatpart.size();
  substring = str.substr(pos);
  return substring;
}

std::string Misc::substring_before(const std::string &str,
                                   const std::string &whatpart) {
  std::string substring;
  if (str.find(whatpart) != std::string::npos) {
    std::size_t pos = str.find(whatpart);
    substring = str.substr(0, pos);
    return substring;
  } else
    return str;
}

bool Misc::findX(const std::vector<std::string> &segment, std::string &theLine,
                 int &linePos) {
  for (int i = linePos; i < segment.size(); ++i) {
    if (segment.at(i).at(0) == 'X') {
      theLine = segment.at(i);
      if (i < segment.size() - 1)
        linePos = i + 1;
      else
        linePos = segment.size() - 1;
      return true;
    }
  }
  return false;
}

int Misc::numDigits(int number) {
  int digits = 0;
  if (number <= 0)
    digits = 1;
  while (number) {
    number /= 10;
    digits++;
  }
  return digits;
}

double Misc::grand() {
  double r, v2, fac;
  r = 2;
  while (r >= 1) {
    double v1 = (2*((double)rand()/(double)RAND_MAX)-1);
    v2 = (2*((double)rand()/(double)RAND_MAX)-1);
    r = v1*v1+v2*v2;
  }
  fac = sqrt(-2*log(r)/r);
  return (v2*fac);
}