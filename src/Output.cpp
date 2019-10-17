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

void Output::relevant_traces(Input &iObj, Matrix &mObj, Simulation &sObj)
{
  // Clear Traces so that we don't repopulate it through the interface
  traces.clear();
  timesteps = &sObj.results.timeAxis;
  std::vector<std::string> tokens, tokens2;

  for(const auto &i : iObj.relevantX) {
    tokens = Misc::tokenize_space(i.substr(i.find_first_of(" \t") + 1));
    for (auto &j : tokens) {
      for (auto &k : tokens) {
        if (k.find('_') != std::string::npos) {
          tokens2 = Misc::tokenize_delimeter(k, "_");
          k = tokens2.back();
          tokens2.pop_back();
          for (int l = tokens2.size() - 1; l >= 0; l--)
            k += "|" + tokens2.at(l);
        } else if (k.find('.') != std::string::npos) {
          std::replace(k.begin(), k.end(), '.', '|');
        }
      }
      if(j.at(0) == 'D' || (j.at(0) == 'P' && j.at(1) == 'H')) { //////////////////// DEVV/DEVI/PHASE /////////////////
          tokens2.clear();  
          std::copy(tokens.begin() + 1, tokens.end(), std::back_inserter(tokens2));
          if(std::find(mObj.rowNames.begin(),mObj.rowNames.end(), tokens2.at(0)) != mObj.rowNames.end()
            || mObj.labelNodes.count(tokens2.at(0)) != 0) {
            if(j.back() == 'V') { //////////////////// DEVV /////////////////
              traces.emplace_back(Trace());
              traces.back().calcData.reserve(iObj.transSim.get_simsize());
              handle_voltage(tokens2, traces.back(), iObj, mObj, sObj);
              break;
            } else if (j.back() == 'I') { //////////////////// DEVI /////////////////
              traces.emplace_back(Trace());
              traces.back().calcData.reserve(iObj.transSim.get_simsize());
              handle_current(tokens2, traces.back(), iObj, mObj, sObj);
              break;
            } else if (j.back() == 'E') {//////////////////// PHASE /////////////////
              traces.emplace_back(Trace());
              traces.back().calcData.reserve(iObj.transSim.get_simsize());
              handle_phase(tokens2, traces.back(), iObj, mObj, sObj);
              break;
            }
          } else {
            Errors::control_errors(static_cast<int>(ControlErrors::UNKNOWN_DEVICE), tokens2.at(0));
          }
      } else if (j.at(0) == 'C' || j.at(0) == 'I') { //////////////////// CURRENT - C() /////////////////
        tokens2 = Misc::tokenize_delimeter(j.substr(1), "(),");
        if(std::find(mObj.rowNames.begin(),mObj.rowNames.end(), tokens2.at(0)) != mObj.rowNames.end()
            || mObj.labelNodes.count(tokens2.at(0)) != 0) {
          traces.emplace_back(Trace());
          traces.back().calcData.reserve(iObj.transSim.get_simsize());
          handle_current(tokens2, traces.back(), iObj, mObj, sObj);
        } else {
          Errors::control_errors(static_cast<int>(ControlErrors::UNKNOWN_DEVICE), tokens2.at(0));
        }
      } else if (j.at(0) == 'P') { //////////////////// PHASE - P() /////////////////
        tokens2 = Misc::tokenize_delimeter(j.substr(1), "(),");
        if(std::find(mObj.rowNames.begin(),mObj.rowNames.end(), tokens2.at(0)) != mObj.rowNames.end()
            || mObj.labelNodes.count(tokens2.at(0)) != 0) {
          traces.emplace_back(Trace());
          traces.back().calcData.reserve(iObj.transSim.get_simsize());
          handle_phase(tokens2, traces.back(), iObj, mObj, sObj);
        } else {
          Errors::control_errors(static_cast<int>(ControlErrors::UNKNOWN_DEVICE), tokens2.at(0));
        }
      } else if (j.at(0) == 'V') { //////////////////// VOLTAGE - P() /////////////////
        tokens2 = Misc::tokenize_delimeter(j.substr(1), "(),");
        if(std::find(mObj.rowNames.begin(),mObj.rowNames.end(), tokens2.at(0)) != mObj.rowNames.end()
            || mObj.labelNodes.count(tokens2.at(0)) != 0) {
          traces.emplace_back(Trace());
          traces.back().calcData.reserve(iObj.transSim.get_simsize());
          handle_voltage(tokens2, traces.back(), iObj, mObj, sObj);
        } else {
          Errors::control_errors(static_cast<int>(ControlErrors::UNKNOWN_DEVICE), tokens2.at(0));
        }
      } else if (j.at(0) == 'N') { //////////////////// NODEV - NODEP /////////////////
        tokens2.clear();
        std::copy(tokens.begin() + 1, tokens.end(), std::back_inserter(tokens2));
        if(std::find(mObj.rowNames.begin(),mObj.rowNames.end(), tokens2.at(0)) != mObj.rowNames.end()
            || mObj.labelNodes.count(tokens2.at(0)) != 0) {
          if(j.back() == 'V') {
            traces.emplace_back(Trace());
            traces.back().calcData.reserve(iObj.transSim.get_simsize());
            handle_voltage(tokens2, traces.back(), iObj, mObj, sObj);
            break;
          } else if(j.back() == 'P') {
            traces.emplace_back(Trace());
            traces.back().calcData.reserve(iObj.transSim.get_simsize());
            handle_phase(tokens2, traces.back(), iObj, mObj, sObj);
            break;
          }
        } else {
          Errors::control_errors(static_cast<int>(ControlErrors::UNKNOWN_DEVICE), tokens2.at(0));
        }
      } else if (j.at(0) == '@') {
        traces.emplace_back(Trace());
        traces.back().calcData.reserve(iObj.transSim.get_simsize());
        for(auto t : tokens) {
          if(t.at(tokens.at(0).size() - 2) == 'C') {
            t = t.substr(1, t.size() - 4);
            if(std::find(mObj.rowNames.begin(),mObj.rowNames.end(), t) != mObj.rowNames.end()
              || mObj.labelNodes.count(t) != 0) {
                tokens2 = Misc::tokenize_delimeter(t, ",");
                handle_current(tokens2, traces.back(), iObj, mObj, sObj);
            } else {
              Errors::control_errors(static_cast<int>(ControlErrors::UNKNOWN_DEVICE), t);
            }
          } else if(t.at(t.size() - 2) == 'V') {
            t = t.substr(1, t.size() - 4);
            if(std::find(mObj.rowNames.begin(),mObj.rowNames.end(), t) != mObj.rowNames.end()
              || mObj.labelNodes.count(t) != 0) {
                tokens2 = Misc::tokenize_delimeter(t, ",");
                handle_voltage(tokens2, traces.back(), iObj, mObj, sObj);
            } else {
              Errors::control_errors(static_cast<int>(ControlErrors::UNKNOWN_DEVICE), t);
            }
          } else if(t.at(t.size() - 2) == 'P') {
            t = t.substr(1, t.size() - 4);
            if(std::find(mObj.rowNames.begin(),mObj.rowNames.end(), t) != mObj.rowNames.end()
              || mObj.labelNodes.count(t) != 0) {
                tokens2 = Misc::tokenize_delimeter(t, ",");
                handle_phase(tokens2, traces.back(), iObj, mObj, sObj);
            } else {
              Errors::control_errors(static_cast<int>(ControlErrors::UNKNOWN_DEVICE), t);
            }
          }
        }
        break;
      }
    }
  }
}

void Output::handle_voltage(const std::vector<std::string> &devToHandle, Trace &result, const Input &iObj, const Matrix &mObj, const Simulation &sObj) {
  result.type = 'V';
  if(mObj.analysisType == JoSIM::AnalysisType::Voltage) {
    if(devToHandle.size() > 1) {
      if(mObj.labelNodes.count(devToHandle.at(0)) == 0) {
        if(devToHandle.at(1) == "0" || devToHandle.at(1) == "GND") {
          result.name = "NODEV_" + devToHandle.at(0) + "_0";
          result.calcData = sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0))));
        } else if(devToHandle.at(0) == "0" || devToHandle.at(0) == "GND") {
          result.name = "NODEV_0_" + devToHandle.at(1);
          std::transform(sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(1)))).begin(),
            sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(1)))).end(),
            std::back_inserter(result.calcData), std::negate<double>());
        } else {
          result.name = "NODEV_" + devToHandle.at(0) + "_" + devToHandle.at(1);
          std::transform(sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).begin(),
            sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).end(),
            sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(1)))).begin(),
            std::back_inserter(result.calcData), std::minus<double>());
        }
      } else {
        std::vector<double> devv1, devv2;
        if(mObj.labelNodes.at(devToHandle.at(0)).negNode == "0" || mObj.labelNodes.at(devToHandle.at(0)).negNode == "GND") {
          devv1 = sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode)));
        } else if(mObj.labelNodes.at(devToHandle.at(0)).posNode == "0" || mObj.labelNodes.at(devToHandle.at(0)).posNode == "GND") {
          std::transform(sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).begin(),
            sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).end(),
            std::back_inserter(devv1), std::negate<double>());
        } else {
          std::transform(sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).begin(),
            sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).end(),
            sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).begin(),
            std::back_inserter(devv1), std::minus<double>());
        }
        if(mObj.labelNodes.at(devToHandle.at(1)).negNode == "0" || mObj.labelNodes.at(devToHandle.at(1)).negNode == "GND") {
          devv2 = sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(1)).posNode)));
        } else if(mObj.labelNodes.at(devToHandle.at(1)).posNode == "0" || mObj.labelNodes.at(devToHandle.at(1)).posNode == "GND") {
          std::transform(sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(1)).negNode))).begin(),
            sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(1)).negNode))).end(),
            std::back_inserter(devv2), std::negate<double>());
        } else {
          std::transform(sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(1)).posNode))).begin(),
            sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(1)).posNode))).end(),
            sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(1)).negNode))).begin(),
            std::back_inserter(devv2), std::minus<double>());
        }
        result.name = "DEVV_" + devToHandle.at(0) + "_" + devToHandle.at(1);
        std::transform(devv1.begin(), devv1.end(), devv2.begin(), std::back_inserter(result.calcData), std::minus<double>());
      }
    } else {
      if(mObj.labelNodes.count(devToHandle.at(0)) == 0) {
        result.name = "NODEV_" + devToHandle.at(0);
        result.calcData = sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0))));
      } else {
        if(mObj.deviceLabelIndex.at(devToHandle.at(0)).type == RowDescriptor::Type::VoltageVS) {
          result.calcData = mObj.sources.at(mObj.deviceLabelIndex.at(devToHandle.at(0)).index);
        } else {
          if(mObj.labelNodes.at(devToHandle.at(0)).negNode == "0" || mObj.labelNodes.at(devToHandle.at(0)).negNode == "GND") {
            result.calcData = sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode)));
          } else if(mObj.labelNodes.at(devToHandle.at(0)).posNode == "0" || mObj.labelNodes.at(devToHandle.at(0)).posNode == "GND") {
            std::transform(sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).begin(),
              sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).end(),
              std::back_inserter(result.calcData), std::negate<double>());
          } else {
            std::transform(sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).begin(),
              sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).end(),
              sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).begin(),
              std::back_inserter(result.calcData), std::minus<double>());
          }
        }
        result.name = "DEVV_" + devToHandle.at(0);
      }
    }
  } else {
    if(devToHandle.size() > 1) {
      if(mObj.labelNodes.count(devToHandle.at(0)) == 0) {
        if(devToHandle.at(1) == "0" || devToHandle.at(1) == "GND") {
          result.name = "NODEV_" + devToHandle.at(0) + "_0";
          result.calcData.emplace_back((JoSIM::Constants::HBAR / (iObj.transSim.get_prstep() * JoSIM::Constants::EV)) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).at(0));
          for (int i = 1; i < sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).size(); i++) {
            result.calcData.emplace_back((JoSIM::Constants::HBAR / (iObj.transSim.get_prstep() * JoSIM::Constants::EV))
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).at(i) - sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).at(i-1))
              - result.calcData.back());
          }
        } else if(devToHandle.at(0) == "0" || devToHandle.at(0) == "GND") {
          result.name = "NODEV_0_" + devToHandle.at(1);
          result.calcData.emplace_back(-((JoSIM::Constants::HBAR / (iObj.transSim.get_prstep() * JoSIM::Constants::EV)) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(1)))).at(0)));
          for (int i = 1; i < sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(1)))).size(); i++) {
            result.calcData.emplace_back(-((JoSIM::Constants::HBAR / (iObj.transSim.get_prstep() * JoSIM::Constants::EV))
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(1)))).at(i) - sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(1)))).at(i-1))
              - result.calcData.back()));
          }
        } else {
          result.name = "NODEV_" + devToHandle.at(0) + "_" + devToHandle.at(1);
          result.calcData.emplace_back(((JoSIM::Constants::HBAR / (iObj.transSim.get_prstep() * JoSIM::Constants::EV)) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).at(0))
            - ((JoSIM::Constants::HBAR / (iObj.transSim.get_prstep() * JoSIM::Constants::EV)) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(1)))).at(0)));
          for (int i = 1; i < sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).size(); i++) {
            result.calcData.emplace_back(((JoSIM::Constants::HBAR / (iObj.transSim.get_prstep() * JoSIM::Constants::EV))
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).at(i) - sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).at(i-1)))
              - ((JoSIM::Constants::HBAR / (iObj.transSim.get_prstep() * JoSIM::Constants::EV))
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(1)))).at(i) - sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(1)))).at(i-1)))
              - result.calcData.back());
          }
        }
      } else {
        std::vector<double> devv1, devv2;
        if(mObj.labelNodes.at(devToHandle.at(0)).negNode == "0" || mObj.labelNodes.at(devToHandle.at(0)).negNode == "GND") {
          devv1.emplace_back((JoSIM::Constants::HBAR / (iObj.transSim.get_prstep() * JoSIM::Constants::EV)) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).at(0));
          for (int i = 1; i < sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).size(); i++) {
            devv1.emplace_back((JoSIM::Constants::HBAR / (iObj.transSim.get_prstep() * JoSIM::Constants::EV))
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).at(i) 
              - sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).at(i-1)) 
              - devv1.back());
          }
        } else if(mObj.labelNodes.at(devToHandle.at(0)).posNode == "0" || mObj.labelNodes.at(devToHandle.at(0)).posNode == "GND") {
          devv1.emplace_back(-((JoSIM::Constants::HBAR / (iObj.transSim.get_prstep() * JoSIM::Constants::EV)) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).at(0)));
          for (int i = 1; i < sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).size(); i++) {
            devv1.emplace_back(-((JoSIM::Constants::HBAR / (iObj.transSim.get_prstep() * JoSIM::Constants::EV))
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).at(i) 
              - sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).at(i-1)) 
              - devv1.back()));
          }
        } else {
          devv1.emplace_back(((JoSIM::Constants::HBAR / (iObj.transSim.get_prstep() * JoSIM::Constants::EV)) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).at(0))
            - ((JoSIM::Constants::HBAR / (iObj.transSim.get_prstep() * JoSIM::Constants::EV)) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).at(0)));
          for (int i = 1; i < sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).size(); i++) {
            devv1.emplace_back(((JoSIM::Constants::HBAR / (iObj.transSim.get_prstep() * JoSIM::Constants::EV))
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).at(i) 
              - sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).at(i-1)))
              - ((JoSIM::Constants::HBAR / (iObj.transSim.get_prstep() * JoSIM::Constants::EV))
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).at(i) 
              - sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).at(i-1)))
              - devv1.back());
          }
        }
        if(mObj.labelNodes.at(devToHandle.at(1)).negNode == "0" || mObj.labelNodes.at(devToHandle.at(1)).negNode == "GND") {
          devv2.emplace_back((JoSIM::Constants::HBAR / (iObj.transSim.get_prstep() * JoSIM::Constants::EV)) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(1)).posNode))).at(0));
          for (int i = 1; i < sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(1)))).size(); i++) {
            devv2.emplace_back((JoSIM::Constants::HBAR / (iObj.transSim.get_prstep() * JoSIM::Constants::EV))
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(1)).posNode))).at(i) 
              - sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(1)).posNode))).at(i-1)) 
              - devv2.back());
          }
        } else if(mObj.labelNodes.at(devToHandle.at(1)).posNode == "0" || mObj.labelNodes.at(devToHandle.at(1)).posNode == "GND") {
          devv2.emplace_back(-((JoSIM::Constants::HBAR / (iObj.transSim.get_prstep() * JoSIM::Constants::EV)) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(1)).negNode))).at(0)));
          for (int i = 1; i < sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(1)))).size(); i++) {
            devv2.emplace_back(-((JoSIM::Constants::HBAR / (iObj.transSim.get_prstep() * JoSIM::Constants::EV))
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(1)).negNode))).at(i) 
              - sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(1)).negNode))).at(i-1)) 
              - devv2.back()));
          }
        } else {
          devv2.emplace_back(((JoSIM::Constants::HBAR / (iObj.transSim.get_prstep() * JoSIM::Constants::EV)) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(1)).posNode))).at(0))
            - ((JoSIM::Constants::HBAR / (iObj.transSim.get_prstep() * JoSIM::Constants::EV)) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(1)).negNode))).at(0)));
          for (int i = 1; i < sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(1)))).size(); i++) {
            devv2.emplace_back(((JoSIM::Constants::HBAR / (iObj.transSim.get_prstep() * JoSIM::Constants::EV))
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(1)).posNode))).at(i) 
              - sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(1)).posNode))).at(i-1)))
              - ((JoSIM::Constants::HBAR / (iObj.transSim.get_prstep() * JoSIM::Constants::EV))
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(1)).negNode))).at(i) 
              - sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(1)).negNode))).at(i-1)))
              - devv2.back());
          }
        }
        result.name = "DEVV_" + devToHandle.at(0) + "_" + devToHandle.at(1);
        std::transform(devv1.begin(), devv1.end(), devv2.begin(), std::back_inserter(result.calcData), std::minus<double>());
      }
    } else {
      if(mObj.labelNodes.count(devToHandle.at(0)) == 0) {
        result.name = "NODEV_" + devToHandle.at(0);
        result.calcData.emplace_back((JoSIM::Constants::HBAR / (iObj.transSim.get_prstep() * JoSIM::Constants::EV)) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).at(0));
        for (int i = 1; i < sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).size(); i++) {
          result.calcData.emplace_back((JoSIM::Constants::HBAR / (iObj.transSim.get_prstep() * JoSIM::Constants::EV)) 
            * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).at(i) 
            - sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).at(i-1))
            - result.calcData.back());
        }
      } else {
        if(mObj.deviceLabelIndex.at(devToHandle.at(0)).type == RowDescriptor::Type::PhaseVS) {
          result.calcData = mObj.sources.at(mObj.deviceLabelIndex.at(devToHandle.at(0)).index);
        } else if(mObj.deviceLabelIndex.at(devToHandle.at(0)).type == RowDescriptor::Type::PhasePS) {
          result.calcData.emplace_back((JoSIM::Constants::HBAR / (iObj.transSim.get_prstep() * JoSIM::Constants::EV)) * mObj.sources.at(mObj.deviceLabelIndex.at(devToHandle.at(0)).index).at(0));
          for (int i = 1; i < mObj.sources.at(mObj.deviceLabelIndex.at(devToHandle.at(0)).index).size(); i++) {
            result.calcData.emplace_back((JoSIM::Constants::HBAR / (iObj.transSim.get_prstep() * JoSIM::Constants::EV)) 
              * (mObj.sources.at(mObj.deviceLabelIndex.at(devToHandle.at(0)).index).at(i) 
              - mObj.sources.at(mObj.deviceLabelIndex.at(devToHandle.at(0)).index).at(i-1))
              - result.calcData.back());
          }
        } else {
          if(mObj.labelNodes.at(devToHandle.at(0)).negNode == "0" || mObj.labelNodes.at(devToHandle.at(0)).negNode == "GND") {
            result.calcData.emplace_back((JoSIM::Constants::HBAR / (iObj.transSim.get_prstep() * JoSIM::Constants::EV)) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).at(0));
            for (int i = 1; i < sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).size(); i++) {
              result.calcData.emplace_back((JoSIM::Constants::HBAR / (iObj.transSim.get_prstep() * JoSIM::Constants::EV)) 
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).at(i)
              - sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).at(i-1))
              - result.calcData.back());
            }
          } else if(mObj.labelNodes.at(devToHandle.at(0)).posNode == "0" || mObj.labelNodes.at(devToHandle.at(0)).posNode == "GND") {
            result.calcData.emplace_back(-((JoSIM::Constants::HBAR / (iObj.transSim.get_prstep() * JoSIM::Constants::EV)) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).at(0)));
            for (int i = 1; i < sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).size(); i++) {
              result.calcData.emplace_back(-((JoSIM::Constants::HBAR / (iObj.transSim.get_prstep() * JoSIM::Constants::EV)) 
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).at(i)
              - sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).at(i-1))
              - result.calcData.back()));
            }
          } else {
            result.calcData.emplace_back((JoSIM::Constants::HBAR / (iObj.transSim.get_prstep() * JoSIM::Constants::EV)) 
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).at(0)
              - sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).at(0)));
            for (int i = 1; i < sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).size(); i++) {
              result.calcData.emplace_back((JoSIM::Constants::HBAR / (iObj.transSim.get_prstep() * JoSIM::Constants::EV)) 
              * ((sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).at(i)
              - sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).at(i-1))
              - (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).at(i)
              - sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).at(i-1)))
              - result.calcData.back());
            }
          }
        }
        result.name = "DEVV_" + devToHandle.at(0);
      }
    }
  }
}

void Output::handle_current(const std::vector<std::string> &devToHandle, Trace &result, const Input &iObj, const Matrix &mObj, const Simulation &sObj) {
  result.type = 'C';
  switch(devToHandle.at(0).at(0)){
    case 'R':
      if(mObj.analysisType == JoSIM::AnalysisType::Voltage) {
        auto R = mObj.components.voltRes.at(mObj.deviceLabelIndex.at(devToHandle.at(0)).index).value;
        auto my_functor = [&](auto left) -> double {return left*(1/R); };
        if(mObj.labelNodes.at(devToHandle.at(0)).negNode == "0" || mObj.labelNodes.at(devToHandle.at(0)).negNode == "GND") {
          std::transform(
            sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).begin(),
            sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).end(),
            std::back_inserter(result.calcData),
            my_functor);
        } else if(mObj.labelNodes.at(devToHandle.at(0)).posNode == "0" || mObj.labelNodes.at(devToHandle.at(0)).posNode == "GND") {
          R = -R;
          std::transform(
            sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).begin(),
            sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).end(),
            std::back_inserter(result.calcData),
            my_functor);
        } else {
          auto my_functor2 = [&](auto left, auto right) -> double {return (left-right)*(1/R); };
          std::transform(
            sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).begin(),
            sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).end(),
            sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).begin(),
            std::back_inserter(result.calcData), my_functor2);
        }
        
      } else {
        result.calcData = sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0))));
      }
      break;
    case 'B':
      if(mObj.analysisType == JoSIM::AnalysisType::Voltage) {
        result.calcData = mObj.components.voltJJ.at(mObj.deviceLabelIndex.at(devToHandle.at(0)).index).jjCur;
      } else {
        result.calcData = mObj.components.phaseJJ.at(mObj.deviceLabelIndex.at(devToHandle.at(0)).index).jjCur;
      }
      break;
    case 'L':
    case 'C':
    case 'V':
      result.calcData = sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0))));
      break;
    case 'I':
      result.calcData = mObj.sources.at(mObj.deviceLabelIndex.at(devToHandle.at(0)).index);
      break;
  }
  result.name = "CURRENT_" + devToHandle.at(0);
}

void Output::handle_phase(const std::vector<std::string> &devToHandle, Trace &result, const Input &iObj, const Matrix &mObj, const Simulation &sObj) {
  result.type = 'P';
  if(mObj.analysisType == JoSIM::AnalysisType::Phase) {
    if(devToHandle.size() > 1) {
      if(mObj.labelNodes.count(devToHandle.at(0)) == 0) {
        if(devToHandle.at(1) == "0" || devToHandle.at(1) == "GND") {
          result.name = "NODEP_" + devToHandle.at(0) + "_0";
          result.calcData = sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0))));
        } else if(devToHandle.at(0) == "0" || devToHandle.at(0) == "GND") {
          result.name = "NODEP_0_" + devToHandle.at(0);
          std::transform(sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(1)))).begin(),
            sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(1)))).end(),
            std::back_inserter(result.calcData), std::negate<double>());
        } else {
          result.name = "NODEP_" + devToHandle.at(0) + "_" + devToHandle.at(1);
          std::transform(sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).begin(),
            sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).end(),
            sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(1)))).begin(),
            std::back_inserter(result.calcData), std::minus<double>());
        }
      } else {
        std::vector<double> devv1, devv2;
        if(mObj.labelNodes.at(devToHandle.at(0)).negNode == "0" || mObj.labelNodes.at(devToHandle.at(0)).negNode == "GND") {
          devv1 = sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode)));
        } else if(mObj.labelNodes.at(devToHandle.at(0)).posNode == "0" || mObj.labelNodes.at(devToHandle.at(0)).posNode == "GND") {
          std::transform(sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).begin(),
            sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).end(),
            std::back_inserter(devv1), std::negate<double>());
        } else {
          std::transform(sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).begin(),
            sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).end(),
            sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).begin(),
            std::back_inserter(devv1), std::minus<double>());
        }
        if(mObj.labelNodes.at(devToHandle.at(1)).negNode == "0" || mObj.labelNodes.at(devToHandle.at(1)).negNode == "GND") {
          devv2 = sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(1)).posNode)));
        } else if(mObj.labelNodes.at(devToHandle.at(1)).posNode == "0" || mObj.labelNodes.at(devToHandle.at(1)).posNode == "GND") {
          std::transform(sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(1)).negNode))).begin(),
            sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(1)).negNode))).end(),
            std::back_inserter(devv2), std::negate<double>());
        } else {
          std::transform(sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(1)).posNode))).begin(),
            sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(1)).posNode))).end(),
            sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(1)).negNode))).begin(),
            std::back_inserter(devv2), std::minus<double>());
        }
        result.name = "PHASE_" + devToHandle.at(0) + "_" + devToHandle.at(1);
        std::transform(devv1.begin(), devv1.end(), devv2.begin(), std::back_inserter(result.calcData), std::minus<double>());
      }
    } else {
        if(mObj.labelNodes.count(devToHandle.at(0)) == 0) {
          result.name = "NODEP_" + devToHandle.at(0);
          result.calcData = sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0))));
        } else {
          if(mObj.deviceLabelIndex.at(devToHandle.at(0)).type == RowDescriptor::Type::PhasePS) {
            result.calcData = mObj.sources.at(mObj.deviceLabelIndex.at(devToHandle.at(0)).index);
          } else {
            if(mObj.labelNodes.at(devToHandle.at(0)).negNode == "0" || mObj.labelNodes.at(devToHandle.at(0)).negNode == "GND") {
              result.calcData = sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode)));
            } else if(mObj.labelNodes.at(devToHandle.at(0)).posNode == "0" || mObj.labelNodes.at(devToHandle.at(0)).posNode == "GND") {
              std::transform(sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).begin(),
                sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).end(),
                std::back_inserter(result.calcData), std::negate<double>());
            } else {
              std::transform(sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).begin(),
                sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).end(),
                sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).begin(),
                std::back_inserter(result.calcData), std::minus<double>());
            }
          }
        }
      result.name = "PHASE_" + devToHandle.at(0);
    }
  } else {
    if(devToHandle.size() > 1) {
      if(mObj.labelNodes.count(devToHandle.at(0)) == 0) {
        if(devToHandle.at(1) == "0" || devToHandle.at(1) == "GND") {
          result.name = "NODEP_" + devToHandle.at(0) + "_0";
          result.calcData.emplace_back(((iObj.transSim.get_prstep() * JoSIM::Constants::EV) / JoSIM::Constants::HBAR) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).at(0));
          for (int i = 1; i < sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).size(); i++) {
            result.calcData.emplace_back(((iObj.transSim.get_prstep() * JoSIM::Constants::EV) / JoSIM::Constants::HBAR)
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).at(i) + sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).at(i-1))
              + result.calcData.back());
          }
        } else if(devToHandle.at(0) == "0" || devToHandle.at(0) == "GND") {
          result.name = "NODEP_0_" + devToHandle.at(1);
          result.calcData.emplace_back(-(((iObj.transSim.get_prstep() * JoSIM::Constants::EV) / JoSIM::Constants::HBAR) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(1)))).at(0)));
          for (int i = 1; i < sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(1)))).size(); i++) {
            result.calcData.emplace_back(-(((iObj.transSim.get_prstep() * JoSIM::Constants::EV) / JoSIM::Constants::HBAR)
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(1)))).at(i) + sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(1)))).at(i-1))
              + result.calcData.back()));
          }
        } else {
          result.name = "NODEP_" + devToHandle.at(0) + "_" + devToHandle.at(1);
          result.calcData.emplace_back((((iObj.transSim.get_prstep() * JoSIM::Constants::EV) / JoSIM::Constants::HBAR) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).at(0))
            - (((iObj.transSim.get_prstep() * JoSIM::Constants::EV) / JoSIM::Constants::HBAR) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(1)))).at(0)));
          for (int i = 1; i < sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).size(); i++) {
            result.calcData.emplace_back((((iObj.transSim.get_prstep() * JoSIM::Constants::EV) / JoSIM::Constants::HBAR)
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).at(i) + sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).at(i-1)))
              - (((iObj.transSim.get_prstep() * JoSIM::Constants::EV) / JoSIM::Constants::HBAR)
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(1)))).at(i) + sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(1)))).at(i-1)))
              + result.calcData.back());
          }
        }
      } else {
        std::vector<double> devv1, devv2;
        if(mObj.labelNodes.at(devToHandle.at(0)).negNode == "0" || mObj.labelNodes.at(devToHandle.at(0)).negNode == "GND") {
          devv1.emplace_back(((iObj.transSim.get_prstep() * JoSIM::Constants::EV) / JoSIM::Constants::HBAR) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).at(0));
          for (int i = 1; i < sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).size(); i++) {
            devv1.emplace_back(((iObj.transSim.get_prstep() * JoSIM::Constants::EV) / JoSIM::Constants::HBAR)
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).at(i) 
              + sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).at(i-1)) 
              + devv1.back());
          }
        } else if(mObj.labelNodes.at(devToHandle.at(0)).posNode == "0" || mObj.labelNodes.at(devToHandle.at(0)).posNode == "GND") {
          devv1.emplace_back(-(((iObj.transSim.get_prstep() * JoSIM::Constants::EV) / JoSIM::Constants::HBAR) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).at(0)));
          for (int i = 1; i < sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).size(); i++) {
            devv1.emplace_back(-(((iObj.transSim.get_prstep() * JoSIM::Constants::EV) / JoSIM::Constants::HBAR)
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).at(i) 
              + sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).at(i-1)) 
              + devv1.back()));
          }
        } else {
          devv1.emplace_back((((iObj.transSim.get_prstep() * JoSIM::Constants::EV) / JoSIM::Constants::HBAR) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).at(0))
            - (((iObj.transSim.get_prstep() * JoSIM::Constants::EV) / JoSIM::Constants::HBAR) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).at(0)));
          for (int i = 1; i < sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).size(); i++) {
            devv1.emplace_back((((iObj.transSim.get_prstep() * JoSIM::Constants::EV) / JoSIM::Constants::HBAR)
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).at(i) 
              + sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).at(i-1)))
              - (((iObj.transSim.get_prstep() * JoSIM::Constants::EV) / JoSIM::Constants::HBAR)
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).at(i) 
              + sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).at(i-1)))
              + devv1.back());
          }
        }
        if(mObj.labelNodes.at(devToHandle.at(1)).negNode == "0" || mObj.labelNodes.at(devToHandle.at(1)).negNode == "GND") {
          devv2.emplace_back(((iObj.transSim.get_prstep() * JoSIM::Constants::EV) / JoSIM::Constants::HBAR) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(1)).posNode))).at(0));
          for (int i = 1; i < sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(1)))).size(); i++) {
            devv2.emplace_back(((iObj.transSim.get_prstep() * JoSIM::Constants::EV) / JoSIM::Constants::HBAR)
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(1)).posNode))).at(i) 
              + sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(1)).posNode))).at(i-1)) 
              + devv2.back());
          }
        } else if(mObj.labelNodes.at(devToHandle.at(1)).posNode == "0" || mObj.labelNodes.at(devToHandle.at(1)).posNode == "GND") {
          devv2.emplace_back(-(((iObj.transSim.get_prstep() * JoSIM::Constants::EV) / JoSIM::Constants::HBAR) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(1)).negNode))).at(0)));
          for (int i = 1; i < sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(1)))).size(); i++) {
            devv2.emplace_back(-(((iObj.transSim.get_prstep() * JoSIM::Constants::EV) / JoSIM::Constants::HBAR)
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(1)).negNode))).at(i) 
              + sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(1)).negNode))).at(i-1)) 
              + devv2.back()));
          }
        } else {
          devv2.emplace_back((((iObj.transSim.get_prstep() * JoSIM::Constants::EV) / JoSIM::Constants::HBAR) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(1)).posNode))).at(0))
            - (((iObj.transSim.get_prstep() * JoSIM::Constants::EV) / JoSIM::Constants::HBAR) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(1)).negNode))).at(0)));
          for (int i = 1; i < sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(1)))).size(); i++) {
            devv2.emplace_back((((iObj.transSim.get_prstep() * JoSIM::Constants::EV) / JoSIM::Constants::HBAR)
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(1)).posNode))).at(i) 
              + sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(1)).posNode))).at(i-1)))
              - (((iObj.transSim.get_prstep() * JoSIM::Constants::EV) / JoSIM::Constants::HBAR)
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(1)).negNode))).at(i) 
              + sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(1)).negNode))).at(i-1)))
              + devv2.back());
          }
        }
        result.name = "PHASE_" + devToHandle.at(0) + "_" + devToHandle.at(1);
        std::transform(devv1.begin(), devv1.end(), devv2.begin(), std::back_inserter(result.calcData), std::minus<double>());
      }
    } else {
      if (devToHandle.at(0).at(0) == 'B') {
        result.calcData = sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0))));
        result.name = "PHASE_" + devToHandle.at(0);
      } else if(mObj.labelNodes.count(devToHandle.at(0)) == 0) {
        result.name = "NODEP_" + devToHandle.at(0);
        result.calcData.emplace_back(((iObj.transSim.get_prstep() * JoSIM::Constants::EV) / JoSIM::Constants::HBAR) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).at(0));
        for (int i = 1; i < sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).size(); i++) {
          result.calcData.emplace_back(((iObj.transSim.get_prstep() * JoSIM::Constants::EV) / JoSIM::Constants::HBAR)
            * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).at(i) 
            + sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).at(i-1))
            + result.calcData.back());
        }
      } else {
        if(mObj.deviceLabelIndex.at(devToHandle.at(0)).type == RowDescriptor::Type::VoltagePS) {
          result.calcData = mObj.sources.at(mObj.deviceLabelIndex.at(devToHandle.at(0)).index);
        } else {
          if(mObj.labelNodes.at(devToHandle.at(0)).negNode == "0" || mObj.labelNodes.at(devToHandle.at(0)).negNode == "GND") {
            result.calcData.emplace_back(((iObj.transSim.get_prstep() * JoSIM::Constants::EV) / JoSIM::Constants::HBAR) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).at(0));
            for (int i = 1; i < sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).size(); i++) {
              result.calcData.emplace_back(((iObj.transSim.get_prstep() * JoSIM::Constants::EV) / JoSIM::Constants::HBAR)
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).at(i)
              + sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).at(i-1))
              + result.calcData.back());
            }
          } else if(mObj.labelNodes.at(devToHandle.at(0)).posNode == "0" || mObj.labelNodes.at(devToHandle.at(0)).posNode == "GND") {
            result.calcData.emplace_back(-(((iObj.transSim.get_prstep() * JoSIM::Constants::EV) / JoSIM::Constants::HBAR)* sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).at(0)));
            for (int i = 1; i < sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).size(); i++) {
              result.calcData.emplace_back(-(((iObj.transSim.get_prstep() * JoSIM::Constants::EV) / JoSIM::Constants::HBAR)
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).at(i)
              + sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).at(i-1))
              + result.calcData.back()));
            }
          } else {
            result.calcData.emplace_back(((iObj.transSim.get_prstep() * JoSIM::Constants::EV) / JoSIM::Constants::HBAR)
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).at(0)
              - sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).at(0)));
            for (int i = 1; i < sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).size(); i++) {
              result.calcData.emplace_back(((iObj.transSim.get_prstep() * JoSIM::Constants::EV) / JoSIM::Constants::HBAR)
              * ((sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).at(i)
              + sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).at(i-1))
              - (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).at(i)
              + sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).at(i-1)))
              + result.calcData.back());
            }
          }
        }
        result.name = "PHASE_" + devToHandle.at(0);
      }
    }
  }
}

void Output::write_data(std::string &outname, const Matrix &mObj, const Simulation &sObj)
{
  int loopsize = 0;
  std::ofstream outfile(outname);
  if (outfile.is_open()) {
    if (!traces.empty()) {
      if (traces.at(0).pointer)
        loopsize = traces.at(0).traceData->size();
      else
        loopsize = traces.at(0).calcData.size();
      outfile << "time"
              << ",";
      for (int i = 0; i < traces.size() - 1; i++) {
        outfile << traces.at(i).name << ",";
      }
      outfile << traces.at(traces.size() - 1).name << "\n";
      for (int i = 0; i < loopsize; i++) {
        outfile << std::fixed << std::scientific << std::setprecision(16)
                << timesteps->at(i) << ",";
        for (int j = 0; j < traces.size() - 1; j++) {
          if (traces.at(j).pointer)
            outfile << std::fixed << std::scientific << std::setprecision(16)
                    << traces.at(j).traceData->at(i) << ",";
          else
            outfile << std::fixed << std::scientific << std::setprecision(16)
                    << traces.at(j).calcData.at(i) << ",";
        }
        if (traces.at(traces.size() - 1).pointer)
          outfile << std::fixed << std::scientific << std::setprecision(16)
                  << traces.at(traces.size() - 1).traceData->at(i) << "\n";
        else
          outfile << std::fixed << std::scientific << std::setprecision(16)
                  << traces.at(traces.size() - 1).calcData.at(i) << "\n";
      }
      outfile.close();
    } else if (traces.empty()) {
      std::cerr << "W: Nothing specified to output." << std::endl;
      std::cerr << "W: Printing all vectors." << std::endl;
      std::cerr << std::endl;
      outfile << "time"
                << ",";
      for (int i = 0; i < mObj.rowNames.size() - 1; i++) {
        outfile << mObj.rowNames.at(i) << ",";
      }
      outfile << mObj.rowNames.at(mObj.rowNames.size() - 1) << "\n";
      for (int i = 0; i < sObj.results.xVect.at(0).size(); i++) {
        outfile << std::fixed << std::scientific << std::setprecision(16)
                  << timesteps->at(i) << ",";
        for (int j = 0; j < sObj.results.xVect.size() - 1; j++) {
          outfile << std::fixed << std::scientific << std::setprecision(16)
                    << sObj.results.xVect.at(j).at(i) << ",";
        }
        outfile << std::fixed << std::scientific << std::setprecision(16)
                  << sObj.results.xVect.at(sObj.results.xVect.size() - 1).at(i)
                  << "\n";
      }
      outfile.close();
    }
  } else {
    std::cerr << "E: " << outname << " could not be opened for saving."
              << std::endl;
    std::cerr << std::endl;
  }
}

void Output::write_legacy_data(std::string &outname, const Matrix &mObj, const Simulation &sObj)
{
  int loopsize = 0;
  std::ofstream outfile(outname);
  if (outfile.is_open()) {
    if (!traces.empty()) {
      if (traces.at(0).pointer)
        loopsize = traces.at(0).traceData->size();
      else
        loopsize = traces.at(0).calcData.size();
      outfile << "time"
              << " ";
      for (int i = 0; i < traces.size() - 1; i++) {
        outfile << traces.at(i).name << " ";
      }
      outfile << traces.at(traces.size() - 1).name << "\n";
      for (int i = 0; i < loopsize; i++) {
        outfile << std::fixed << std::scientific << std::setprecision(16)
                << timesteps->at(i) << " ";
        for (int j = 0; j < traces.size() - 1; j++) {
          if (traces.at(j).pointer)
            outfile << std::fixed << std::scientific << std::setprecision(16)
                    << traces.at(j).traceData->at(i) << " ";
          else
            outfile << std::fixed << std::scientific << std::setprecision(16)
                    << traces.at(j).calcData.at(i) << " ";
        }
        if (traces.at(traces.size() - 1).pointer)
          outfile << std::fixed << std::scientific << std::setprecision(16)
                  << traces.at(traces.size() - 1).traceData->at(i) << "\n";
        else
          outfile << std::fixed << std::scientific << std::setprecision(16)
                  << traces.at(traces.size() - 1).calcData.at(i) << "\n";
      }
      outfile.close();
    } else if (traces.empty()) {
      std::cerr << "W: Nothing specified to output." << std::endl;
      std::cerr << "W: Printing all vectors." << std::endl;
      std::cerr << std::endl;
      outfile << "time"
                << " ";
      for (int i = 0; i < mObj.rowNames.size() - 1; i++) {
        outfile << mObj.rowNames.at(i) << " ";
      }
      outfile << mObj.rowNames.at(mObj.rowNames.size() - 1) << "\n";
      for (int i = 0; i < sObj.results.xVect.at(0).size(); i++) {
        outfile << std::fixed << std::scientific << std::setprecision(16)
                  << timesteps->at(i) << " ";
        for (int j = 0; j < sObj.results.xVect.size() - 1; j++) {
          outfile << std::fixed << std::scientific << std::setprecision(16)
                    << sObj.results.xVect.at(j).at(i) << " ";
        }
        outfile << std::fixed << std::scientific << std::setprecision(16)
                  << sObj.results.xVect.at(sObj.results.xVect.size() - 1).at(i)
                  << "\n";
      }
      outfile.close();
    }
  } else {
    std::cerr << "E: " << outname << " could not be opened for saving."
              << std::endl;
    std::cerr << std::endl;
  }
}

void Output::write_wr_data(std::string &outname)
{
  std::string label;
  std::vector<std::string> tokens;
  int loopsize = 0;
  std::ofstream outfile(outname);
  if (outfile.is_open()) {
    if (!traces.empty()) {
      outfile << "Title: CKT1\n";
      std::time_t result = std::time(nullptr);
      outfile << "Date: " << std::asctime(std::localtime(&result));
      outfile << "Plotname: Transient analysis JoSIM\n";
      outfile << "Flags: real\n";
      outfile << "No. Variables: " << traces.size() + 1 << "\n";
      if (traces.at(0).pointer)
        loopsize = traces.at(0).traceData->size();
      else
        loopsize = traces.at(0).calcData.size();
      outfile << "No. Points: " << loopsize << "\n";
      outfile << "Command: version 4.3.8\n";
      outfile << "Variables:\n";
      outfile << " 0 time S\n";
      for (int i = 0; i < traces.size(); i++) {
        if (traces.at(i).type == 'V') {
          if (traces.at(i).name.substr(0, 5) == "NODEV") {
            tokens = Misc::tokenize_delimeter(traces.at(i).name, "_");
            if (tokens.size() > 2)
              outfile << " " << i + 1 << " v(" << tokens[1] << "," << tokens[2]
                      << ") V\n";
            else
              outfile << " " << i + 1 << " v(" << tokens[1] << ") V\n";
          } else
            outfile << " " << i + 1 << " " << traces.at(i).name << " V\n";
        } else if (traces.at(i).type == 'P') {
          if (traces.at(i).name.substr(0, 5) == "NODEP" || traces.at(i).name.substr(0, 5) == "PHASE") {
            tokens = Misc::tokenize_delimeter(traces.at(i).name, "_");
            if (tokens.size() > 2)
              outfile << " " << i + 1 << " p(" << tokens[1] << "," << tokens[2]
                      << ") P\n";
            else
              outfile << " " << i + 1 << " p(" << tokens[1] << ") P\n";
          } else
            outfile << " " << i + 1 << " " << traces.at(i).name << " P\n";
        } else if (traces.at(i).type == 'C') {
          label = traces.at(i).name;
          std::replace(label.begin(), label.end(), '|', '.');
          outfile << " " << i + 1 << " "
                  << label << "#branch A\n";
        }
      }
      outfile << "Values:\n";
      for (int i = 0; i < loopsize; i++) {
        outfile << " " << i << " " << std::fixed << std::scientific
                << std::setprecision(16) << timesteps->at(i) << "\n";
        for (int j = 0; j < traces.size(); j++) {
          if (traces.at(j).pointer)
            outfile << " " << std::string(Misc::numDigits(i), ' ') << " "
                    << std::fixed << std::scientific << std::setprecision(16)
                    << traces.at(j).traceData->at(i) << "\n";
          else
            outfile << " " << std::string(Misc::numDigits(i), ' ') << " "
                    << std::fixed << std::scientific << std::setprecision(16)
                    << traces.at(j).calcData.at(i) << "\n";
        }
      }
      outfile.close();
    } else if (traces.empty()) {
      std::cerr << "W: Nothing specified to save." << std::endl;
      std::cerr << std::endl;
      outfile.close();
    }
  } else {
    std::cerr << "E: " << outname << " could not be opened for saving."
              << std::endl;
    std::cerr << std::endl;
  }
}

void Output::write_cout(const Matrix &mObj, const Simulation &sObj)
{
  int loopsize = 0;
  if (!traces.empty()) {
    if (traces.at(0).pointer)
      loopsize = traces.at(0).traceData->size();
    else
      loopsize = traces.at(0).calcData.size();
    std::cout << "time"
              << " ";
    for (int i = 0; i < traces.size() - 1; i++) {
      std::cout << traces.at(i).name << " ";
    }
    std::cout << traces.at(traces.size() - 1).name << "\n";
    for (int i = 0; i < loopsize; i++) {
      std::cout << std::fixed << std::scientific << std::setprecision(16)
                << timesteps->at(i) << " ";
      for (int j = 0; j < traces.size() - 1; j++) {
        if (traces.at(j).pointer)
          std::cout << std::fixed << std::scientific << std::setprecision(16)
                    << traces.at(j).traceData->at(i) << " ";
        else
          std::cout << std::fixed << std::scientific << std::setprecision(16)
                    << traces.at(j).calcData.at(i) << " ";
      }
      if (traces.at(traces.size() - 1).pointer)
        std::cout << std::fixed << std::scientific << std::setprecision(16)
                  << traces.at(traces.size() - 1).traceData->at(i) << "\n";
      else
        std::cout << std::fixed << std::scientific << std::setprecision(16)
                  << traces.at(traces.size() - 1).calcData.at(i) << "\n";
    }
  } else if (traces.empty()) {
    std::cerr << "W: Nothing specified to output." << std::endl;
    std::cerr << "W: Printing all vectors." << std::endl;
    std::cerr << std::endl;
    std::cout << "time"
              << " ";
    for (int i = 0; i < mObj.rowNames.size() - 1; i++) {
      std::cout << mObj.rowNames.at(i) << " ";
    }
    std::cout << mObj.rowNames.at(mObj.rowNames.size() - 1) << "\n";
    for (int i = 0; i < sObj.results.xVect.at(0).size(); i++) {
      std::cout << std::fixed << std::scientific << std::setprecision(16)
                << timesteps->at(i) << " ";
      for (int j = 0; j < sObj.results.xVect.size() - 1; j++) {
        std::cout << std::fixed << std::scientific << std::setprecision(16)
                  << sObj.results.xVect.at(j).at(i) << " ";
      }
      std::cout << std::fixed << std::scientific << std::setprecision(16)
                << sObj.results.xVect.at(sObj.results.xVect.size() - 1).at(i)
                << "\n";
    }
  }
}
