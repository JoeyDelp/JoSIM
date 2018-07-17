// Copyright (c) 2018 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#include "j_input.hpp"

/*
  Model parsing. Split a model line into parameters
*/
void parse_model(std::string s, std::unordered_map<std::string, Model>& m)
{
  std::vector<std::string> t, p;
  std::string sname;
  t = tokenize_delimeter(s, " \t(),");
  m[t.at(1)].modelname = sname = t.at(1);
  if(t.at(2) == "JJ") {
    m.at(sname).modelType = RCSJ;
    for (int i = 3; i < t.size(); i++) {
      p = tokenize_delimeter(t.at(i), "=:");
      if(p.at(0) == "RTYPE") m.at(sname).jj.rtype = p.at(1);
      else if(p.at(0) == "CAP") m.at(sname).jj.cap = p.at(1);
      else if(p.at(0) == "CCT") m.at(sname).jj.cct = p.at(1);
      else if(p.at(0) == "DELV") m.at(sname).jj.delv = p.at(1);
      else if(p.at(0) == "ICON") m.at(sname).jj.icon = p.at(1);
      else if(p.at(0) == "ICRIT") m.at(sname).jj.icrit = p.at(1);
      else if(p.at(0) == "RNORMAL" || p.at(0) == "RN") m.at(sname).jj.rnormal = p.at(1);
      else if(p.at(0) == "RZERO" || p.at(0) == "R0") m.at(sname).jj.rzero = p.at(1);
      else if(p.at(0) == "VG") m.at(sname).jj.vg = p.at(1);
      else model_errors(PARAM_TYPE_ERROR, p.at(0));
    }
  }
  else if(t.at(2) == "MTJ") {
    m.at(sname).modelType = MTJ;
  }
  else if(t.at(2) == "NTRON") {
    m.at(sname).modelType = NTRON;
  }
  else if(t.at(2) == "CSHE"){
    m.at(sname).modelType = CSHE;
  }
  else model_errors(UNKNOWN_MODEL_TYPE, t.at(2));
}
/*
  Input File Constructor
*/
InputFile::InputFile(std::string iFileName)
{
  std::string line;
  // Create a new input file stream using the specified file name
  std::fstream ifile(iFileName);
  // Check to see if the file is open
  if (ifile.is_open()) {
    // While not end of the file
    while (!ifile.eof()) {
      // Read each line into the line variable
      getline(ifile, line);
      // Transform the line variable to uppercase to simplify program
      std::transform(line.begin(), line.end(), line.begin(), toupper);
      // If not empty and ends with carret return, remove the character
      if (!line.empty() && line.back() == '\r')
        line.pop_back();
      // If the line is not empty push it back into the vector of lines
      if (!line.empty() && !starts_with(line, '*'))
        lines.push_back(line);
    }
  }
  // Error if file could not be opened
  else {
    error_handling(CANNOT_OPEN_FILE);
    exit(0);
  }
}
/*
  Recursive function, for subcircuit traversal
*/
std::vector<std::string> recurseSubckt(std::unordered_map<std::string, Subcircuit> subckts, std::string part) {
  std::vector<std::string> t, ckts;
  ckts.clear();
  if(subckts.at(part).containsSubckt) {
    for(auto i : subckts.at(part).subckts) {
        ckts = recurseSubckt(subckts, i);
    }
  }
  if(!subckts.at(part).subckts.empty()) 
    ckts.insert(ckts.end(), subckts.at(part).subckts.begin(), subckts.at(part).subckts.end());
  return ckts;
}

/*
  Split the circuit into subcircuits, main circuit and identify circuit stats
*/
void
InputFile::circuit_to_segments(InputFile& iFile)
{
  // Variable declarations
  bool startCkt = false;
  bool controlSection = false;
  std::string subcktName;
  subCircuitCount = 0;
  std::vector<std::string> parameterTokens, tokens, v;
  // Identify the segments of the circuit
  for (const auto& i : lines) {
    // Clear the tokens to remove any garbage
    tokens.clear();
    // Split the line into tokens
    tokens = tokenize_space(i);
    // If the line in question is the start of a subcircuit
    if (tokens.at(0) == ".SUBCKT") {
      // The second token will/should be the subcircuit name
      subcktName = tokens.at(1);
      // The rest of the tokens will be the IO labels
      subcircuitSegments[subcktName].name = subcktName;
      subcircuitSegments.at(subcktName).io.assign(tokens.begin() + 2, tokens.end());
      // Set the subcircuit flag to true
      startCkt = true;
      // Increase the subcircuit counter
      subCircuitCount++;
    }
    // Else if the subcircuit flag is set
    else if (startCkt) {
      // If the line is not a control card
      if (tokens.at(0).at(0) != '.') {
        // Push the line back to the corresponding subcircuit vector
        subcircuitSegments.at(subcktName).lines.push_back(i);
        if(tokens.at(0).at(0) != 'X') subcircuitSegments.at(subcktName).componentCount++;
        if(tokens.at(0).at(0) == 'B') subcircuitSegments.at(subcktName).jjCount++;
        if(tokens.at(0).at(0) == 'X') {
          subcircuitSegments.at(subcktName).containsSubckt = true;
          if (subcktConv == 0) subcircuitSegments.at(subcktName).subckts.push_back(tokens.at(1));
          else if (subcktConv == 1) subcircuitSegments.at(subcktName).subckts.push_back(tokens.back());
        }
      }
      // Else if the control card is a parameter
      else if (tokens.at(0) == ".PARAM") {
        // Tokenize the string using a space and an '=' sign
        // Elementary method will improve in the future
        parameterTokens = tokenize_delimeter(i, " =");
        // Parse the expression to the right of the '=' sign and save as
        // variable of name to the left of '=' sign + subcircuit name
        parse_expression(parameterTokens.at(1),
                        parameterTokens.at(2), subcircuitSegments.at(subcktName).parVal, iFile.parVal,
                        subcktName);
      }
      // Else if the control card is a model
      else if (tokens.at(0) == ".MODEL") {
        // Add the model to the corresponding subcircuit models vector
        parse_model(i, subcircuitSegments.at(subcktName).subcktModels);
      }
      // Else if the control card ends the subcircuit
      else if (tokens.at(0) == ".ENDS") {
        // Set the subcircuit flag to false
        startCkt = false;
        // Mark the location of the last subcircuit within the lines vector
      }
    }
    // Else if the line is not a control card and not part of the control
    // section
    else if (tokens.at(0).at(0) != '.' && !controlSection) {
      // Push the line back to the corresponding subcircuit vector
      maincircuitSegment.push_back(i);
      if(tokens.at(0).at(0) != 'X') mainComponents++;
      if(tokens.at(0).at(0) == 'B') mainJJs++;
      if(tokens.at(0).at(0) == 'X') {
        if (subcktConv == 0) subckts.push_back(tokens.at(1));
        else if (subcktConv == 1) subckts.push_back(tokens.back());
      }
    }
    // Else if this is the start of the control section
    else if (tokens.at(0) == ".CONTROL") {
      // Set control section flag to true
      controlSection = true;
    }
    // Else if not part of the control section
    else if (!controlSection) {
      // Identify the models in the main part of the circuit
      if (tokens.at(0) == ".MODEL") {
        parse_model(i, mainModels);
      }
      // Identify the parametes in the main part of the circuit
      else if (tokens.at(0) == ".PARAM") {
        // Tokenize the string using a space and an '=' sign
        // Elementary method will improve in the future
        parameterTokens = tokenize_delimeter(i, " =");
        // Parse the expression to the right of the '=' sign and save as
        // variable of name to the left of '=' sign
        parse_expression(parameterTokens.at(1), parameterTokens.at(2), iFile.parVal, iFile.parVal);
      }
      // Identify the end of the control section
      else if (tokens.at(0) == ".ENDC")
        controlSection = false;
      // Identify the controls in the main part of the circuit
      else controlPart.push_back(i);
    } else if (controlSection) {
      // Identify the models in the main part of the circuit
      if (tokens.at(0) == "MODEL") {
        parse_model(i, mainModels);
      }
      // Identify the parametes in the main part of the circuit
      else if (tokens.at(0) == "PARAM") {
        // Tokenize the string using a space and an '=' sign
        // Elementary method will improve in the future
        parameterTokens = tokenize_delimeter(i, " =");
        // Parse the expression to the right of the '=' sign and save as
        // variable of name to the left of '=' sign
        parse_expression(parameterTokens.at(1), parameterTokens.at(2), iFile.parVal, iFile.parVal);
      }
      // Identify the end of the control section
      else if (tokens.at(0) == "ENDC")
        controlSection = false;
      // Identify the controls in the main part of the circuit
      else controlPart.push_back(i);
    }
  }
  std::vector<std::string> mainSubckts = iFile.subckts;
  for(auto i : mainSubckts) {
    v = recurseSubckt(iFile.subcircuitSegments, i);
    if(!v.empty()) iFile.subckts.insert(iFile.subckts.end(), v.begin(), v.end());
    v.empty();
  }
  circuitComponentCount += mainComponents;
  circuitJJCount += mainJJs;
  for (int i = 0; i < subckts.size(); i++) {
    circuitComponentCount += subcircuitSegments.at(subckts.at(i)).componentCount;
    circuitJJCount += subcircuitSegments.at(subckts.at(i)).jjCount;
  }
}

/*
  Substitute subcircuits into the main circuit
*/
void
InputFile::sub_in_subcircuits(InputFile& iFile, std::vector<std::string>& segment, std::string label)
{
  std::vector<std::string> tokens, duplicateSegment, io;
  std::string subckt,
              parString,
              modelLabel,
              i,
              origLabel = label;
  // Loop through the specified segment
  #pragma omp parallel for private(subckt, parString, modelLabel, i, origLabel, tokens, io, label)
  for (int o = 0; o < segment.size(); o++) {
    i = segment.at(o);
    // If the line in the segment is identified as a subcircuit
    if (i.at(0) == 'X') {
      tokens = tokenize_space(i);
      try {
        // Identify the subcircuit label
        modelLabel = tokens.at(0);
        // Check to see if it is a subcircuit within a subcircuit so that
        // labeling can be done right
        if (label.empty())
          label = tokens.at(0);
        else
          label.append("|" + tokens.at(0));
        // Depending on subcircuit convention, use 2nd or last token
        if (subcktConv == 0) {
          // Identify the type of subcircuit
          if(tokens.at(1).find("|") != std::string::npos) subckt = tokens.at(1).substr(0, tokens.at(1).find_first_of("|"));
          else subckt = tokens.at(1);
          // The rest of the tokens should only be the IO nodes which will be
          // matched to the corresponding subcircuit IO
          io.clear();
          io.insert(io.begin(), tokens.begin() + 2, tokens.end());
        }
        else if (subcktConv == 1) {
          // Identify the type of subcircuit
          if(tokens.back().find("|") != std::string::npos) subckt = tokens.back().substr(0, tokens.back().find_first_of("|"));
          else subckt = tokens.back();
          // The rest of the tokens should only be the IO nodes which will be
          // matched to the corresponding subcircuit IO
          io.clear();
          io.insert(io.begin(), tokens.begin() + 1, tokens.end() - 1);
        }
        // Create the subcircuit name map to identify subcircuits by their labels
        if(iFile.subcircuitNameMap.count(label) == 0) {
          iFile.subcircuitNameMap[label] = subckt;
        }
        // Check whether the correct amount of nodes was specified for the
        // subcircuit
        if (io.size() != iFile.subcircuitSegments.at(subckt).io.size())
          invalid_component_errors(INVALID_SUBCIRCUIT_NODES, label);
        // Loop through the lines of the identified subcircuit segment
        for (auto j : iFile.subcircuitSegments.at(subckt).lines) {
          // If the line is not a subcircuit line
          if (j.at(0) != 'X') {
            // Tokenize the line so that tokens can be altered
            tokens = tokenize_space(j);
            // Append the overarching subcircuit label to the label using a
            // underscore. Eg L01 becomes L01_X01 or L01_X01_X02 depending on
            // the depth
            tokens.at(0) = tokens.at(0) + "|" + label;
            // Check if the second token (positive node) is identified as IO
            // node
            if (std::find(iFile.subcircuitSegments.at(subckt).io.begin(),
                          iFile.subcircuitSegments.at(subckt).io.end(),
                          tokens.at(1)) !=
                iFile.subcircuitSegments.at(subckt).io.end()) {
              for (const auto& k : iFile.subcircuitSegments.at(subckt).io) {
                if (k == tokens.at(1)) {
                  // Sub the token for corresponding overarching subcircuit IO
                  // node
                  tokens.at(1) =
                    io.at(index_of(iFile.subcircuitSegments.at(subckt).io, k));
                  break;
                }
              }
              // If the positive node was identified as IO then the negative
              // node will not be IO as well. Append label
              if (tokens.at(2) != "0" && tokens.at(2) != "GND")
                tokens.at(2) = tokens.at(2) + "|" + label;
            }
            // If the third token (negative node) is identified as IO node
            else if (std::find(iFile.subcircuitSegments.at(subckt).io.begin(),
                                iFile.subcircuitSegments.at(subckt).io.end(),
                                tokens.at(2)) !=
                      iFile.subcircuitSegments.at(subckt).io.end()) {
              for (const auto& k : iFile.subcircuitSegments.at(subckt).io) {
                // Sub the token for corresponding overarching subcircuit IO
                // node
                if (k == tokens.at(2))
                  tokens.at(2) =
                    io.at(index_of(iFile.subcircuitSegments.at(subckt).io, k));
              }
              // If the negative node was identified as IO then the positive
              // node will not be IO as well. Append label
              if (tokens.at(1) != "0" && tokens.at(1) != "GND")
                tokens.at(1) = tokens.at(1) + "|" + label;
            }
            // If neither nodes are IO append the label
            else {
              if (tokens.at(1) != "0" && tokens.at(1) != "GND")
                tokens.at(1) = tokens.at(1) + "|" + label;
              if (tokens.at(2) != "0" && tokens.at(2) != "GND")
                tokens.at(2) = tokens.at(2) + "|" + label;
              // If the device is a junction then the 4th or 5th (WRSpice)
              // token will be the model therefore append label so it can
              // later be identified.
              if (j.at(0) == 'B') {
                if (tokens.size() > 5) {
                  if (tokens.at(5).find("AREA=") != std::string::npos) {
                    if (tokens.at(3) != "0" && tokens.at(3) != "GND")
                      tokens.at(3) = tokens.at(3) + "|" + label;
                    tokens.at(4) = subckt + "|" + tokens.at(4);
                  }
                } else
                  tokens.at(3) = subckt + "|" + tokens.at(3);
              }
            }
            // Now for the tricky part:
            // Subbing in the values if they are parameterized within the
            // subcircuit.
            for (int k = 3; k < tokens.size(); k++) {
              // If the line is a junction then look of the AREA part as this
              // is the only thing that can be parameterized (FOR NOW)
              if (tokens.at(0).at(0) == 'B') {
                if (tokens.at(k).find("AREA=") != std::string::npos)
                  if (parVal.find(subckt + "|" +
                                  substring_after(tokens.at(k), "AREA=")) !=
                      parVal.end())
                    tokens.at(k) = precise_to_string(
                      parVal.at(subckt + "|" +
                              substring_after(tokens.at(k), "AREA=")));
              }
              // Else loop through all the remaining tokens subbing in
              // parameter values if they exist
              else {
                // If token starts with a brace
                if (tokens.at(k).at(0) == '(') {
                  // Remove the brace
                  parString = tokens.at(k).substr(1);
                  // Check if the value is a parameter value
                  if (parVal.find(subckt + "_" + parString) != parVal.end())
                    // Replace if it is and put back the brace
                    tokens.at(k) = "(" + precise_to_string(
                                        parVal.at(subckt + "_" + parString));
                // Else if the token ends with a brace
                } else if (tokens.at(k).back() == ')') {
                  // Remove the brace
                  parString = tokens.at(k).substr(0, tokens.at(k).size() - 1);
                  // Check if the value is a parameter value
                  if (parVal.find(subckt + "_" + parString) != parVal.end())
                    // Replace if it is and put back the brace
                    tokens.at(k) =
                      precise_to_string(parVal.at(subckt + "_" + parString)) +
                      ")";
                // If the token starts with something like PWL OR PULSE
                } else if (tokens.at(k).find("PWL(") != std::string::npos) {
                  parString = substring_after(tokens.at(k), "PWL(");
                  if (parVal.find(subckt + "_" + parString) != parVal.end())
                    tokens.at(k) = "PWL(" + precise_to_string(
                                            parVal.at(subckt + "_" + parString));
                } else if (tokens.at(k).find("PULSE(") != std::string::npos) {
                  parString = substring_after(tokens.at(k), "PULSE(");
                  if (parVal.find(subckt + "_" + parString) != parVal.end())
                    tokens.at(k) =
                      "PULSE(" +
                      precise_to_string(parVal.at(subckt + "_" + parString));
                // Else check the token and replace if it is a parementer
                } else {
                  if (parVal.find(subckt + "_" + tokens.at(k)) != parVal.end())
                    tokens.at(k) =
                      precise_to_string(parVal.at(subckt + "_" + tokens.at(k)));
                }
              }
            }
            // Recreate string from the tokens
            std::string line = tokens.at(0);
            for (int k = 1; k < tokens.size(); k++) {
              line = line + " " + tokens.at(k);
            }
            #pragma omp critical
            duplicateSegment.push_back(line);
          }
          // Else if line is a subcircuit (subcircuit in subcircuit)
          else {
            // Tokenize the line
            tokens = tokenize_space(j);
            // Attach the subcircuit label to the label
            tokens.at(0) = tokens.at(0) + "|" + label;
            // The string to reconstruct the line
            std::string line = tokens.at(0);
            // Loop through tokens replacing
            for (int k = 1; k < tokens.size(); k++) {
              // If node is IO
              if (std::find(iFile.subcircuitSegments.at(subckt).io.begin(),
                            iFile.subcircuitSegments.at(subckt).io.end(),
                            tokens.at(k)) !=
                  iFile.subcircuitSegments.at(subckt).io.end()) {
                // Loop through subcircuit IO
                for (auto l : iFile.subcircuitSegments.at(subckt).io) {
                  if (l == tokens.at(k))
                    // Rplace IO with top level nodes
                    tokens.at(k) =
                      io.at(index_of(iFile.subcircuitSegments.at(subckt).io, l));
                }
              } else {
                // If node is not ground, append label
                if (tokens.at(k) != "0" && tokens.at(k) != "GND")
                  tokens.at(k) = tokens.at(k) + "|" + label;
              }
            }
            // Reconstruct the line
            for (int k = 1; k < tokens.size(); k++) {
              line = line + " " + tokens.at(k);
            }
            #pragma omp critical
            duplicateSegment.push_back(line);
          }
        }
      } catch (const std::out_of_range&) {
      }
    } else
      #pragma omp critical
      duplicateSegment.push_back(i);
    label = origLabel;
  }
  #pragma omp barrier
  #pragma omp critical
  segment = duplicateSegment;
}
