// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#include "JoSIM/j_input.h"

#include "JoSIM/InputType.hpp"

using namespace JoSIM;

void Input::read_input_file(std::string &fileName,
                            std::vector<std::string> &fileLines) {
  std::string line;
  std::fstream ifile(fileName);
  if (ifile.is_open()) {
    while (!ifile.eof()) {
      getline(ifile, line);
      std::transform(line.begin(), line.end(), line.begin(), toupper);
      if (!line.empty() && line.back() == '\r')
        line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
      if (!line.empty() && !Misc::starts_with(line, '*'))
        fileLines.emplace_back(line);
    }
  } else
    Errors::input_errors(CANNOT_OPEN_FILE, fileName);
}

void Input::split_netlist(std::vector<std::string> &fileLines,
                          std::vector<std::string> &controls,
                          Parameters &parameters, Netlist &netlist) {
  bool subckt = false;
  bool control = false;
  std::string subcktName = "";
  std::vector<std::string> tokens;
  for (int i = 0; i < fileLines.size(); i++) {
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
            for (int j = 2; j < tokens.size(); j++) {
              netlist.subcircuits[subcktName].io.push_back(tokens.at(j));
            }
          } else
            Errors::input_errors(MISSING_SUBCKT_IO);
        } else
          Errors::input_errors(MISSING_SUBCKT_NAME);
        // If parameter, add to unparsed parameters list
      } else if (fileLines.at(i).find(".PARAM") != std::string::npos) {
        if (subckt)
          parameters.unparsedParams.push_back(
              std::make_pair(subcktName, fileLines.at(i)));
        else
          parameters.unparsedParams.push_back(
              std::make_pair("", fileLines.at(i)));
        // If control, set flag as start of controls
      } else if (fileLines.at(i).find(".CONTROL") != std::string::npos)
        control = true;
      // End subcircuit, set flag
      else if (fileLines.at(i).find(".ENDS") != std::string::npos)
        subckt = false;
      // End control section, set flag
      else if (fileLines.at(i).find(".ENDC") != std::string::npos)
        control = false;
      // If model, add model to models list
      else if (fileLines.at(i).find(".MODEL") != std::string::npos) {
        tokens = Misc::tokenize_space(fileLines.at(i));
        if (subckt)
          netlist.models[std::make_pair(tokens[1], subcktName)] =
              fileLines.at(i);
        else
          netlist.models[std::make_pair(tokens[1], "")] = fileLines.at(i);
        // If neither of these, normal control, add to controls list
      } else {
        if (!subckt)
          controls.push_back(fileLines.at(i));
        else
          Errors::input_errors(SUBCKT_CONTROLS, subcktName);
      }
      // If control section flag set
    } else if (control) {
      // If parameter, add to unparsed parameter list
      if (fileLines.at(i).find("PARAM") != std::string::npos) {
        if (subckt)
          parameters.unparsedParams.push_back(
              std::make_pair(subcktName, fileLines.at(i)));
        else
          parameters.unparsedParams.push_back(
              std::make_pair("", fileLines.at(i)));
        // If model, add to models list
      } else if (fileLines.at(i).find("MODEL") != std::string::npos) {
        tokens = Misc::tokenize_space(fileLines.at(i));
        if (subckt)
          netlist.models[std::make_pair(tokens[1], subcktName)] =
              fileLines.at(i);
        else
          netlist.models[std::make_pair(tokens[1], "")] = fileLines.at(i);
        // If neither, add to controls list
      } else {
        if (!subckt)
          controls.push_back(fileLines.at(i));
        else
          Errors::input_errors(SUBCKT_CONTROLS, subcktName);
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
    Errors::input_errors(MISSING_MAIN);
}

void Input::expand_subcircuits() {
  std::vector<std::string> tokens, io;
  std::vector<std::pair<std::string, std::string>> moddedLines;
  std::string subcktName, label, line;
  for (const auto &i : netlist.subcircuits) {
    for (const auto &j : i.second.lines) {
      if (j.first[0] == 'X') {
        netlist.subcircuits.at(i.first).containsSubckt = true;
        netlist.nestedSubcktCount++;
      }
    }
  }
  while (netlist.nestedSubcktCount != 0) {
    for (const auto &i : netlist.subcircuits) {
      for (int j = 0; j < netlist.subcircuits.at(i.first).lines.size(); j++) {
        if (netlist.subcircuits.at(i.first).lines.at(j).first[0] == 'X') {
          tokens = Misc::tokenize_space(
              netlist.subcircuits.at(i.first).lines.at(j).first);
          label = tokens.at(0);
          if (argConv == InputType::Jsim) /* LEFT */ {
            subcktName = tokens.at(1);
            io.assign(tokens.begin() + 2, tokens.end());
          } else if (argConv == InputType::WrSpice) /* RIGHT */ {
            subcktName = tokens.back();
            io.assign(tokens.begin() + 1, tokens.end() - 1);
          }
          if (netlist.subcircuits.count(subcktName) != 0) {
            if (!netlist.subcircuits.at(subcktName).containsSubckt) {
              for (int k = 0;
                   k < netlist.subcircuits.at(subcktName).lines.size(); k++) {
                tokens = Misc::tokenize_space(
                    netlist.subcircuits.at(subcktName).lines.at(k).first);
                tokens[0] = tokens[0] + "|" + label;
                if (std::count(netlist.subcircuits.at(subcktName).io.begin(),
                               netlist.subcircuits.at(subcktName).io.end(),
                               tokens[1]) != 0) {
                  for (int l = 0;
                       l < netlist.subcircuits.at(subcktName).io.size(); l++) {
                    if (tokens[1] ==
                        netlist.subcircuits.at(subcktName).io.at(l)) {
                      tokens[1] = io.at(l);
                      break;
                    }
                  }
                } else if (tokens[1] != "0" && tokens[1] != "GND")
                  tokens[1] = tokens[1] + "|" + label;
                if (std::count(netlist.subcircuits.at(subcktName).io.begin(),
                               netlist.subcircuits.at(subcktName).io.end(),
                               tokens[2]) != 0) {
                  for (int l = 0;
                       l < netlist.subcircuits.at(subcktName).io.size(); l++) {
                    if (tokens[2] ==
                        netlist.subcircuits.at(subcktName).io.at(l)) {
                      tokens[2] = io.at(l);
                      break;
                    }
                  }
                } else if (tokens[2] != "0" && tokens[2] != "GND")
                  tokens[2] = tokens[2] + "|" + label;
                line = tokens[0];
                for (int m = 1; m < tokens.size(); m++)
                  line += " " + tokens.at(m);
                moddedLines.push_back(std::make_pair(
                    line,
                    netlist.subcircuits.at(subcktName).lines.at(k).second));
              }
              netlist.subcircuits.at(i.first).lines.erase(
                  netlist.subcircuits.at(i.first).lines.begin() + j);
              netlist.subcircuits.at(i.first).lines.insert(
                  netlist.subcircuits.at(i.first).lines.begin() + j,
                  moddedLines.begin(), moddedLines.end());
              moddedLines.clear();
              netlist.nestedSubcktCount--;
              netlist.subcircuits.at(i.first).containsSubckt = false;
            }
          } else
            Errors::input_errors(UNKNOWN_SUBCKT, subcktName);
        }
      }
    }
  }
}

void Input::expand_maindesign() {
  std::vector<std::string> tokens, io;
  std::vector<std::pair<std::string, std::string>> moddedLines;
  std::string subcktName, label, line;
  for (int i = 0; i < netlist.maindesign.size(); i++) {
    if (netlist.maindesign.at(i)[0] == 'X') {
      tokens = Misc::tokenize_space(netlist.maindesign.at(i));
      label = tokens.at(0);
      if (argConv == InputType::Jsim) /* LEFT */ {
        subcktName = tokens.at(1);
        io.assign(tokens.begin() + 2, tokens.end());
      } else if (argConv == InputType::WrSpice) /* RIGHT */ {
        subcktName = tokens.back();
        io.assign(tokens.begin() + 1, tokens.end() - 1);
      }
      if (netlist.subcircuits.count(subcktName) != 0) {
        for (int k = 0; k < netlist.subcircuits.at(subcktName).lines.size();
             k++) {
          tokens = Misc::tokenize_space(
              netlist.subcircuits.at(subcktName).lines.at(k).first);
          tokens[0] = tokens[0] + "|" + label;
          if (std::count(netlist.subcircuits.at(subcktName).io.begin(),
                         netlist.subcircuits.at(subcktName).io.end(),
                         tokens[1]) != 0) {
            for (int l = 0; l < netlist.subcircuits.at(subcktName).io.size();
                 l++) {
              if (tokens[1] == netlist.subcircuits.at(subcktName).io.at(l)) {
                tokens[1] = io.at(l);
                break;
              }
            }
          } else if (tokens[1] != "0" && tokens[1] != "GND")
            tokens[1] = tokens[1] + "|" + label;
          if (std::count(netlist.subcircuits.at(subcktName).io.begin(),
                         netlist.subcircuits.at(subcktName).io.end(),
                         tokens[2]) != 0) {
            for (int l = 0; l < netlist.subcircuits.at(subcktName).io.size();
                 l++) {
              if (tokens[2] == netlist.subcircuits.at(subcktName).io.at(l)) {
                tokens[2] = io.at(l);
                break;
              }
            }
          } else if (tokens[2] != "0" && tokens[2] != "GND")
            tokens[2] = tokens[2] + "|" + label;
          line = tokens[0];
          for (int m = 1; m < tokens.size(); m++)
            line += " " + tokens.at(m);
          moddedLines.push_back(std::make_pair(
              line, netlist.subcircuits.at(subcktName).lines.at(k).second));
        }
        expNetlist.insert(expNetlist.end(), moddedLines.begin(),
                          moddedLines.end());
        moddedLines.clear();
      } else
        Errors::input_errors(UNKNOWN_SUBCKT, subcktName);
    } else
      expNetlist.push_back(std::make_pair(netlist.maindesign.at(i), ""));
  }
}
