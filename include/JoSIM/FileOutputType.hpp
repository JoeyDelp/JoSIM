// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_FILEOUTPUTTYPE_HPP
#define JOSIM_FILEOUTPUTTYPE_HPP

#include <algorithm>
#include <filesystem>

#include "JoSIM/Errors.hpp"

namespace JoSIM {

enum class FileOutputType { Csv = 0, Dat = 1, Raw = 2 };

class OutputFile {
 private:
  FileOutputType type_ = FileOutputType::Csv;
  std::string name_;

 public:
  OutputFile(){};
  OutputFile(std::string value) {
    name(value);
    type(value);
  };
  std::string name() { return name_; }
  void name(std::string value) { name_ = value; }
  FileOutputType type() { return type_; }
  void type(std::string value) {
    std::string ext = std::filesystem::path(value).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::toupper);
    if (ext.empty()) {
      type_ = FileOutputType::Raw;
    } else if (ext == ".CSV") {
      type_ = FileOutputType::Csv;
    } else if (ext == ".DAT") {
      type_ = FileOutputType::Dat;
    } else if (ext == ".RAW") {
      type_ = FileOutputType::Raw;
    } else {
      Errors::cli_errors(CLIErrors::UNKNOWN_OUTPUT_TYPE, ext);
      type_ = FileOutputType::Csv;
    }
  }
};

}  // namespace JoSIM

#endif  // JOSIM_FILEOUTPUTTYPE_HPP
