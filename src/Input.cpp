// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/Input.hpp"

#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <thread>

#include "JoSIM/ProgressBar.hpp"

using namespace JoSIM;

std::vector<tokens_t> Input::read_input(LineInput& input, string_o fileName) {
  // When the standard input is selected, "fileName" will be in the nullopt
  // state.
  if (fileName != std::nullopt) {
    if (std::filesystem::path(fileName.value()).has_parent_path()) {
      fileParentPath =
          std::filesystem::path(fileName.value()).parent_path().string();
    }
  }
  // Variable to store the read line
  std::string line;
  // Variable to store all the read lines
  std::vector<tokens_t> fileLines;
  // Variable to store the tokenized line
  tokens_t tokens;
  // Do this until the end of file is reached.
  while (input.next()) {
    // Create a mutable line
    line = input.line();
    // Remove all Carriage Return characters
    line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
    // Remove all the leading, trailing and extra white spaces
    line = std::regex_replace(line, std::regex("^ +| +$|( ) +"), "$1");
    if (!line.empty() && line.at(0) != '*' && line.at(0) != '#') {
      // Tokenize the line
      tokens = Misc::tokenize(line);
      // Uppercase just the first token for ".INCLUDE" and ".FILE" check
      std::transform(tokens.begin(), tokens.begin() + 1, tokens.begin(),
                     [](std::string& c) -> std::string {
                       std::transform(c.begin(), c.end(), c.begin(), toupper);
                       return c;
                     });
      // If the line contains a "INCLUDE" statement
      if (tokens.at(0) == ".INCLUDE" || tokens.at(0) == "INCLUDE") {
        // Variable to store path to file to include
        std::string includeFile;
        // If reading from a file
        if (fileName) {
          // Sanity check to prevent cyclic includes
          if (std::filesystem::path(fileName.value()).c_str() ==
              std::filesystem::path(fileName.value())
                  .parent_path()
                  .append(tokens.at(1))
                  .c_str()) {
            Errors::input_errors(InputErrors::CYCLIC_INCLUDE, fileName);
          }
          includeFile = std::filesystem::path(fileName.value())
                            .parent_path()
                            .append(tokens.at(1))
                            .string();
        } else {
          includeFile =
              std::filesystem::current_path().append(tokens.at(1)).string();
        }
        // Create a new LineInput variable for the included file
        FileInput file(includeFile);
        // Attempt to read the contents of the included file
        std::vector<tokens_t> tempInclude =
            Input::read_input(file, includeFile);
        // Insert the lines from the included file in place
        fileLines.insert(fileLines.end(), tempInclude.begin(),
                         tempInclude.end());
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
            path = std::filesystem::path(fileParentPath.value())
                       .append(tokens.at(1));
          }
          output_files.emplace_back(OutputFile(path.string()));
        }
        fileLines.emplace_back(tokens);
        // If the line contains a "END" statement
      } else if (tokens.at(0) == ".END" || tokens.at(0) == "END") {
        break;
        // If the line does not contain an "INCLUDE", "FILE" or "END" statement
      } else {
        // Transform the entire line to upper case
        std::transform(tokens.begin() + 1, tokens.end(), tokens.begin() + 1,
                       [](std::string& c) -> std::string {
                         std::transform(c.begin(), c.end(), c.begin(), toupper);
                         return c;
                       });
        // If the line starts with a '+' append it to the previous line.
        if (tokens.at(0).at(0) == '+') {
          // Remove the '+'
          tokens.at(0) = tokens.at(0).substr(1);
          if (tokens.at(0).empty()) {
            fileLines.back().insert(fileLines.back().end(), tokens.begin() + 1,
                                    tokens.end());
          } else {
            fileLines.back().insert(fileLines.back().end(), tokens.begin(),
                                    tokens.end());
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
    return fileLines;
  }
  // Return to satisfy return type. This should never be reached.
  return {};
}

void Input::parse_input(string_o fileName) {
  srand(time(NULL));
  // Create a seperate thread that will be used for printing creation progress
  std::thread printingThread;
  // Variable to store the tokenized input
  std::vector<tokens_t> fileLines;
  // Create a optional string to store subcircuit names
  string_o subckt;
  // Create boolean to determine if in a control block
  bool control = false;
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
    bar.set_total((float)fileLines.size());
  }
  // If not minimal printing
  netlist.argMin = argMin;
  // Loop through all the tokenized input
  for (int64_t i = 0; i < fileLines.size(); ++i) {
    // If not minimal printing
    if (!argMin) {
      // Report progress
      bar.update((float)i);
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
              Errors::input_errors(InputErrors::DUPLICATE_SUBCIRCUIT,
                                   subckt.value());
              netlist.subcircuits[subckt.value()].io.clear();
              netlist.subcircuits[subckt.value()].lines.clear();
            }
            // Populate the a subcircuit at given name with its IO nodes
            for (int64_t j = 2; j < fileLines.at(i).size(); ++j) {
              netlist.subcircuits[subckt.value()].io.push_back(
                  fileLines.at(i).at(j));
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
        netlist.models[std::make_pair(fileLines.at(i).at(1), subckt)] =
            fileLines.at(i);
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
        netlist.models[std::make_pair(fileLines.at(i).at(1), subckt)] =
            fileLines.at(i);
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
        netlist.subcircuits[subckt.value()].lines.push_back(
            std::make_pair(fileLines.at(i), subckt));
        // If not, add line to main design
      } else {
        netlist.maindesign.push_back(fileLines.at(i));
      }
    }
  }
  // Sanity check all conrols for syntax and functionality
  syntax_check_controls(controls);
  // Let the user know the input reading is complete
  if (!argMin) {
    bar.complete();
    std::cout << "\n";
  }
  // If main is empty, complain
  if (netlist.maindesign.empty())
    Errors::input_errors(InputErrors::MISSING_MAIN);
}

void Input::syntax_check_controls(std::vector<tokens_t>& controls) {
  // This will simply check controls, complaining if any are not allowed
  std::vector<std::string> v = {"PRINT", "TRAN", "SAVE",   "PLOT", "END",
                                "TEMP",  "NEB",  "SPREAD", "FILE", "IV"};
  for (auto i : controls) {
    if (std::find(v.begin(), v.end(), i.at(0)) == v.end()) {
      Errors::input_errors(InputErrors::UNKNOWN_CONTROL, i.at(0));
    }
  }
}