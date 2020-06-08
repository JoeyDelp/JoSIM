// Copyright (c) 2020 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/Netlist.hpp"
#include "JoSIM/AnalysisType.hpp"
#include "JoSIM/Errors.hpp"
#include "JoSIM/Misc.hpp"
#include "JoSIM/InputType.hpp"

using namespace JoSIM;

void Netlist::id_io_subc_label(
  const tokens_t &lineTokens, tokens_t &io, 
  std::string &subcktName, std::string &label) {
  // Id the label 
  label = lineTokens.front();
  // Check the convention
  if (argConv == InputType::Jsim) /* LEFT */ {
    // Set subcircuit name
    subcktName = lineTokens.at(1);
    // Assign the IO
    io.assign(lineTokens.begin() + 2, lineTokens.end());
  } else if (argConv == InputType::WrSpice) /* RIGHT */ {
    // Set subcircuit name
    subcktName = lineTokens.back();
    // Assign the IO
    io.assign(lineTokens.begin() + 1, lineTokens.end() - 1);
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
    // If the subcircuit IO contains the first node 
    if (rename_io_nodes(tokens.at(1), subc.io, io) && tokens.at(1) != "GND") {
      // Append the parent label to the node name
      tokens.at(1) = tokens.at(1) + "|" + label;
    }
    // If the subcircuit IO contains the second node
    if (rename_io_nodes(tokens.at(2), subc.io, io) && tokens.at(2) != "GND") {
      // Append the parent label to the node name
      tokens.at(2) = tokens.at(2) + "|" + label;
    }
    // If device type identifier is any of "EFGHT" check the next two nodes
    if(std::string("EFGHT").find(tokens.at(0).at(0)) != std::string::npos) {
      // If the subcircuit IO contains the third node
      if (rename_io_nodes(tokens.at(3), subc.io, io) && tokens.at(3) != "GND") {
        // Append the parent label to the node name
        tokens.at(3) = tokens.at(3) + "|" + label;
      }
      // If the subcircuit IO contains the fourth node 
      if (rename_io_nodes(tokens.at(4), subc.io, io) && tokens.at(4) != "GND") {
        // Append the parent label to the node name
        tokens.at(4) = tokens.at(4) + "|" + label;
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
  // While we are nested (depth not zero)
  while (nestedSubcktCount != 0) {
    // Loop through subcircuits
    for (const auto &i : subcircuits) {
      for (int j = 0; j < subcircuits.at(i.first).lines.size(); ++j) {
        // Shorthand for the current subcircuit
        Subcircuit &subcircuit = subcircuits.at(i.first);
        // Shorthand for the current subcircuit line
        tokens_t &subcCurrentLine = subcircuit.lines.at(j).first;
        // If the line denotes a subcircuit
        if (subcCurrentLine.front().at(0) == 'X') {
          id_io_subc_label(subcCurrentLine, io, subcktName, label);
          // If the identified subcircuit exists within known subcircuits
          if (subcircuits.count(subcktName) != 0) {
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
              // Reduce the total nested subcircuit count by 1
              --nestedSubcktCount;
              // If this subcircuit subcircuit counter becomes zero
              if(subcircuit.subcktCounter == 0)
                // This subcircuit no longer contains subcircuits
                subcircuit.containsSubckt = false;
            }
          // Complain if the subcircuit is not within known subcircuits
          } else
            Errors::input_errors(InputErrors::UNKNOWN_SUBCKT, subcktName);
        }
      }
    }
  }
}

void Netlist::expand_maindesign() {
  // Variable to store io
  tokens_t io;
  // std::vector<std::pair<std::string, std::string>> moddedLines;
  std::string subcktName, label;
  // Loop through the identified main design, line by line
  for (int i = 0; i < maindesign.size(); ++i) {
    // If the line denotes a subcircuit
    if (maindesign.at(i).front().at(0) == 'X') {
      id_io_subc_label(maindesign.at(i), io, subcktName, label);
      // If the identified subcircuit exists within known subcircuits
      if (subcircuits.count(subcktName) != 0) {
        // Copy of subcircuit for this instance
        Subcircuit subc = subcircuits.at(subcktName);
        // Expand the appropriate IO nodes of the subcircuit for this instance
        expand_io(subc, io, label);
        // Add the expanded subcircuit lines to the expanded netlist
        expNetlist.insert(expNetlist.end(), subc.lines.begin(),
                          subc.lines.end());
      // Complain if subcircuit does not exist
      } else {
        Errors::input_errors(InputErrors::UNKNOWN_SUBCKT, subcktName);
      }
    // If the line is not a subcircuit 
    } else {
      // Add the line tokens to the expanded netlist
      expNetlist.push_back(std::make_pair(maindesign.at(i), std::nullopt));
    }
  }
}