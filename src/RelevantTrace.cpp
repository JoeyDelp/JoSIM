// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/RelevantTrace.hpp"

#include <filesystem>

#include "JoSIM/Input.hpp"
#include "JoSIM/Matrix.hpp"

using namespace JoSIM;

void JoSIM::find_relevant_traces(Input& iObj, Matrix& mObj) {
  // Temporary trace to store in known traces
  RelevantTrace temp;
  int64_t fIndex = -1;
  std::vector<tokens_t>& c = iObj.controls;
  // Loop through and handle plot, print and save commands
  for (int64_t i = 0; i < c.size(); ++i) {
    if ((c.at(i).front() == "PRINT") || (c.at(i).front() == "PLOT") ||
        (c.at(i).front() == "SAVE")) {
      // Mutable version of the control line tokens
      tokens_t t = c.at(i);
      // Ensure that the store command conforms to syntax
      if (t.size() < 2 && t.at(0) != "SAVE") {
        Errors::control_errors(ControlErrors::INVALID_OUTPUT_COMMAND,
                               Misc::vector_to_string(t));
      }
      // Fix any naming issues, '.' should be '|'
      for (auto& l : t) {
        if (l.find('.') != std::string::npos) {
          std::replace(l.begin(), l.end(), '.', '|');
        }
      }
      // Remove any "TRAN" tokens. We don't do that here.
      for (int64_t j = 0; j < t.size(); ++j) {
        if (t.at(j) == "TRAN") t.erase(t.begin() + j);
      }
      // Determine the type of plot
      for (int64_t j = 1; j < t.size(); ++j) {
        switch (t.at(j).back()) {
          case ')':
            switch (t.at(j).at(0)) {
              case 'I':
                t.at(j).erase(0, 2);
                t.at(j).erase(t.at(j).size() - 1, t.at(j).size());
                handle_current(t.at(j), mObj, fIndex);
                break;
              case 'V':
                t.at(j).erase(0, 2);
                t.at(j).erase(t.at(j).size() - 1, t.at(j).size());
                handle_voltage_or_phase(t.at(j), true, mObj, fIndex);
                break;
              case 'P':
                t.at(j).erase(0, 2);
                t.at(j).erase(t.at(j).size() - 1, t.at(j).size());
                handle_voltage_or_phase(t.at(j), false, mObj, fIndex);
                break;
            }
            break;
          case 'V':
            if (t.size() < 4) {
              handle_voltage_or_phase(t.at(j + 1), true, mObj, fIndex);
            } else {
              handle_voltage_or_phase(t.at(j + 1) + " " + t.at(j + 2), true,
                                      mObj, fIndex);
            }
            j = t.size();
            break;
          case 'E':
            handle_voltage_or_phase(t.at(j + 1), false, mObj, fIndex);
            j = t.size();
            break;
          case 'P':
            if (t.size() < 4) {
              handle_voltage_or_phase(t.at(j + 1), false, mObj, fIndex);
            } else {
              handle_voltage_or_phase(t.at(j + 1) + " " + t.at(j + 2), false,
                                      mObj, fIndex);
            }
            j = t.size();
            break;
          case 'I':
            handle_current(t.at(j + 1), mObj, fIndex);
            j = t.size();
            break;
          case 'H':
            if (t.at(j).find("#BRANCH") != std::string::npos) {
              t.at(j) = t.at(j).substr(0, t.at(j).find("#BRANCH"));
              handle_current(t.at(j), mObj, fIndex);
              break;
            } else {
              Errors::control_errors(
                  ControlErrors::INVALID_OUTPUT_COMMAND,
                  t.at(j) + " in line:\n" + Misc::vector_to_string(c.at(i)));
              break;
            }
          case ']':
            if (t.at(j).at(0) == '@') {
              switch (t.at(j).at(t.at(j).size() - 2)) {
                case 'C':
                  t.at(j).erase(0, 1);
                  t.at(j).erase(t.at(j).size() - 3, t.at(j).size());
                  handle_current(t.at(j), mObj, fIndex);
                  break;
                case 'V':
                  t.at(j).erase(0, 1);
                  t.at(j).erase(t.at(j).size() - 3, t.at(j).size());
                  handle_voltage_or_phase(t.at(j), true, mObj, fIndex);
                  break;
                case 'P':
                  t.at(j).erase(0, 1);
                  t.at(j).erase(t.at(j).size() - 3, t.at(j).size());
                  handle_voltage_or_phase(t.at(j), false, mObj, fIndex);
                  break;
              }
              break;
            } else {
              Errors::control_errors(
                  ControlErrors::INVALID_OUTPUT_COMMAND,
                  t.at(j) + " in line:\n" + Misc::vector_to_string(c.at(i)));
              break;
            }
          default:
            Errors::control_errors(
                ControlErrors::INVALID_OUTPUT_COMMAND,
                t.at(j) + " in line:\n" + Misc::vector_to_string(c.at(i)));
            break;
        }
        if (t.at(1) == "NODE") {
          break;
        }
      }
    } else if (c.at(i).front() == "FILE") {
      fIndex++;
    }
  }
  // Store the indices of the identified traces
  for (const auto& i : mObj.relevantTraces) {
    if (i.index1) {
      mObj.relevantIndices.emplace_back(i.index1.value());
    }
    if (i.index2) {
      mObj.relevantIndices.emplace_back(i.index2.value());
    }
    if (i.variableIndex) {
      mObj.relevantIndices.emplace_back(i.variableIndex.value());
    }
  }
  // Store the indices of all the transmission lines
  for (const auto& i : mObj.components.txIndices) {
    const auto& tempLocal =
        std::get<TransmissionLine>(mObj.components.devices.at(i));
    if (tempLocal.indexInfo.posIndex_) {
      mObj.relevantIndices.emplace_back(tempLocal.indexInfo.posIndex_.value());
    }
    if (tempLocal.indexInfo.negIndex_) {
      mObj.relevantIndices.emplace_back(tempLocal.indexInfo.negIndex_.value());
    }
    if (tempLocal.posIndex2_) {
      mObj.relevantIndices.emplace_back(tempLocal.posIndex2_.value());
    }
    if (tempLocal.negIndex2_) {
      mObj.relevantIndices.emplace_back(tempLocal.negIndex2_.value());
    }
    mObj.relevantIndices.emplace_back(
        tempLocal.indexInfo.currentIndex_.value());
    mObj.relevantIndices.emplace_back(tempLocal.currentIndex2_);
  }
  // Remove any dupicate indices used for storing
  mObj.relevantIndices.erase(
      uniquify(mObj.relevantIndices.begin(), mObj.relevantIndices.end()),
      mObj.relevantIndices.end());
}

void JoSIM::handle_current(const std::string& s, Matrix& mObj, int64_t fIndex) {
  RelevantTrace temp;
  temp.fIndex = fIndex;
  temp.storageType = StorageType::Current;
  if (s.at(0) != 'I') {
    for (int64_t j = 0; j < mObj.components.devices.size(); ++j) {
      const auto& l = std::visit(
          [](const auto& device) noexcept -> const std::string& {
            return device.netlistInfo.label_;
          },
          mObj.components.devices.at(j));

      if (l == s) {
        temp.deviceLabel = "\"I(" + s + ")\"";
        temp.device = true;
        temp.index1 = std::visit(
            [](const auto& device) noexcept -> const int64_t& {
              return device.indexInfo.currentIndex_.value();
            },
            mObj.components.devices.at(j));
        mObj.relevantTraces.emplace_back(temp);
        break;
      }
    }
    if (!temp.deviceLabel) {
      Errors::control_errors(ControlErrors::NODECURRENT, s);
    }
  } else {
    for (int64_t i = 0; i < mObj.components.currentsources.size(); ++i) {
      if (s == mObj.components.currentsources.at(i).netlistInfo.label_) {
        temp.deviceLabel = "\"I(" + s + ")\"";
        temp.device = true;
        temp.sourceIndex = mObj.components.currentsources.at(i).sourceIndex_;
        mObj.relevantTraces.emplace_back(temp);
        break;
      }
    }
  }
}

void JoSIM::handle_voltage_or_phase(const std::string& s, bool voltage,
                                    Matrix& mObj, int64_t fIndex) {
  std::vector<std::string> tokens = Misc::tokenize(s, " ,");
  RelevantTrace temp;
  temp.fIndex = fIndex;
  if (voltage) {
    temp.storageType = StorageType::Voltage;
  } else {
    temp.storageType = StorageType::Phase;
  }
  for (int64_t j = 0; j < mObj.components.devices.size(); ++j) {
    const auto& l = std::visit(
        [](const auto& device) noexcept -> const std::string& {
          return device.netlistInfo.label_;
        },
        mObj.components.devices.at(j));

    if (l == tokens.at(0)) {
      if (tokens.size() < 1) {
        Errors::control_errors(ControlErrors::MATHOPS, s);
      } else {
        if (s.at(0) != 'B') {
          if (voltage) {
            temp.deviceLabel = "\"V(" + s + ")\"";
          } else {
            temp.deviceLabel = "\"P(" + s + ")\"";
          }
          temp.device = true;
          temp.index1 = std::visit(
              [](const auto& device) noexcept -> const int_o& {
                return device.indexInfo.posIndex_;
              },
              mObj.components.devices.at(j));
          temp.index2 = std::visit(
              [](const auto& device) noexcept -> const int_o& {
                return device.indexInfo.negIndex_;
              },
              mObj.components.devices.at(j));
          mObj.relevantTraces.emplace_back(temp);
          break;
        } else {
          if (voltage) {
            temp.deviceLabel = "\"V(" + s + ")\"";
          } else {
            temp.deviceLabel = "\"P(" + s + ")\"";
          }
          temp.device = true;
          temp.index1 = std::visit(
              [](const auto& device) noexcept -> const int_o& {
                return device.indexInfo.posIndex_;
              },
              mObj.components.devices.at(j));
          temp.index2 = std::visit(
              [](const auto& device) noexcept -> const int_o& {
                return device.indexInfo.negIndex_;
              },
              mObj.components.devices.at(j));
          try {
            temp.variableIndex =
                std::get<JJ>(mObj.components.devices.at(j)).variableIndex_;
            mObj.relevantTraces.emplace_back(temp);
            break;
          } catch (const std::bad_variant_access&) {
          }
        }
      }
    }
  }
  if (!temp.deviceLabel) {
    if (tokens.size() > 1) {
      if (mObj.nm.count(tokens.at(0)) != 0) {
        temp.index1 = mObj.nm.at(tokens.at(0));
        if (mObj.nm.count(tokens.at(1)) != 0) {
          temp.index2 = mObj.nm.at(tokens.at(1));
        } else {
          if (tokens.at(1) != "0" &&
              tokens.at(1).find("GND") == std::string::npos) {
            Errors::control_errors(ControlErrors::UNKNOWN_DEVICE, tokens.at(1));
          }
        }
        if (voltage) {
          temp.deviceLabel = "\"V(" + tokens.at(0) + "," + tokens.at(1) + ")\"";
        } else {
          temp.deviceLabel = "\"P(" + tokens.at(0) + "," + tokens.at(1) + ")\"";
        }
        mObj.relevantTraces.emplace_back(temp);
      } else {
        if (tokens.at(0) != "0" &&
            tokens.at(0).find("GND") == std::string::npos) {
          Errors::control_errors(ControlErrors::UNKNOWN_DEVICE, tokens.at(0));
        }
      }
    } else {
      if (mObj.nm.count(s) != 0) {
        temp.index1 = mObj.nm.at(s);
        if (voltage) {
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