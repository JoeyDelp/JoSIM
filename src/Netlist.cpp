// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/Netlist.hpp"
#include "JoSIM/AnalysisType.hpp"
#include "JoSIM/Errors.hpp"
#include "JoSIM/Misc.hpp"
#include "JoSIM/InputType.hpp"

using namespace JoSIM;

void Netlist::expand_subcircuits() {
  std::vector<std::string> tokens, io;
  std::vector<std::pair<std::string, std::string>> moddedLines;
  std::string subcktName, label, line;
  for (const auto &i : subcircuits) {
    for (const auto &j : i.second.lines) {
      if (j.first.at(0) == 'X') {
        subcircuits.at(i.first).containsSubckt = true;
        subcircuits.at(i.first).subcktCounter++;
        nestedSubcktCount++;
      }
    }
  }
  while (nestedSubcktCount != 0) {
    for (const auto &i : subcircuits) {
      for (int j = 0; j < subcircuits.at(i.first).lines.size(); ++j) {
        if (subcircuits.at(i.first).lines.at(j).first.at(0) == 'X') {
          tokens = Misc::tokenize_space(
              subcircuits.at(i.first).lines.at(j).first);
          label = tokens.at(0);
          if (argConv == InputType::Jsim) /* LEFT */ {
            subcktName = tokens.at(1);
            io.assign(tokens.begin() + 2, tokens.end());
          } else if (argConv == InputType::WrSpice) /* RIGHT */ {
            subcktName = tokens.back();
            io.assign(tokens.begin() + 1, tokens.end() - 1);
          }
          if (subcircuits.count(subcktName) != 0) {
            if (!subcircuits.at(subcktName).containsSubckt) {
              for (int k = 0; k < subcircuits.at(subcktName).lines.size(); ++k) {
                tokens = Misc::tokenize_space(
                    subcircuits.at(subcktName).lines.at(k).first);
                tokens.at(0) = tokens.at(0) + "|" + label;
                if (std::count(subcircuits.at(subcktName).io.begin(),
                               subcircuits.at(subcktName).io.end(),
                               tokens.at(1)) != 0) {
                  for (int l = 0;
                       l < subcircuits.at(subcktName).io.size(); ++l) {
                    if (tokens.at(1) ==
                        subcircuits.at(subcktName).io.at(l)) {
                      tokens.at(1) = io.at(l);
                      break;
                    }
                  }
                } else if (tokens.at(1) != "0" && tokens.at(1) != "GND") {
                  tokens.at(1) = tokens.at(1) + "|" + label;
                }
                if (std::count(subcircuits.at(subcktName).io.begin(),
                               subcircuits.at(subcktName).io.end(),
                               tokens.at(2)) != 0) {
                  for (int l = 0;
                       l < subcircuits.at(subcktName).io.size(); ++l) {
                    if (tokens.at(2) ==
                        subcircuits.at(subcktName).io.at(l)) {
                      tokens.at(2) = io.at(l);
                      break;
                    }
                  }
                } else if (tokens.at(2) != "0" && tokens.at(2) != "GND") {
                  tokens.at(2) = tokens.at(2) + "|" + label;
                }
                if(std::string("EFGHT").find(tokens.at(0).at(0)) != std::string::npos) {
                  if (std::count(subcircuits.at(subcktName).io.begin(),
                               subcircuits.at(subcktName).io.end(),
                               tokens.at(3)) != 0) {
                    for (int l = 0;
                        l < subcircuits.at(subcktName).io.size(); ++l) {
                      if (tokens.at(3) ==
                          subcircuits.at(subcktName).io.at(l)) {
                        tokens.at(3) = io.at(l);
                        break;
                      }
                    }
                  } else if (tokens.at(3) != "0" && tokens.at(3) != "GND") {
                    tokens.at(3) = tokens.at(3) + "|" + label;
                  }
                  if (std::count(subcircuits.at(subcktName).io.begin(),
                                subcircuits.at(subcktName).io.end(),
                                tokens.at(4)) != 0) {
                    for (int l = 0;
                        l < subcircuits.at(subcktName).io.size(); ++l) {
                      if (tokens.at(4) ==
                          subcircuits.at(subcktName).io.at(l)) {
                        tokens.at(4) = io.at(l);
                        break;
                      }
                    }
                  } else if (tokens.at(4) != "0" && tokens.at(4) != "GND") {
                    tokens.at(4) = tokens.at(4) + "|" + label;
                  }
                }
                line = tokens.at(0);
                for (int m = 1; m < tokens.size(); ++m)
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
            Errors::input_errors(InputErrors::UNKNOWN_SUBCKT, subcktName);
        }
      }
    }
  }
}

void Netlist::expand_maindesign() {
  std::vector<std::string> tokens, io;
  std::vector<std::pair<std::string, std::string>> moddedLines;
  std::string subcktName, label, line;
  for (int i = 0; i < maindesign.size(); ++i) {
    if (maindesign.at(i)[0] == 'X') {
      tokens = Misc::tokenize_space(maindesign.at(i));
      label = tokens.at(0);
      if (argConv == InputType::Jsim) /* LEFT */ {
        subcktName = tokens.at(1);
        io.assign(tokens.begin() + 2, tokens.end());
      } else if (argConv == InputType::WrSpice) /* RIGHT */ {
        subcktName = tokens.back();
        io.assign(tokens.begin() + 1, tokens.end() - 1);
      }
      if (io.size() == 0) {
        Errors::netlist_errors(NetlistErrors::MISSING_IO, maindesign.at(i));
      }
      if (subcircuits.count(subcktName) != 0) {
        for (int k = 0; k < subcircuits.at(subcktName).lines.size();
             ++k) {
          tokens = Misc::tokenize_space(
              subcircuits.at(subcktName).lines.at(k).first);
          tokens.at(0) = tokens.at(0) + "|" + label;
          if (std::count(subcircuits.at(subcktName).io.begin(),
                         subcircuits.at(subcktName).io.end(),
                         tokens.at(1)) != 0) {
            for (int l = 0; l < subcircuits.at(subcktName).io.size();
                 ++l) {
              if (tokens.at(1) == subcircuits.at(subcktName).io.at(l)) {
                tokens.at(1) = io.at(l);
                break;
              }
            }
          } else if (tokens.at(1) != "0" && tokens.at(1) != "GND") {
            tokens.at(1) = tokens.at(1) + "|" + label;
          }
          if (std::count(subcircuits.at(subcktName).io.begin(),
                         subcircuits.at(subcktName).io.end(),
                         tokens.at(2)) != 0) {
            for (int l = 0; l < subcircuits.at(subcktName).io.size();
                 ++l) {
              if (tokens.at(2) == subcircuits.at(subcktName).io.at(l)) {
                tokens.at(2) = io.at(l);
                break;
              }
            }
          } else if (tokens.at(2) != "0" && tokens.at(2) != "GND") {
            tokens.at(2) = tokens.at(2) + "|" + label;
          }
          if(std::string("EFGHT").find(tokens.at(0).at(0)) != std::string::npos) {
            if (std::count(subcircuits.at(subcktName).io.begin(),
                         subcircuits.at(subcktName).io.end(),
                         tokens.at(3)) != 0) {
              for (int l = 0; l < subcircuits.at(subcktName).io.size();
                  ++l) {
                if (tokens.at(3) == subcircuits.at(subcktName).io.at(l)) {
                  tokens.at(3) = io.at(l);
                  break;
                }
              }
            } else if (tokens.at(3) != "0" && tokens.at(3) != "GND") {
              tokens.at(3) = tokens.at(3) + "|" + label;
            }
            if (std::count(subcircuits.at(subcktName).io.begin(),
                          subcircuits.at(subcktName).io.end(),
                          tokens.at(4)) != 0) {
              for (int l = 0; l < subcircuits.at(subcktName).io.size();
                  ++l) {
                if (tokens.at(4) == subcircuits.at(subcktName).io.at(l)) {
                  tokens.at(4) = io.at(l);
                  break;
                }
              }
            } else if (tokens.at(4) != "0" && tokens.at(4) != "GND") {
              tokens.at(4) = tokens.at(4) + "|" + label;
            }
          }
          line = tokens.at(0);
          for (int m = 1; m < tokens.size(); ++m)
            line += " " + tokens.at(m);
          moddedLines.push_back(std::make_pair(
              line, subcircuits.at(subcktName).lines.at(k).second));
        }
        expNetlist.insert(expNetlist.end(), moddedLines.begin(),
                          moddedLines.end());
        moddedLines.clear();
      } else
        Errors::input_errors(InputErrors::UNKNOWN_SUBCKT, subcktName);
    } else
      expNetlist.push_back(std::make_pair(maindesign.at(i), ""));
  }
}