// Copyright (c) 2020 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/Input.hpp"

#include <fstream>
#include <iostream>
#include <regex>

using namespace JoSIM;

std::vector<std::string> Input::read_file(const std::string &fileName){
  std::string line;
  std::fstream ifile(fileName);
  std::vector<std::string> fileLines;
  if (ifile.is_open()) {
    while (!ifile.eof()) {
      getline(ifile, line);
      line = std::regex_replace(line, std::regex("^ +"), "");
      std::transform(line.begin(), line.begin() + 9, line.begin(), toupper);
      if (!line.empty() && line.back() == '\r')
          line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
      if (line.find(".INCLUDE") != std::string::npos) {
        std::vector<std::string> tempInclude = Input::read_file(fileName.substr(0, fileName.find_last_of('/') + 1) + line.substr(9));
        fileLines.insert(fileLines.end(), tempInclude.begin(), tempInclude.end());
      } else {
        std::transform(line.begin(), line.end(), line.begin(), toupper);
        if (!line.empty() && !Misc::starts_with(line, '*') && !Misc::starts_with(line, '#') && line.find_first_not_of(' ') != std::string::npos) {
          if(Misc::starts_with(line, '+')) 
            fileLines.back() = fileLines.back() + line.substr(line.find_first_of('+') + 1);
          else 
            fileLines.emplace_back(line);
        }
      }
    }
    if (fileLines.empty()) { 
      Errors::input_errors(InputErrors::EMPTY_FILE, fileName);
    } else {
      return fileLines;
    }
  } else {
    Errors::input_errors(InputErrors::CANNOT_OPEN_FILE, fileName);
  }
  return {};
}

std::vector<std::string> Input::read_input() {
  std::string line;
  std::vector<std::string> fileLines;
  for (std::string line; std::getline(std::cin, line);) {
    std::transform(line.begin(), line.begin() + 9, line.begin(), toupper);
    if(line == ".END") break;
    if (!line.empty() && line.back() == '\r')
        line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
    if (line.find(".INCLUDE") != std::string::npos) {
      std::vector<std::string> tempInclude = Input::read_file(line.substr(9));
      fileLines.insert(fileLines.end(), tempInclude.begin(), tempInclude.end());
    } else {
      std::transform(line.begin(), line.end(), line.begin(), toupper);
      if (!line.empty() && !Misc::starts_with(line, '*') && !Misc::starts_with(line, '#') && line.find_first_not_of(' ') != std::string::npos) {
        if(Misc::starts_with(line, '+')) 
          fileLines.back() = fileLines.back() + line.substr(line.find_first_of('+') + 1);
        else 
          fileLines.emplace_back(line);
      }
    }
  }
  if (fileLines.empty()) { 
    Errors::input_errors(InputErrors::EMPTY_FILE, "standard_input");
  } else {
    return fileLines;
  }
  return {};
}

void Input::parse_file(const std::string &fileName) {
  std::vector<std::string> fileLines;
  if(fileName == "standard_input") {
    fileLines = Input::read_input();
  } else {
    fileLines = Input::read_file(fileName);
  }

  bool subckt = false;
  bool control = false;
  std::string subcktName = "";
  std::vector<std::string> tokens;
  for (int i = 0; i < fileLines.size(); ++i) {
    // If line starts with '.' it is a control but could be the start or end
    // of a subcircuit section.
    if (fileLines.at(i)[0] == '.') {
      if (fileLines.at(i).find(".SUBCKT") != std::string::npos) {
        subckt = true;
        netlist.containsSubckt = true;
        tokens = Misc::tokenize_space(fileLines.at(i));
        if (tokens.size() > 1) {
          if (tokens.size() > 2) {
            subcktName = tokens.at(1);
            for (int j = 2; j < tokens.size(); ++j) {
              netlist.subcircuits[subcktName].io.push_back(tokens.at(j));
            }
          } else
            Errors::input_errors(InputErrors::MISSING_SUBCKT_IO);
        } else
          Errors::input_errors(InputErrors::MISSING_SUBCKT_NAME);
        // If parameter, add to unparsed parameters list
      } else if (fileLines.at(i).find(".PARAM") != std::string::npos) {
        if (subckt) {
          create_parameter(std::make_pair(subcktName, fileLines.at(i)), parameters);
        } else {
          create_parameter(std::make_pair("", fileLines.at(i)), parameters);
        }
        // If control, set flag as start of controls
      } else if (fileLines.at(i).find(".CONTROL") != std::string::npos) {
        control = true;
        // End subcircuit, set flag
      } else if (fileLines.at(i).find(".ENDS") != std::string::npos) {
        subckt = false;
        // End control section, set flag
      } else if (fileLines.at(i).find(".ENDC") != std::string::npos) {
        control = false;
        // If model, add model to models list
      } else if (fileLines.at(i).find(".MODEL") != std::string::npos) {
        tokens = Misc::tokenize_space(fileLines.at(i));
        if (subckt)
          netlist.models[std::make_pair(tokens.at(1), subcktName)] =
              fileLines.at(i);
        else
          netlist.models[std::make_pair(tokens.at(1), "")] = fileLines.at(i);
        // If neither of these, normal control, add to controls list
      } else {
        if (!subckt)
          controls.push_back(fileLines.at(i));
        else
          Errors::input_errors(InputErrors::SUBCKT_CONTROLS, subcktName);
      }
      // If control section flag set
    } else if (control) {
      // If parameter, add to unparsed parameter list
      if (fileLines.at(i).find("PARAM") != std::string::npos) {
        if (subckt) {
          create_parameter(std::make_pair(subcktName, fileLines.at(i)), parameters);
        } else {
          create_parameter(std::make_pair("", fileLines.at(i)), parameters);
        }
        // If model, add to models list
      } else if (fileLines.at(i).find("MODEL") != std::string::npos) {
        tokens = Misc::tokenize_space(fileLines.at(i));
        if (subckt)
          netlist.models[std::make_pair(tokens.at(1), subcktName)] =
              fileLines.at(i);
        else
          netlist.models[std::make_pair(tokens.at(1), "")] = fileLines.at(i);
        // If neither, add to controls list
      } else {
        if (!subckt)
          controls.push_back(fileLines.at(i));
        else
          Errors::input_errors(InputErrors::SUBCKT_CONTROLS, subcktName);
      }
      // If not a control, normal device line
    } else {
      // If subcircuit flag, add line to relevant subcircuit
      if (subckt)
        netlist.subcircuits[subcktName].lines.push_back(
            std::make_pair(fileLines.at(i), subcktName));
      // If not, add line to main design
      else
        netlist.maindesign.push_back(fileLines.at(i));
    }
  }
  // If main is empty, complain
  if (netlist.maindesign.empty())
    Errors::input_errors(InputErrors::MISSING_MAIN);
}