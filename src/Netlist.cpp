// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/Netlist.hpp"
#include "JoSIM/AnalysisType.hpp"
#include "JoSIM/Errors.hpp"
#include "JoSIM/Misc.hpp"
#include "JoSIM/InputType.hpp"

void Netlist::expand_subcircuits() {
  std::vector<std::string> tokens, io;
  std::vector<std::pair<std::string, std::string>> moddedLines;
  std::string subcktName, label, line;
  for (const auto &i : subcircuits) {
    for (const auto &j : i.second.lines) {
      if (j.first[0] == 'X') {
        subcircuits.at(i.first).containsSubckt = true;
        subcircuits.at(i.first).subcktCounter++;
        nestedSubcktCount++;
      }
    }
  }
  while (nestedSubcktCount != 0) {
    for (const auto &i : subcircuits) {
      for (int j = 0; j < subcircuits.at(i.first).lines.size(); j++) {
        if (subcircuits.at(i.first).lines.at(j).first[0] == 'X') {
          tokens = Misc::tokenize_space(
              subcircuits.at(i.first).lines.at(j).first);
          label = tokens.at(0);
          if (argConv == JoSIM::InputType::Jsim) /* LEFT */ {
            subcktName = tokens.at(1);
            io.assign(tokens.begin() + 2, tokens.end());
          } else if (argConv == JoSIM::InputType::WrSpice) /* RIGHT */ {
            subcktName = tokens.back();
            io.assign(tokens.begin() + 1, tokens.end() - 1);
          }
          if (subcircuits.count(subcktName) != 0) {
            if (!subcircuits.at(subcktName).containsSubckt) {
              for (int k = 0; k < subcircuits.at(subcktName).lines.size(); k++) {
                tokens = Misc::tokenize_space(
                    subcircuits.at(subcktName).lines.at(k).first);
                tokens[0] = tokens[0] + "|" + label;
                if (std::count(subcircuits.at(subcktName).io.begin(),
                               subcircuits.at(subcktName).io.end(),
                               tokens[1]) != 0) {
                  for (int l = 0;
                       l < subcircuits.at(subcktName).io.size(); l++) {
                    if (tokens[1] ==
                        subcircuits.at(subcktName).io.at(l)) {
                      tokens[1] = io.at(l);
                      break;
                    }
                  }
                } else if (tokens[1] != "0" && tokens[1] != "GND")
                  tokens[1] = tokens[1] + "|" + label;
                if (std::count(subcircuits.at(subcktName).io.begin(),
                               subcircuits.at(subcktName).io.end(),
                               tokens[2]) != 0) {
                  for (int l = 0;
                       l < subcircuits.at(subcktName).io.size(); l++) {
                    if (tokens[2] ==
                        subcircuits.at(subcktName).io.at(l)) {
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
                    subcircuits.at(subcktName).lines.at(k).second));
              }
              subcircuits.at(i.first).lines.erase(
                  subcircuits.at(i.first).lines.begin() + j);
              subcircuits.at(i.first).lines.insert(
                  subcircuits.at(i.first).lines.begin() + j,
                  moddedLines.begin(), moddedLines.end());
              moddedLines.clear();
              subcircuits.at(i.first).subcktCounter--;
              nestedSubcktCount--;
              if(subcircuits.at(i.first).subcktCounter == 0) 
                subcircuits.at(i.first).containsSubckt = false;
            }
          } else
            Errors::input_errors(static_cast<int>(InputErrors::UNKNOWN_SUBCKT), subcktName);
        }
      }
    }
  }
}

void Netlist::expand_maindesign() {
  std::vector<std::string> tokens, io;
  std::vector<std::pair<std::string, std::string>> moddedLines;
  std::string subcktName, label, line;
  for (int i = 0; i < maindesign.size(); i++) {
    if (maindesign.at(i)[0] == 'X') {
      tokens = Misc::tokenize_space(maindesign.at(i));
      label = tokens.at(0);
      if (argConv == JoSIM::InputType::Jsim) /* LEFT */ {
        subcktName = tokens.at(1);
        io.assign(tokens.begin() + 2, tokens.end());
      } else if (argConv == JoSIM::InputType::WrSpice) /* RIGHT */ {
        subcktName = tokens.back();
        io.assign(tokens.begin() + 1, tokens.end() - 1);
      }
      if (subcircuits.count(subcktName) != 0) {
        for (int k = 0; k < subcircuits.at(subcktName).lines.size();
             k++) {
          tokens = Misc::tokenize_space(
              subcircuits.at(subcktName).lines.at(k).first);
          tokens[0] = tokens[0] + "|" + label;
          if (std::count(subcircuits.at(subcktName).io.begin(),
                         subcircuits.at(subcktName).io.end(),
                         tokens[1]) != 0) {
            for (int l = 0; l < subcircuits.at(subcktName).io.size();
                 l++) {
              if (tokens[1] == subcircuits.at(subcktName).io.at(l)) {
                tokens[1] = io.at(l);
                break;
              }
            }
          } else if (tokens[1] != "0" && tokens[1] != "GND")
            tokens[1] = tokens[1] + "|" + label;
          if (std::count(subcircuits.at(subcktName).io.begin(),
                         subcircuits.at(subcktName).io.end(),
                         tokens[2]) != 0) {
            for (int l = 0; l < subcircuits.at(subcktName).io.size();
                 l++) {
              if (tokens[2] == subcircuits.at(subcktName).io.at(l)) {
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
              line, subcircuits.at(subcktName).lines.at(k).second));
        }
        expNetlist.insert(expNetlist.end(), moddedLines.begin(),
                          moddedLines.end());
        moddedLines.clear();
      } else
        Errors::input_errors(static_cast<int>(InputErrors::UNKNOWN_SUBCKT), subcktName);
    } else
      expNetlist.push_back(std::make_pair(maindesign.at(i), ""));
  }
}