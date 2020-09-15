// Copyright (c) 2020 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/Output.hpp"
#include "JoSIM/AnalysisType.hpp"
#include "JoSIM/Input.hpp"
#include "JoSIM/Simulation.hpp"
#include "JoSIM/Constants.hpp"
#include "JoSIM/Errors.hpp"

#include <algorithm>
#include <cassert>
#include <functional>
#include <iterator>
#include <fstream>
#include <iostream>
#include <filesystem>

using namespace JoSIM;

void Output::write_output(
  const Input &iObj, Matrix &mObj, Simulation &sObj) {
  traces.emplace_back("time");
  traces.back().data_ = sObj.results.timeAxis;
  traces.back().type_ = 'T';
  if(mObj.relevantTraces.size() != 0){
    for (const auto &i : mObj.relevantTraces) {
      if(i.storageType == StorageType::Voltage) {
        traces.emplace_back(i.deviceLabel.value());
        double voltN1 = 0;
        for(int j = 0; j < sObj.results.timeAxis.size(); ++j) {
          double value;
          if(iObj.argAnal == AnalysisType::Voltage) {
            if(!i.index2) {
              value = sObj.results.xVector.at(i.index1.value()).value().at(j);
            } else if (!i.index1) {
              value = -sObj.results.xVector.at(i.index2.value()).value().at(j);
            } else {
              value = sObj.results.xVector.at(i.index1.value()).value().at(j) 
                      - sObj.results.xVector.at(i.index2.value()).value().at(j);
            }
          } else {
            if(i.deviceLabel.value().at(3) == 'B') {
              value = sObj.results.xVector.at(i.variableIndex.value()).value().at(j);
            } else {
              if(!i.index2) {
                if (j == 0) {
                  value = ((Constants::SIGMA) / iObj.transSim.get_prstep()) 
                          * sObj.results.xVector.at(i.index1.value()).value().at(j);
                  voltN1 = 0.0;
                } else if (j == 1) {
                  value = ((Constants::SIGMA) / iObj.transSim.get_prstep())  
                          * (sObj.results.xVector.at(i.index1.value()).value().at(j)
                          -  2 * sObj.results.xVector.at(i.index1.value()).value().at(j - 1)) + voltN1;
                  voltN1 = 0.0;
                } else {
                  value = ((Constants::SIGMA) / iObj.transSim.get_prstep())  
                          * (sObj.results.xVector.at(i.index1.value()).value().at(j)
                          -  2 * sObj.results.xVector.at(i.index1.value()).value().at(j - 1)
                          + sObj.results.xVector.at(i.index1.value()).value().at(j - 2)) + voltN1;
                  voltN1 = value;
                }
              } else if (!i.index1) {
                if (j == 0) {
                  value = ((Constants::SIGMA) / iObj.transSim.get_prstep()) 
                          * (-sObj.results.xVector.at(i.index2.value()).value().at(j));
                  voltN1 = 0.0;
                } else if (j == 1) {
                  value = ((Constants::SIGMA) / iObj.transSim.get_prstep())  
                          * ((-sObj.results.xVector.at(i.index2.value()).value().at(j))
                          -  2 * (-sObj.results.xVector.at(i.index2.value()).value().at(j - 1))) + voltN1;
                  voltN1 = 0.0;
                } else {
                  value = ((Constants::SIGMA) / iObj.transSim.get_prstep())  
                          * ((-sObj.results.xVector.at(i.index2.value()).value().at(j))
                          -  2 * (-sObj.results.xVector.at(i.index2.value()).value().at(j - 1))
                          + (-sObj.results.xVector.at(i.index2.value()).value().at(j - 2))) + voltN1;
                  voltN1 = value;
                }
              } else {
                if (j == 0) {
                  value = ((Constants::SIGMA) / iObj.transSim.get_prstep())  
                          * (sObj.results.xVector.at(i.index1.value()).value().at(j)
                          - sObj.results.xVector.at(i.index2.value()).value().at(j));
                  voltN1 = 0.0;
                } else if (j == 1) {
                  value = ((Constants::SIGMA) / iObj.transSim.get_prstep())  
                          * ((-sObj.results.xVector.at(i.index2.value()).value().at(j))
                          -  2 * (-sObj.results.xVector.at(i.index2.value()).value().at(j - 1))) + voltN1;
                  voltN1 = 0.0;
                } else {
                  value = ((Constants::SIGMA) / iObj.transSim.get_prstep())  
                          * ((sObj.results.xVector.at(i.index1.value()).value().at(j) 
                          - sObj.results.xVector.at(i.index2.value()).value().at(j))
                          - 2 * (sObj.results.xVector.at(i.index1.value()).value().at(j - 1) 
                          - sObj.results.xVector.at(i.index2.value()).value().at(j - 1))
                          + (sObj.results.xVector.at(i.index1.value()).value().at(j - 2) 
                          - sObj.results.xVector.at(i.index2.value()).value().at(j - 2))) + voltN1;
                  voltN1 = value;
                }
              }
            }
          }
          traces.back().data_.emplace_back(value);
          traces.back().type_ = 'V';
        }
      } else if(i.storageType == StorageType::Phase) {
        traces.emplace_back(i.deviceLabel.value());
        double phaseN1, phaseN2;
        phaseN1 = phaseN2 = 0.0;
        for(int j = 0; j < sObj.results.timeAxis.size(); ++j) {
          double value;
          if(iObj.argAnal == AnalysisType::Phase) {
            if(!i.index2) {
              value = sObj.results.xVector.at(i.index1.value()).value().at(j);
            } else if (!i.index1) {
              value = -sObj.results.xVector.at(i.index2.value()).value().at(j);
            } else {
              value = sObj.results.xVector.at(i.index1.value()).value().at(j) 
                      - sObj.results.xVector.at(i.index2.value()).value().at(j);
            }
          } else {
            if(i.deviceLabel.value().at(3) == 'B') {
              value = sObj.results.xVector.at(i.variableIndex.value()).value().at(j);
            } else {
              if(!i.index2) {
                if (j == 0) {
                  value = (iObj.transSim.get_prstep() / (2 * Constants::SIGMA)) 
                          * sObj.results.xVector.at(i.index1.value()).value().at(j);
                  phaseN1 = value;
                } else if (j == 1) {
                  value = (iObj.transSim.get_prstep() / (2 * Constants::SIGMA))  
                          * (sObj.results.xVector.at(i.index1.value()).value().at(j)) + 2 * phaseN1 - phaseN2;
                  phaseN2 = phaseN1;
                  phaseN1 = value;
                } else {
                  value = (iObj.transSim.get_prstep() / (2 * Constants::SIGMA))  
                          * (sObj.results.xVector.at(i.index1.value()).value().at(j)
                          - sObj.results.xVector.at(i.index1.value()).value().at(j - 2)) + 2 * phaseN1 - phaseN2;
                  phaseN2 = phaseN1;
                  phaseN1 = value;
                }
              } else if (!i.index1) {
                if (j == 0) {
                  value = (iObj.transSim.get_prstep() / (2 * Constants::SIGMA)) 
                          * (-sObj.results.xVector.at(i.index2.value()).value().at(j));
                  phaseN1 = value;
                } else if (j == 1) {
                  value = (iObj.transSim.get_prstep() / (2 * Constants::SIGMA))  
                          * (-sObj.results.xVector.at(i.index2.value()).value().at(j)) + 2 * phaseN1 - phaseN2;
                  phaseN2 = phaseN1;
                  phaseN1 = value;
                } else {
                  value = (iObj.transSim.get_prstep() / (2 * Constants::SIGMA))  
                          * ((-sObj.results.xVector.at(i.index2.value()).value().at(j))
                          - (-sObj.results.xVector.at(i.index2.value()).value().at(j - 2))) + 2 * phaseN1 - phaseN2;
                  phaseN2 = phaseN1;
                  phaseN1 = value;
                }
              } else {
                if (j == 0) {
                  value = (iObj.transSim.get_prstep() / (2 * Constants::SIGMA)) 
                          * (sObj.results.xVector.at(i.index1.value()).value().at(j)
                          - sObj.results.xVector.at(i.index2.value()).value().at(j));
                  phaseN1 = value;
                } else if (j == 1) {
                  value = (iObj.transSim.get_prstep() / (2 * Constants::SIGMA))  
                          * (sObj.results.xVector.at(i.index1.value()).value().at(j)
                          - sObj.results.xVector.at(i.index2.value()).value().at(j)) + 2 * phaseN1 - phaseN2;
                  phaseN2 = phaseN1;
                  phaseN1 = value;
                } else {
                  value = (iObj.transSim.get_prstep() / (2 * Constants::SIGMA))  
                          * ((sObj.results.xVector.at(i.index1.value()).value().at(j)
                          - sObj.results.xVector.at(i.index2.value()).value().at(j))
                          - (sObj.results.xVector.at(i.index1.value()).value().at(j - 2)
                          -sObj.results.xVector.at(i.index2.value()).value().at(j - 2))) + 2 * phaseN1 - phaseN2;
                  phaseN2 = phaseN1;
                  phaseN1 = value;
                }
              }
            }
          }
          traces.back().data_.emplace_back(value);
          traces.back().type_ = 'P';
        }
      } else if(i.storageType == StorageType::Current) {
        traces.emplace_back(i.deviceLabel.value());
        if(i.deviceLabel.value().at(3) != 'I'){ 
          for(int j = 0; j < sObj.results.timeAxis.size(); ++j) {
            double value = sObj.results.xVector.at(i.index1.value()).value().at(j);
            traces.back().data_.emplace_back(value);
            traces.back().type_ = 'I';
          }
        } else {
          for(int j = 0; j < sObj.results.timeAxis.size(); ++j) {
            double value = mObj.sourcegen.at(i.sourceIndex.value()).value(
              sObj.results.timeAxis.at(j));
            traces.back().data_.emplace_back(value);
            traces.back().type_ = 'I';
          }
        }
      }
    }
  } else {
    for (const auto &i : mObj.nm) {
      if(iObj.argAnal == AnalysisType::Voltage) {
        traces.emplace_back("V(" + i.first + ")");
        traces.back().type_ = 'V';
      } else {
        traces.emplace_back("P(" + i.first + ")");
        traces.back().type_ = 'P';
      }
      for(int j = 0; j < sObj.results.timeAxis.size(); ++j) {
        traces.back().data_.emplace_back(sObj.results.xVector.at(i.second).value().at(j));
      }
    }
  }
}

void Output::format_csv_or_dat(
  const std::string &filename, const char &delimiter) {
  std::ofstream outfile(filename);
  outfile << std::setprecision(15);
  if (outfile.is_open()) {
    for (int i = 0; i < traces.size() - 1; ++i) {
      outfile << traces.at(i).name_ << delimiter;
    }
    outfile << traces.at(traces.size() - 1).name_ << "\n";
    for (int j = 0; j < traces.at(0).data_.size(); ++j) {
      for (int i = 0; i < traces.size() - 1; ++i) {
        outfile << std::scientific << std::setprecision(6) << 
          traces.at(i).data_.at(j) << delimiter;
      }
      outfile << std::scientific << std::setprecision(6) << 
        traces.at(traces.size() - 1).data_.at(j) << "\n";
    }
  } else {
    Errors::output_errors(OutputErrors::CANNOT_OPEN_FILE, filename);
  }
}

// Writes the output to a standard spice raw file
void Output::format_raw(const std::string &filename) {
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
      outfile << "No. Variables: " << traces.size() << "\n";
      loopsize = traces.at(0).data_.size();
      outfile << "No. Points: " << loopsize << "\n";
      outfile << "Command: JoSIM v" << VERSION << "\n";
      outfile << "Variables:\n";
      outfile << " 0 time Seconds\n";
      // Determine the variable name and type in a format acceptable for raw
      for (int i = 1; i < traces.size(); ++i) {
        // Make a copy of the name so we can alter it
        std::string name = traces.at(i).name_;
        // Erase all the double quote characters
        name.erase(std::remove(name.begin(), name.end(), '\"'), name.end());
        // If this is a voltage variable
        if (traces.at(i).type_ == 'V') {
          outfile << " " << i << " " << name << " Voltage\n";
        // If this is a phase variable
        } else if (traces.at(i).type_ == 'P') {
          outfile << " " << i << " " << name << " Phase\n";
        // If this is a current variable
        } else if (traces.at(i).type_ == 'I') {
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
      for (int i = 0; i < loopsize; ++i) {
        // Point and time value
        outfile << std::left << std::setw(pointSizeSpacing) << i << 
          traces.at(0).data_.at(i) << "\n";
        // Fill in rest of variable values
        for (int j = 1; j < traces.size(); ++j) {
          outfile << std::left << std::setw(pointSizeSpacing) << "" <<
            traces.at(j).data_.at(i) << "\n";
        }
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

void Output::format_cout(const bool &argMin) {
  if(!argMin) {
    for (int i = 0; i < traces.size() - 1; ++i) {
      std::cout << traces.at(i).name_ << " ";
    }
    std::cout << traces.at(traces.size() - 1).name_ << "\n";
    for (int j = 0; j < traces.at(0).data_.size(); ++j) {
      for (int i = 0; i < traces.size() - 1; ++i) {
        std::cout << std::setw(15) << std::scientific << std::setprecision(6) << 
          traces.at(i).data_.at(j) << " ";
      }
      std::cout << std::setw(15) << std::scientific << std::setprecision(6) <<
        traces.at(traces.size() - 1).data_.at(j) << "\n";
    }
  }
}