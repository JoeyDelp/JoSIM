// Copyright (c) 2019 Johannes Delport
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

std::vector<std::vector<std::string>> Output::write_output(const Input &iObj, const Matrix &mObj, const Simulation &sObj) {
  std::vector<std::vector<std::string>> unformattedOutput = {{"time"}};
  unformattedOutput.resize(sObj.results.timeAxis.size() + 1);
  std::ostringstream fstring;
  for (int i = 0; i < sObj.results.timeAxis.size(); ++i) {
    fstring.str("");
    fstring << std::fixed << std::scientific << std::setprecision(16) << sObj.results.timeAxis.at(i);
    unformattedOutput.at(i+1).emplace_back(fstring.str());
  }
  if(mObj.relevantTraces.size() != 0){
    for (const auto &i : mObj.relevantTraces) {
      if(i.storageType == JoSIM::StorageType::Voltage) {
        unformattedOutput.at(0).emplace_back(i.deviceLabel.value());
        double voltN1 = 0;
        for(int j = 0; j < sObj.results.timeAxis.size(); ++j) {
          double value;
          if(iObj.argAnal == JoSIM::AnalysisType::Voltage) {
            if(!i.index2) {
              value = sObj.results.xVector_new.at(i.index1.value()).value().at(j);
            } else if (!i.index1) {
              value = -sObj.results.xVector_new.at(i.index2.value()).value().at(j);
            } else {
              value = sObj.results.xVector_new.at(i.index1.value()).value().at(j) 
                      - sObj.results.xVector_new.at(i.index2.value()).value().at(j);
            }
          } else {
            if(!i.index2) {
              if (j == 0) {
                value = ((2 * JoSIM::Constants::SIGMA) / iObj.transSim.get_prstep()) 
                        * sObj.results.xVector_new.at(i.index1.value()).value().at(j);
                voltN1 = value;
              } else {
                value = ((2 * JoSIM::Constants::SIGMA) / iObj.transSim.get_prstep())  
                        * (sObj.results.xVector_new.at(i.index1.value()).value().at(j)
                        -  sObj.results.xVector_new.at(i.index1.value()).value().at(j - 1)) - voltN1;
                voltN1 = value;
              }
            } else if (!i.index1) {
              if (j == 0) {
                value = ((2 * JoSIM::Constants::SIGMA) / iObj.transSim.get_prstep())  
                        * (-sObj.results.xVector_new.at(i.index2.value()).value().at(j));
                voltN1 = value;
              } else {
                value = ((2 * JoSIM::Constants::SIGMA) / iObj.transSim.get_prstep())  
                        * ((-sObj.results.xVector_new.at(i.index2.value()).value().at(j))
                        -  (-sObj.results.xVector_new.at(i.index2.value()).value().at(j - 1))) - voltN1;
                voltN1 = value;
              }
            } else {
              if (j == 0) {
                value = ((2 * JoSIM::Constants::SIGMA) / iObj.transSim.get_prstep())  
                        * (sObj.results.xVector_new.at(i.index1.value()).value().at(j)
                        - sObj.results.xVector_new.at(i.index2.value()).value().at(j));
                voltN1 = value;
              } else {
                value = ((2 * JoSIM::Constants::SIGMA) / iObj.transSim.get_prstep())  
                        * ((sObj.results.xVector_new.at(i.index1.value()).value().at(j) 
                        - sObj.results.xVector_new.at(i.index2.value()).value().at(j))
                        -  (sObj.results.xVector_new.at(i.index1.value()).value().at(j - 1) 
                        - sObj.results.xVector_new.at(i.index2.value()).value().at(j - 1))) - voltN1;
                voltN1 = value;
              }
            }
          }
          fstring.str("");
          fstring << std::fixed << std::scientific << std::setprecision(16) << value;
          unformattedOutput.at(j+1).emplace_back(fstring.str());
        }
      } else if(i.storageType == JoSIM::StorageType::Phase) {
        unformattedOutput.at(0).emplace_back(i.deviceLabel.value());
        double phaseN1 = 0;
        for(int j = 0; j < sObj.results.timeAxis.size(); ++j) {
          double value;
          if(i.deviceLabel.value().at(3) == 'B' || i.deviceLabel.value().at(3) == 'P') {
            value = sObj.results.xVector_new.at(i.index1.value()).value().at(j);
          } else {
            if(iObj.argAnal == JoSIM::AnalysisType::Phase) {
              if(!i.index2) {
                value = sObj.results.xVector_new.at(i.index1.value()).value().at(j);
              } else if (!i.index1) {
                value = -sObj.results.xVector_new.at(i.index2.value()).value().at(j);
              } else {
                value = sObj.results.xVector_new.at(i.index1.value()).value().at(j) 
                        - sObj.results.xVector_new.at(i.index2.value()).value().at(j);
              }
            } else {
              if(!i.index2) {
                if (j == 0) {
                  value = (iObj.transSim.get_prstep() / (2 * JoSIM::Constants::SIGMA)) 
                          * sObj.results.xVector_new.at(i.index1.value()).value().at(j);
                  phaseN1 = value;
                } else {
                  value = (iObj.transSim.get_prstep() / (2 * JoSIM::Constants::SIGMA))  
                          * (sObj.results.xVector_new.at(i.index1.value()).value().at(j)
                          +  sObj.results.xVector_new.at(i.index1.value()).value().at(j - 1)) + phaseN1;
                  phaseN1 = value;
                }
              } else if (!i.index1) {
                if (j == 0) {
                  value = (iObj.transSim.get_prstep() / (2 * JoSIM::Constants::SIGMA))  
                          * (-sObj.results.xVector_new.at(i.index2.value()).value().at(j));
                  phaseN1 = value;
                } else {
                  value = (iObj.transSim.get_prstep() / (2 * JoSIM::Constants::SIGMA)) 
                          * ((-sObj.results.xVector_new.at(i.index2.value()).value().at(j))
                          +  (-sObj.results.xVector_new.at(i.index2.value()).value().at(j - 1))) + phaseN1;
                  phaseN1 = value;
                }
              } else {
                if (j == 0) {
                  value = (iObj.transSim.get_prstep() / (2 * JoSIM::Constants::SIGMA)) 
                          * (sObj.results.xVector_new.at(i.index1.value()).value().at(j)
                          - sObj.results.xVector_new.at(i.index2.value()).value().at(j));
                  phaseN1 = value;
                } else {
                  value = (iObj.transSim.get_prstep() / (2 * JoSIM::Constants::SIGMA)) 
                          * ((sObj.results.xVector_new.at(i.index1.value()).value().at(j) 
                          - sObj.results.xVector_new.at(i.index2.value()).value().at(j))
                          +  (sObj.results.xVector_new.at(i.index1.value()).value().at(j - 1) 
                          - sObj.results.xVector_new.at(i.index2.value()).value().at(j - 1))) + phaseN1;
                  phaseN1 = value;
                }
              }
            }
          }
          fstring.str("");
          fstring << std::fixed << std::scientific << std::setprecision(16) << value;
          unformattedOutput.at(j+1).emplace_back(fstring.str());
        }
      } else if(i.storageType == JoSIM::StorageType::Current) {
        unformattedOutput.at(0).emplace_back(i.deviceLabel.value());
        if(i.deviceLabel.value().at(3) != 'I'){ 
          for(int j = 0; j < sObj.results.timeAxis.size(); ++j) {
            double value = sObj.results.xVector_new.at(i.index1.value()).value().at(j);
            fstring.str("");
            fstring << std::fixed << std::scientific << std::setprecision(16) << value;
            unformattedOutput.at(j+1).emplace_back(fstring.str());
          }
        } else {
          for(int j = 0; j < sObj.results.timeAxis.size(); ++j) {
            double value = mObj.sources.at(i.sourceIndex.value()).at(j);
            fstring.str("");
            fstring << std::fixed << std::scientific << std::setprecision(16) << value;
            unformattedOutput.at(j+1).emplace_back(fstring.str());
          }
        }
      }
    }
  } else {
    for (const auto &i : mObj.nm) {
      if(iObj.argAnal == JoSIM::AnalysisType::Voltage) {
        unformattedOutput.at(0).emplace_back("V(" + i.first + ")");
      } else {
        unformattedOutput.at(0).emplace_back("P(" + i.first + ")");
      }
      for(int j = 0; j < sObj.results.timeAxis.size(); ++j) {
        fstring.str("");
        fstring << std::fixed << std::scientific << std::setprecision(16) << sObj.results.xVector_new.at(i.second).value().at(j);
        unformattedOutput.at(j+1).emplace_back(fstring.str());
      }
    }
  }

  return unformattedOutput;
}

void Output::format_csv_or_dat(const std::string &filename, const std::vector<std::vector<std::string>> &output, const char &delimiter) {
  std::ofstream outfile(filename);
  if (outfile.is_open()) {
    for (const auto &i : output) {
      for (int j = 0; j < i.size() - 1; ++j) {
        outfile << i.at(j) << delimiter;
      }
      outfile << i.back();
      outfile << "\n";
    }
  } else {
    Errors::output_errors(OutputErrors::CANNOT_OPEN_FILE, filename);
  }
}

void Output::format_raw(const std::string &filename, const std::vector<std::vector<std::string>> &output) {

}

void Output::format_cout(const std::vector<std::vector<std::string>> &output) {
  for (const auto &i : output) {
    for (const auto &j : i) {
      std::cout << j << " ";
    }
    std::cout << "\n";
  }
}