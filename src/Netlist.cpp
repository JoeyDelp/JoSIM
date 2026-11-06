// Copyright (c) 2025 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/Netlist.hpp"

#include "JoSIM/AnalysisType.hpp"
#include "JoSIM/Components.hpp"
#include "JoSIM/Errors.hpp"
#include "JoSIM/Misc.hpp"

#include <algorithm>

using namespace JoSIM;

void Netlist::id_io_subc_label(const tokens_t&                                    lineTokens,
                               tokens_t&                                          io,
                               s_map&                                             params,
                               std::string&                                       subcktName,
                               std::string&                                       label,
                               const std::unordered_map<std::string, Subcircuit>& subcircuits) {
    label = lineTokens.front();
    const auto firstParameter = parameter_start(lineTokens, 1);
    for (const auto& assignment : parameter_assignments(lineTokens, firstParameter)) {
        params.emplace(assignment);
    }
    // Both documented instance conventions put the type at one end of the IO.
    size_t type = firstParameter;
    if (firstParameter > 1 && subcircuits.count(lineTokens[1])) { type = 1; }
    else if (firstParameter > 1 && subcircuits.count(lineTokens[firstParameter - 1])) { type = firstParameter - 1; }
    if (type == firstParameter) { Errors::input_errors(InputErrors::UNKNOWN_SUBCKT, Misc::vector_to_string(lineTokens)); }
    subcktName = lineTokens[type];
    for (size_t i = 1; i < firstParameter; ++i) {
        if (i != type) { io.push_back(lineTokens[i]); }
    }
}

static std::string bind_node(const std::string& node, const tokens_t& formal, const tokens_t& actual,
                             const std::string& label) {
    const auto pin = std::find(formal.begin(), formal.end(), node);
    if (pin != formal.end()) { return actual.at(static_cast<size_t>(pin - formal.begin())); }
    if (label.empty() || node == "0" || node == "GND") { return node; }
    return node + "|" + label;
}

void Netlist::expand_line(tokens_t& tokens, const tokens_t& formal, const tokens_t& actual,
                          const std::string& label, const string_o& scope) {
    const auto map_node = [&](const std::string& node) { return bind_node(node, formal, actual, label); };
    if (!label.empty()) { tokens.front() += "|" + label; }
    size_t nodeCount = std::string("EFGHT").find(tokens.front().front()) != std::string::npos ? 4 : 2;
    if (tokens.front().front() == 'B' && tokens.size() > 4) {
        const auto* model = Model::find_model(tokens[4], models_new, scope);
        if (model && model->gateWidthSpecified()) { nodeCount = 3; }
    }
    if (tokens.size() <= nodeCount) {
        Errors::invalid_component_errors(ComponentErrors::INVALID_COMPONENT_DECLARATION, Misc::vector_to_string(tokens));
    }
    for (size_t n = 1; n <= nodeCount; ++n) { tokens[n] = map_node(tokens[n]); }
    for (auto& token : tokens) {
        if (token.rfind("GC=", 0) == 0) { token = "GC=" + map_node(token.substr(3)); }
    }
}

void Netlist::insert_parameter(tokens_t& t, s_map& params) {
    std::string twonode  = "RCLK";
    std::string fournode = "EFHGT";
    if (twonode.find(t.front().front()) != std::string::npos) {
        t.at(3) = params.at(t.front());
    } else if (fournode.find(t.front().front()) != std::string::npos) {
        if (t.front().front() == 'T') {
            for (auto& i : t) {
                if (i.find("TD=") != std::string::npos) { i = "TD=" + params.at(t.front()); }
            }
        } else {
            t.at(5) = params.at(t.front());
        }
    } else if (t.front().front() == 'B') {
        for (auto& i : t) {
            if (i.find("AREA=") != std::string::npos) {
                i = "AREA=" + params.at(t.front());
            } else if (i.find("IC=") != std::string::npos) {
                i = "IC=" + params.at(t.front());
            }
        }
    }
}

void Netlist::expand_subcircuits() {
    // Check every definition, including unused ones, without destroying the
    // templates needed to bind each instance's parameters before its children.
    std::unordered_map<std::string, std::unordered_set<std::string>> dependencies;
    subcktNodeCounts.clear();
    for (const auto& entry : subcircuits) {
        auto& required = dependencies[entry.first];
        if (sanityCheck && sanityCheckSubckts.count(entry.first)) {
            for (const auto& pin : entry.second.io) { increment_subcircuit_node_count(entry.first, pin); }
        }
        for (const auto& line : entry.second.lines) {
            if (line.first.front().front() != 'X') {
                if (sanityCheck && sanityCheckSubckts.count(entry.first)) {
                    for (size_t node = 1; node < std::min(size_t(3), line.first.size()); ++node) {
                        increment_subcircuit_node_count(entry.first, line.first[node]);
                    }
                }
                continue;
            }
            tokens_t io;
            s_map parameters;
            std::string type, label;
            id_io_subc_label(line.first, io, parameters, type, label, subcircuits);
            if (io.size() != subcircuits.at(type).io.size()) { Errors::input_errors(InputErrors::IO_MISMATCH, label); }
            required.insert(type);
            if (sanityCheck && sanityCheckSubckts.count(entry.first)) {
                for (const auto& node : io) { increment_subcircuit_node_count(entry.first, node); }
            }
        }
    }
    while (!dependencies.empty()) {
        tokens_t ready;
        for (const auto& entry : dependencies) {
            if (entry.second.empty()) { ready.push_back(entry.first); }
        }
        if (ready.empty()) {
            tokens_t unresolved;
            for (const auto& entry : dependencies) { unresolved.push_back(entry.first); }
            Errors::netlist_errors(NetlistErrors::RECURSIVE_SUBCIRCUIT, Misc::vector_to_string(unresolved));
        }
        for (const auto& type : ready) { dependencies.erase(type); }
        for (auto& entry : dependencies) {
            for (const auto& type : ready) { entry.second.erase(type); }
        }
    }
}

void Netlist::expand_maindesign() {
    param_map parameters;
    expand_design(parameters, false);
}

void Netlist::expand_maindesign(param_map& parameters) {
    expand_design(parameters, true);
}

void Netlist::expand_design(param_map& parameters, bool scoped) {
    expand_subcircuits();
    std::unordered_map<std::string, std::vector<std::pair<std::string, Parameter>>> definitions;
    std::vector<ParameterName> globals;
    for (const auto& entry : parameters) {
        if (!entry.first.subcircuit()) { globals.push_back(entry.first); }
        else { definitions[*entry.first.subcircuit()].emplace_back(entry.first.name(), entry.second); }
    }
    parse_parameters(parameters, globals);
    const auto parsedModels = models_new;
    if (scoped) { models_new.clear(); }
    for (const auto& entry : parsedModels) {
        if (scoped && !entry.second && !models.count({entry.first.modelName(), std::nullopt})) { models_new.push_back(entry); }
    }
    std::unordered_map<std::string, std::vector<tokens_t>> localModels;
    for (const auto& entry : models) {
        if (entry.first.second) { localModels[*entry.first.second].push_back(entry.second); }
        else if (scoped) { Model::parse_model({entry.second, std::nullopt}, models_new, parameters); }
    }
    struct Frame {
        const Subcircuit* definition = nullptr;
        tokens_t io;
        std::string path;
        string_o scope;
        size_t next = 0;
        size_t first = 0;
        s_map componentOverrides;
    };
    std::vector<Frame> stack(1);
    expNetlist.clear();
    nodeAliases.clear();
    mainNodeCounts.clear();
    while (!stack.empty()) {
        auto& frame = stack.back();
        const auto size = frame.definition ? frame.definition->lines.size() : maindesign.size();
        if (frame.next == size) {
            // Legacy component-value overrides apply after child expansion,
            // preserving an outer instance's precedence over an inner one.
            for (size_t i = frame.first; !frame.componentOverrides.empty() && i < expNetlist.size(); ++i) {
                auto& tokens = expNetlist[i].first;
                if (frame.componentOverrides.count(tokens.front())) { insert_parameter(tokens, frame.componentOverrides); }
            }
            stack.pop_back();
            continue;
        }
        auto tokens = frame.definition ? frame.definition->lines[frame.next++].first : maindesign[frame.next++];
        if (tokens.front().front() != 'X') {
            const tokens_t empty;
            const auto& formal = frame.definition ? frame.definition->io : empty;
            expand_line(tokens, formal, frame.io, frame.path, frame.scope);
            expNetlist.emplace_back(std::move(tokens), frame.scope);
            continue;
        }
        tokens_t io;
        s_map assignments;
        std::string type, label;
        id_io_subc_label(tokens, io, assignments, type, label, subcircuits);
        const auto& definition = subcircuits.at(type);
        if (io.size() != definition.io.size()) { Errors::input_errors(InputErrors::IO_MISMATCH, tokens.front()); }
        if (sanityCheck && !frame.definition) {
            for (const auto& node : io) { increment_maindesign_node_count(node); }
        }
        if (frame.definition) {
            for (auto& node : io) { node = bind_node(node, frame.definition->io, frame.io, frame.path); }
        }
        Frame child;
        child.definition = &definition;
        child.io = std::move(io);
        child.path = label + (frame.path.empty() ? "" : "|" + frame.path);
        // Leading '|' distinguishes instance environments from type defaults.
        child.scope = scoped ? "|" + child.path : type;
        child.first = expNetlist.size();
        std::vector<ParameterName> names;
        const auto locals = definitions.find(type);
        if (locals != definitions.end()) {
            for (const auto& entry : locals->second) {
                const ParameterName name(entry.first, child.scope);
                parameters[name] = entry.second;
                parameters[name].reset_value();
                names.push_back(name);
            }
        }
        for (const auto& assignment : assignments) {
            // Overrides are evaluated in the caller, before local shadowing.
            if (!scoped) {
                child.componentOverrides[assignment.first + "|" + child.path] = assignment.second;
                continue;
            }
            const double value = parse_param(assignment.second, parameters, frame.scope);
            const ParameterName name(assignment.first, child.scope);
            auto& parameter = parameters[name];
            // The bound value is fixed; reparsing must not capture a child
            // variable with the same name as the caller expression.
            parameter.set_expression(Misc::precise_to_string(value));
            parameter.set_value(value);
            const bool declared = locals != definitions.end()
                && std::any_of(locals->second.begin(), locals->second.end(), [&](const auto& entry) { return entry.first == assignment.first; });
            if (!declared) {
                child.componentOverrides[assignment.first + "|" + child.path] = Misc::precise_to_string(value);
            }
        }
        parse_parameters(parameters, names);
        if (scoped) {
            for (const auto& entry : localModels[type]) { Model::parse_model({entry, child.scope}, models_new, parameters); }
        }
        for (const auto& entry : parsedModels) {
            if (scoped && entry.second == string_o(type) && !models.count({entry.first.modelName(), type})) {
                models_new.emplace_back(entry.first, child.scope);
            }
        }
        for (size_t pin = 0; pin < definition.io.size(); ++pin) {
            nodeAliases[definition.io[pin] + "|" + child.path] = child.io[pin];
        }
        stack.push_back(std::move(child));
    }
    subcktTotal = subcircuits.size();
    subcircuits.clear();
}

void Netlist::increment_maindesign_node_count(std::string node) {
    if (node == "0" || node == "GND") { return; }
    if (mainNodeCounts.count(node) == 0) {
        mainNodeCounts[node] = 1;
    } else {
        mainNodeCounts.at(node)++;
    }
}

void Netlist::increment_subcircuit_node_count(std::string subcktName, std::string node) {
    if (node == "0" || node == "GND") { return; }
    if (subcktNodeCounts.count(subcktName) == 0) { subcktNodeCounts[subcktName]; }
    if (subcktNodeCounts.at(subcktName).count(node) == 0) {
        subcktNodeCounts.at(subcktName)[node] = 1;
    } else {
        subcktNodeCounts.at(subcktName).at(node)++;
    }
}

void sanity_check_node(std::string node, int32_t count, bool& is_clean) {
    if (count < 2) {
        is_clean = false;
        std::cout << "  - Node '" << node << "' looks either undefined or unused." << std::endl;
    } else if (count > 2) {
        is_clean = false;
        std::cout << "  - Node '" << node << "' looks overused (" << count << ")." << std::endl;
    }
}

void Netlist::sanity_check_maindesign() {
    std::cout << "main design:" << std::endl;
    bool is_clean = true;
    for (auto kv : mainNodeCounts) { sanity_check_node(kv.first, kv.second, is_clean); }
    if (is_clean) {
        std::cout << "  Everything looks fine! (Checked " << mainNodeCounts.size() << " nodes.)" << std::endl;
    }
}

void Netlist::sanity_check_subcircuits() {
    for (auto subcktName : sanityCheckSubckts) {
        if (subcktNodeCounts.count(subcktName) == 0) {
            std::cout << "Subcircuit '" << subcktName << "' does not exist. Skip." << std::endl;
            continue;
        }
        std::cout << "'" << subcktName << "':" << std::endl;
        bool is_clean = true;
        for (auto kv : subcktNodeCounts.at(subcktName)) { sanity_check_node(kv.first, kv.second, is_clean); }
        if (is_clean) {
            std::cout << "  Everything looks fine! (Checked " << subcktNodeCounts.at(subcktName).size() << " nodes.)"
                      << std::endl;
        }
    }
}

void Netlist::sanity_check(Components components) {
    if (!sanityCheck) { return; }

    // Update node counts in main design
    for (auto device : components.devices) {
        // If device belongs to a subcircuit
        if (std::visit([](const auto device) { return device.netlistInfo.label_.find("|") != std::string::npos; },
                       device)) {
            continue;
        }
        // Only consider nodes in maindesign (top)
        auto nodes = std::visit([](const auto device) { return device.nodes; }, device);
        for (auto node : nodes) { increment_maindesign_node_count(node); }
    }
    for (auto source : components.currentsources) {
        // If device belongs to a subcircuit
        if (source.netlistInfo.label_.find("|") != std::string::npos) { continue; }
        // Only consider nodes in maindesign (top)
        for (auto node : source.nodes) { increment_maindesign_node_count(node); }
    }

    std::cout << "\nSanity check results:" << std::endl;
    sanity_check_maindesign();
    sanity_check_subcircuits();
    std::cout << std::endl;
}
