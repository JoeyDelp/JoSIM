// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/Output.hpp"
#include "JoSIM/AnalysisType.hpp"
#include "JoSIM/FileOutputType.hpp"
#include "JoSIM/Input.hpp"
#include "JoSIM/Simulation.hpp"
#include "JoSIM/Constants.hpp"
#include "JoSIM/Errors.hpp"
#include "JoSIM/ProgressBar.hpp"

#include <algorithm>
#include <cassert>
#include <functional>
#include <iterator>
#include <fstream>
#include <iostream>
#include <filesystem>

using namespace JoSIM;

Output::Output(Input& iObj, Matrix& mObj, Simulation& sObj) {
  // Write the output in type agnostic format
  write_output(iObj, mObj, sObj);
  // Format the output into the relevant type
  if (iObj.cli_output_file) {
    if (iObj.cli_output_file.value().type() == FileOutputType::Csv) {
      format_csv_or_dat(
        iObj.cli_output_file.value().name(), ',', iObj.argMin);
    } else if (iObj.cli_output_file.value().type() == FileOutputType::Dat) {
      format_csv_or_dat(
        iObj.cli_output_file.value().name(), ' ', iObj.argMin);
    } else if (iObj.cli_output_file.value().type() == FileOutputType::Raw) {
      format_raw(
        iObj.cli_output_file.value().name(), iObj.argMin);
    }
  }
  if (!iObj.output_files.empty()) {
    for (int i = 0; i < iObj.output_files.size(); ++i) {
      if (iObj.output_files.at(i).type() == FileOutputType::Csv) {
        format_csv_or_dat(
          iObj.output_files.at(i).name(), ',', iObj.argMin, i);
      } else if (iObj.output_files.at(i).type() == FileOutputType::Dat) {
        format_csv_or_dat(
          iObj.output_files.at(i).name(), ' ', iObj.argMin, i);
      } else if (iObj.output_files.at(i).type() == FileOutputType::Raw) {
        format_raw(
          iObj.output_files.at(i).name(), iObj.argMin, i);
      }
    }
  }
  if (!iObj.cli_output_file && iObj.output_files.empty()) {
    format_cout(iObj.argMin);
  }
}

void Output::write_output(
  const Input& iObj, Matrix& mObj, Simulation& sObj) {
  // Shorhand for the results vector
  auto& x = sObj.results.xVector;
  // Add the time axis trace to the set of traces to print
  traces.emplace_back("time");
  traces.back().type_ = 'T';
  // Only the requested time pieces
  int printStartIndex = 0, cc = 0;
  double prevPoint = iObj.transSim.prstart() - iObj.transSim.prstep();
  for (auto& i : sObj.results.timeAxis) {
    if (i >= iObj.transSim.prstart()) {
      if (Misc::isclose(i, iObj.transSim.prstart(), 1E-14)) {
        printStartIndex = cc;
      }
      if (Misc::isclose(i, (prevPoint + iObj.transSim.prstep()), 1E-14)) {
        traces.back().data_.emplace_back(i);
        prevPoint += iObj.transSim.prstep();
      }
    }
    cc++;
  }
  // Check if there are any traces to print
  if (mObj.relevantTraces.size() != 0) {
    // Create progress bar
    ProgressBar bar;
    if (!iObj.argMin) {
      bar.create_thread();
      bar.set_bar_width(30);
      bar.fill_bar_progress_with("O");
      bar.fill_bar_remainder_with(" ");
      bar.set_status_text("Formatting Output");
      bar.set_total((float)mObj.relevantTraces.size());
    }
    // Reset progress counter
    cc = 0;
    for (const auto& i : mObj.relevantTraces) {
      auto& st = i.storageType;
      if (!iObj.argMin) {
        bar.update(cc);
      }
      // Shorthand for the optional value
      auto i1 = i.index1.has_value() ? i.index1.value() : -1;
      auto i2 = i.index2.has_value() ? i.index2.value() : -1;
      auto si = i.sourceIndex.has_value() ? i.sourceIndex.value() : -1;
      auto vi = i.variableIndex.has_value() ? i.variableIndex.value() : -1;
      // If this is a voltage we are storing
      if (st == StorageType::Voltage) {
        // Set the label for the plot
        traces.emplace_back(i.deviceLabel.value());
        traces.back().fileIndex = i.fIndex;
        // Temporary values used for lookback
        double valin1n1 = 0, valin1n2 = valin1n1;
        double valin2n1 = 0, valin2n2 = valin2n1;
        double voltN1 = 0;
        prevPoint = iObj.transSim.prstart() - iObj.transSim.prstep();
        // Add the values for each value on the time axis
        for (int j = printStartIndex; j < sObj.results.timeAxis.size(); ++j) {
          double value;
          // Shorthand values
          auto valin1 = i1 != -1 ? x.at(i1).value().at(j) : 0;
          auto valin2 = i2 != -1 ? x.at(i2).value().at(j) : 0;
          auto valvi = vi != -1 ? x.at(vi).value().at(j) : 0;
          // If the analysis method was voltage
          if (iObj.argAnal == AnalysisType::Voltage) {
            value = valin1 - valin2;
            // Else calculate the voltage from the phase value
          } else {
            if (i.deviceLabel.value().at(3) == 'B' && vi != -1) {
              value = valvi;
            } else {
              value = ((Constants::SIGMA) / iObj.transSim.tstep())
                * ((valin1 - valin2) - 2 * (valin1n1 - valin2n1)
                  + (valin1n2 - valin2n2)) + voltN1;
              valin1n2 = valin1n1;
              valin1n1 = valin1;
              valin2n2 = valin2n1;
              valin2n1 = valin2;
              if (j > 1) voltN1 = value;
            }
          }
          traces.back().type_ = 'V';
          if (Misc::isclose(sObj.results.timeAxis.at(j),
            (prevPoint + iObj.transSim.prstep()), 1E-14)) {
            traces.back().data_.emplace_back(value);
            prevPoint += iObj.transSim.prstep();
          }
        }
      } else if (st == StorageType::Phase) {
        // Set the label for the plot
        traces.emplace_back(i.deviceLabel.value());
        traces.back().fileIndex = i.fIndex;
        // Temporary values for lookback
        double valin1n1 = 0, valin1n2 = valin1n1;
        double valin2n1 = 0, valin2n2 = valin2n1;
        double phaseN1 = 0, phaseN2 = phaseN1;
        prevPoint = iObj.transSim.prstart() - iObj.transSim.prstep();
        // Add the values for each value on the time axis
        for (int j = printStartIndex; j < sObj.results.timeAxis.size(); ++j) {
          double value;
          // Shorthand values
          auto valin1 = i1 != -1 ? x.at(i1).value().at(j) : 0;
          auto valin2 = i2 != -1 ? x.at(i2).value().at(j) : 0;
          auto valvi = vi != -1 ? x.at(vi).value().at(j) : 0;
          // If the analysis type is phase
          if (iObj.argAnal == AnalysisType::Phase) {
            value = valin1 - valin2;
            // Else if the analysis type was voltage
          } else {
            if (i.deviceLabel.value().at(3) == 'B' && vi != -1) {
              value = valvi;
            } else {
              value = (iObj.transSim.tstep() / (2 * Constants::SIGMA))
                * ((valin1 - valin2) - (valin1n2 - valin2n2)) +
                2 * phaseN1 - phaseN2;
              valin1n2 = valin1n1;
              valin1n1 = valin1;
              valin2n2 = valin2n1;
              valin2n1 = valin2;
              phaseN2 = phaseN1;
              phaseN1 = value;
            }
          }
          traces.back().type_ = 'P';
          if (Misc::isclose(sObj.results.timeAxis.at(j),
            (prevPoint + iObj.transSim.prstep()), 1E-14)) {
            traces.back().data_.emplace_back(value);
            prevPoint += iObj.transSim.prstep();
          }
        }
      } else if (st == StorageType::Current) {
        traces.emplace_back(i.deviceLabel.value());
        traces.back().fileIndex = i.fIndex;
        if (i.deviceLabel.value().at(3) != 'I') {
          prevPoint = iObj.transSim.prstart() - iObj.transSim.prstep();
          for (
            int j = printStartIndex; j < sObj.results.timeAxis.size(); ++j) {
            double value = x.at(i.index1.value()).value().at(j);
            traces.back().type_ = 'I';
            if (Misc::isclose(sObj.results.timeAxis.at(j),
              (prevPoint + iObj.transSim.prstep()), 1E-14)) {
              traces.back().data_.emplace_back(value);
              prevPoint += iObj.transSim.prstep();
            }
          }
        } else {
          prevPoint = iObj.transSim.prstart() - iObj.transSim.prstep();
          for (
            int j = printStartIndex; j < sObj.results.timeAxis.size(); ++j) {
            double value = mObj.sourcegen.at(i.sourceIndex.value()).value(
              sObj.results.timeAxis.at(j));
            traces.back().type_ = 'I';
            if (Misc::isclose(sObj.results.timeAxis.at(j),
              (prevPoint + iObj.transSim.prstep()), 1E-14)) {
              traces.back().data_.emplace_back(value);
              prevPoint += iObj.transSim.prstep();
            }
          }
        }
      }
      ++cc;
    }
    if (!iObj.argMin) {
      bar.complete();
      std::cout << "\n";
    }
  } else {
    ProgressBar bar;
    if (!iObj.argMin) {
      bar.create_thread();
      bar.set_bar_width(30);
      bar.fill_bar_progress_with("O");
      bar.fill_bar_remainder_with(" ");
      bar.set_status_text("Formatting Output");
      bar.set_total((float)mObj.nm.size());
    }
    int cc = 0;
    for (const auto& i : mObj.nm) {
      if (!iObj.argMin) {
        bar.update(cc);
      }
      if (iObj.argAnal == AnalysisType::Voltage) {
        traces.emplace_back("V(" + i.first + ")");
        traces.back().type_ = 'V';
      } else {
        traces.emplace_back("P(" + i.first + ")");
        traces.back().type_ = 'P';
      }
      for (int j = 0; j < sObj.results.timeAxis.size(); ++j) {
        traces.back().data_.emplace_back(x.at(i.second).value().at(j));
      }
      ++cc;
    }
    if (!iObj.argMin) {
      bar.update(100);
      bar.complete();
      std::cout << "\n";
    }
  }
}

void Output::format_csv_or_dat(
  const std::string& filename, const char& delimiter, bool argmin, 
  int fIndex) {
  std::vector<int> tIndices = { 0 };
  for (int i = 1; i < traces.size(); ++i) {
    if (traces.at(i).fileIndex == fIndex || fIndex == -1) {
      tIndices.emplace_back(i);
    }
  }
  std::ofstream outfile(filename);
  outfile << std::setprecision(15);
  if (outfile.is_open()) {
    for (int i = 0; i < tIndices.size() - 1; ++i) {
        outfile << traces.at(tIndices.at(i)).name_ << delimiter;
    }
    outfile << traces.at(tIndices.at(tIndices.size() - 1)).name_ << "\n";
    ProgressBar bar;
    if (!argmin) {
      bar.create_thread();
      bar.set_bar_width(30);
      bar.fill_bar_progress_with("O");
      bar.fill_bar_remainder_with(" ");
      bar.set_status_text("Writing Output");
      bar.set_total((float)traces.at(0).data_.size());
    }
    for (int j = 0; j < traces.at(0).data_.size(); ++j) {
      if (!argmin) {
        bar.update(j);
      }
      for (int i = 0; i < tIndices.size() - 1; ++i) {
        outfile << std::scientific << std::setprecision(6) <<
          traces.at(tIndices.at(i)).data_.at(j) << delimiter;
      }
      outfile << std::scientific << std::setprecision(6) <<
        traces.at(tIndices.at(tIndices.size() - 1)).data_.at(j) << "\n";
    }
    if (!argmin) {
      bar.complete();
      std::cout << "\n\n";
    }
  } else {
    Errors::output_errors(OutputErrors::CANNOT_OPEN_FILE, filename);
  }
}

// Writes the output to a standard spice raw file
void Output::format_raw(const std::string& filename, bool argmin, int fIndex) {
  std::vector<int> tIndices = { 0 };
  for (int i = 1; i < traces.size(); ++i) {
    if (traces.at(i).fileIndex == fIndex || fIndex == -1) {
      tIndices.emplace_back(i);
    }
  }
  // Variable to store the total number of points to be saved
  int loopsize = 0;
  // Opens an output stream with provided file name
  std::ofstream outfile(filename);
  // Set the output presicion
  outfile << std::setprecision(6);
  // Ensure that the file could be opened
  if (outfile.is_open()) {
    // If the traces are not empty
    if (!traces.empty()) {
      // Write the rawfile preamble
      outfile << "Title: " << std::filesystem::path(filename).stem() << "\n";
      std::time_t result = std::time(nullptr);
      outfile << "Date: " << std::asctime(std::localtime(&result));
      outfile << "Plotname: Transient Analysis\n";
      outfile << "Flags: real\n";
      outfile << "No. Variables: " << tIndices.size() << "\n";
      loopsize = traces.at(0).data_.size();
      outfile << "No. Points: " << loopsize << "\n";
      outfile << "Command: JoSIM v" << VERSION << "\n";
      outfile << "Variables:\n";
      outfile << " 0 time Seconds\n";
      // Determine the variable name and type in a format acceptable for raw
      for (int i = 1; i < tIndices.size(); ++i) {
        // Make a copy of the name so we can alter it
        std::string name = traces.at(tIndices.at(i)).name_;
        // Erase all the double quote characters
        name.erase(std::remove(name.begin(), name.end(), '\"'), name.end());
        // If this is a voltage variable
        if (traces.at(tIndices.at(i)).type_ == 'V') {
          outfile << " " << i << " " << name << " Voltage\n";
          // If this is a phase variable
        } else if (traces.at(tIndices.at(i)).type_ == 'P') {
          outfile << " " << i << " " << name << " Phase\n";
          // If this is a current variable
        } else if (traces.at(tIndices.at(i)).type_ == 'I') {
          std::replace(name.begin(), name.end(), '|', '.');
          name = name.substr(2);
          name = name.substr(0, name.size() - 1);
          // Append '#branch' since currents always flow in branches
          outfile << " " << i << " " << name << "#branch Current\n";
        }
      }
      // Start filling the values
      outfile << "Values:\n";
      int pointSizeSpacing = std::to_string(loopsize).length() + 1;
      ProgressBar bar;
      if (!argmin) {
        bar.create_thread();
        bar.set_bar_width(30);
        bar.fill_bar_progress_with("O");
        bar.fill_bar_remainder_with(" ");
        bar.set_status_text("Writing Output");
        bar.set_total((float)loopsize);
      }
      for (int i = 0; i < loopsize; ++i) {
        if (!argmin) {
          bar.update(i);
        }
        // Point and time value
        outfile << std::left << std::setw(pointSizeSpacing) << i <<
          traces.at(0).data_.at(i) << "\n";
        // Fill in rest of variable values
        for (int j = 1; j < tIndices.size(); ++j) {
          outfile << std::left << std::setw(pointSizeSpacing) << "" <<
            traces.at(tIndices.at(j)).data_.at(i) << "\n";
        }
      }
      if (!argmin) {
        bar.complete();
        std::cout << "\n\n";
      }
      // If the traces were empty (aka nothing to plot)
    } else if (traces.empty()) {
      // Complain about it
      Errors::output_errors(OutputErrors::NOTHING_SPECIFIED);
    }
    outfile.close();
    // If the file could not be opened for writing
  } else {
    // Complain about it
    Errors::output_errors(OutputErrors::CANNOT_OPEN_FILE, filename);
  }
}

void Output::format_cout(const bool& argMin) {
  if (!argMin) {
    for (int i = 0; i < traces.size() - 1; ++i) {
      std::cout << traces.at(i).name_ << " ";
    }
    std::cout << traces.at(traces.size() - 1).name_ << "\n";
    for (int j = 0; j < traces.at(0).data_.size(); ++j) {
      for (int i = 0; i < traces.size() - 1; ++i) {
        std::cout << std::setw(15) << std::scientific << std::setprecision(6)
          << traces.at(i).data_.at(j) << " ";
      }
      std::cout << std::setw(15) << std::scientific << std::setprecision(6) <<
        traces.at(traces.size() - 1).data_.at(j) << "\n";
    }
  }
}