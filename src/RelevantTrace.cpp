// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/RelevantTrace.hpp"
#include "JoSIM/Matrix.hpp"

void RelevantTrace::find_relevant_traces(const std::vector<std::string> &c, Matrix &mObj) {
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
              handle_current(j, mObj);
              break;
            case 'V':
              handle_voltage_or_phase(j, true, mObj);
              break;
            case 'P':
              handle_voltage_or_phase(j, false, mObj);
              break;
          }
        }
        break;
      case 'V':
        handle_voltage_or_phase(tokens.at(2), true, mObj);
        break;
      case 'E':
      case 'P':
        handle_voltage_or_phase(tokens.at(2), false, mObj);
        break;
      case 'I':
        handle_current(tokens.at(2), mObj);
        break;
      default:
        Errors::control_errors(static_cast<int>(ControlErrors::INVALID_OUTPUT_COMMAND), i);
        break;
    }
  }

  for(const auto &i : mObj.relevantTraces) {
    if(i.index1) {
      mObj.relevantIndices.emplace_back(i.index1.value());
    }
    if(i.index2) {
      mObj.relevantIndices.emplace_back(i.index2.value());
    }
  }
}

void RelevantTrace::handle_current(const std::string &s, Matrix &mObj) {
  RelevantTrace temp;
  temp.storageType = static_cast<int>(JoSIM::StorageType::Current);
  for(int j = 0; j < mObj.components_new.devices.size(); ++j) {
    const auto& l = std::visit([](const auto& device) noexcept -> const std::string& {
      return device.get_label();
    }, mObj.components_new.devices.at(j));

    if(l == s) {
      temp.deviceLabel = s;
      temp.device = true;
      temp.index1 = std::visit([](const auto& device) noexcept -> const int& {
        return device.get_currentIndex();
      }, mObj.components_new.devices.at(j));
      mObj.relevantTraces.emplace_back(temp);
      break;
    }
  }
  if(!temp.deviceLabel) {
    Errors::control_errors(static_cast<int>(ControlErrors::NODECURRENT), s);
  }
}

void RelevantTrace::handle_voltage_or_phase(const std::string &s, bool voltage, Matrix &mObj) {
  std::vector<std::string> tokens = Misc::tokenize_delimiter(s, ",");
  RelevantTrace temp;
  if(voltage) {
    temp.storageType = static_cast<int>(JoSIM::StorageType::Voltage);
  } else {
    temp.storageType = static_cast<int>(JoSIM::StorageType::Phase);
  }
  for(int j = 0; j < mObj.components_new.devices.size(); ++j) {
    const auto& l = std::visit([](const auto& device) noexcept -> const std::string& {
      return device.get_label();
    }, mObj.components_new.devices.at(j));

    if(l == s) {
      if(tokens.size() < 1) {
        Errors::control_errors(static_cast<int>(ControlErrors::MATHOPS), s);
      } else {
        if(s.at(0) != 'B') {
          temp.deviceLabel = s;
          temp.device = true;
          temp.index1 = std::visit([](const auto& device) noexcept -> const std::optional<int>& {
            return device.get_posIndex();
          }, mObj.components_new.devices.at(j));
          temp.index2 = std::visit([](const auto& device) noexcept -> const std::optional<int>& {
            return device.get_negIndex();
          }, mObj.components_new.devices.at(j));
          mObj.relevantTraces.emplace_back(temp);
          break;
        } else {
            temp.deviceLabel = s;
            temp.device = true;
            try {
              temp.index1 = std::get<JJ>(mObj.components_new.devices.at(j)).get_variableIndex();
              mObj.relevantTraces.emplace_back(temp);
              break;
            } catch (const std::bad_variant_access&) {}
        }
      }
    }
  }
  if(!temp.deviceLabel) {
    if (tokens.size() < 1) {
      if(mObj.nm.count(tokens.at(0)) != 0) {
        temp.index1 = mObj.nm.at(tokens.at(0));
        if(mObj.nm.count(tokens.at(1)) != 0) {
          temp.index2 = mObj.nm.at(tokens.at(1));
        } else {
          Errors::control_errors(static_cast<int>(ControlErrors::UNKNOWN_DEVICE), s);
        }
        mObj.relevantTraces.emplace_back(temp);
      } else {
        Errors::control_errors(static_cast<int>(ControlErrors::UNKNOWN_DEVICE), s);
      }
    } else {
      if(mObj.nm.count(s) != 0) {
        temp.index1 = mObj.nm.at(s);
        mObj.relevantTraces.emplace_back(temp);
      } else {
        Errors::control_errors(static_cast<int>(ControlErrors::UNKNOWN_DEVICE), s);
      }
    }
  }
}