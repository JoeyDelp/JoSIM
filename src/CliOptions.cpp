// Copyright (c) 2025 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/CliOptions.hpp"

#include "JoSIM/Errors.hpp"
#include "JoSIM/Misc.hpp"
#include "JoSIM/TypeDefines.hpp"

#include <cstring>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>
#include <unordered_map>

using namespace JoSIM;

tokens_t CliOptions::argv_to_tokens(const int64_t& argc, const char** argv) {
    // Tokens variable of size argc to store arguments
    // If argument count is less or equal to one
    if (argc <= 1) {
        // Complain that josim was run without arguments
        Errors::cli_errors(CLIErrors::TOO_FEW_ARGUMENTS);
    }
    tokens_t tokens(argc - 1);
    // Loop through arguments, starting at 1 (0 is josim-cli)
    for (int64_t i = 1; i < argc; ++i) {
        // Add the arguments at the appropriate position
        tokens.at(i - 1) = std::string(argv[i]);
    }
    // Return the tokens
    return tokens;
}

vector_pair_t<char_o, string_o> CliOptions::argument_pairs(const tokens_t& tokens) {
    vector_pair_t<char_o, string_o> ap;
    const std::unordered_map<std::string, char> switches = {
        {"-a", 'a'}, {"--analysis", 'a'}, {"-h", 'h'}, {"--help", 'h'},
        {"-i", 'i'}, {"--input", 'i'}, {"-m", 'm'}, {"--minimal", 'm'},
        {"-o", 'o'}, {"--output", 'o'}, {"-p", 'p'}, {"--parallel", 'p'},
        {"-s", 's'}, {"--sanity", 's'}, {"-V", 'V'}, {"--verbose", 'V'},
        {"-v", 'v'}, {"--version", 'v'}, {"-x", 'x'}, {"--solver", 'x'}};
    bool positional = false;
    for (size_t i = 0; i < tokens.size(); ++i) {
        const auto& token = tokens[i];
        if (token.empty()) { Errors::cli_errors(CLIErrors::INVALID_ARGUMENT, "Empty argument"); }
        if (!positional && token == "--") {
            positional = true;
            continue;
        }
        if (positional || token.front() != '-') {
            ap.emplace_back(std::nullopt, token);
            continue;
        }
        const auto equal = token.find('=');
        const auto name = token.substr(0, equal);
        const auto found = switches.find(name);
        if (found == switches.end()) { Errors::cli_errors(CLIErrors::UNKNOWN_SWITCH, name); }
        const char option = found->second;
        string_o value;
        const bool flag = option == 'h' || option == 'i' || option == 'v' || option == 'p';
        if (equal != std::string::npos) {
            if (flag) { Errors::cli_errors(CLIErrors::INVALID_ARGUMENT, name + " takes no value"); }
            value = token.substr(equal + 1);
        } else if (!flag && i + 1 < tokens.size()) {
            const auto& next = tokens[i + 1];
            const bool numeric = !next.empty() && ((next.front() >= '0' && next.front() <= '9')
                || ((next.front() == '-' || next.front() == '+') && next.size() > 1
                    && next[1] >= '0' && next[1] <= '9'));
            // Optional numeric values must not swallow a following filename.
            const bool numeric_option = option == 'm' || option == 'V';
            const bool can_consume = !next.empty() && (next.front() != '-' || numeric);
            const bool trailing_sanity_file = option == 's' && i + 2 == tokens.size();
            if (can_consume && (!numeric_option || numeric) && !trailing_sanity_file) { value = tokens[++i]; }
        }
        ap.emplace_back(option, value);
    }
    return ap;
}

CliOptions CliOptions::parse(int64_t argc, const char** argv) {
    // Variable where all the CLI options will be stored
    CliOptions                      out;
    bool                            explicit_stdin_input = false; // Only used to suppress warning
    // Parse and generate argument pairs from tokens
    vector_pair_t<char_o, string_o> ap                   = out.argument_pairs(out.argv_to_tokens(argc, argv));
    // Loop through the argument pairs
    for (auto i : ap) {
        // If the argument is not accompanied by any switch
        if (!i.first) {
            // This argument is the file name
            if (out.cir_file_name) { Errors::cli_errors(CLIErrors::INVALID_ARGUMENT, "Multiple input files"); }
            out.cir_file_name = i.second;
            // Else if the switch is not empty
        } else {
            // Use the char in a case statment
            switch (i.first.value()) {
                    // Set analysis type
                case 'a':
                    if (!i.second) {
                        Errors::cli_errors(CLIErrors::NO_ANALYSIS);
                    } else if (i.second.value() == "0") {
                        out.analysis_type = AnalysisType::Voltage;
                    } else if (i.second.value() == "1") {
                        out.analysis_type = AnalysisType::Phase;
                    } else {
                        Errors::cli_errors(CLIErrors::INVALID_ANALYSIS);
                    }
                    break;
                    // Show help menu
                case 'h':
                    display_help();
                    exit(0);
                    // Set input to standard input
                case 'i':
                    out.cir_file_name    = std::nullopt;
                    explicit_stdin_input = true;
                    break;
                    // Enable minimal reporting
                case 'm':
                    if (i.second.value_or("1") == "1") { out.minimal = true; }
                    else if (i.second == "0") { out.minimal = false; }
                    else { Errors::cli_errors(CLIErrors::INVALID_MINIMAL); }
                    break;
                    // Set output file
                case 'o':
                    // If no file name is specified but output is specified
                    if (!i.second) {
                        // Store the output in a file called output.csv at cwd
                        out.output_file = OutputFile(std::filesystem::current_path().append("output.csv").string());
                        // If a file name was given
                    } else {
                        out.output_file = OutputFile(i.second.value());
                    }
                    break;
                case 'p':
                    Errors::cli_errors(CLIErrors::UNSUPPORTED_PARALLEL);
                    break;
                case 'x':
                    Errors::cli_errors(CLIErrors::INVALID_SOLVER);
                    break;
                    // Sanity check input circuit netlist
                case 's':
                    out.sanityCheck = true;
                    if (i.second) {
                        std::transform(
                                i.second.value().begin(), i.second.value().end(), i.second.value().begin(), toupper);
                        out.sanityCheckSubckts.insert(i.second.value());
                    }
                    break;
                    // Enable verbose mode
                case 'V': {
                    const auto value = i.second.value_or("1");
                    if (value.size() != 1 || value[0] < '0' || value[0] > '3') {
                        Errors::cli_errors(CLIErrors::INVALID_ARGUMENT, "Verbose level must be 0, 1, 2 or 3");
                    }
                    out.verbose = value[0] - '0';
                    break;
                }
                    // Show version info
                case 'v':
                    exit(0);
                    break;
                    // Unknown switch was specified
                default: Errors::cli_errors(CLIErrors::UNKNOWN_SWITCH, std::string(1, i.first.value()));
            }
        }
    }
    if (explicit_stdin_input) { out.cir_file_name = std::nullopt; }
    // Do a few sanity checks
    // If the input was not specified
    if (!out.cir_file_name && !(out.minimal && explicit_stdin_input)) {
        // Complain and assume standard input
        // Suppress warning if -i AND -m options were given
        Errors::cli_errors(CLIErrors::NO_INPUT);
    }
    // If an output file and an input file name were specified
    if (out.output_file && out.cir_file_name) {
        // But they match each other
        if (out.output_file.value().name() == out.cir_file_name.value()) {
            // Complain and exit since continuing will overwrite input file
            Errors::cli_errors(CLIErrors::INPUT_SAME_OUTPUT);
        }
    }
    // If minimal flag is not specified
    if (!out.minimal) {
        // Print to screen the input and output locations
        if (out.cir_file_name) {
            std::cout << "Input: " << out.cir_file_name.value() << std::endl;
        } else {
            std::cout << "Input: "
                      << "standard input" << std::endl;
        }
        std::cout << std::endl;
        if (out.output_file) {
            std::cout << "Output: " << out.output_file.value().name() << std::endl;
            std::cout << std::endl;
        }
    }

    return out;
}

void CliOptions::display_help() {
    // Format the help interface
    // Preamble
    std::cout << "JoSIM help interface\n";
    std::cout << "====================\n";
    // Analysis type
    // ---------------------------------------------------------------------------
    std::cout << std::setw(16) << std::left << "-a" << std::setw(3) << std::left << "|"
              << "Specifies the analysis type." << std::endl;
    std::cout << std::setw(16) << std::left << "--analysis=" << std::setw(3) << std::left << "|"
              << "0 for Voltage analysis." << std::endl;
    std::cout << std::setw(16) << std::left << "  " << std::setw(3) << std::left << "|"
              << "1 for Phase analysis (Default)." << std::endl;
    std::cout << std::setw(16) << std::left << "  " << std::setw(3) << std::left << "|" << std::endl;
    // Help menu
    // ---------------------------------------------------------------------------
    std::cout << std::setw(16) << std::left << "-h" << std::setw(3) << std::left << "|"
              << "Displays this help menu" << std::endl;
    std::cout << std::setw(16) << std::left << "--help" << std::setw(3) << std::left << "|"
              << " " << std::endl;
    std::cout << std::setw(16) << std::left << "  " << std::setw(3) << std::left << "|" << std::endl;
    // Input from standard in
    // ---------------------------------------------------------------------------
    std::cout << std::setw(16) << std::left << "-i" << std::setw(3) << std::left << "|"
              << "Input circuit netlist from standard input." << std::endl;
    std::cout << std::setw(16) << std::left << "--input" << std::setw(3) << std::left << "|"
              << "This command ignores any additional input file specified." << std::endl;
    std::cout << std::setw(16) << std::left << "  " << std::setw(3) << std::left << "|"
              << "Reads from standard input until the "
              << ".end command or EOF character is specified." << std::endl;
    std::cout << std::setw(16) << std::left << "  " << std::setw(3) << std::left << "|" << std::endl;
    // Minimal output
    // ---------------------------------------------------------------------------
    std::cout << std::setw(16) << std::left << "-m" << std::setw(3) << std::left << "|"
              << "Disables most output." << std::endl;
    std::cout << std::setw(16) << std::left << "--minimal" << std::setw(3) << std::left << "|"
              << " " << std::endl;
    std::cout << std::setw(16) << std::left << "  " << std::setw(3) << std::left << "|" << std::endl;
    // Output
    // ---------------------------------------------------------------------------
    std::cout << std::setw(16) << std::left << "-o" << std::setw(3) << std::left << "|"
              << "Specify output file for simulation results (.csv)." << std::endl;
    std::cout << std::setw(16) << std::left << "--output=" << std::setw(3) << std::left << "|"
              << "Default will be output.csv if no file is specified." << std::endl;
    std::cout << std::setw(16) << std::left << "  " << std::setw(3) << std::left << "|" << std::endl;
    // Sanity check switch
    // ---------------------------------------------------------------------------
    std::cout << std::setw(16) << std::left << "-s" << std::setw(3) << std::left << "|"
              << "(EXPERIMENTAL) Sanity check circuit netlist (main design). "
              << "Subcircuits can also be checked if specified." << std::endl;
    std::cout << std::setw(16) << std::left << "--sanitycheck" << std::setw(3) << std::left << "|"
              << "Multiple subcircuits can be specified by providing CLI option "
              << "multiple times. (e.g. -s SUBCKT1 -s SUBCKT2)" << std::endl;
    std::cout << std::setw(16) << std::left << "  " << std::setw(3) << std::left << "|"
              << "Checks every superconductor circuit cell node connects to exactly "
              << "one other node, assuming the design denotes high-level connections." << std::endl;
    std::cout << std::setw(16) << std::left << "  " << std::setw(3) << std::left << "|" << std::endl;
    // Verbose switch
    // ---------------------------------------------------------------------------
    std::cout << std::setw(16) << std::left << "-V" << std::setw(3) << std::left << "|"
              << "Runs JoSIM in verbose mode." << std::endl;
    std::cout << std::setw(16) << std::left << "--Verbose=" << std::setw(3) << std::left << "|"
              << "Defaults to minimal(1), can be medium(2) or maximum(3)" << std::endl;
    std::cout << std::setw(16) << std::left << "  " << std::setw(3) << std::left << "|" << std::endl;
    // Version info
    // ---------------------------------------------------------------------------
    std::cout << std::setw(16) << std::left << "-v" << std::setw(3) << std::left << "|"
              << "Displays the JoSIM version info only." << std::endl;
    std::cout << std::setw(16) << std::left << "--version" << std::setw(3) << std::left << "|"
              << " " << std::endl;
    std::cout << std::setw(16) << std::left << "  " << std::setw(3) << std::left << "|" << std::endl;

    // ---------------------------------------------------------------------------
    std::cout << std::endl;
    std::cout << "Example command: josim -o ./output.csv test.cir" << std::endl;
    std::cout << std::endl;
    exit(0);
}

void CliOptions::version_info() {
    std::cout << std::endl;
    std::cout << "JoSIM: Josephson Junction Superconductive SPICE Circuit Simulator" << std::endl;
    std::cout << "Copyright (C) 2020 by Johannes Delport (jdelport@sun.ac.za)" << std::endl;
    std::cout << "v" << VERSION << "." << GIT_COMMIT_HASH << " compiled on " << __DATE__ << " at " << __TIME__
              << std::endl;
#ifndef NDEBUG
    std::cout << "(Debug)" << std::endl;
#endif
    std::cout << std::endl;
}
