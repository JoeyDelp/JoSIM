// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/RelevantTrace.hpp"
#include "JoSIM/Matrix.hpp"

void JoSIM::RelevantTrace::find_relevant_traces(const std::vector<std::string> &c, JoSIM::Matrix &mObj) {
  std::vector<std::string> storeCommands, tokens2;
  JoSIM::RelevantTrace temp;

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
    // Tokenize each store command using spaces
    std::vector<std::string> tokens = Misc::tokenize_space(i);
    // Complain if any of the commands have less than 3 tokens
    if(tokens.size() < 2 && tokens.at(0) != "SAVE") {
      Errors::control_errors(ControlErrors::INVALID_OUTPUT_COMMAND, i);
    }
    // Fix any naming issues
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
    // Remove any "TRAN" tokens. We don't do that here. 
    for (int j = 0; j < tokens.size(); ++j) {
      if (tokens.at(j).find("TRAN") != std::string::npos) {
        tokens.erase(tokens.begin() + j);
      }
    }
    // Determine the type of plot
    for (int j = 1; j < tokens.size(); ++j) {
      switch(tokens.at(j).back()) {
        case ')':
          switch(tokens.at(j).at(0)) {
            case 'I':
              tokens.at(j).erase(0, 2);
              tokens.at(j).erase(tokens.at(j).size() - 1, tokens.at(j).size());
              handle_current(tokens.at(j), mObj);
              break;
            case 'V':
              tokens.at(j).erase(0, 2);
              tokens.at(j).erase(tokens.at(j).size() - 1, tokens.at(j).size());
              handle_voltage_or_phase(tokens.at(j), true, mObj);
              break;
            case 'P':
              tokens.at(j).erase(0, 2);
              tokens.at(j).erase(tokens.at(j).size() - 1, tokens.at(j).size());
              handle_voltage_or_phase(tokens.at(j), false, mObj);
              break;
          }
          break;
        case 'V':
          if (tokens.size() < 4) {
            handle_voltage_or_phase(tokens.at(j+1), true, mObj);
          } else {
            handle_voltage_or_phase(tokens.at(j+1) + " " + tokens.at(j+2), true, mObj);
          }
          j = tokens.size();
          break;
        case 'E':
          handle_voltage_or_phase(tokens.at(j+1), false, mObj);
          j = tokens.size();
          break;
        case 'P':
          if (tokens.size() < 4) {
            handle_voltage_or_phase(tokens.at(j+1), false, mObj);
          } else {
            handle_voltage_or_phase(tokens.at(j+1) + " " + tokens.at(j+2), false, mObj);
          }
          j = tokens.size();
          break;
        case 'I':
          handle_current(tokens.at(j+1), mObj);
          j = tokens.size();
          break;
        case 'H':
          if(tokens.at(j).find("#BRANCH") != std::string::npos) {
            tokens.at(j) = tokens.at(j).substr(0, tokens.at(j).find("#BRANCH"));
            handle_current(tokens.at(j), mObj);
            break;
          } else {
            Errors::control_errors(ControlErrors::INVALID_OUTPUT_COMMAND, tokens.at(j) + " in line:\n" + i);
            break;
          }
        case ']':
          if(tokens.at(j).at(0) == '@') { 
            switch(tokens.at(j).at(tokens.at(j).size() - 2)){
              case 'C':
                tokens.at(j).erase(0, 1);
                tokens.at(j).erase(tokens.at(j).size() - 3, tokens.at(j).size());
                handle_current(tokens.at(j), mObj);
                break;
              case 'V':
                tokens.at(j).erase(0, 1);
                tokens.at(j).erase(tokens.at(j).size() - 3, tokens.at(j).size());
                handle_voltage_or_phase(tokens.at(j), true, mObj);
                break;
              case 'P':
                tokens.at(j).erase(0, 1);
                tokens.at(j).erase(tokens.at(j).size() - 3, tokens.at(j).size());
                handle_voltage_or_phase(tokens.at(j), false, mObj);
                break;
            }
            break;
          } else {
            Errors::control_errors(ControlErrors::INVALID_OUTPUT_COMMAND, tokens.at(j) + " in line:\n" + i);
            break;
          }
        default:
          Errors::control_errors(ControlErrors::INVALID_OUTPUT_COMMAND, tokens.at(j) + " in line:\n" + i);
          break;
      }
      if(tokens.at(1).find("NODE") != std::string::npos) {
        break;
      }
    }
  }

  for(const auto &i : mObj.relevantTraces) {
    if(i.index1) {
      mObj.relevantIndices.emplace_back(i.index1.value());
    }
    if(i.index2) {
      mObj.relevantIndices.emplace_back(i.index2.value());
    }
    if(i.variableIndex) {
      mObj.relevantIndices.emplace_back(i.variableIndex.value());
    }
  }

  for(const auto &i : mObj.components.txIndices) {
    const auto &tempLocal = std::get<TransmissionLine>(mObj.components.devices.at(i));
    if(tempLocal.get_posIndex()) {
      mObj.relevantIndices.emplace_back(tempLocal.get_posIndex().value());
    }
    if(tempLocal.get_negIndex()) {
      mObj.relevantIndices.emplace_back(tempLocal.get_negIndex().value());
    }
    if(tempLocal.get_posIndex2()) {
      mObj.relevantIndices.emplace_back(tempLocal.get_posIndex2().value());
    }
    if(tempLocal.get_negIndex2()) {
      mObj.relevantIndices.emplace_back(tempLocal.get_negIndex2().value());
    }
    mObj.relevantIndices.emplace_back(tempLocal.get_currentIndex());
    mObj.relevantIndices.emplace_back(tempLocal.get_currentIndex2());
  }
  
  mObj.relevantIndices.erase(uniquify(mObj.relevantIndices.begin(), mObj.relevantIndices.end()), mObj.relevantIndices.end());
}

void JoSIM::RelevantTrace::handle_current(const std::string &s, JoSIM::Matrix &mObj) {
  JoSIM::RelevantTrace temp;
  temp.storageType = JoSIM::StorageType::Current;
  if(s.at(0) != 'I'){
    for(int j = 0; j < mObj.components.devices.size(); ++j) {
      const auto& l = std::visit([](const auto& device) noexcept -> const std::string& {
        return device.get_label();
      }, mObj.components.devices.at(j));

      if(l == s) {
        temp.deviceLabel = "\"I(" + s + ")\"";
        temp.device = true;
        temp.index1 = std::visit([](const auto& device) noexcept -> const int& {
          return device.get_currentIndex();
        }, mObj.components.devices.at(j));
        mObj.relevantTraces.emplace_back(temp);
        break;
      }
    }
    if(!temp.deviceLabel) {
      Errors::control_errors(ControlErrors::NODECURRENT, s);
    }
  } else {
    for (int i = 0; i < mObj.components.currentsources.size(); ++i) {
      if(s == mObj.components.currentsources.at(i).get_label()) {
        temp.deviceLabel = "\"I(" + s + ")\"";
        temp.device = true;
        temp.sourceIndex = mObj.components.currentsources.at(i).get_sourceIndex();
        mObj.relevantTraces.emplace_back(temp);
        break;
      }
    }
  }
}

void JoSIM::RelevantTrace::handle_voltage_or_phase(const std::string &s, bool voltage, JoSIM::Matrix &mObj) {
  std::vector<std::string> tokens = Misc::tokenize_delimiter(s, " ,");
  JoSIM::RelevantTrace temp;
  if(voltage) {
    temp.storageType = JoSIM::StorageType::Voltage;
  } else {
    temp.storageType = JoSIM::StorageType::Phase;
  }
  for(int j = 0; j < mObj.components.devices.size(); ++j) {
    const auto& l = std::visit([](const auto& device) noexcept -> const std::string& {
      return device.get_label();
    }, mObj.components.devices.at(j));

    if(l == tokens.at(0)) {
      if(tokens.size() < 1) {
        Errors::control_errors(ControlErrors::MATHOPS, s);
      } else {
        if(s.at(0) != 'B') {
          if(voltage) {
            temp.deviceLabel = "\"V(" + s + ")\"";
          } else {
            temp.deviceLabel = "\"P(" + s + ")\"";
          }
          temp.device = true;
          temp.index1 = std::visit([](const auto& device) noexcept -> const std::optional<int>& {
            return device.get_posIndex();
          }, mObj.components.devices.at(j));
          temp.index2 = std::visit([](const auto& device) noexcept -> const std::optional<int>& {
            return device.get_negIndex();
          }, mObj.components.devices.at(j));
          mObj.relevantTraces.emplace_back(temp);
          break;
        } else {
            if(voltage) {
              temp.deviceLabel = "\"V(" + s + ")\"";
            } else {
              temp.deviceLabel = "\"P(" + s + ")\"";
            }
            temp.device = true;
            temp.index1 = std::visit([](const auto& device) noexcept -> const std::optional<int>& {
            return device.get_posIndex();
            }, mObj.components.devices.at(j));
            temp.index2 = std::visit([](const auto& device) noexcept -> const std::optional<int>& {
              return device.get_negIndex();
            }, mObj.components.devices.at(j));
            try {
              temp.variableIndex = std::get<JJ>(mObj.components.devices.at(j)).get_variableIndex();
              mObj.relevantTraces.emplace_back(temp);
              break;
            } catch (const std::bad_variant_access&) {}
        }
      }
    }
  }
  if(!temp.deviceLabel) {
    if (tokens.size() > 1) {
      if(mObj.nm.count(tokens.at(0)) != 0) {
        temp.index1 = mObj.nm.at(tokens.at(0));
        if(mObj.nm.count(tokens.at(1)) != 0) {
          temp.index2 = mObj.nm.at(tokens.at(1));
        } else {
          if (tokens.at(1) != "0" && tokens.at(1).find("GND") == std::string::npos) {
            Errors::control_errors(ControlErrors::UNKNOWN_DEVICE, tokens.at(1));
          }
        }
        if(voltage) {
          temp.deviceLabel = "\"V(" + tokens.at(0) + "," + tokens.at(1) + ")\"";
        } else {
          temp.deviceLabel = "\"P(" + tokens.at(0) + "," + tokens.at(1) + ")\"";
        }
        mObj.relevantTraces.emplace_back(temp);
      } else {
       if (tokens.at(0) != "0" && tokens.at(0).find("GND") == std::string::npos) {
            Errors::control_errors(ControlErrors::UNKNOWN_DEVICE, tokens.at(0));
          }
      }
    } else {
      if(mObj.nm.count(s) != 0) {
        temp.index1 = mObj.nm.at(s);
        if(voltage) {
          temp.deviceLabel = "\"V(" + tokens.at(0) + ")\"";
        } else {
          temp.deviceLabel = "\"P(" + tokens.at(0) + ")\"";
        }
        mObj.relevantTraces.emplace_back(temp);
      } else {
        Errors::control_errors(ControlErrors::UNKNOWN_DEVICE, s);
      }
    }
  }
}