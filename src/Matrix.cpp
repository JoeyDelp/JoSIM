// Copyright (c) 2020 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/Matrix.hpp"
#include "JoSIM/Misc.hpp"
#include "JoSIM/Constants.hpp"
#include "JoSIM/Errors.hpp"
#include "JoSIM/Function.hpp"

#include <algorithm>
#include <string>
#include <iostream>
#include <cmath>
#include <thread>

using namespace JoSIM;

// Function that prints progress in a seperate thread
void Matrix::print_progess(const int &i, const int &size) {
  if(i == 0) {
    std::cout << "Matrix Creation Progress:" << std::endl;
    std::cout << "0%\r" << std::flush;
  } else {
    std::cout << std::ceil((double) i / (double) size * 100) 
      << "%\r" << std::flush;
  }
}

void Matrix::create_matrix(Input &iObj)
{
  // Create a seperate thread that will be used for printing creation progress
  std::thread printingThread;
  // Create a node counter variable
  int nodeCounter = 0;
  // Variables to store node configs since they are already identified here
  NodeConfig nodeConfig = NodeConfig::GND;
  // Optional, only available when 4 node components are detected
  std::optional<NodeConfig> nodeConfig2;
  for (auto &i : iObj.netlist.expNetlist) {
    // Expand all inline parameters
    expand_inline_parameters(i, iObj.parameters);
    // All devices should atleast have 4 parts
    if(i.first.size() < 4) {
      // Complain if it doesnt
      Errors::invalid_component_errors(
        ComponentErrors::INVALID_COMPONENT_DECLARATION, 
        Misc::vector_to_string(i.first));
    }
    // Create a node map that maps the node names to numbers
    if(i.first.at(1).find("GND") == std::string::npos && i.first.at(1) != "0") {
      // Add the first node to the map if not ground
      if(nm.count(i.first.at(1)) == 0) nm[i.first.at(1)] = nodeCounter++;
      nodeConfig = NodeConfig::POSGND;
    }
    if(i.first.at(2).find("GND") == std::string::npos && i.first.at(2) != "0") {
      // Add the second node to the map if not ground
      if(nm.count(i.first.at(2)) == 0) nm[i.first.at(2)] = nodeCounter++;
      if (nodeConfig == NodeConfig::POSGND) {
        nodeConfig = NodeConfig::POSNEG;
      } else {
        nodeConfig = NodeConfig::GNDNEG;
      }
    }
    // If the device is as 4 node device
    if(std::string("EFGHT").find(i.first.front().at(0)) != std::string::npos) {
      nodeConfig2 = NodeConfig::GND;
      // Ensure the device has at least 6 parts
      if(i.first.size() < 6) {
        // Complain if it isnt
        Errors::invalid_component_errors(
          ComponentErrors::INVALID_COMPONENT_DECLARATION, 
          Misc::vector_to_string(i.first));
      }
      if(i.first.at(3).find("GND") == std::string::npos && 
        i.first.at(3) != "0") {
        // Add the third node to the map if not ground
        if(nm.count(i.first.at(3)) == 0) nm[i.first.at(3)] = nodeCounter++;
        nodeConfig2 = NodeConfig::POSGND;
      }
      if(i.first.at(4).find("GND") == std::string::npos && 
        i.first.at(4) != "0") {
        // Add the fourth node to the map if not ground
        if(nm.count(i.first.at(4)) == 0) nm[i.first.at(4)] = nodeCounter++;
        if (nodeConfig2 == NodeConfig::POSGND) {
          nodeConfig2 = NodeConfig::POSNEG;
        } else {
          nodeConfig2 = NodeConfig::GNDNEG;
        }
      }
    }
  }
  // Resize the node connection vector to match the size of the node map
  nc.resize(nm.size());
  // Set the index to the first branch current to the size of the node map
  branchIndex = nm.size();
  // Counter for progress report
  int creationCounter = 0;
  // Loop through all the components in the netlist
  for (const auto &i : iObj.netlist.expNetlist) {
    // If not minimal printing
    if(!iObj.argMin) {
      // Report progress
      printingThread = std::thread(
        print_progess, creationCounter, iObj.netlist.expNetlist.size());
    }
    // First character of first token indicates device type
    switch(i.first.front().at(0)){
      // Inductors (most common so do them first)
      case 'L':
        // Create an inductor and add it to the component list
        components.devices.emplace_back(
          Inductor(i, nodeConfig, nm, lm, nc, iObj.parameters, iObj.argAnal,
          iObj.transSim.get_prstep(), branchIndex));
        // Store this inductor's component list index for reference
        components.inductorIndices.emplace_back(components.devices.size() - 1);
        break;
      // Josephson junction (JJ)
      case 'B':
        // Create a JJ and add it to the component list
        components.devices.emplace_back(JJ::create_jj(i, 
            nm, lm, nc, iObj.parameters, iObj.netlist.models_new, 
            iObj.argAnal, iObj.argInt,
            iObj.transSim.get_prstep(), branchIndex));
        // Store this JJ's component list index for reference    
        components.junctionIndices.emplace_back(components.devices.size() - 1);
        break;  
      // Resistors
      case 'R':
        // Create a resistor and add it to the component list
        components.devices.emplace_back(
          Resistor(i, nodeConfig, nm, lm, nc, iObj.parameters, iObj.argAnal,
            iObj.transSim.get_prstep(), branchIndex));
        // Store this resistor's component list index for reference
        components.resistorIndices.emplace_back(components.devices.size() - 1);
        break;
      // Capacitors
      case 'C':
        // Create a capacitor and add it to the component list
        components.devices.emplace_back(Capacitor::create_capacitor(i, 
            nm, lm, nc, iObj.parameters, 
            iObj.argAnal, iObj.argInt,
            iObj.transSim.get_prstep(), branchIndex));
        // Store this capacitor's component list index for reference
        components.capacitorIndices.emplace_back(components.devices.size() - 1);
        break;
      case 'V':
        if(iObj.argAnal == AnalysisType::Phase) {
          components.devices.emplace_back(PhaseSource::create_phasesource(i, 
            nm, lm, nc, branchIndex));
          sources.emplace_back(Function::parse_function(i.first, iObj, i.second));
          Function::voltage_to_phase(sources.back(), iObj);
          std::get<PhaseSource>(components.devices.back()).set_sourceIndex(sources.size() - 1);
          components.psIndices.emplace_back(components.devices.size() - 1);
          break;
        } else {
          components.devices.emplace_back(VoltageSource::create_voltagesource(i, 
            nm, lm, nc, branchIndex));
          sources.emplace_back(Function::parse_function(i.first, iObj, i.second));
          std::get<VoltageSource>(components.devices.back()).set_sourceIndex(sources.size() - 1);
          components.vsIndices.emplace_back(components.devices.size() - 1);
          break;
        }
      case 'P':
        if(iObj.argAnal == AnalysisType::Voltage) {
          components.devices.emplace_back(VoltageSource::create_voltagesource(i, 
            nm, lm, nc, branchIndex));
          sources.emplace_back(Function::parse_function(i.first, iObj, i.second));
          Function::phase_to_voltage(sources.back(), iObj);
          std::get<VoltageSource>(components.devices.back()).set_sourceIndex(sources.size() - 1);
          components.vsIndices.emplace_back(components.devices.size() - 1);
          break;
        } else {
          components.devices.emplace_back(PhaseSource::create_phasesource(i, 
            nm, lm, nc, branchIndex));
          sources.emplace_back(Function::parse_function(i.first, iObj, i.second));
          std::get<PhaseSource>(components.devices.back()).set_sourceIndex(sources.size() - 1);
          components.psIndices.emplace_back(components.devices.size() - 1);
          break;
        }
      case 'I':
        components.currentsources.emplace_back(CurrentSource::create_currentsource(i, nm, lm));
        sources.emplace_back(Function::parse_function(i.first, iObj, i.second));
        components.currentsources.back().set_sourceIndex(sources.size() - 1);
        break;
      case 'T':
        components.devices.emplace_back(TransmissionLine::create_transmissionline(i, 
            nm, lm, nc, 
            iObj.parameters, iObj.argAnal, iObj.argInt,
            iObj.transSim.get_prstep(), branchIndex));
        components.txIndices.emplace_back(components.devices.size() - 1);        
        break;
      case 'K':
        components.mutualinductances.emplace_back(i);
        break;
      case 'G':
        components.devices.emplace_back(VCCS::create_VCCS(i, 
            nm, lm, nc, 
            iObj.parameters, branchIndex, iObj));
        components.vccsIndices.emplace_back(components.devices.size() - 1);        
        break;
      case 'F':
        components.devices.emplace_back(CCCS::create_CCCS(i, 
            nm, lm, nc, 
            iObj.parameters, branchIndex));
        components.cccsIndices.emplace_back(components.devices.size() - 1);        
        break;
      case 'E':
        components.devices.emplace_back(VCVS::create_VCVS(i, 
            nm, lm, nc, 
            iObj.parameters, branchIndex));
        components.vcvsIndices.emplace_back(components.devices.size() - 1);        
        break;
      case 'H':
        components.devices.emplace_back(CCVS::create_CCVS(i, 
            nm, lm, nc, 
            iObj.parameters, branchIndex, iObj));
        components.ccvsIndices.emplace_back(components.devices.size() - 1);        
        break;
    }
    creationCounter++;
  }
  printingThread.join();

  for (const auto &s : components.mutualinductances) {
    std::vector<std::string> tokens = Misc::tokenize(s.first);
    if(tokens.size() < 4) {
      Errors::invalid_component_errors(ComponentErrors::MUT_ERROR, s.first);
    }

    int_o ind1Index, ind2Index;
    for (int i = 0; i < components.devices.size(); ++i) {
      const auto& label = std::visit([](const auto& device) noexcept -> const std::string& {
        return device.get_label();
      }, components.devices.at(i));
      if(label == tokens.at(1)) {
        ind1Index = i;
      }
    }
    if(!ind1Index) {
      Errors::invalid_component_errors(ComponentErrors::MISSING_INDUCTOR, tokens.at(1));
    }

    for (int i = 0; i < components.devices.size(); ++i) {
      const auto& label = std::visit([](const auto& device) noexcept -> const std::string& {
        return device.get_label();
      }, components.devices.at(i));
      if(label == tokens.at(2)) {
        ind2Index = i;
      }
    }
    if(!ind2Index) {
      Errors::invalid_component_errors(ComponentErrors::MISSING_INDUCTOR, tokens.at(2));
    }

    auto &ind1 = std::get<Inductor>(components.devices.at(ind1Index.value()));
    auto &ind2 = std::get<Inductor>(components.devices.at(ind2Index.value()));

    double cf = parse_param(tokens.at(3), iObj.parameters, s.second);
    double mutual = cf * std::sqrt(ind1.get_inductance() * ind2.get_inductance());

    
    ind1.add_mutualInductance(mutual, iObj.argAnal, iObj.argInt, iObj.transSim.get_prstep(), ind2.get_currentIndex());
    ind2.add_mutualInductance(mutual, iObj.argAnal, iObj.argInt, iObj.transSim.get_prstep(), ind1.get_currentIndex());

    ind1.set_mutualInductance(std::make_pair(ind2Index.value(), mutual));
    ind2.set_mutualInductance(std::make_pair(ind1Index.value(), mutual));
  }

  create_csr();
  if(!iObj.argMin) {
    std::cout << "100%" << std::endl;
    std::cout << "\n";
  }

}

void Matrix::create_csr() {
  create_nz();
  create_ci();
  create_rp();
}

void Matrix::create_nz() {
  nz.clear();
  nz05.clear();
  nz2.clear();
  nz4.clear();
  nz8.clear();
  nz16.clear();

  for(const auto &it : nc) {
    for (const auto &ti : it) {
      nz.emplace_back(ti.first);
      nz05.emplace_back(ti.first);
      nz2.emplace_back(ti.first);
      nz4.emplace_back(ti.first);
      nz8.emplace_back(ti.first);
      nz16.emplace_back(ti.first);
    }
  }

  for (auto &i : components.devices) {
    const auto& nonZeros = std::visit([](const auto& device) noexcept -> const std::vector<double>& {
          return device.get_nonZeros();
        }, i);

    nz.insert(nz.end(), nonZeros.begin(), nonZeros.end());
  }
}

void Matrix::create_ci() {
  ci.clear();

  for(const auto &it : nc) {
    for (const auto &ti : it) {
      ci.emplace_back(ti.second);
    }
  }

  for (auto &i : components.devices) {
    const auto& columnIndices = std::visit([](const auto& device) noexcept -> const std::vector<int>& {
          return device.get_columnIndex();
        }, i);

    ci.insert(ci.end(), columnIndices.begin(), columnIndices.end());
  }
}

void Matrix::create_rp() {
  rp.clear();

  rp.emplace_back(0);

  for (const auto &it : nc) {
    rp.emplace_back(rp.back() + it.size());
  }

  for (auto &i : components.devices) {
    const auto& rowPointer = std::visit([](const auto& device) noexcept -> const std::vector<int>& {
          return device.get_rowPointer();
        }, i);
    for (const auto &ti : rowPointer) {
      rp.emplace_back(rp.back() + ti);
    }
  }
}