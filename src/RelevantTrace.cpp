// Copyright (c) 2025 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/RelevantTrace.hpp"

#include "JoSIM/Input.hpp"
#include "JoSIM/Matrix.hpp"

#include <filesystem>

using namespace JoSIM;

namespace {
tokens_t trace_operands(const std::string& s) {
    const auto operands = Misc::tokenize(s, " ,\t");
    const auto comma = s.find(',');
    if (operands.empty() || operands.size() > 2
        || (comma != std::string::npos && (s.find(',', comma + 1) != std::string::npos
            || Misc::tokenize(s.substr(0, comma)).empty() || Misc::tokenize(s.substr(comma + 1)).empty()))) {
        Errors::control_errors(ControlErrors::INVALID_OUTPUT_COMMAND, s);
    }
    return operands;
}

void voltage_or_phase(const std::string& s, bool voltage, Matrix& matrix, int64_t file,
                      const s_map* aliases);
}

void JoSIM::find_relevant_traces(Input& iObj, Matrix& mObj) {
    const auto node_trace = [&](const std::string& s, bool voltage, Matrix& matrix, int64_t file) {
        voltage_or_phase(s, voltage, matrix, file, &iObj.netlist.nodeAliases);
    };
    // Temporary trace to store in known traces
    RelevantTrace          temp;
    int64_t                fIndex = -1;
    std::vector<tokens_t>& c      = iObj.controls;
    // Loop through and handle plot, print and save commands
    for (auto i = 0; i < c.size(); ++i) {
        if ((c.at(i).front() == "PRINT") || (c.at(i).front() == "PLOT") || (c.at(i).front() == "SAVE")) {
            // Mutable version of the control line tokens
            tokens_t t = c.at(i);
            // Ensure that the store command conforms to syntax
            if (t.size() < 2 && t.at(0) != "SAVE") {
                Errors::control_errors(ControlErrors::INVALID_OUTPUT_COMMAND, Misc::vector_to_string(t));
            }
            // Fix any naming issues, '.' should be '|'
            for (auto& l : t) {
                if (l.find('.') != std::string::npos) { std::replace(l.begin(), l.end(), '.', '|'); }
            }
            // TRAN is an optional analysis selector directly after PRINT/PLOT.
            if (t.size() > 1 && t[1] == "TRAN") { t.erase(t.begin() + 1); }
            if (t.size() < 2 && t.front() != "SAVE") {
                Errors::control_errors(ControlErrors::INVALID_OUTPUT_COMMAND, Misc::vector_to_string(t));
            }
            // Determine the type of plot
            for (size_t j = 1; j < t.size(); ++j) {
                if (t[j].empty()) { Errors::control_errors(ControlErrors::INVALID_OUTPUT_COMMAND); }
                const auto suffix = t[j].back();
                if (suffix == ')' && (t[j].size() < 4 || t[j][1] != '(')) {
                    Errors::control_errors(ControlErrors::INVALID_OUTPUT_COMMAND, t[j]);
                }
                if (suffix != ')' && suffix != ']' && suffix != 'H') {
                    const auto& command = t[j];
                    if (command != "DEVV" && command != "NODEV" && command != "DEVI"
                        && command != "PHASE" && command != "NODEP" && command != "DEVP") {
                        Errors::control_errors(ControlErrors::UNKNOWN_PLOT, command);
                    }
                    if (j + 1 >= t.size()) { Errors::control_errors(ControlErrors::INVALID_OUTPUT_COMMAND, command); }
                    const auto operands = t.size() - j - 1;
                    if (operands > 2 || ((command == "DEVI" || command == "PHASE") && operands > 1)) {
                        Errors::control_errors(ControlErrors::INVALID_OUTPUT_COMMAND, Misc::vector_to_string(t));
                    }
                }
                switch (t.at(j).back()) {
                    case ')':
                        switch (t.at(j).at(0)) {
                            case 'I':
                            case 'C':
                                t.at(j).erase(0, 2);
                                t.at(j).erase(t.at(j).size() - 1, t.at(j).size());
                                handle_current(t.at(j), mObj, fIndex);
                                break;
                            case 'V':
                                t.at(j).erase(0, 2);
                                t.at(j).erase(t.at(j).size() - 1, t.at(j).size());
                                node_trace(t.at(j), true, mObj, fIndex);
                                break;
                            case 'P':
                                t.at(j).erase(0, 2);
                                t.at(j).erase(t.at(j).size() - 1, t.at(j).size());
                                node_trace(t.at(j), false, mObj, fIndex);
                                break;
                            default: Errors::control_errors(ControlErrors::UNKNOWN_PLOT, t[j]);
                        }
                        break;
                    case 'V':
                        if (t.size() - j < 3) {
                            node_trace(t.at(j + 1), true, mObj, fIndex);
                        } else {
                            node_trace(t.at(j + 1) + " " + t.at(j + 2), true, mObj, fIndex);
                        }
                        j = t.size();
                        break;
                    case 'E':
                        node_trace(t.at(j + 1), false, mObj, fIndex);
                        j = t.size();
                        break;
                    case 'P':
                        if (t.size() - j < 3) {
                            node_trace(t.at(j + 1), false, mObj, fIndex);
                        } else {
                            node_trace(t.at(j + 1) + " " + t.at(j + 2), false, mObj, fIndex);
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
                            Errors::control_errors(ControlErrors::INVALID_OUTPUT_COMMAND,
                                                   t.at(j) + " in line:\n" + Misc::vector_to_string(c.at(i)));
                            break;
                        }
                    case ']':
                        if (t.at(j).size() >= 4 && t.at(j).at(0) == '@') {
                            switch (t.at(j).at(t.at(j).size() - 2)) {
                                case 'C':
                                    t.at(j).erase(0, 1);
                                    t.at(j).erase(t.at(j).size() - 3, t.at(j).size());
                                    handle_current(t.at(j), mObj, fIndex);
                                    break;
                                case 'V':
                                    t.at(j).erase(0, 1);
                                    t.at(j).erase(t.at(j).size() - 3, t.at(j).size());
                                    node_trace(t.at(j), true, mObj, fIndex);
                                    break;
                                case 'P':
                                    t.at(j).erase(0, 1);
                                    t.at(j).erase(t.at(j).size() - 3, t.at(j).size());
                                    node_trace(t.at(j), false, mObj, fIndex);
                                    break;
                                default: Errors::control_errors(ControlErrors::UNKNOWN_PLOT, t[j]);
                            }
                            break;
                        } else {
                            Errors::control_errors(ControlErrors::INVALID_OUTPUT_COMMAND,
                                                   t.at(j) + " in line:\n" + Misc::vector_to_string(c.at(i)));
                            break;
                        }
                    default:
                        Errors::control_errors(ControlErrors::INVALID_OUTPUT_COMMAND,
                                               t.at(j) + " in line:\n" + Misc::vector_to_string(c.at(i)));
                        break;
                }
                if (t.at(1) == "NODE") { break; }
            }
        } else if (c.at(i).front() == "FILE") {
            fIndex++;
        }
    }
    // Store the indices of the identified traces
    const auto store_indices = [&](const RelevantTrace& i) {
        if (i.index1) { mObj.relevantIndices.emplace_back(i.index1.value()); }
        if (i.index2) { mObj.relevantIndices.emplace_back(i.index2.value()); }
        if (i.variableIndex) { mObj.relevantIndices.emplace_back(i.variableIndex.value()); }
    };
    for (const auto& i : mObj.relevantTraces) {
        store_indices(i);
        if (i.subtract) { store_indices(*i.subtract); }
    }
    // Store the indices of all the transmission lines
    for (const auto& i : mObj.components.txIndices) {
        const auto& tempLocal = std::get<TransmissionLine>(mObj.components.devices.at(i));
        if (tempLocal.indexInfo.posIndex_) { mObj.relevantIndices.emplace_back(tempLocal.indexInfo.posIndex_.value()); }
        if (tempLocal.indexInfo.negIndex_) { mObj.relevantIndices.emplace_back(tempLocal.indexInfo.negIndex_.value()); }
        if (tempLocal.posIndex2_) { mObj.relevantIndices.emplace_back(tempLocal.posIndex2_.value()); }
        if (tempLocal.negIndex2_) { mObj.relevantIndices.emplace_back(tempLocal.negIndex2_.value()); }
        mObj.relevantIndices.emplace_back(tempLocal.indexInfo.currentIndex_.value());
        mObj.relevantIndices.emplace_back(tempLocal.currentIndex2_);
    }
    // Remove any dupicate indices used for storing
    mObj.relevantIndices.erase(uniquify(mObj.relevantIndices.begin(), mObj.relevantIndices.end()),
                               mObj.relevantIndices.end());
}

void JoSIM::handle_current(const std::string& s, Matrix& mObj, int64_t fIndex) {
    if (s.empty()) { Errors::control_errors(ControlErrors::INVALID_OUTPUT_COMMAND, s); }
    const auto operands = trace_operands(s);
    if (operands.size() == 2) {
        const auto start = mObj.relevantTraces.size();
        handle_current(operands[0], mObj, fIndex);
        handle_current(operands[1], mObj, fIndex);
        auto first = mObj.relevantTraces[start];
        first.subtract = std::make_shared<RelevantTrace>(mObj.relevantTraces[start + 1]);
        first.deviceLabel = "\"I(" + s + ")\"";
        mObj.relevantTraces.resize(start);
        mObj.relevantTraces.emplace_back(std::move(first));
        return;
    }
    RelevantTrace temp;
    temp.fIndex      = fIndex;
    temp.storageType = StorageType::Current;
    if (s.at(0) != 'I') {
        for (auto j = 0; j < mObj.components.devices.size(); ++j) {
            const auto& l = std::visit(
                    [](const auto& device) noexcept -> const std::string& { return device.netlistInfo.label_; },
                    mObj.components.devices.at(j));

            if (l == s) {
                temp.deviceLabel = "\"I(" + s + ")\"";
                temp.device      = true;
                temp.index1      = std::visit(
                        [](const auto& device) noexcept -> const int64_t& {
                            return device.indexInfo.currentIndex_.value();
                        },
                        mObj.components.devices.at(j));
                mObj.relevantTraces.emplace_back(temp);
                break;
            }
        }
        if (!temp.deviceLabel) { Errors::control_errors(ControlErrors::NODECURRENT, s); }
    } else {
        for (auto i = 0; i < mObj.components.currentsources.size(); ++i) {
            if (s == mObj.components.currentsources.at(i).netlistInfo.label_) {
                temp.deviceLabel = "\"I(" + s + ")\"";
                temp.device      = true;
                temp.sourceIndex = mObj.components.currentsources.at(i).sourceIndex_;
                mObj.relevantTraces.emplace_back(temp);
                break;
            }
        }
    }
    if (!temp.deviceLabel) { Errors::control_errors(ControlErrors::UNKNOWN_DEVICE, s); }
}

namespace {
void voltage_or_phase(const std::string& s, bool voltage, Matrix& mObj, int64_t fIndex, const s_map* aliases) {
    const auto tokens = trace_operands(s);
    if (tokens.size() == 2 && (mObj.lm.count(tokens[0]) || mObj.lm.count(tokens[1]))) {
        const auto start = mObj.relevantTraces.size();
        voltage_or_phase(tokens[0], voltage, mObj, fIndex, aliases);
        voltage_or_phase(tokens[1], voltage, mObj, fIndex, aliases);
        auto first = mObj.relevantTraces[start];
        first.subtract = std::make_shared<RelevantTrace>(mObj.relevantTraces[start + 1]);
        first.deviceLabel = "\"" + std::string(voltage ? "V(" : "P(") + s + ")\"";
        mObj.relevantTraces.resize(start);
        mObj.relevantTraces.emplace_back(std::move(first));
        return;
    }
    RelevantTrace            temp;
    temp.fIndex = fIndex;
    if (voltage) {
        temp.storageType = StorageType::Voltage;
    } else {
        temp.storageType = StorageType::Phase;
    }
    for (auto j = 0; j < mObj.components.devices.size(); ++j) {
        const auto& l = std::visit(
                [](const auto& device) noexcept -> const std::string& { return device.netlistInfo.label_; },
                mObj.components.devices.at(j));

        if (l == tokens.at(0) && tokens.size() == 1) {
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
                            [](const auto& device) noexcept -> const int_o& { return device.indexInfo.posIndex_; },
                            mObj.components.devices.at(j));
                    temp.index2 = std::visit(
                            [](const auto& device) noexcept -> const int_o& { return device.indexInfo.negIndex_; },
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
                            [](const auto& device) noexcept -> const int_o& { return device.indexInfo.posIndex_; },
                            mObj.components.devices.at(j));
                    temp.index2 = std::visit(
                            [](const auto& device) noexcept -> const int_o& { return device.indexInfo.negIndex_; },
                            mObj.components.devices.at(j));
                    try {
                        temp.variableIndex = std::get<JJ>(mObj.components.devices.at(j)).variableIndex_;
                        mObj.relevantTraces.emplace_back(temp);
                        break;
                    } catch (const std::bad_variant_access&) {}
                }
            }
        }
    }
    if (!temp.deviceLabel && tokens.size() == 1) {
        for (const auto& source : mObj.components.currentsources) {
            if (source.netlistInfo.label_ == tokens[0]) {
                temp.device = true;
                temp.index1 = source.indexInfo.posIndex_;
                temp.index2 = source.indexInfo.negIndex_;
                temp.deviceLabel = "\"" + std::string(voltage ? "V(" : "P(") + s + ")\"";
                mObj.relevantTraces.emplace_back(temp);
                break;
            }
        }
    }
    if (!temp.deviceLabel) {
        if (tokens.empty() || tokens.size() > 2) {
            Errors::control_errors(ControlErrors::INVALID_OUTPUT_COMMAND, s);
        }
        const auto node_index = [&](std::string node) -> int_o {
            if (aliases) {
                const auto alias = aliases->find(node);
                if (alias != aliases->end()) { node = alias->second; }
            }
            if (node == "0" || node == "GND") { return std::nullopt; }
            const auto found = mObj.nm.find(node);
            if (found == mObj.nm.end()) { Errors::control_errors(ControlErrors::UNKNOWN_DEVICE, node); }
            return found->second;
        };
        temp.index1 = node_index(tokens[0]);
        if (tokens.size() == 2) { temp.index2 = node_index(tokens[1]); }
        temp.deviceLabel = "\"" + std::string(voltage ? "V(" : "P(") + tokens[0]
                         + (tokens.size() == 2 ? "," + tokens[1] : "") + ")\"";
        mObj.relevantTraces.emplace_back(temp);
    }
}
} // namespace

void JoSIM::handle_voltage_or_phase(const std::string& s, bool voltage, Matrix& mObj, int64_t fIndex) {
    voltage_or_phase(s, voltage, mObj, fIndex, nullptr);
}
