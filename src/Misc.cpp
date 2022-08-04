// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/Misc.hpp"

#include <cassert>
#include <cctype>
#include <cfloat>
#include <cmath>
#include <fstream>
#include <random>

#include "JoSIM/Constants.hpp"

using namespace JoSIM;

double Misc::string_constant(const std::string& s) {
  if (s == "PI")
    return Constants::PI;
  else if (s == "PHI_ZERO")
    return Constants::PHI_ZERO;
  else if (s == "BOLTZMANN")
    return Constants::BOLTZMANN;
  else if (s == "EV")
    return Constants::EV;
  else if (s == "HBAR")
    return Constants::HBAR;
  else if (s == "C")
    return Constants::C;
  else if (s == "MU0")
    return Constants::MU0;
  else if (s == "EPS0")
    return Constants::EPS0;
  else if (s == "SIGMA")
    return Constants::SIGMA;
  return 0.0;
}

bool Misc::isclose(const double& a, const double& b) {
  return fabs(a - b) < DBL_EPSILON;
}

std::string Misc::file_from_path(const std::string& path) {
  auto posLastSlash = path.find_last_of("/\\");
  if (posLastSlash == std::string::npos) {
    posLastSlash = 0;
    return path.substr(posLastSlash);
  } else
    return path.substr(posLastSlash + 1);
}

bool Misc::has_suffix(const std::string& str, const std::string& suffix) {
  return str.size() >= suffix.size() &&
         str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool Misc::starts_with(const std::string& input, char test) {
  for (const auto i : input) {
    if (i != ' ') {
      return i == test;
    }
  }
  return false;
}

std::string Misc::vector_to_string(const tokens_t& s, std::string d) {
  std::stringstream ss;
  for (const auto& i : s) {
    ss << i << d;
  }
  return ss.str();
}

tokens_t Misc::tokenize(const std::string& c, std::string d, bool trimEmpty,
                        bool trimSpaces, int64_t count) {
  // Create a position token to point to the found delimiter
  size_t pos = 0, lastPos = 0;
  // Tokens to return
  tokens_t tokens;
  // Counter to count times delimiter was found
  int64_t counter = 0;
  // If times to delimit is 0 then delimit as much as we can
  if (count == 0) {
    count = c.length();
  }
  // While the delimiter can be found in the string
  while ((pos = c.find_first_of(d, lastPos)) != std::string::npos) {
    // If we reach the number of times to delimit
    if (counter == count) {
      // Break from the while loop
      break;
    }
    // If trim empty tokens is enabled
    if (trimEmpty) {
      // Check if the substring represents an empty string
      if (!c.substr(lastPos, pos - lastPos).empty()) {
        // Store the string between the identified positions
        tokens.emplace_back(c.substr(lastPos, pos - lastPos));
      }
      // If we want to keep empty tokens
    } else {
      // Store the string between the identified positions
      tokens.emplace_back(c.substr(lastPos, pos - lastPos));
    }
    // If trim spaces is enabled
    if (trimSpaces) {
      if (!tokens.empty()) {
        // Remove trailing, leading and duplicate spaces between tokens
        tokens.back() = std::regex_replace(tokens.back(),
                                           std::regex("^ +| +$|( ) +"), "$1");
      }
    }
    lastPos = pos + 1;
    ++counter;
  }
  // If trim empty tokens is enabled
  if (trimEmpty) {
    // Check if the substring represents an empty string
    if (!c.substr(lastPos).empty()) {
      // Store the string between the identified positions
      tokens.emplace_back(c.substr(lastPos));
    }
    // If we want to keep empty tokens
  } else {
    // Store the string between the identified positions
    tokens.emplace_back(c.substr(lastPos));
  }
  // If trim spaces is enabled
  if (trimSpaces) {
    // Remove trailing, leading and duplicate spaces between tokens
    tokens.back() =
        std::regex_replace(tokens.back(), std::regex("^ +| +$|( ) +"), "$1");
  }
  // Return the tokens
  return tokens;
}

void Misc::ltrim(std::string& s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int64_t ch) {
            return !std::isspace(ch);
          }));
}

void Misc::rtrim(std::string& s) {
  s.erase(std::find_if(s.rbegin(), s.rend(),
                       [](int64_t ch) { return !std::isspace(ch); })
              .base(),
          s.end());
}

double Misc::modifier(const std::string& value) {
  std::string::size_type sz;
  double number;
  try {
    number = std::stod(value, &sz);
  } catch (const std::invalid_argument&) {
    Errors::misc_errors(MiscErrors::STOD_ERROR, value);
  } catch (std::exception& e) {
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

void Misc::unique_push(std::vector<std::string>& vector,
                       const std::string& string) {
  if (std::find(vector.begin(), vector.end(), string) == vector.end()) {
    vector.push_back(string);
  }
}

int64_t Misc::index_of(const std::vector<std::string>& vector,
                       const std::string& value) {
  int64_t counter = 0;
  for (const auto& i : vector) {
    /* Value found, return counter */
    if (value == vector.at(counter)) return counter;
    counter++;
  }
  /* Value was not found, set counter equal to -1 and return */
  counter = -1;
  return counter;
}

std::string Misc::substring_after(const std::string& str,
                                  const std::string& whatpart) {
  std::size_t pos = 0;
  std::string substring;
  if (str.find(whatpart) != std::string::npos)
    pos = str.find(whatpart) + whatpart.size();
  substring = str.substr(pos);
  return substring;
}

std::string Misc::substring_before(const std::string& str,
                                   const std::string& whatpart) {
  std::string substring;
  if (str.find(whatpart) != std::string::npos) {
    std::size_t pos = str.find(whatpart);
    substring = str.substr(0, pos);
    return substring;
  } else
    return str;
}

bool Misc::findX(const std::vector<std::string>& segment, std::string& theLine,
                 int64_t& linePos) {
  for (int64_t i = linePos; i < segment.size(); ++i) {
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

int64_t Misc::numDigits(int64_t number) {
  int64_t digits = 0;
  if (number <= 0) digits = 1;
  while (number) {
    number /= 10;
    digits++;
  }
  return digits;
}

double Misc::grand() {
  double r, u1, u2, lt;
  double scale = 1.0 / 1024.0 / 1024.0 / 1024.0 / 2.0;
  u1 = (static_cast<double>(rand()) + 1) / (static_cast<double>(RAND_MAX) + 1);
  u2 = static_cast<double>(rand()) * 2 * Constants::PI /
       static_cast<double>(RAND_MAX);
  lt = sqrt(-2.0 * log(u1));
  r = cos(u2) * lt;
  return r;
}