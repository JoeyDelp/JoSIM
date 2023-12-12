// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/Matrix.hpp"

#include <algorithm>
#include <iostream>
#include <string>

#include "JoSIM/Constants.hpp"
#include "JoSIM/Errors.hpp"
#include "JoSIM/Function.hpp"
#include "JoSIM/Misc.hpp"
#include "JoSIM/Noise.hpp"
#include "JoSIM/ProgressBar.hpp"

using namespace JoSIM;

void Matrix::create_matrix(Input& iObj) {
  while (needsTR_) {
    // Do matrix setup
    setup(iObj);
    // Create the components
    create_components(iObj);
    if (needsTR_) {
      reduce_step(iObj);
    }
  }
  // Handle mutual inductances
  handle_mutual_inductance(iObj);
  // Create the compressed storage row format required for simulation
  create_csr();
}

void Matrix::setup(Input& iObj) {
  needsTR_ = false;
  spread.get_spreads(iObj);
  Noise::determine_global_temperature(iObj);
  Noise::determine_noise_effective_bandwidth(iObj);
  // Create a node counter variable
  int64_t nodeCounter = 0;
  // Variables to store node configs since they are already identified here
  nodeConfig.resize(iObj.netlist.expNetlist.size(), NodeConfig::GND);
  // Only available when 4 node components are detected
  nodeConfig2.resize(iObj.netlist.expNetlist.size(), NodeConfig::GND);
  int64_t cc = 0;
  for (auto& i : iObj.netlist.expNetlist) {
    if (i.first.at(0).at(0) != 'K') {
      // Expand all inline parameters
      expand_inline_parameters(i, iObj.parameters);
      // All devices should atleast have 4 parts
      if (i.first.size() < 4) {
        // Complain if it doesnt
        Errors::invalid_component_errors(
            ComponentErrors::INVALID_COMPONENT_DECLARATION,
            Misc::vector_to_string(i.first));
      }
      // Create a node map that maps the node names to numbers
      if (i.first.at(1).find("GND") == std::string::npos &&
          i.first.at(1) != "0") {
        // Add the first node to the map if not ground
        if (nm.count(i.first.at(1)) == 0) nm[i.first.at(1)] = nodeCounter++;
        nodeConfig.at(cc) = NodeConfig::POSGND;
      }
      if (i.first.at(2).find("GND") == std::string::npos &&
          i.first.at(2) != "0") {
        // Add the second node to the map if not ground
        if (nm.count(i.first.at(2)) == 0) nm[i.first.at(2)] = nodeCounter++;
        if (nodeConfig.at(cc) == NodeConfig::POSGND) {
          nodeConfig.at(cc) = NodeConfig::POSNEG;
        } else {
          nodeConfig.at(cc) = NodeConfig::GNDNEG;
        }
      }
      // If the device is as 4 node device
      if (std::string("EFGHT").find(i.first.front().at(0)) !=
          std::string::npos) {
        // Ensure the device has at least 6 parts
        if (i.first.size() < 6) {
          // Complain if it isnt
          Errors::invalid_component_errors(
              ComponentErrors::INVALID_COMPONENT_DECLARATION,
              Misc::vector_to_string(i.first));
        }
        if (i.first.at(3).find("GND") == std::string::npos &&
            i.first.at(3) != "0") {
          // Add the third node to the map if not ground
          if (nm.count(i.first.at(3)) == 0) nm[i.first.at(3)] = nodeCounter++;
          nodeConfig2.at(cc) = NodeConfig::POSGND;
        }
        if (i.first.at(4).find("GND") == std::string::npos &&
            i.first.at(4) != "0") {
          // Add the fourth node to the map if not ground
          if (nm.count(i.first.at(4)) == 0) nm[i.first.at(4)] = nodeCounter++;
          if (nodeConfig2.at(cc) == NodeConfig::POSGND) {
            nodeConfig2.at(cc) = NodeConfig::POSNEG;
          } else {
            nodeConfig2.at(cc) = NodeConfig::GNDNEG;
          }
        }
      }
    }
    ++cc;
  }
  // Resize the node connection vector to match the size of the node map
  nc.resize(nm.size());
  // Set the index to the first branch current to the size of the node map
  branchIndex = nm.size();
}

void Matrix::create_components(Input& iObj) {
  int64_t cc = 0;
  ProgressBar bar;
  if (!iObj.argMin) {
    bar.create_thread();
    bar.set_bar_width(30);
    bar.fill_bar_progress_with("O");
    bar.fill_bar_remainder_with(" ");
    bar.set_status_text("Creating Matrix");
    bar.set_total((float)iObj.netlist.expNetlist.size());
  }
  // Loop through all the components in the netlist
  for (auto it = iObj.netlist.expNetlist.begin();
       it != iObj.netlist.expNetlist.end(); it++) {
    const auto& i = *it;
    // If not minimal printing
    if (!iObj.argMin) {
      // Report progress
      bar.update((float)cc);
    }
    // First character of first token indicates device type
    switch (i.first.front().at(0)) {
        // Inductors (most common so do them first)
      case 'L':
        // Create an inductor and add it to the component list
        components.devices.emplace_back(Inductor(
            i, nodeConfig.at(cc), nm, lm, nc, iObj, spread, branchIndex));
        // Store this inductor's component list index for reference
        components.inductorIndices.emplace_back(components.devices.size() - 1);
        break;
        // Josephson junction (JJ)
      case 'B':
        // Create a JJ and add it to the component list
        components.devices.emplace_back(
            JJ(i, nodeConfig.at(cc), nm, lm, nc, iObj, spread, branchIndex));
        // Store this JJ's component list index for reference
        components.junctionIndices.emplace_back(components.devices.size() - 1);
        break;
        // Resistors
      case 'R':
        // Create a resistor and add it to the component list
        components.devices.emplace_back(Resistor(
            i, nodeConfig.at(cc), nm, lm, nc, iObj, spread, branchIndex));
        // Store this resistor's component list index for reference
        components.resistorIndices.emplace_back(components.devices.size() - 1);
        break;
        // Current Source
      case 'I':
        // Create a current source and add it to the current sources list
        components.currentsources.emplace_back(
            CurrentSource(i, nodeConfig.at(cc), nm, lm, sourcegen.size()));
        // Add it to the global sources list
        sourcegen.emplace_back();
        sourcegen.back().parse_function(Misc::vector_to_string(i.first), iObj,
                                        i.second);
        break;
        // Capacitors
      case 'C':
        // Create a capacitor and add it to the component list
        components.devices.emplace_back(Capacitor(
            i, nodeConfig.at(cc), nm, lm, nc, iObj, spread, branchIndex));
        // Store this capacitor's component list index for reference
        components.capacitorIndices.emplace_back(components.devices.size() - 1);
        break;
        // Voltage/Phase Source
      case 'P':
        // if(iObj.argAnal == AnalysisType::Phase) {
        //  Create a phase source and add it to the component list
        components.devices.emplace_back(PhaseSource(
            i, nodeConfig.at(cc), nm, lm, nc, branchIndex, sourcegen.size()));
        // Add it to the global sources list
        sourcegen.emplace_back();
        sourcegen.back().parse_function(Misc::vector_to_string(i.first), iObj,
                                        i.second);
        // Store this phase source component list index for reference
        components.psIndices.emplace_back(components.devices.size() - 1);
        break;
        //} else if(iObj.argAnal == AnalysisType::Voltage) {
      case 'V':
        // Create a voltage source and add it to the component list
        components.devices.emplace_back(VoltageSource(
            i, nodeConfig.at(cc), nm, lm, nc, branchIndex, sourcegen.size()));
        // Add it to the global sources list
        sourcegen.emplace_back();
        sourcegen.back().parse_function(Misc::vector_to_string(i.first), iObj,
                                        i.second);
        // Store this voltage source component list index for reference
        components.vsIndices.emplace_back(components.devices.size() - 1);
        //}
        break;
        // Mutual inductance
      case 'K':
        // Add line to the mutual inductance list for later inspection
        components.mutualinductances.emplace_back(i);
        break;
        // Transmission lines (4 node device)
      case 'T':
        // Create a transmission line and add it to the component list
        components.devices.emplace_back(TransmissionLine(
            i, nodeConfig.at(cc), nodeConfig2.at(cc), nm, lm, nc,
            iObj.parameters, iObj.argAnal, iObj.transSim.tstep(), branchIndex));
        if (std::get<TransmissionLine>(components.devices.back())
                .timestepDelay_ < 1) {
          needsTR_ = true;
          return;
        }
        // Store the transmission line component list index for reference
        components.txIndices.emplace_back(components.devices.size() - 1);
        break;
        // Voltage controlled current source (4 node device)
      case 'G':
        // Create a vccs and add it to the component list
        components.devices.emplace_back(VCCS(
            i, nodeConfig.at(cc), nodeConfig2.at(cc), nm, lm, nc,
            iObj.parameters, branchIndex, iObj.argAnal, iObj.transSim.tstep()));
        // Store the vccs component list index for reference
        components.vccsIndices.emplace_back(components.devices.size() - 1);
        break;
        // Current controlled current source (4 node device)
      case 'F':
        // Create a cccs and add it to the component list
        components.devices.emplace_back(CCCS(i, nodeConfig.at(cc),
                                             nodeConfig2.at(cc), nm, lm, nc,
                                             iObj.parameters, branchIndex));
        // Store the cccs component list index for reference
        components.cccsIndices.emplace_back(components.devices.size() - 1);
        break;
        // Voltage controlled voltage source (4 node device)
      case 'E':
        // Create a vcvs and add it to the component list
        components.devices.emplace_back(VCVS(i, nodeConfig.at(cc),
                                             nodeConfig2.at(cc), nm, lm, nc,
                                             iObj.parameters, branchIndex));
        // Store the vcvs component list index for reference
        components.vcvsIndices.emplace_back(components.devices.size() - 1);
        break;
        // Current controlled voltage source (4 node device)
      case 'H':
        // Create a ccvs and add it to the component list
        components.devices.emplace_back(CCVS(
            i, nodeConfig.at(cc), nodeConfig2.at(cc), nm, lm, nc,
            iObj.parameters, branchIndex, iObj.argAnal, iObj.transSim.tstep()));
        // Store the ccvs component list index for reference
        components.ccvsIndices.emplace_back(components.devices.size() - 1);
        break;
        // This is an error. This means that the component was unidentified.
      default:
        Errors::invalid_component_errors(ComponentErrors::UNKNOWN_DEVICE_TYPE,
                                         i.first.front());
    }
    // Increment the component counter
    ++cc;
  }
  if (!iObj.argMin) {
    bar.complete();
    std::cout << "\n";
  }
}

void Matrix::handle_mutual_inductance(Input& iObj) {
  int64_t cc = 0;
  ProgressBar bar2;
  if (!iObj.argMin && components.mutualinductances.size() != 0) {
    bar2.create_thread();
    bar2.set_bar_width(30);
    bar2.fill_bar_progress_with("O");
    bar2.fill_bar_remainder_with(" ");
    bar2.set_status_text("Adding Mutual Inductances");
    bar2.set_total((float)iObj.netlist.expNetlist.size());
  }
  // Loop through all identified mutual inductances
  for (const auto& s : components.mutualinductances) {
    // If not minimal printing
    if (!iObj.argMin && components.mutualinductances.size() != 0) {
      // Report progress
      bar2.update((float)cc);
    }
    // If the mutual inductance line has less than 4 tokens
    if (s.first.size() < 4) {
      // Complain
      Errors::invalid_component_errors(ComponentErrors::MUT_ERROR,
                                       Misc::vector_to_string(s.first));
    }
    // Variables to store the indices of the relevant inductors
    int_o ind1Index, ind2Index;
    // Loop through all the components in the simulation
    for (int64_t i = 0; i < components.devices.size(); ++i) {
      // Find the component label
      const auto& label = std::visit(
          [](const auto& device) noexcept -> const std::string& {
            return device.netlistInfo.label_;
          },
          components.devices.at(i));
      // See if it matches the first inductor label of the mutual inductance
      if (label == s.first.at(1)) {
        ind1Index = i;
      }
      // See if it matches the second inductor label of the mutual inductance
      if (label == s.first.at(2)) {
        ind2Index = i;
      }
      if (ind2Index && ind1Index) break;
    }
    // Complain if any of these inductors do not exist
    if (!ind1Index) {
      Errors::invalid_component_errors(ComponentErrors::MISSING_INDUCTOR,
                                       s.first.at(1));
    }
    if (!ind2Index) {
      Errors::invalid_component_errors(ComponentErrors::MISSING_INDUCTOR,
                                       s.first.at(2));
    }
    // Fetch the relevant inductors in the component list given the indices
    auto& ind1 = std::get<Inductor>(components.devices.at(ind1Index.value()));
    auto& ind2 = std::get<Inductor>(components.devices.at(ind2Index.value()));
    // Calculate the coupling factor
    double cf = parse_param(s.first.at(3), iObj.parameters, s.second);
    // Calculate the mutual inductance for this pair
    double mutual =
        cf * std::sqrt(ind1.netlistInfo.value_ * ind2.netlistInfo.value_);
    if (mutual != 0) {
      // Add the mutual inductance to each inductor respectively
      ind1.add_mutualInductance(mutual, iObj.argAnal, iObj.transSim.tstep(),
                                ind2.indexInfo.currentIndex_.value());
      ind2.add_mutualInductance(mutual, iObj.argAnal, iObj.transSim.tstep(),
                                ind1.indexInfo.currentIndex_.value());
      // Add this mutual inductance to each inductor's matrix stamp
      ind1.set_mutualInductance(std::make_pair(ind2Index.value(), mutual));
      ind2.set_mutualInductance(std::make_pair(ind1Index.value(), mutual));
    }
    ++cc;
  }
  // Led the user know the matrix creation is complete
  if (!iObj.argMin && components.mutualinductances.size() != 0) {
    bar2.complete();
    std::cout << "\n";
  }
}

void Matrix::reduce_step(Input& iObj) {
  iObj.transSim.tstep(iObj.transSim.tstep() / 2);
  nodeConfig.clear();
  nodeConfig2.clear();
  Components newComponents;
  components = newComponents;
  nm.clear();
  nodeconnections newNC;
  nc = newNC;
  lm.clear();
  branchIndex = 0;
  nz.clear();
  ci.clear();
  rp.clear();
  relevantIndices.clear();
  relevantTraces.clear();
}

void Matrix::create_csr() {
  // Create the non zero vector
  create_nz();
  // Create the column index vector
  create_ci();
  // Create the row pointer vector
  create_rp();
}

void Matrix::create_nz() {
  // Non zero vector should be empty, but clear it to make sure
  nz.clear();
  // Loop through all the node connections
  for (const auto& it : nc) {
    // Loop through each connection
    for (const auto& ti : it) {
      // Store the non zero values of the connection
      nz.emplace_back(ti.first);
    }
  }
  // Loop through all the devices
  for (auto& i : components.devices) {
    // Get each device's non zero vector
    const auto& nonZeros = std::visit(
        [](const auto& device) noexcept -> const std::vector<double>& {
          return device.matrixInfo.nonZeros_;
        },
        i);
    // Insert the non zero vector of the device into the greater vector
    nz.insert(nz.end(), nonZeros.begin(), nonZeros.end());
  }
}

void Matrix::create_ci() {
  // Clear the column index vector as safety precaution
  ci.clear();
  // Loop through all the node connections
  for (const auto& it : nc) {
    // Loop through each connection
    for (const auto& ti : it) {
      // Add the column index for the connection to the column index vector
      ci.emplace_back(ti.second);
    }
  }
  int sum = 0;
  for (auto& i : ci) {
    sum += i;
  }
  // Loop through all the devices
  for (auto& i : components.devices) {
    // Get each device's column index vector
    const auto& columnIndices = std::visit(
        [](const auto& device) noexcept -> const std::vector<int64_t>& {
          return device.matrixInfo.columnIndex_;
        },
        i);
    // Insert the column index vector of the device into the greater vector
    ci.insert(ci.end(), columnIndices.begin(), columnIndices.end());
  }
}

void Matrix::create_rp() {
  // Clear the row pointer vector
  rp.clear();
  // First entry should always be zero
  rp.emplace_back(0);
  // Add the number of node connections in each row to the row pointer vector
  for (const auto& it : nc) {
    rp.emplace_back(rp.back() + it.size());
  }
  // Loop through all the devices
  for (auto& i : components.devices) {
    // Get the row pointer vector for each device
    const auto& rowPointer = std::visit(
        [](const auto& device) noexcept -> const std::vector<int64_t>& {
          return device.matrixInfo.rowPointer_;
        },
        i);
    // For each entry in the row pointer vector of the device
    for (const auto& ti : rowPointer) {
      // Add it to the last entry of the greater row pointer vector
      rp.emplace_back(rp.back() + ti);
    }
  }
}