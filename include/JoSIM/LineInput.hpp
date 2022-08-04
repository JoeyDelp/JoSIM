// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_LINEINPUT_HPP
#define JOSIM_LINEINPUT_HPP

#include <fstream>
#include <iostream>
#include <string>
#include <string_view>

#include "JoSIM/Errors.hpp"

namespace JoSIM {

class LineInput {
 public:
  virtual std::string_view line() = 0;
  virtual bool next() = 0;
};

class ConsoleInput : public LineInput {
  std::string buffer_;

 public:
  std::string_view line() override { return buffer_; }
  bool next() override { return bool(std::getline(std::cin, buffer_)); }
  ConsoleInput(){};
};

class FileInput : public LineInput {
  std::string buffer_;
  std::ifstream file_;

 private:
  bool open(const std::string& file) {
    file_.open(file, std::ios::in | std::ios::binary);
    if (!file_.is_open()) {
      Errors::input_errors(InputErrors::CANNOT_OPEN_FILE, file);
    }
    return true;
  }

 public:
  std::string_view line() override { return buffer_; }
  bool next() override { return bool(std::getline(file_, buffer_)); }
  FileInput(const std::string& file) { open(file); }
};

}  // namespace JoSIM

#endif  // JOSIM_LINEINPUT_HPP
