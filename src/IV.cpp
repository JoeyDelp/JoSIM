// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/IV.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>

#include "JoSIM/Errors.hpp"
#include "JoSIM/Misc.hpp"
#include "JoSIM/Parameters.hpp"

using namespace JoSIM;

IV::IV(const Input iObj) {
  for (auto i : iObj.controls) {
    if (i.front() == "IV" || i.front() == ".IV") {
      setup_iv(i, iObj);
    }
  }
}

void IV::setup_iv(const tokens_t& i, const Input& iObj) {
  if (i.size() < 4) {
    Errors::control_errors(ControlErrors::INVALID_IV_COMMAND,
                           Misc::vector_to_string(i));
  }
  // Model can be part of a subcircuit so find subcircuit and model name
  tokens_t t = Misc::tokenize(i.at(1), ".|");
  auto& model = t.at(0);
  string_o subc;
  if (t.size() > 1) {
    subc = t.at(1);
  }
  // Check to see if model exists
  if (iObj.netlist.models.count(std::make_pair(model, subc)) == 0) {
    Errors::control_errors(ControlErrors::IV_MODEL_NOT_FOUND,
                           model + ((subc) ? "|" + subc.value() : ""));
  }
  // Determine the maximum current to which to draw the IV curve
  double maxC = parse_param(i.at(2), iObj.parameters);
  // Create an input object for this simulation
  Input ivInp = iObj;
  ivInp.controls.clear();
  tokens_t jj = {"B01", "1", "0", model, "AREA=1"};
  tokens_t ib = {"IB01", "0", "1", "PWL(0", "0", "10P", "0", "50P", "2.5U)"};
  ivInp.netlist.expNetlist = {std::make_pair(jj, subc),
                              std::make_pair(ib, std::nullopt)};
  ivInp.transSim.tstep(0.05E-12);
  ivInp.transSim.tstop(500E-12);
  ivInp.argAnal = JoSIM::AnalysisType::Voltage;
  ivInp.argMin = true;
  std::vector<std::pair<double, double>> iv_data;
  iv_data = generate_iv(maxC, ivInp);
  // Sanity check, if parent path of output file is empty then change path to
  // input file path, otherwise file is written in executable location
  auto path = std::filesystem::path(i.at(3));
  if (!path.has_parent_path() && iObj.fileParentPath) {
    path = std::filesystem::path(iObj.fileParentPath.value()).append(i.at(3));
  }
  write_iv(iv_data, path.string());
}

std::vector<std::pair<double, double>> IV::generate_iv(double maxC,
                                                       Input ivInp) {
  std::vector<std::pair<double, double>> iv_data;
  double currentIncrement = 25E-7;
  double currentCurr = 0.0;
  Matrix ivMat;
  ivMat.create_matrix(ivInp);
  while (currentCurr <= maxC) {
    // Create a matrix object using the input object
    currentCurr += currentIncrement;
    ivMat.sourcegen.back().ampValues({0, 0, currentCurr});
    iv_data.emplace_back(do_simulate(ivInp, ivMat));
    ivInp.transSim.tstep(0.05E-12);
  }
  while (currentCurr >= 0) {
    // Create a matrix object using the input object
    currentCurr -= currentIncrement;
    ivMat.sourcegen.back().ampValues({0, maxC, currentCurr});
    iv_data.emplace_back(do_simulate(ivInp, ivMat));
    ivInp.transSim.tstep(0.05E-12);
  }
  while (currentCurr >= -maxC) {
    // Create a matrix object using the input object
    currentCurr -= currentIncrement;
    ivMat.sourcegen.back().ampValues({0, 0, currentCurr});
    iv_data.emplace_back(do_simulate(ivInp, ivMat));
    ivInp.transSim.tstep(0.05E-12);
  }
  while (currentCurr <= 0) {
    // Create a matrix object using the input object
    currentCurr += currentIncrement;
    ivMat.sourcegen.back().ampValues({0, -maxC, currentCurr});
    iv_data.emplace_back(do_simulate(ivInp, ivMat));
    ivInp.transSim.tstep(0.05E-12);
  }
  return iv_data;
}

std::pair<double, double> IV::do_simulate(Input& ivInp, Matrix& ivMat) {
  // Simulate the iv curve
  Simulation ivSim(ivInp, ivMat);
  // Add the results to the stack
  double& current = ivSim.results.xVector.back().value().back();
  auto& voltVect = ivSim.results.xVector.front().value();
  double voltage = 0;
  for (int64_t i = voltVect.size() / 2; i < voltVect.size(); ++i) {
    voltage += voltVect.at(i);
  }
  voltage = voltage / (voltVect.size() / 2);
  return std::make_pair(voltage, current);
}

void IV::write_iv(std::vector<std::pair<double, double>>& iv_data,
                  const std::string& output_path) {
  auto path = std::filesystem::path(output_path);
  std::ofstream outfile(path.string());
  outfile << std::setprecision(6);
  outfile << std::scientific;
  if (outfile.is_open()) {
    outfile << "V,I\n";
    for (auto i : iv_data) {
      outfile << i.first << "," << i.second << "\n";
    }
    outfile.close();
  } else {
    Errors::output_errors(OutputErrors::CANNOT_OPEN_FILE, path.string());
  }
}