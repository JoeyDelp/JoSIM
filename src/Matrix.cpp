// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/Matrix.hpp"
#include "JoSIM/Misc.hpp"
#include "JoSIM/Constants.hpp"
#include "JoSIM/Errors.hpp"

#include <algorithm>
#include <string>
#include <iostream>
#include <cmath>

using namespace JoSIM;

void JoSIM::Matrix::create_matrix(JoSIM::Input &iObj)
{
  int nodeCounter = 0;
  for (const auto &i : iObj.netlist.expNetlist) {
    if(i.first.at(0) != 'K' && i.first.at(0) != 'T') {
      std::vector<std::string> tokens = Misc::tokenize_space(i.first);
      // Ensure the device has at least 4 parts: LABEL PNODE NNODE VALUE
      if(tokens.size() < 4) {
        Errors::invalid_component_errors(ComponentErrors::INVALID_COMPONENT_DECLARATION, i.first);
      }
      if(tokens.at(1).find("GND") != std::string::npos || tokens.at(1) != "0") {
        if(nm.count(tokens.at(1)) == 0) nm[tokens.at(1)] = nodeCounter++;
      }
      if(tokens.at(2).find("GND") != std::string::npos || tokens.at(2) != "0") {
        if(nm.count(tokens.at(2)) == 0) nm[tokens.at(2)] = nodeCounter++;
      }
    } else if (i.first.at(0) == 'T') {
      std::vector<std::string> tokens = Misc::tokenize_space(i.first);
      // Ensure the device has at least 4 parts: LABEL PNODE NNODE VALUE
      if(tokens.size() < 6) {
        Errors::invalid_component_errors(ComponentErrors::INVALID_COMPONENT_DECLARATION, i.first);
      }
      if(tokens.at(1).find("GND") != std::string::npos || tokens.at(1) != "0") {
        if(nm.count(tokens.at(1)) == 0) nm[tokens.at(1)] = nodeCounter++;
      }
      if(tokens.at(2).find("GND") != std::string::npos || tokens.at(2) != "0") {
        if(nm.count(tokens.at(2)) == 0) nm[tokens.at(2)] = nodeCounter++;
      }
      if(tokens.at(3).find("GND") != std::string::npos || tokens.at(3) != "0") {
        if(nm.count(tokens.at(3)) == 0) nm[tokens.at(3)] = nodeCounter++;
      }
      if(tokens.at(4).find("GND") != std::string::npos || tokens.at(4) != "0") {
        if(nm.count(tokens.at(4)) == 0) nm[tokens.at(4)] = nodeCounter++;
      }
    }
  }

  nc.resize(nm.size());
  branchIndex = nm.size();

  int fqtr, sqtr, tqtr;
  fqtr = iObj.netlist.expNetlist.size()/4;
  sqtr = iObj.netlist.expNetlist.size()/2;
  tqtr = iObj.netlist.expNetlist.size()/4 * 3;

  std::cout << "Matrix Creation Progress:" << std::endl;
  std::cout << "0%\r" << std::flush;
  int creationCounter = 0;
  for (const auto &i : iObj.netlist.expNetlist) {
    if(creationCounter == fqtr) std::cout << "25%\r" << std::flush;
    if(creationCounter == sqtr) std::cout << "50%\r" << std::flush;
    if(creationCounter == tqtr) std::cout << "75%\r" << std::flush;
    switch(i.first.at(0)){
      case 'R':
        components.devices.emplace_back(Resistor::create_resistor(i, 
            nm, lm, nc, iObj.parameters, 
            iObj.argAnal, 
            iObj.transSim.get_prstep(), branchIndex));
        components.resistorIndices.emplace_back(components.devices.size() - 1);
        break;
      case 'L':
        components.devices.emplace_back(Inductor::create_inductor(i, 
            nm, lm, nc, iObj.parameters, 
            iObj.argAnal, 
            iObj.transSim.get_prstep(), branchIndex));
        components.inductorIndices.emplace_back(components.devices.size() - 1);
        break;
      case 'C':
        components.devices.emplace_back(Capacitor::create_capacitor(i, 
            nm, lm, nc, iObj.parameters, 
            iObj.argAnal, 
            iObj.transSim.get_prstep(), branchIndex));
        components.capacitorIndices.emplace_back(components.devices.size() - 1);
        break;
      case 'B':
        components.devices.emplace_back(JJ::create_jj(i, 
            nm, lm, nc, iObj.parameters, iObj.netlist.models_new, 
            iObj.argAnal, 
            iObj.transSim.get_prstep(), branchIndex));
        components.junctionIndices.emplace_back(components.devices.size() - 1);
        break;
      case 'V':
        components.devices.emplace_back(VoltageSource::create_voltagesource(i, 
            nm, lm, nc, branchIndex));
        sources.emplace_back(Misc::parse_function(i.first, iObj, i.second));
        std::get<VoltageSource>(components.devices.back()).set_sourceIndex(sources.size() - 1);
        components.vsIndices.emplace_back(components.devices.size() - 1);
        break;
      case 'P':
        components.devices.emplace_back(PhaseSource::create_phasesource(i, 
            nm, lm, nc, branchIndex));
        sources.emplace_back(Misc::parse_function(i.first, iObj, i.second));
        std::get<PhaseSource>(components.devices.back()).set_sourceIndex(sources.size() - 1);
        components.psIndices.emplace_back(components.devices.size() - 1);
        break;
      case 'I':
        components.currentsources.emplace_back(CurrentSource::create_currentsource(i, nm, lm));
        sources.emplace_back(Misc::parse_function(i.first, iObj, i.second));
        components.currentsources.back().set_sourceIndex(sources.size() - 1);
        break;
      case 'T':
        components.devices.emplace_back(TransmissionLine::create_transmissionline(i, 
            nm, lm, nc, 
            iObj.parameters, iObj.argAnal, 
            iObj.transSim.get_prstep(), branchIndex));
        components.txIndices.emplace_back(components.devices.size() - 1);        
        break;
      case 'K':
        components.mutualinductances.emplace_back(i);
        break;
      case 'G':
        components.devices.emplace_back(VCCS::create_VCCS(i, 
            nm, lm, nc, 
            iObj.parameters, branchIndex));
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
            iObj.parameters, branchIndex));
        components.ccvsIndices.emplace_back(components.devices.size() - 1);        
        break;
    }
    creationCounter++;
  }

  for (const auto &s : components.mutualinductances) {
    std::vector<std::string> tokens = Misc::tokenize_space(s.first);
    if(tokens.size() < 4) {
      Errors::invalid_component_errors(ComponentErrors::MUT_ERROR, s.first);
    }

    std::optional<int> ind1Index, ind2Index;
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

    double cf = JoSIM::Parameters::parse_param(tokens.at(3), iObj.parameters, s.second);
    double mutual = cf * std::sqrt(ind1.get_inductance() * ind2.get_inductance());

    
    ind1.add_mutualInductance(mutual, iObj.argAnal, iObj.transSim.get_prstep(), ind2.get_currentIndex());
    ind2.add_mutualInductance(mutual, iObj.argAnal, iObj.transSim.get_prstep(), ind1.get_currentIndex());

    ind1.set_mutualInductance(std::make_pair(ind2Index.value(), mutual));
    ind2.set_mutualInductance(std::make_pair(ind1Index.value(), mutual));
  }

  create_csr();

  std::cout << "100%" << std::endl;
  std::cout << "\n";

}

void JoSIM::Matrix::create_csr() {
  create_nz();
  create_ci();
  create_rp();
}

void JoSIM::Matrix::create_nz() {
  nz.clear();

  for(const auto &it : nc) {
    for (const auto &ti : it) {
      nz.emplace_back(ti.first);
    }
  }

  for (auto &i : components.devices) {
    const auto& nonZeros = std::visit([](const auto& device) noexcept -> const std::vector<double>& {
          return device.get_nonZeros();
        }, i);

    nz.insert(nz.end(), nonZeros.begin(), nonZeros.end());
  }
}

void JoSIM::Matrix::create_ci() {
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

void JoSIM::Matrix::create_rp() {
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