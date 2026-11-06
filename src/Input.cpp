// Copyright (c) 2025 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/Input.hpp"

#include "JoSIM/ProgressBar.hpp"
#include "JoSIM/Rng.hpp"

#include <charconv>
#include <cctype>
#include <cmath>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <thread>

using namespace JoSIM;

namespace {
void normalize_netlist_line(tokens_t& tokens) {
    if (tokens.front() == ".FILE" || tokens.front() == "FILE") { return; }
    const bool iv = tokens.front() == ".IV" || tokens.front() == "IV";
    const std::string filename = iv && tokens.size() > 3 ? tokens[3] : "";
    const auto original = Misc::vector_to_string(tokens);
    auto normalized = original;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char c) { return std::toupper(c); });
    // CUS paths are filesystem names, including on continued logical lines.
    static const std::regex custom_file(R"(\bCUS\s*\(\s*([^\s,)]+))", std::regex::icase);
    for (auto match = std::sregex_iterator(original.begin(), original.end(), custom_file);
         match != std::sregex_iterator(); ++match) {
        normalized.replace(match->position(1), match->length(1), match->str(1));
    }
    tokens = Misc::tokenize(normalized);
    if (iv && tokens.size() > 3) { tokens[3] = filename; }
}
} // namespace

std::vector<tokens_t> Input::read_input(LineInput& input, string_o fileName) {
    struct IncludeStackGuard {
        std::vector<std::string>& stack;
        size_t size;
        ~IncludeStackGuard() { stack.resize(size); }
    } guard{activeIncludes_, activeIncludes_.size()};
    // When the standard input is selected, "fileName" will be in the nullopt
    // state.
    if (fileName != std::nullopt) {
        const auto path = std::filesystem::weakly_canonical(fileName.value());
        for (const auto& ancestor : activeIncludes_) {
            if (!ancestor.empty() && std::filesystem::equivalent(path, ancestor)) {
                Errors::input_errors(InputErrors::CYCLIC_INCLUDE, path.string());
            }
        }
        if (activeIncludes_.empty()) { fileParentPath = path.parent_path().string(); }
        activeIncludes_.push_back(path.string());
        Misc::unique_push(inputFiles, path.string());
    } else {
        // Keep stdin as the root context while reading included files.
        activeIncludes_.push_back("");
    }
    // Variable to store the read line
    std::string           line;
    // Variable to store all the read lines
    std::vector<tokens_t> fileLines;
    // Variable to store the tokenized line
    tokens_t              tokens;
    // Do this until the end of file is reached.
    while (input.next()) {
        // Create a mutable line
        line = input.line();
        // Remove all Carriage Return characters
        line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
        line.erase(line.begin(), std::find_if(line.begin(), line.end(),
                                             [](unsigned char c) { return !std::isspace(c); }));
        // Remove all the leading, trailing and extra white spaces
        line = std::regex_replace(line, std::regex("^ +| +$|( ) +"), "$1");
        if (!line.empty() && line.at(0) != '*' && line.at(0) != '#') {
            // Tokenize the line
            tokens = Misc::tokenize(line);
            if (tokens.empty()) { continue; }
            // Uppercase just the first token for ".INCLUDE" and ".FILE" check
            if (tokens.front().front() != '+') {
                std::transform(tokens.begin(), tokens.begin() + 1, tokens.begin(), [](std::string& c) -> std::string {
                    std::transform(c.begin(), c.end(), c.begin(), toupper);
                    return c;
                });
            }
            // If the line contains a "INCLUDE" statement
            if (tokens.at(0) == ".INCLUDE" || tokens.at(0) == "INCLUDE") {
                if (tokens.size() != 2) {
                    Errors::input_errors(InputErrors::INVALID_INCLUDE, line);
                }
                // Variable to store path to file to include
                std::string includeFile;
                // If reading from a file
                if (fileName) {
                    includeFile = std::filesystem::path(fileName.value()).parent_path().append(tokens.at(1)).string();
                } else {
                    includeFile = std::filesystem::current_path().append(tokens.at(1)).string();
                }
                // Create a new LineInput variable for the included file
                FileInput             file(includeFile);
                // Attempt to read the contents of the included file
                std::vector<tokens_t> tempInclude = Input::read_input(file, includeFile);
                // Insert the lines from the included file in place
                fileLines.insert(fileLines.end(), tempInclude.begin(), tempInclude.end());
                // If the line contains a "FILE" statement
            } else if (tokens.at(0) == ".FILE" || tokens.at(0) == "FILE") {
                // Ensure there is a second token
                if (tokens.size() < 2) {
                    Errors::control_errors(ControlErrors::INVALID_FILE_COMMAND, line);
                } else {
                    // Sanity check, if parent path of output file is empty then
                    // change path to input file path, otherwise file is written
                    // in executable location
                    auto path = std::filesystem::path(tokens.at(1));
                    if (!path.has_parent_path() && fileParentPath) {
                        path = std::filesystem::path(fileParentPath.value()).append(tokens.at(1));
                    }
                    output_files.emplace_back(OutputFile(path.string()));
                }
                fileLines.emplace_back(tokens);
                // If the line contains a "END" statement
            } else if (tokens.at(0) == ".END" || tokens.at(0) == "END") {
                break;
                // If the line does not contain an "INCLUDE", "FILE" or "END" statement
            } else {
                // If the line starts with a '+' append it to the previous line.
                if (tokens.at(0).at(0) == '+') {
                    if (fileLines.empty()) { Errors::input_errors(InputErrors::INVALID_CONTINUATION, line); }
                    // Remove the '+'
                    tokens.at(0) = tokens.at(0).substr(1);
                    if (tokens.at(0).empty()) {
                        fileLines.back().insert(fileLines.back().end(), tokens.begin() + 1, tokens.end());
                    } else {
                        fileLines.back().insert(fileLines.back().end(), tokens.begin(), tokens.end());
                    }
                    // Add the line to the read in lines variable
                } else {
                    fileLines.emplace_back(tokens);
                }
            }
        }
    }
    // If the file was read in but has no contents report an error
    if (fileLines.empty()) {
        Errors::input_errors(InputErrors::EMPTY_FILE, fileName);
        // Return the read in file lines.
    } else {
        for (auto& logical_line : fileLines) { normalize_netlist_line(logical_line); }
        return fileLines;
    }
    // Return to satisfy return type. This should never be reached.
    return {};
}

void Input::parse_input(string_o fileName) {
    // srand(time(NULL));
    //  Create a seperate thread that will be used for printing creation progress
    std::thread           printingThread;
    // Variable to store the tokenized input
    std::vector<tokens_t> fileLines;
    // Create a optional string to store subcircuit names
    string_o              subckt;
    // Create boolean to determine if in a control block
    bool                  control = false;
    // If a file name is provided then input is from file
    if (fileName) {
        // Create a stream input from file
        FileInput input(fileName.value());
        // Use stream input to read lines and return tokenized input
        fileLines = Input::read_input(input, fileName);
        // If no filename then input from standard input
    } else {
        // Create standard input stream
        ConsoleInput input;
        // Use stream input to read lines and return tokenized input
        fileLines = Input::read_input(input);
    }
    ProgressBar bar;
    if (!argMin) {
        bar.create_thread();
        bar.set_bar_width(30);
        bar.fill_bar_progress_with("O");
        bar.fill_bar_remainder_with(" ");
        bar.set_status_text("Parsing Input");
        bar.set_total((float) fileLines.size());
    }
    // If not minimal printing
    netlist.argMin = argMin;
    // Loop through all the tokenized input
    for (auto i = 0; i < fileLines.size(); ++i) {
        // If not minimal printing
        if (!argMin) {
            // Report progress
            bar.update((float) i);
        }
        // Determine if the line is a control (subcircuit, analysis, print, etc.)
        if (fileLines.at(i).front().at(0) == '.') {
            // If the line is the begining of a subcircuit
            if (::strcmp(fileLines.at(i).front().c_str(), ".SUBCKT") == 0) {
                // Indicate that subcircuits exist within this netlist
                netlist.containsSubckt = true;
                // Subcircuit is required to atleast have a name
                if (fileLines.at(i).size() > 1) {
                    // Subcircuit is required to have at least some form of IO
                    if (fileLines.at(i).size() > 2) {
                        // Set the subcircuit name the second token
                        subckt = fileLines.at(i).at(1);
                        if (netlist.subcircuits.count(subckt.value()) != 0) {
                            Errors::input_errors(InputErrors::DUPLICATE_SUBCIRCUIT, subckt.value());
                            netlist.subcircuits[subckt.value()].io.clear();
                            netlist.subcircuits[subckt.value()].lines.clear();
                        }
                        // Populate the a subcircuit at given name with its IO nodes
                        for (auto j = 2; j < fileLines.at(i).size(); ++j) {
                            netlist.subcircuits[subckt.value()].io.push_back(fileLines.at(i).at(j));
                        }
                        // Complain if no IO is found
                    } else {
                        Errors::input_errors(InputErrors::MISSING_SUBCKT_IO);
                    }
                    // Complain if no subcircuit name is found
                } else {
                    Errors::input_errors(InputErrors::MISSING_SUBCKT_NAME);
                }
                // If parameter, create a parameter
            } else if (::strcmp(fileLines.at(i).front().c_str(), ".PARAM") == 0) {
                create_parameter(fileLines.at(i), parameters, subckt);
                // If control, set flag as start of controls
            } else if (::strcmp(fileLines.at(i).front().c_str(), ".CONTROL") == 0) {
                control = true;
                // End subcircuit, set flag
            } else if (::strcmp(fileLines.at(i).front().c_str(), ".ENDS") == 0) {
                subckt = std::nullopt;
                // End control section, set flag
            } else if (::strcmp(fileLines.at(i).front().c_str(), ".ENDC") == 0) {
                control = false;
                // If model, add model to models list
            } else if (::strcmp(fileLines.at(i).front().c_str(), ".MODEL") == 0) {
                netlist.models[std::make_pair(fileLines.at(i).at(1), subckt)] = fileLines.at(i);
                // If neither of these, normal control, add to controls list
            } else {
                // Controls cannot exist within a subcircuit
                if (!subckt) {
                    // Remove the preceding '.' from the first token
                    fileLines.at(i).front() = fileLines.at(i).front().substr(1);
                    // Store the control
                    controls.push_back(fileLines.at(i));
                    // Complain that controls exist in subcircuit
                } else {
                    Errors::input_errors(InputErrors::SUBCKT_CONTROLS, subckt.value());
                }
            }
            // If control section flag set
        } else if (control) {
            // If parameter, add to unparsed parameter list
            if (::strcmp(fileLines.at(i).front().c_str(), "PARAM") == 0) {
                create_parameter(fileLines.at(i), parameters, subckt);
                // If model, add to models list
            } else if (::strcmp(fileLines.at(i).front().c_str(), "MODEL") == 0) {
                netlist.models[std::make_pair(fileLines.at(i).at(1), subckt)] = fileLines.at(i);
                // If neither, add to controls list
            } else {
                if (!subckt) {
                    controls.push_back(fileLines.at(i));
                } else {
                    Errors::input_errors(InputErrors::SUBCKT_CONTROLS, subckt.value());
                }
            }
            // If not a control, normal device line
        } else {
            // If subcircuit flag, add line to relevant subcircuit
            if (subckt) {
                netlist.subcircuits[subckt.value()].lines.push_back(std::make_pair(fileLines.at(i), subckt));
                // If not, add line to main design
            } else {
                netlist.maindesign.push_back(fileLines.at(i));
            }
        }
    }
    // Sanity check all conrols for syntax and functionality
    syntax_check_controls(controls);
    // Seed deterministic RNG streams (noise + spread) once per parsed run.
    if (auto seed = find_seed_option(); seed.has_value()) {
        Rng::start_run(*seed);
    } else {
        Rng::start_run_auto();
    }
    // Let the user know the input reading is complete
    if (!argMin) {
        bar.complete();
        std::cout << "\n";
    }
    if (argVerb) { std::cout << "RNG seed: " << Rng::base_seed() << "\n"; }
    // If main is empty, complain
    if (netlist.maindesign.empty()) { Errors::input_errors(InputErrors::MISSING_MAIN); }
    if (cli_output_file) { ensure_safe_output(cli_output_file.value().name()); }
    for (auto& output : output_files) { ensure_safe_output(output.name()); }
}

void Input::ensure_safe_output(const std::string& filename) const {
    const auto output = std::filesystem::weakly_canonical(filename);
    for (const auto& source : inputFiles) {
        std::error_code error;
        const bool equivalent = std::filesystem::equivalent(output, source, error);
        if (output == std::filesystem::path(source) || (!error && equivalent)) {
            Errors::cli_errors(CLIErrors::INPUT_SAME_OUTPUT, filename);
        }
    }
}

void Input::syntax_check_controls(std::vector<tokens_t>& controls) {
    // This will simply check controls, complaining if any are not allowed
    std::vector<std::string> v
            = {"PRINT", "TRAN", "SAVE", "PLOT", "END", "TEMP", "NEB", "SPREAD", "FILE", "IV", "OPTION"};
    for (auto i : controls) {
        if (std::find(v.begin(), v.end(), i.at(0)) == v.end()) {
            Errors::input_errors(InputErrors::UNKNOWN_CONTROL, i.at(0));
        }
    }
}

std::optional<uint64_t> Input::find_seed_option() const {
    for (const auto& c : controls) {
        if (c.empty()) { continue; }
        if (c.front() != "OPTION") { continue; }

        for (size_t k = 1; k < c.size(); ++k) {
            std::string tok = c.at(k);

            // SEED=123
            if (tok.rfind("SEED=", 0) == 0) {
                tok = tok.substr(5);
            }
            // SEED 123
            else if (tok == "SEED" && (k + 1) < c.size()) {
                tok = c.at(++k);
            } else {
                continue;
            }

            // Allow AUTO to mean "random seed"
            if (tok == "AUTO") { return std::nullopt; }

            // First try integer literal:
            uint64_t u{};
            auto [p, ec] = std::from_chars(tok.data(), tok.data() + tok.size(), u);
            if (ec == std::errc{} && p == tok.data() + tok.size()) { return u; }

            // Fallback: expression -> double (but then RANGE CHECK!)
            double dv = parse_param(tok, parameters);
            if (!std::isfinite(dv) || dv < 0.0 || dv >= std::ldexp(1.0, 64)) {
                Errors::input_errors(InputErrors::UNKNOWN_CONTROL, "OPTION SEED");
            }

            // If you want only integral seeds:
            double iv = std::floor(dv);
            if (iv != dv) { Errors::input_errors(InputErrors::UNKNOWN_CONTROL, "OPTION SEED"); }

            return static_cast<uint64_t>(iv);
        }
    }
    return std::nullopt;
}
