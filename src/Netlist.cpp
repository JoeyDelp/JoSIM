// Copyright (c) 2020 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/Netlist.hpp"
#include "JoSIM/AnalysisType.hpp"
#include "JoSIM/Errors.hpp"
#include "JoSIM/Misc.hpp"
#include "JoSIM/InputType.hpp"
#include "JoSIM/ProgressBar.hpp"

#include <thread>

using namespace JoSIM;

void Netlist::id_io_subc_label(
  const tokens_t &lineTokens, tokens_t &io, 
  std::string &subcktName, std::string &label,
  const std::unordered_map<std::string, Subcircuit> &subcircuits) {
  // Id the label 
  label = lineTokens.front();
  // Check the convention
  // At this point in the program all subcircuits should have been identified
  // Thus we can determine the convention from code
  // If the token right after the label exists in the subcircuits map
  if(subcircuits.count(lineTokens.at(1)) != 0) {
    // This is the subcircuit name
    subcktName = lineTokens.at(1);
    // Assign the IO
    io.assign(lineTokens.begin() + 2, lineTokens.end());
  // Else if the last token exists in the subcircuit map
  } else if(subcircuits.count(lineTokens.back()) != 0) {
    // Then this is the subcircuit name
    subcktName = lineTokens.back();
    // Assign the IO
    io.assign(lineTokens.begin() + 1, lineTokens.end() - 1);
  // Else if the neither then this subcircuit surely does not exist
  } else {
    Errors::input_errors(InputErrors::UNKNOWN_SUBCKT, subcktName);
  }
}

bool Netlist::rename_io_nodes(
  std::string &node, const tokens_t &subIO, const tokens_t &parentIO) {
  // If the subcircuit IO contains the first node 
  if (std::count(subIO.begin(), subIO.end(), node) != 0) {
    // Loop through the subcircuit IO
    for (int l = 0; l < subIO.size(); ++l) {
      // If the nodes match
      if (::strcmp(node.c_str(), subIO.at(l).c_str()) == 0) {
        // Replace the node with the IO node
        node = parentIO.at(l);
        // Return true
        return true;
      }
    }
  }
  // Return false
  return false;
}

void Netlist::expand_io(
  Subcircuit &subc, const tokens_t &io, const std::string &label) {
  // Loop through the identified subcircuit
  for (int k = 0; k < subc.lines.size(); ++k) {
    // Set shorthand for long variable name
    tokens_t &tokens = subc.lines.at(k).first;
    // Append the label of the parent to the subcircuit label
    tokens.at(0) = tokens.at(0) + "|" + label;
    // Determine amount of nodes to process
    int nodeCount = 2;
    // If device type identifier is any of "EFGHT" check the next two nodes
    if(std::string("EFGHT").find(tokens.at(0).at(0)) != std::string::npos) {
      nodeCount = 4;
    }
    // Loop through all the nodes changing where necessary
    for (int n = 1; n < nodeCount + 1; ++n) {
      // Ensure nodes don't identify as ground
      if (tokens.at(n) != "0" && tokens.at(n) != "GND") {
        // Ensure the node is not part of the IO
        if (!rename_io_nodes(tokens.at(n), subc.io, io)) {
          // Append the label to the node number to make it unique
          tokens.at(n) = tokens.at(n) + "|" + label;
        }
      }
    }
  }
}

void Netlist::expand_subcircuits() {
  // Variable to store io
  tokens_t io;
  // std::vector<std::pair<tokens_t, string_o>> moddedLines;
  std::string subcktName, label;
  // Loop through subcircuits, line by line
  for (const auto &i : subcircuits) {
    for (const auto &j : i.second.lines) {
      // If a line is found that starts with an 'X'
      if (j.first.front().at(0) == 'X') {
        // This subcircuit contains a subcircuit
        subcircuits.at(i.first).containsSubckt = true;
        // Increase the expansion counter
        subcircuits.at(i.first).subcktCounter++;
        // Increase the nest depth
        nestedSubcktCount++;
      }
    }
  }
  ProgressBar bar;
  if(!argMin) {
    bar.create_thread();
    bar.set_bar_width(30);
    bar.fill_bar_progress_with("O");
    bar.fill_bar_remainder_with(" ");
    bar.set_status_text("Expanding Subcircuits");
    bar.set_total((float)nestedSubcktCount);
  }
  // If not minimal printing
  int cc = 0;
  // While we are nested (depth not zero)
  while (nestedSubcktCount != 0) {
    // If not minimal printing
    if(!argMin) {
      // Report progress
      bar.update(cc);
    }
    // Loop through subcircuits
    for (const auto &i : subcircuits) {
      for (int j = 0; j < subcircuits.at(i.first).lines.size(); ++j) {
        // Shorthand for the current subcircuit
        Subcircuit &subcircuit = subcircuits.at(i.first);
        // Shorthand for the current subcircuit line
        tokens_t &subcCurrentLine = subcircuit.lines.at(j).first;
        // If the line denotes a subcircuit
        if (subcCurrentLine.front().at(0) == 'X') {
          id_io_subc_label(
            subcCurrentLine, io, subcktName, label, subcircuits);
          // Create a copy of the subircuit for this instance
          Subcircuit subc = subcircuits.at(subcktName);
          // If not nested
          if (!subc.containsSubckt) {
            // Expand the IO nodes of the subcircuit for this instance
            expand_io(subc, io, label);
            // Erase the current line (tokens)
            subcircuit.lines.erase(subcircuit.lines.begin() + j);
            // Insert the subcircuit token lines at the erased line position
            subcircuit.lines.insert(subcircuit.lines.begin() + j,
              subc.lines.begin(), subc.lines.end());
            // Reduce the total amound of subcircuits in this subcircuit by 1
            --subcircuit.subcktCounter;
            ++cc;
            // Reduce the total nested subcircuit count by 1
            --nestedSubcktCount;
            // If this subcircuit subcircuit counter becomes zero
            if(subcircuit.subcktCounter == 0)
              // This subcircuit no longer contains subcircuits
              subcircuit.containsSubckt = false;
          }
        }
      }
    }
  }
  // Let the user know subcircuit expansion is complete
  if(!argMin) {
    bar.update(100);
    bar.complete();
    std::cout << "\n";
  }
}

void Netlist::expand_maindesign() {
  // Variable to store io
  tokens_t io;
  // std::vector<std::pair<std::string, std::string>> moddedLines;
  std::string subcktName, label;
  ProgressBar bar;
  if(!argMin) {
    bar.create_thread();
    bar.set_bar_width(30);
    bar.fill_bar_progress_with("O");
    bar.fill_bar_remainder_with(" ");
    bar.set_status_text("Expanding Main Circuit");
    bar.set_total((float)maindesign.size());
  }
  // Loop through the identified main design, line by line
  for (int i = 0; i < maindesign.size(); ++i) {
    // If not minimal printing
    if(!argMin) {
      // Report progress
      bar.update(i);
    }
    // If the line denotes a subcircuit
    if (maindesign.at(i).front().at(0) == 'X') {
      id_io_subc_label(maindesign.at(i), io, subcktName, label, subcircuits);
      // Copy of subcircuit for this instance
      Subcircuit subc = subcircuits.at(subcktName);
      // Expand the appropriate IO nodes of the subcircuit for this instance
      expand_io(subc, io, label);
      // Add the expanded subcircuit lines to the expanded netlist
      expNetlist.insert(expNetlist.end(), subc.lines.begin(),
                        subc.lines.end());
    // If the line is not a subcircuit 
    } else {
      // Add the line tokens to the expanded netlist
      expNetlist.push_back(std::make_pair(maindesign.at(i), std::nullopt));
    }
  }
  // Let the user know main design expansion is complete
  if(!argMin) {
    bar.update(100);
    bar.complete();
    std::cout << "\n";
  }
  subcktTotal = subcircuits.size();
  subcircuits.clear();
}