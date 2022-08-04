// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/Verbose.hpp"

#include <iomanip>
#include <iostream>

#include "JoSIM/Misc.hpp"

using namespace JoSIM;

void Verbose::handle_verbosity(const int64_t& vl, const Input& iObj,
                               const Matrix& mObj) {
  switch (vl) {
    case 0:
      break;
    case 1:
      print_circuit_stats(iObj, mObj);
      break;
    case 2:
      print_parameters(iObj);
      print_circuit_stats(iObj, mObj);
      break;
    case 3:
      print_expanded_netlist(iObj);
      print_parameters(iObj);
      print_circuit_stats(iObj, mObj);
      break;
    default:
      Errors::verbosity_errors(VerbosityErrors::NO_SUCH_LEVEL,
                               std::to_string(vl));
      break;
  }
}

void Verbose::print_circuit_stats(const Input& iObj, const Matrix& mObj) {
  std::cout << "Printing circuit statistics:" << std::endl;
  // Print the total main design lines
  std::cout << std::left << std::setw(26)
            << "Main design lines:" << iObj.netlist.maindesign.size() << "\n";
  // Print the total number of subcircuits lines
  std::cout << std::left << std::setw(26)
            << "Subcircuit count:" << iObj.netlist.subcktTotal << "\n";
  // Print the total component count
  std::cout << std::left << std::setw(26)
            << "Component count:" << mObj.components.devices.size() << "\n";
  // Print the total non-zero count
  std::cout << std::left << std::setw(26) << "Non-zero count:" << mObj.nz.size()
            << "\n";
  // Print matrix dimensions
  std::cout << std::left << std::setw(26) << "MxN size:" << mObj.rp.size()
            << "x" << mObj.rp.size() << "\n";
  // Print the total jj count
  if (mObj.components.junctionIndices.size() != 0) {
    std::cout << std::left << std::setw(26)
              << "JJ count:" << mObj.components.junctionIndices.size() << "\n";
  }
  // Print the total inductor count
  if (mObj.components.inductorIndices.size() != 0) {
    std::cout << std::left << std::setw(26)
              << "Inductor count:" << mObj.components.inductorIndices.size()
              << "\n";
  }
  // Print the total Mutual inductance count
  if (mObj.components.mutualinductances.size() != 0) {
    std::cout << std::left << std::setw(26) << "Mutual inductance count:"
              << mObj.components.mutualinductances.size() << "\n";
  }
  // Print the total resistor count
  if (mObj.components.resistorIndices.size() != 0) {
    std::cout << std::left << std::setw(26)
              << "Resistor count:" << mObj.components.resistorIndices.size()
              << "\n";
  }
  // Print the total capacitor count
  if (mObj.components.capacitorIndices.size() != 0) {
    std::cout << std::left << std::setw(26)
              << "Capacitor count:" << mObj.components.capacitorIndices.size()
              << "\n";
  }
  // Print the total Current source count
  if (mObj.components.currentsources.size() != 0) {
    std::cout << std::left << std::setw(26) << "Current source count:"
              << mObj.components.currentsources.size() << "\n";
  }
  // Print the total Voltage source count
  if (mObj.components.vsIndices.size() != 0) {
    std::cout << std::left << std::setw(26)
              << "Voltage source count:" << mObj.components.vsIndices.size()
              << "\n";
  }
  // Print the total Phase source count
  if (mObj.components.psIndices.size() != 0) {
    std::cout << std::left << std::setw(26)
              << "Phase source count:" << mObj.components.psIndices.size()
              << "\n";
  }
  // Print the total TX line count
  if (mObj.components.txIndices.size() != 0) {
    std::cout << std::left << std::setw(26)
              << "TX line count:" << mObj.components.txIndices.size() << "\n";
  }
  // Print the total CCCS count
  if (mObj.components.cccsIndices.size() != 0) {
    std::cout << std::left << std::setw(26)
              << "CCCS count:" << mObj.components.cccsIndices.size() << "\n";
  }
  // Print the total CCVS count
  if (mObj.components.ccvsIndices.size() != 0) {
    std::cout << std::left << std::setw(26)
              << "CCVS count:" << mObj.components.ccvsIndices.size() << "\n";
  }
  // Print the total VCCS count
  if (mObj.components.vccsIndices.size() != 0) {
    std::cout << std::left << std::setw(26)
              << "VCCS count:" << mObj.components.vccsIndices.size() << "\n";
  }
  // Print the total VCVS count
  if (mObj.components.vcvsIndices.size() != 0) {
    std::cout << std::left << std::setw(26)
              << "VCVS count:" << mObj.components.vcvsIndices.size() << "\n";
  }
  std::cout << std::endl;
}

void Verbose::print_parameters(const Input& iObj) {
  if (iObj.parameters.size() != 0) {
    std::cout << "Printing parsed parameters" << std::endl;
    tokens_t sortedParams;
    for (auto i : iObj.parameters) {
      std::stringstream value;
      value << std::scientific << std::setprecision(6)
            << i.second.get_value().value();
      sortedParams.emplace_back(i.first.subcircuit().value_or("") + " : " +
                                i.first.name() + " = " + value.str());
    }
    std::sort(sortedParams.begin(), sortedParams.end());
    for (auto i : sortedParams) {
      std::cout << i << std::endl;
    }
    std::cout << std::endl;
  }
}

void Verbose::print_expanded_netlist(const Input& iObj) {
  std::cout << "Printing expanded netlist:" << std::endl;
  // Print the entire expanded netlist to standard output
  for (const auto& i : iObj.netlist.expNetlist) {
    std::cout << Misc::vector_to_string(i.first) << std::endl;
  }
  std::cout << std::endl;
}
