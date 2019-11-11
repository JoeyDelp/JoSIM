// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/RelevantTrace.hpp"

void RelevantTrace::find_relevant_trace(const std::vector<std::string> &c, Matrix &mObj) {
  std::vector<std::string> storeCommands, tokens2;
  RelevantTrace temp;

  for (const auto &i : c) {
    if (i.find("PRINT") != std::string::npos) {
      if (i.at(0) == '.') {
        storeCommands.emplace_back(i.substr(1));
      } else {
        storeCommands.emplace_back(i);
      }
    } else if (i.find("PLOT") != std::string::npos) {
      if (i.at(0) == '.') {
        storeCommands.emplace_back(i.substr(1));
      } else {
        storeCommands.emplace_back(i);
      }
    } else if (i.find("SAVE") != std::string::npos) {
      if (i.at(0) == '.') {
        storeCommands.emplace_back(i.substr(1));
      } else {
        storeCommands.emplace_back(i);
      }
    }
  }

  storeCommands.erase(uniquify(storeCommands.begin(), storeCommands.end()), storeCommands.end());

  for (const auto &i : storeCommands) {
    std::vector<std::string> tokens = Misc::tokenize_space(i);
    if(tokens.size() < 3) {
      Errors::control_errors(static_cast<int>(ControlErrors::INVALID_OUTPUT_COMMAND), i);
    }
    for (auto &l : tokens) {
      if (l.find('_') != std::string::npos) {
        tokens2 = Misc::tokenize_delimiter(l, "_");
        l = tokens2.back();
        tokens2.pop_back();
        for (int k = tokens2.size() - 1; k >= 0; k--)
          l += "|" + tokens2.at(k);
      } else if (l.find('.') != std::string::npos) {
        std::replace(l.begin(), l.end(), '.', '|');
      }
    }
    
    switch(tokens.at(1).back()) {
      case ')':
        tokens2 = Misc::tokenize_delimiter(i, ")");
        tokens2.at(0) = tokens2.at(0).substr(tokens.at(2).find_last_of(" "));
        for(const auto &j : tokens2) {
          switch(j.at(0)) {
            case 'I':
              break;
            case 'V':
              break;
            case 'P':
              break;
          }
        }
        break;
      case 'V':
        break;
      case 'E':
      case 'P':
        break;
      case 'I':
        break;
      default:
        Errors::control_errors(static_cast<int>(ControlErrors::INVALID_OUTPUT_COMMAND), i);
        break;
    }
  }
}