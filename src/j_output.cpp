// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#include "JoSIM/j_output.h"
#include "JoSIM/AnalysisType.hpp"
#include "JoSIM/j_errors.h"
#include "JoSIM/j_input.h"
#include "JoSIM/j_simulation.h"

#include <algorithm>
#include <cassert>
#include <functional>
#include <iterator>

void Output::relevant_traces(Input &iObj, Matrix &mObj, Simulation &sObj)
{
  // Clear Traces so that we don't repopulate it through the interface
  traces.clear();
  timesteps = &sObj.results.timeAxis;

/*
  
  std::vector<std::string> tokens, tokens2;
  std::string label, label2;
  int index1, index2;
  for (const auto &i : iObj.controls) {
    if (i.find("PRINT") != std::string::npos) {
      Trace thisTrace;
      tokens = Misc::tokenize_space(i);
      if (tokens.at(1) == "DEVI") {
        if (tokens.size() > 4)
          Errors::control_errors(PRINT_TOO_MANY_ARGS, i);
        label = tokens.at(2);
        if (label.find('_') != std::string::npos) {
          tokens = Misc::tokenize_delimeter(label, "_");
          label = tokens.back();
          for (int j = 0; j < tokens.size() - 1; j++)
            label += "|" + tokens.at(j);
        } else if (label.find('.') != std::string::npos) {
          std::replace(label.begin(), label.end(), '.', '|');
        }
        thisTrace.name = label;
        thisTrace.type = 'C';
        if (mObj.deviceLabelIndex.count(label) != 0) {
          const auto &dev = mObj.deviceLabelIndex.at(label);
          switch (dev.type) {
          case RowDescriptor::Type::VoltageResistor:
            if (mObj.components.voltRes.at(dev.index).posNCol == -1) {
              std::transform(
                  sObj.results.xVect
                      .at(std::distance(
                          mObj.relXInd.begin(),
                          std::find(
                              mObj.relXInd.begin(), mObj.relXInd.end(),
                              mObj.components.voltRes.at(dev.index).negNCol)))
                      .begin(),
                  sObj.results.xVect
                      .at(std::distance(
                          mObj.relXInd.begin(),
                          std::find(
                              mObj.relXInd.begin(), mObj.relXInd.end(),
                              mObj.components.voltRes.at(dev.index).negNCol)))
                      .end(),
                  std::back_inserter(thisTrace.calcData),
                  std::bind(std::multiplies<double>(), std::placeholders::_1,
                            (1 / mObj.components.voltRes.at(dev.index).value)));
            } else if (mObj.components.voltRes.at(dev.index).negNCol == -1) {
              std::transform(
                  sObj.results.xVect
                      .at(std::distance(
                          mObj.relXInd.begin(),
                          std::find(
                              mObj.relXInd.begin(), mObj.relXInd.end(),
                              mObj.components.voltRes.at(dev.index).posNCol)))
                      .begin(),
                  sObj.results.xVect
                      .at(std::distance(
                          mObj.relXInd.begin(),
                          std::find(
                              mObj.relXInd.begin(), mObj.relXInd.end(),
                              mObj.components.voltRes.at(dev.index).posNCol)))
                      .end(),
                  std::back_inserter(thisTrace.calcData),
                  std::bind(std::multiplies<double>(), std::placeholders::_1,
                            (1 / mObj.components.voltRes.at(dev.index).value)));
            } else {
              std::transform(
                  sObj.results.xVect
                      .at(std::distance(
                          mObj.relXInd.begin(),
                          std::find(
                              mObj.relXInd.begin(), mObj.relXInd.end(),
                              mObj.components.voltRes.at(dev.index).posNCol)))
                      .begin(),
                  sObj.results.xVect
                      .at(std::distance(
                          mObj.relXInd.begin(),
                          std::find(
                              mObj.relXInd.begin(), mObj.relXInd.end(),
                              mObj.components.voltRes.at(dev.index).posNCol)))
                      .end(),
                  sObj.results.xVect
                      .at(std::distance(
                          mObj.relXInd.begin(),
                          std::find(
                              mObj.relXInd.begin(), mObj.relXInd.end(),
                              mObj.components.voltRes.at(dev.index).negNCol)))
                      .begin(),
                  std::back_inserter(thisTrace.calcData), std::minus<double>());
              std::transform(
                  thisTrace.calcData.begin(), thisTrace.calcData.end(),
                  thisTrace.calcData.begin(),
                  std::bind(std::multiplies<double>(), std::placeholders::_1,
                            (1 / mObj.components.voltRes.at(dev.index).value)));
            }
            thisTrace.pointer = false;
            traces.push_back(thisTrace);
            break;
          case RowDescriptor::Type::PhaseResistor:
            thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                mObj.relXInd.begin(),
                std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                          mObj.components.phaseRes.at(dev.index).curNCol)));
            traces.push_back(thisTrace);
            break;
          case RowDescriptor::Type::VoltageInductor:
            thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                mObj.relXInd.begin(),
                std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                          mObj.components.voltInd.at(dev.index).curNCol)));
            traces.push_back(thisTrace);
            break;
          case RowDescriptor::Type::PhaseInductor:
            thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                mObj.relXInd.begin(),
                std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                          mObj.components.phaseInd.at(dev.index).curNCol)));
            traces.push_back(thisTrace);
            break;
          case RowDescriptor::Type::VoltageCapacitor:
            thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                mObj.relXInd.begin(),
                std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                          mObj.components.voltCap.at(dev.index).curNCol)));
            traces.push_back(thisTrace);
            break;
          case RowDescriptor::Type::PhaseCapacitor:
            thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                mObj.relXInd.begin(),
                std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                          mObj.components.phaseCap.at(dev.index).curNCol)));
            traces.push_back(thisTrace);
            break;
          case RowDescriptor::Type::VoltageJJ:
            thisTrace.traceData = &mObj.components.voltJJ.at(dev.index).jjCur;
            traces.push_back(thisTrace);
            break;
          case RowDescriptor::Type::PhaseJJ:
            thisTrace.traceData = &mObj.components.phaseJJ.at(dev.index).jjCur;
            traces.push_back(thisTrace);
            break;
          case RowDescriptor::Type::VoltageVS:
            Errors::control_errors(CURRENT_THROUGH_VOLT, i);
            break;
          case RowDescriptor::Type::PhaseVS:
            Errors::control_errors(CURRENT_THROUGH_VOLT, i);
            break;
          case RowDescriptor::Type::VoltageCS:
            thisTrace.traceData = &mObj.sources.at(dev.index);
            traces.push_back(thisTrace);
            break;
          default:
            Errors::control_errors(UNKNOWN_DEVICE, label);
            break;
          }
        } else
          Errors::control_errors(UNKNOWN_DEVICE, label);
      } else if (tokens.at(1) == "DEVV") {
        if (tokens.size() > 4)
          Errors::control_errors(PRINT_TOO_MANY_ARGS, i);
        label = tokens.at(2);
        if (label.find('_') != std::string::npos) {
          tokens = Misc::tokenize_delimeter(label, "_");
          label = tokens.back();
          for (int j = 0; j < tokens.size() - 1; j++)
            label += "|" + tokens.at(j);
        } else if (label.find('.') != std::string::npos) {
          std::replace(label.begin(), label.end(), '.', '|');
        }
        thisTrace.name = label;
        thisTrace.type = 'V';
        if (mObj.deviceLabelIndex.count(label) != 0) {
          const auto &dev = mObj.deviceLabelIndex.at(label);
          switch (dev.type) {
          case RowDescriptor::Type::VoltageResistor:
            if (mObj.components.voltRes.at(dev.index).posNCol == -1) {
              thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                  mObj.relXInd.begin(),
                  std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                            mObj.components.voltRes.at(dev.index).negNCol)));
              traces.push_back(thisTrace);
            } else if (mObj.components.voltRes.at(dev.index).negNCol == -1) {
              thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                  mObj.relXInd.begin(),
                  std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                            mObj.components.voltRes.at(dev.index).posNCol)));
              traces.push_back(thisTrace);
            } else {
              std::transform(
                  sObj.results.xVect
                      .at(std::distance(
                          mObj.relXInd.begin(),
                          std::find(
                              mObj.relXInd.begin(), mObj.relXInd.end(),
                              mObj.components.voltRes.at(dev.index).posNCol)))
                      .begin(),
                  sObj.results.xVect
                      .at(std::distance(
                          mObj.relXInd.begin(),
                          std::find(
                              mObj.relXInd.begin(), mObj.relXInd.end(),
                              mObj.components.voltRes.at(dev.index).posNCol)))
                      .end(),
                  sObj.results.xVect
                      .at(std::distance(
                          mObj.relXInd.begin(),
                          std::find(
                              mObj.relXInd.begin(), mObj.relXInd.end(),
                              mObj.components.voltRes.at(dev.index).negNCol)))
                      .begin(),
                  std::back_inserter(thisTrace.calcData), std::minus<double>());
              thisTrace.pointer = false;
              traces.push_back(thisTrace);
            }
            break;
          case RowDescriptor::Type::PhaseResistor:
            Errors::control_errors(VOLT_WHEN_PHASE, label);
            if (mObj.components.phaseRes.at(dev.index).posNCol == -1) {
              thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                  mObj.relXInd.begin(),
                  std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                            mObj.components.phaseRes.at(dev.index).negNCol)));
              traces.push_back(thisTrace);
            } else if (mObj.components.phaseRes.at(dev.index).negNCol == -1) {
              thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                  mObj.relXInd.begin(),
                  std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                            mObj.components.phaseRes.at(dev.index).posNCol)));
              traces.push_back(thisTrace);
            } else {
              std::transform(
                  sObj.results.xVect
                      .at(std::distance(
                          mObj.relXInd.begin(),
                          std::find(
                              mObj.relXInd.begin(), mObj.relXInd.end(),
                              mObj.components.phaseRes.at(dev.index).posNCol)))
                      .begin(),
                  sObj.results.xVect
                      .at(std::distance(
                          mObj.relXInd.begin(),
                          std::find(
                              mObj.relXInd.begin(), mObj.relXInd.end(),
                              mObj.components.phaseRes.at(dev.index).posNCol)))
                      .end(),
                  sObj.results.xVect
                      .at(std::distance(
                          mObj.relXInd.begin(),
                          std::find(
                              mObj.relXInd.begin(), mObj.relXInd.end(),
                              mObj.components.phaseRes.at(dev.index).negNCol)))
                      .begin(),
                  std::back_inserter(thisTrace.calcData), std::minus<double>());
              thisTrace.pointer = false;
              traces.push_back(thisTrace);
            }
            break;
          case RowDescriptor::Type::VoltageInductor:
            if (mObj.components.voltInd.at(dev.index).posNCol == -1) {
              thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                  mObj.relXInd.begin(),
                  std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                            mObj.components.voltInd.at(dev.index).negNCol)));
              traces.push_back(thisTrace);
            } else if (mObj.components.voltInd.at(dev.index).negNCol == -1) {
              thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                  mObj.relXInd.begin(),
                  std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                            mObj.components.voltInd.at(dev.index).posNCol)));
              traces.push_back(thisTrace);
            } else {
              std::transform(
                  sObj.results.xVect
                      .at(std::distance(
                          mObj.relXInd.begin(),
                          std::find(
                              mObj.relXInd.begin(), mObj.relXInd.end(),
                              mObj.components.voltInd.at(dev.index).posNCol)))
                      .begin(),
                  sObj.results.xVect
                      .at(std::distance(
                          mObj.relXInd.begin(),
                          std::find(
                              mObj.relXInd.begin(), mObj.relXInd.end(),
                              mObj.components.voltInd.at(dev.index).posNCol)))
                      .end(),
                  sObj.results.xVect
                      .at(std::distance(
                          mObj.relXInd.begin(),
                          std::find(
                              mObj.relXInd.begin(), mObj.relXInd.end(),
                              mObj.components.voltInd.at(dev.index).negNCol)))
                      .begin(),
                  std::back_inserter(thisTrace.calcData), std::minus<double>());
              thisTrace.pointer = false;
              traces.push_back(thisTrace);
            }
            break;
          case RowDescriptor::Type::PhaseInductor:
            Errors::control_errors(VOLT_WHEN_PHASE, label);
            if (mObj.components.phaseInd.at(dev.index).posNCol == -1) {
              thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                  mObj.relXInd.begin(),
                  std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                            mObj.components.phaseInd.at(dev.index).negNCol)));
              traces.push_back(thisTrace);
            } else if (mObj.components.phaseInd.at(dev.index).negNCol == -1) {
              thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                  mObj.relXInd.begin(),
                  std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                            mObj.components.phaseInd.at(dev.index).posNCol)));
              traces.push_back(thisTrace);
            } else {
              std::transform(
                  sObj.results.xVect
                      .at(std::distance(
                          mObj.relXInd.begin(),
                          std::find(
                              mObj.relXInd.begin(), mObj.relXInd.end(),
                              mObj.components.phaseInd.at(dev.index).posNCol)))
                      .begin(),
                  sObj.results.xVect
                      .at(std::distance(
                          mObj.relXInd.begin(),
                          std::find(
                              mObj.relXInd.begin(), mObj.relXInd.end(),
                              mObj.components.phaseInd.at(dev.index).posNCol)))
                      .end(),
                  sObj.results.xVect
                      .at(std::distance(
                          mObj.relXInd.begin(),
                          std::find(
                              mObj.relXInd.begin(), mObj.relXInd.end(),
                              mObj.components.phaseInd.at(dev.index).negNCol)))
                      .begin(),
                  std::back_inserter(thisTrace.calcData), std::minus<double>());
              thisTrace.pointer = false;
              traces.push_back(thisTrace);
            }
            break;
          case RowDescriptor::Type::VoltageCapacitor:
            if (mObj.components.voltCap.at(dev.index).posNCol == -1) {
              thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                  mObj.relXInd.begin(),
                  std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                            mObj.components.voltCap.at(dev.index).negNCol)));
              traces.push_back(thisTrace);
            } else if (mObj.components.voltCap.at(dev.index).negNCol == -1) {
              thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                  mObj.relXInd.begin(),
                  std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                            mObj.components.voltCap.at(dev.index).posNCol)));
              traces.push_back(thisTrace);
            } else {
              std::transform(
                  sObj.results.xVect
                      .at(std::distance(
                          mObj.relXInd.begin(),
                          std::find(
                              mObj.relXInd.begin(), mObj.relXInd.end(),
                              mObj.components.voltCap.at(dev.index).posNCol)))
                      .begin(),
                  sObj.results.xVect
                      .at(std::distance(
                          mObj.relXInd.begin(),
                          std::find(
                              mObj.relXInd.begin(), mObj.relXInd.end(),
                              mObj.components.voltCap.at(dev.index).posNCol)))
                      .end(),
                  sObj.results.xVect
                      .at(std::distance(
                          mObj.relXInd.begin(),
                          std::find(
                              mObj.relXInd.begin(), mObj.relXInd.end(),
                              mObj.components.voltCap.at(dev.index).negNCol)))
                      .begin(),
                  std::back_inserter(thisTrace.calcData), std::minus<double>());
              thisTrace.pointer = false;
              traces.push_back(thisTrace);
            }
            break;
          case RowDescriptor::Type::PhaseCapacitor:
            Errors::control_errors(VOLT_WHEN_PHASE, label);
            if (mObj.components.phaseCap.at(dev.index).posNCol == -1) {
              thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                  mObj.relXInd.begin(),
                  std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                            mObj.components.phaseCap.at(dev.index).negNCol)));
              traces.push_back(thisTrace);
            } else if (mObj.components.phaseCap.at(dev.index).negNCol == -1) {
              thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                  mObj.relXInd.begin(),
                  std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                            mObj.components.phaseCap.at(dev.index).posNCol)));
              traces.push_back(thisTrace);
            } else {
              std::transform(
                  sObj.results.xVect
                      .at(std::distance(
                          mObj.relXInd.begin(),
                          std::find(
                              mObj.relXInd.begin(), mObj.relXInd.end(),
                              mObj.components.phaseCap.at(dev.index).posNCol)))
                      .begin(),
                  sObj.results.xVect
                      .at(std::distance(
                          mObj.relXInd.begin(),
                          std::find(
                              mObj.relXInd.begin(), mObj.relXInd.end(),
                              mObj.components.phaseCap.at(dev.index).posNCol)))
                      .end(),
                  sObj.results.xVect
                      .at(std::distance(
                          mObj.relXInd.begin(),
                          std::find(
                              mObj.relXInd.begin(), mObj.relXInd.end(),
                              mObj.components.phaseCap.at(dev.index).negNCol)))
                      .begin(),
                  std::back_inserter(thisTrace.calcData), std::minus<double>());
              thisTrace.pointer = false;
              traces.push_back(thisTrace);
            }
            break;
          case RowDescriptor::Type::VoltageJJ: {
            auto &voltjj = mObj.components.voltJJ.at(dev.index);

            if (voltjj.posNCol == -1) {
              thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                  mObj.relXInd.begin(),
                  std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                            mObj.components.voltJJ.at(dev.index).negNCol)));
              traces.push_back(thisTrace);
            } else if (voltjj.negNCol == -1) {
              thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                  mObj.relXInd.begin(),
                  std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                            mObj.components.voltJJ.at(dev.index).posNCol)));
              traces.push_back(thisTrace);
            } else {
              auto posncol_iterator = std::find(
                  mObj.relXInd.begin(), mObj.relXInd.end(), voltjj.posNCol);
              auto negncol_iterator = std::find(
                  mObj.relXInd.begin(), mObj.relXInd.end(), voltjj.negNCol);

              assert(posncol_iterator != mObj.relXInd.end());
              assert(negncol_iterator != mObj.relXInd.end());

              auto index_posncol =
                  std::distance(mObj.relXInd.begin(), posncol_iterator);
              auto index_negncol =
                  std::distance(mObj.relXInd.begin(), negncol_iterator);

              auto &vector_posncol = sObj.results.xVect.at(index_posncol);
              auto &vector_negncol = sObj.results.xVect.at(index_negncol);

              std::transform(vector_posncol.begin(), vector_posncol.end(),
                             vector_negncol.begin(),
                             std::back_inserter(thisTrace.calcData),
                             std::minus<double>());

              thisTrace.pointer = false;
              traces.push_back(thisTrace);
            }
          } break;
          case RowDescriptor::Type::PhaseJJ:
            Errors::control_errors(VOLT_WHEN_PHASE, label);
            if (mObj.components.phaseJJ.at(dev.index).posNCol == -1) {
              thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                  mObj.relXInd.begin(),
                  std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                            mObj.components.phaseJJ.at(dev.index).negNCol)));
              traces.push_back(thisTrace);
            } else if (mObj.components.phaseJJ.at(dev.index).negNCol == -1) {
              thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                  mObj.relXInd.begin(),
                  std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                            mObj.components.phaseJJ.at(dev.index).posNCol)));
              traces.push_back(thisTrace);
            } else {
              std::transform(
                  sObj.results.xVect
                      .at(std::distance(
                          mObj.relXInd.begin(),
                          std::find(
                              mObj.relXInd.begin(), mObj.relXInd.end(),
                              mObj.components.phaseJJ.at(dev.index).posNCol)))
                      .begin(),
                  sObj.results.xVect
                      .at(std::distance(
                          mObj.relXInd.begin(),
                          std::find(
                              mObj.relXInd.begin(), mObj.relXInd.end(),
                              mObj.components.phaseJJ.at(dev.index).posNCol)))
                      .end(),
                  sObj.results.xVect
                      .at(std::distance(
                          mObj.relXInd.begin(),
                          std::find(
                              mObj.relXInd.begin(), mObj.relXInd.end(),
                              mObj.components.phaseJJ.at(dev.index).negNCol)))
                      .begin(),
                  std::back_inserter(thisTrace.calcData), std::minus<double>());
              thisTrace.pointer = false;
              traces.push_back(thisTrace);
            }
            break;
          case RowDescriptor::Type::VoltageVS:
            thisTrace.traceData = &mObj.sources.at(dev.index);
            traces.push_back(thisTrace);
            break;
          case RowDescriptor::Type::PhaseVS:
            thisTrace.traceData = &mObj.sources.at(dev.index);
            traces.push_back(thisTrace);
            break;
          case RowDescriptor::Type::VoltageCS:
            Errors::control_errors(VOLT_ACROSS_CURRENT, i);
            break;
          case RowDescriptor::Type::PhaseCS:
            Errors::control_errors(VOLT_ACROSS_CURRENT, i);
            break;
          default:
            Errors::control_errors(UNKNOWN_DEVICE, label);
            break;
          }
        } else
          Errors::control_errors(UNKNOWN_DEVICE, label);
      } else if (tokens.at(1) == "NODEV") {
        thisTrace.type = 'V';
        if (tokens.size() > 4) {
          Errors::control_errors(PRINT_TOO_MANY_ARGS, i);
        } else if (tokens.size() == 3) {
          label = tokens.at(2);
          if (label.find('_') != std::string::npos) {
            tokens = Misc::tokenize_delimeter(label, "_");
            label = tokens.back();
            for (int j = 0; j < tokens.size() - 1; j++)
              label += "|" + tokens.at(j);
          } else if (label.find('.') != std::string::npos) {
            std::replace(label.begin(), label.end(), '.', '|');
          }
          if (mObj.deviceLabelIndex.count(label) != 0) {
            index1 = mObj.deviceLabelIndex.at(label).index;
            if (iObj.argAnal == JoSIM::AnalysisType::Phase) {
              Errors::control_errors(NODEVOLT_WHEN_PHASE, label);
              thisTrace.type = 'P';
              thisTrace.name = "NP_" + label;
            } else
              thisTrace.name = "NV_" + label;
            thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                mObj.relXInd.begin(),
                std::find(mObj.relXInd.begin(), mObj.relXInd.end(), index1)));
            traces.push_back(thisTrace);
          } else
            Errors::control_errors(UNKNOWN_NODE, label);
        } else if (tokens.size() == 4) {
          label = tokens.at(2);
          label2 = tokens.at(3);
          if (label.find('_') != std::string::npos) {
            tokens = Misc::tokenize_delimeter(label, "_");
            label = tokens.back();
            for (int j = 0; j < tokens.size() - 1; j++)
              label += "|" + tokens.at(j);
          } else if (label.find('.') != std::string::npos) {
            std::replace(label.begin(), label.end(), '.', '|');
          }
          if (label2.find('_') != std::string::npos) {
            tokens = Misc::tokenize_delimeter(label2, "_");
            label2 = tokens.back();
            for (int j = 0; j < tokens.size() - 1; j++)
              label2 = label + "|" + tokens.at(j);
          } else if (label2.find('.') != std::string::npos) {
            std::replace(label2.begin(), label2.end(), '.', '|');
          }
          if (label == "0" || label == "GND") {
            if (mObj.deviceLabelIndex.count(label2) != 0) {
              index2 = mObj.deviceLabelIndex.at(label2).index;
              if (iObj.argAnal == JoSIM::AnalysisType::Phase) {
                Errors::control_errors(NODEVOLT_WHEN_PHASE, label2);
                thisTrace.type = 'P';
                thisTrace.name = "NP_" + label2;
              } else
                thisTrace.name = "NV_" + label2;
              thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                  mObj.relXInd.begin(),
                  std::find(mObj.relXInd.begin(), mObj.relXInd.end(), index2)));
              traces.push_back(thisTrace);
            } else
              Errors::control_errors(UNKNOWN_NODE, label2);
          } else if (label2 == "0" || label2 == "GND") {
            if (mObj.deviceLabelIndex.count(label) != 0) {
              index1 = mObj.deviceLabelIndex.at(label).index;
              if (iObj.argAnal == JoSIM::AnalysisType::Phase) {
                Errors::control_errors(NODEVOLT_WHEN_PHASE, label);
                thisTrace.type = 'P';
                thisTrace.name = "NP_" + label;
              } else
                thisTrace.name = "NV_" + label;
              thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                  mObj.relXInd.begin(),
                  std::find(mObj.relXInd.begin(), mObj.relXInd.end(), index1)));
              traces.push_back(thisTrace);
            } else
              Errors::control_errors(UNKNOWN_NODE, label);
          } else {
            if (mObj.deviceLabelIndex.count(label2) != 0) {
              if (mObj.deviceLabelIndex.count(label) != 0) {
                if (iObj.argAnal == JoSIM::AnalysisType::Phase) {
                  Errors::control_errors(NODEVOLT_WHEN_PHASE, label);
                  thisTrace.type = 'P';
                  thisTrace.name = "NP_";
                  thisTrace.name += label;
                  thisTrace.name += "_";
                  thisTrace.name += label2;
                } else
                  thisTrace.name = "NV_";
                  thisTrace.name += label;
                  thisTrace.name += "_"; 
                  thisTrace.name += label2;
                std::transform(sObj.results.xVect
                                   .at(std::distance(
                                       mObj.relXInd.begin(),
                                       std::find(mObj.relXInd.begin(),
                                                 mObj.relXInd.end(), index1)))
                                   .begin(),
                               sObj.results.xVect
                                   .at(std::distance(
                                       mObj.relXInd.begin(),
                                       std::find(mObj.relXInd.begin(),
                                                 mObj.relXInd.end(), index1)))
                                   .end(),
                               sObj.results.xVect
                                   .at(std::distance(
                                       mObj.relXInd.begin(),
                                       std::find(mObj.relXInd.begin(),
                                                 mObj.relXInd.end(), index2)))
                                   .begin(),
                               std::back_inserter(thisTrace.calcData),
                               std::minus<double>());
                thisTrace.pointer = false;
                traces.push_back(thisTrace);
              } else
                Errors::control_errors(UNKNOWN_NODE, label);
            } else
              Errors::control_errors(UNKNOWN_NODE, label2);
          }
        } else
          Errors::control_errors(INVALID_NODEV, i);
      } else if (tokens.at(1) == "NODEP") {
        thisTrace.type = 'P';
        if (tokens.size() > 4) {
          Errors::control_errors(PRINT_TOO_MANY_ARGS, i);
        } else if (tokens.size() == 3) {
          label = tokens.at(2);
          if (label.find('_') != std::string::npos) {
            tokens = Misc::tokenize_delimeter(label, "_");
            label = tokens.back();
            for (int j = 0; j < tokens.size() - 1; j++)
              label += "|" + tokens.at(j);
          } else if (label.find('.') != std::string::npos) {
            std::replace(label.begin(), label.end(), '.', '|');
          }
          if (mObj.deviceLabelIndex.count(label) != 0) {
            index1 = mObj.deviceLabelIndex.at(label).index;
            if (iObj.argAnal == JoSIM::AnalysisType::Voltage) {
              Errors::control_errors(NODEPHASE_WHEN_VOLT, label);
              thisTrace.type = 'V';
              thisTrace.name = "NV_" + label;
            } else
              thisTrace.name = "NP_" + label;
            thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                mObj.relXInd.begin(),
                std::find(mObj.relXInd.begin(), mObj.relXInd.end(), index1)));
            traces.push_back(thisTrace);
          } else
            Errors::control_errors(UNKNOWN_NODE, label);
        } else if (tokens.size() == 4) {
          label = tokens.at(2);
          label2 = tokens.at(3);
          if (label.find('_') != std::string::npos) {
            tokens = Misc::tokenize_delimeter(label, "_");
            label = tokens.back();
            for (int j = 0; j < tokens.size() - 1; j++)
              label += "|" + tokens.at(j);
          } else if (label.find('.') != std::string::npos) {
            std::replace(label.begin(), label.end(), '.', '|');
          }
          if (label2.find('_') != std::string::npos) {
            tokens = Misc::tokenize_delimeter(label2, "_");
            label2 = tokens.back();
            for (int j = 0; j < tokens.size() - 1; j++)
              label2 = label + "|" + tokens.at(j);
          } else if (label2.find('.') != std::string::npos) {
            std::replace(label2.begin(), label2.end(), '.', '|');
          }
          if (label == "0" || label == "GND") {
            if (mObj.deviceLabelIndex.count(label2) != 0) {
              index2 = mObj.deviceLabelIndex.at(label2).index;
              if (iObj.argAnal == JoSIM::AnalysisType::Voltage) {
                Errors::control_errors(NODEPHASE_WHEN_VOLT, label2);
                thisTrace.type = 'V';
                thisTrace.name = "NV_" + label2;
              } else
                thisTrace.name = "NP_" + label2;
              thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                  mObj.relXInd.begin(),
                  std::find(mObj.relXInd.begin(), mObj.relXInd.end(), index2)));
              traces.push_back(thisTrace);
            } else
              Errors::control_errors(UNKNOWN_NODE, label2);
          } else if (label2 == "0" || label2 == "GND") {
            if (mObj.deviceLabelIndex.count(label) != 0) {
              index1 = mObj.deviceLabelIndex.at(label).index;
              if (iObj.argAnal == JoSIM::AnalysisType::Voltage) {
                Errors::control_errors(NODEPHASE_WHEN_VOLT, label);
                thisTrace.type = 'V';
                thisTrace.name = "NV_";
                thisTrace.name += label;
              } else
                thisTrace.name = "NP_";
                thisTrace.name += label;
              thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                  mObj.relXInd.begin(),
                  std::find(mObj.relXInd.begin(), mObj.relXInd.end(), index1)));
              traces.push_back(thisTrace);
            } else
              Errors::control_errors(UNKNOWN_NODE, label);
          } else {
            if (mObj.deviceLabelIndex.count(label2) != 0) {
              if (mObj.deviceLabelIndex.count(label) != 0) {
                if (iObj.argAnal == JoSIM::AnalysisType::Voltage) {
                  Errors::control_errors(NODEPHASE_WHEN_VOLT, label);
                  thisTrace.type = 'V';
                  thisTrace.name = "NV_";
                  thisTrace.name += label;
                  thisTrace.name += "_";
                  thisTrace.name += label2;
                } else
                  thisTrace.name = "NP_";
                  thisTrace.name += label;
                  thisTrace.name += "_";
                  thisTrace.name += label2;
                std::transform(sObj.results.xVect
                                   .at(std::distance(
                                       mObj.relXInd.begin(),
                                       std::find(mObj.relXInd.begin(),
                                                 mObj.relXInd.end(), index1)))
                                   .begin(),
                               sObj.results.xVect
                                   .at(std::distance(
                                       mObj.relXInd.begin(),
                                       std::find(mObj.relXInd.begin(),
                                                 mObj.relXInd.end(), index1)))
                                   .end(),
                               sObj.results.xVect
                                   .at(std::distance(
                                       mObj.relXInd.begin(),
                                       std::find(mObj.relXInd.begin(),
                                                 mObj.relXInd.end(), index2)))
                                   .begin(),
                               std::back_inserter(thisTrace.calcData),
                               std::minus<double>());
                thisTrace.pointer = false;
                traces.push_back(thisTrace);
              } else
                Errors::control_errors(UNKNOWN_NODE, label);
            } else
              Errors::control_errors(UNKNOWN_NODE, label2);
          }
        } else
          Errors::control_errors(INVALID_NODEV, i);
      } else if (tokens.at(1) == "PHASE" || tokens.at(1) == "DEVP") {
        if (tokens.size() > 4)
          Errors::control_errors(PRINT_TOO_MANY_ARGS, i);
        label = tokens.at(2);
        if (label.find('_') != std::string::npos) {
          tokens = Misc::tokenize_delimeter(label, "_");
          label = tokens.back();
          for (int j = 0; j < tokens.size() - 1; j++)
            label += "|" + tokens.at(j);
        } else if (label.find('.') != std::string::npos) {
          std::replace(label.begin(), label.end(), '.', '|');
        }
        thisTrace.name = label;
        thisTrace.type = 'P';
        if (mObj.deviceLabelIndex.count(label) != 0) {
          const auto &dev = mObj.deviceLabelIndex.at(label);
          switch (dev.type) {
          case RowDescriptor::Type::VoltageResistor:
            Errors::control_errors(PHASE_WHEN_VOLT, i);
            break;
          case RowDescriptor::Type::PhaseResistor:
            if (mObj.components.phaseRes.at(dev.index).posNCol == -1) {
              thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                  mObj.relXInd.begin(),
                  std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                            mObj.components.phaseRes.at(dev.index).negNCol)));
              traces.push_back(thisTrace);
            } else if (mObj.components.phaseRes.at(dev.index).negNCol == -1) {
              thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                  mObj.relXInd.begin(),
                  std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                            mObj.components.phaseRes.at(dev.index).posNCol)));
              traces.push_back(thisTrace);
            } else {
              std::transform(
                  sObj.results.xVect
                      .at(std::distance(
                          mObj.relXInd.begin(),
                          std::find(
                              mObj.relXInd.begin(), mObj.relXInd.end(),
                              mObj.components.phaseRes.at(dev.index).posNCol)))
                      .begin(),
                  sObj.results.xVect
                      .at(std::distance(
                          mObj.relXInd.begin(),
                          std::find(
                              mObj.relXInd.begin(), mObj.relXInd.end(),
                              mObj.components.phaseRes.at(dev.index).posNCol)))
                      .end(),
                  sObj.results.xVect
                      .at(std::distance(
                          mObj.relXInd.begin(),
                          std::find(
                              mObj.relXInd.begin(), mObj.relXInd.end(),
                              mObj.components.phaseRes.at(dev.index).negNCol)))
                      .begin(),
                  std::back_inserter(thisTrace.calcData), std::minus<double>());
              thisTrace.pointer = false;
              traces.push_back(thisTrace);
            }
            break;
          case RowDescriptor::Type::VoltageInductor:
            Errors::control_errors(PHASE_WHEN_VOLT, i);
            break;
          case RowDescriptor::Type::PhaseInductor:
            if (mObj.components.phaseInd.at(dev.index).posNCol == -1) {
              thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                  mObj.relXInd.begin(),
                  std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                            mObj.components.phaseInd.at(dev.index).negNCol)));
              traces.push_back(thisTrace);
            } else if (mObj.components.phaseInd.at(dev.index).negNCol == -1) {
              thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                  mObj.relXInd.begin(),
                  std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                            mObj.components.phaseInd.at(dev.index).posNCol)));
              traces.push_back(thisTrace);
            } else {
              std::transform(
                  sObj.results.xVect
                      .at(std::distance(
                          mObj.relXInd.begin(),
                          std::find(
                              mObj.relXInd.begin(), mObj.relXInd.end(),
                              mObj.components.phaseInd.at(dev.index).posNCol)))
                      .begin(),
                  sObj.results.xVect
                      .at(std::distance(
                          mObj.relXInd.begin(),
                          std::find(
                              mObj.relXInd.begin(), mObj.relXInd.end(),
                              mObj.components.phaseInd.at(dev.index).posNCol)))
                      .end(),
                  sObj.results.xVect
                      .at(std::distance(
                          mObj.relXInd.begin(),
                          std::find(
                              mObj.relXInd.begin(), mObj.relXInd.end(),
                              mObj.components.phaseInd.at(dev.index).negNCol)))
                      .begin(),
                  std::back_inserter(thisTrace.calcData), std::minus<double>());
              thisTrace.pointer = false;
              traces.push_back(thisTrace);
            }
            break;
          case RowDescriptor::Type::VoltageCapacitor:
            Errors::control_errors(PHASE_WHEN_VOLT, i);
            break;
          case RowDescriptor::Type::PhaseCapacitor:
            if (mObj.components.phaseCap.at(dev.index).posNCol == -1) {
              thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                  mObj.relXInd.begin(),
                  std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                            mObj.components.phaseCap.at(dev.index).negNCol)));
              traces.push_back(thisTrace);
            } else if (mObj.components.phaseCap.at(dev.index).negNCol == -1) {
              thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                  mObj.relXInd.begin(),
                  std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                            mObj.components.phaseCap.at(dev.index).posNCol)));
              traces.push_back(thisTrace);
            } else {
              std::transform(
                  sObj.results.xVect
                      .at(std::distance(
                          mObj.relXInd.begin(),
                          std::find(
                              mObj.relXInd.begin(), mObj.relXInd.end(),
                              mObj.components.phaseCap.at(dev.index).posNCol)))
                      .begin(),
                  sObj.results.xVect
                      .at(std::distance(
                          mObj.relXInd.begin(),
                          std::find(
                              mObj.relXInd.begin(), mObj.relXInd.end(),
                              mObj.components.phaseCap.at(dev.index).posNCol)))
                      .end(),
                  sObj.results.xVect
                      .at(std::distance(
                          mObj.relXInd.begin(),
                          std::find(
                              mObj.relXInd.begin(), mObj.relXInd.end(),
                              mObj.components.phaseCap.at(dev.index).negNCol)))
                      .begin(),
                  std::back_inserter(thisTrace.calcData), std::minus<double>());
              thisTrace.pointer = false;
              traces.push_back(thisTrace);
            }
            break;
          case RowDescriptor::Type::VoltageJJ:
            thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                mObj.relXInd.begin(),
                std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                          mObj.components.voltJJ.at(dev.index).phaseNCol)));
            traces.push_back(thisTrace);
            break;
          case RowDescriptor::Type::PhaseJJ:
            if (mObj.components.phaseJJ.at(dev.index).posNCol == -1) {
              thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                  mObj.relXInd.begin(),
                  std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                            mObj.components.phaseJJ.at(dev.index).negNCol)));
              traces.push_back(thisTrace);
            } else if (mObj.components.phaseJJ.at(dev.index).negNCol == -1) {
              thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                  mObj.relXInd.begin(),
                  std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                            mObj.components.phaseJJ.at(dev.index).posNCol)));
              traces.push_back(thisTrace);
            } else {
              std::transform(
                  sObj.results.xVect
                      .at(std::distance(
                          mObj.relXInd.begin(),
                          std::find(
                              mObj.relXInd.begin(), mObj.relXInd.end(),
                              mObj.components.phaseJJ.at(dev.index).posNCol)))
                      .begin(),
                  sObj.results.xVect
                      .at(std::distance(
                          mObj.relXInd.begin(),
                          std::find(
                              mObj.relXInd.begin(), mObj.relXInd.end(),
                              mObj.components.phaseJJ.at(dev.index).posNCol)))
                      .end(),
                  sObj.results.xVect
                      .at(std::distance(
                          mObj.relXInd.begin(),
                          std::find(
                              mObj.relXInd.begin(), mObj.relXInd.end(),
                              mObj.components.phaseJJ.at(dev.index).negNCol)))
                      .begin(),
                  std::back_inserter(thisTrace.calcData), std::minus<double>());
              thisTrace.pointer = false;
              traces.push_back(thisTrace);
            }
            break;
          case RowDescriptor::Type::VoltageVS:
            Errors::control_errors(PHASE_OF_VOLT, i);
            break;
          case RowDescriptor::Type::PhaseVS:
            Errors::control_errors(PHASE_OF_VOLT, i);
            break;
          case RowDescriptor::Type::PhasePS:
            thisTrace.traceData = &mObj.sources.at(dev.index);
            traces.push_back(thisTrace);
            break;
          case RowDescriptor::Type::VoltageCS:
            Errors::control_errors(PHASE_OF_CURRENT, i);
            break;
          case RowDescriptor::Type::PhaseCS:
            Errors::control_errors(PHASE_OF_CURRENT, i);
            break;
          default:
            Errors::control_errors(UNKNOWN_DEVICE, label);
            break;
          }
        } else
          Errors::control_errors(UNKNOWN_DEVICE, label);
      }
    } else if (i.find("PLOT") != std::string::npos) {
      tokens = Misc::tokenize_space(i);
      for (int j = 1; j < tokens.size(); j++) {
        Trace thisTrace;
        if (tokens.at(j)[0] == 'V') {
          thisTrace.type = 'V';
          tokens2 = Misc::tokenize_delimeter(tokens.at(j), "V() ,");
          if (tokens2.size() == 1) {
            label = tokens2.at(0);
            if (label.find('_') != std::string::npos) {
              tokens2 = Misc::tokenize_delimeter(label, "_");
              label = tokens2.back();
              for (int k = 0; k < tokens2.size() - 1; k++)
                label += "|" + tokens2.at(k);
            } else if (label.find('.') != std::string::npos) {
              std::replace(label.begin(), label.end(), '.', '|');
            }
            thisTrace.name = label;
            if (mObj.deviceLabelIndex.count(label) != 0) {
              const auto &dev = mObj.deviceLabelIndex.at(label);
              switch (dev.type) {
              case RowDescriptor::Type::VoltageResistor:
                if (mObj.components.voltRes.at(dev.index).posNCol == -1) {
                  thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                      mObj.relXInd.begin(),
                      std::find(
                          mObj.relXInd.begin(), mObj.relXInd.end(),
                          mObj.components.voltRes.at(dev.index).negNCol)));
                  traces.push_back(thisTrace);
                } else if (mObj.components.voltRes.at(dev.index).negNCol ==
                           -1) {
                  thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                      mObj.relXInd.begin(),
                      std::find(
                          mObj.relXInd.begin(), mObj.relXInd.end(),
                          mObj.components.voltRes.at(dev.index).posNCol)));
                  traces.push_back(thisTrace);
                } else {
                  std::transform(
                      sObj.results.xVect
                          .at(std::distance(
                              mObj.relXInd.begin(),
                              std::find(mObj.relXInd.begin(),
                                        mObj.relXInd.end(),
                                        mObj.components.voltRes.at(dev.index)
                                            .posNCol)))
                          .begin(),
                      sObj.results.xVect
                          .at(std::distance(
                              mObj.relXInd.begin(),
                              std::find(mObj.relXInd.begin(),
                                        mObj.relXInd.end(),
                                        mObj.components.voltRes.at(dev.index)
                                            .posNCol)))
                          .end(),
                      sObj.results.xVect
                          .at(std::distance(
                              mObj.relXInd.begin(),
                              std::find(mObj.relXInd.begin(),
                                        mObj.relXInd.end(),
                                        mObj.components.voltRes.at(dev.index)
                                            .negNCol)))
                          .begin(),
                      std::back_inserter(thisTrace.calcData),
                      std::minus<double>());
                  thisTrace.pointer = false;
                  traces.push_back(thisTrace);
                }
                break;
              case RowDescriptor::Type::PhaseResistor:
                Errors::control_errors(VOLT_WHEN_PHASE, label);
                if (mObj.components.phaseRes.at(dev.index).posNCol == -1) {
                  thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                      mObj.relXInd.begin(),
                      std::find(
                          mObj.relXInd.begin(), mObj.relXInd.end(),
                          mObj.components.phaseRes.at(dev.index).negNCol)));
                  traces.push_back(thisTrace);
                } else if (mObj.components.phaseRes.at(dev.index).negNCol ==
                           -1) {
                  thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                      mObj.relXInd.begin(),
                      std::find(
                          mObj.relXInd.begin(), mObj.relXInd.end(),
                          mObj.components.phaseRes.at(dev.index).posNCol)));
                  traces.push_back(thisTrace);
                } else {
                  std::transform(
                      sObj.results.xVect
                          .at(std::distance(
                              mObj.relXInd.begin(),
                              std::find(mObj.relXInd.begin(),
                                        mObj.relXInd.end(),
                                        mObj.components.phaseRes.at(dev.index)
                                            .posNCol)))
                          .begin(),
                      sObj.results.xVect
                          .at(std::distance(
                              mObj.relXInd.begin(),
                              std::find(mObj.relXInd.begin(),
                                        mObj.relXInd.end(),
                                        mObj.components.phaseRes.at(dev.index)
                                            .posNCol)))
                          .end(),
                      sObj.results.xVect
                          .at(std::distance(
                              mObj.relXInd.begin(),
                              std::find(mObj.relXInd.begin(),
                                        mObj.relXInd.end(),
                                        mObj.components.phaseRes.at(dev.index)
                                            .negNCol)))
                          .begin(),
                      std::back_inserter(thisTrace.calcData),
                      std::minus<double>());
                  thisTrace.pointer = false;
                  traces.push_back(thisTrace);
                }
                break;
              case RowDescriptor::Type::VoltageInductor:
                if (mObj.components.voltInd.at(dev.index).posNCol == -1) {
                  thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                      mObj.relXInd.begin(),
                      std::find(
                          mObj.relXInd.begin(), mObj.relXInd.end(),
                          mObj.components.voltInd.at(dev.index).negNCol)));
                  traces.push_back(thisTrace);
                } else if (mObj.components.voltInd.at(dev.index).negNCol ==
                           -1) {
                  thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                      mObj.relXInd.begin(),
                      std::find(
                          mObj.relXInd.begin(), mObj.relXInd.end(),
                          mObj.components.voltInd.at(dev.index).posNCol)));
                  traces.push_back(thisTrace);
                } else {
                  std::transform(
                      sObj.results.xVect
                          .at(std::distance(
                              mObj.relXInd.begin(),
                              std::find(mObj.relXInd.begin(),
                                        mObj.relXInd.end(),
                                        mObj.components.voltInd.at(dev.index)
                                            .posNCol)))
                          .begin(),
                      sObj.results.xVect
                          .at(std::distance(
                              mObj.relXInd.begin(),
                              std::find(mObj.relXInd.begin(),
                                        mObj.relXInd.end(),
                                        mObj.components.voltInd.at(dev.index)
                                            .posNCol)))
                          .end(),
                      sObj.results.xVect
                          .at(std::distance(
                              mObj.relXInd.begin(),
                              std::find(mObj.relXInd.begin(),
                                        mObj.relXInd.end(),
                                        mObj.components.voltInd.at(dev.index)
                                            .negNCol)))
                          .begin(),
                      std::back_inserter(thisTrace.calcData),
                      std::minus<double>());
                  thisTrace.pointer = false;
                  traces.push_back(thisTrace);
                }
                break;
              case RowDescriptor::Type::PhaseInductor:
                Errors::control_errors(VOLT_WHEN_PHASE, label);
                if (mObj.components.phaseInd.at(dev.index).posNCol == -1) {
                  thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                      mObj.relXInd.begin(),
                      std::find(
                          mObj.relXInd.begin(), mObj.relXInd.end(),
                          mObj.components.phaseInd.at(dev.index).negNCol)));
                  traces.push_back(thisTrace);
                } else if (mObj.components.phaseInd.at(dev.index).negNCol ==
                           -1) {
                  thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                      mObj.relXInd.begin(),
                      std::find(
                          mObj.relXInd.begin(), mObj.relXInd.end(),
                          mObj.components.phaseInd.at(dev.index).posNCol)));
                  traces.push_back(thisTrace);
                } else {
                  std::transform(
                      sObj.results.xVect
                          .at(std::distance(
                              mObj.relXInd.begin(),
                              std::find(mObj.relXInd.begin(),
                                        mObj.relXInd.end(),
                                        mObj.components.phaseInd.at(dev.index)
                                            .posNCol)))
                          .begin(),
                      sObj.results.xVect
                          .at(std::distance(
                              mObj.relXInd.begin(),
                              std::find(mObj.relXInd.begin(),
                                        mObj.relXInd.end(),
                                        mObj.components.phaseInd.at(dev.index)
                                            .posNCol)))
                          .end(),
                      sObj.results.xVect
                          .at(std::distance(
                              mObj.relXInd.begin(),
                              std::find(mObj.relXInd.begin(),
                                        mObj.relXInd.end(),
                                        mObj.components.phaseInd.at(dev.index)
                                            .negNCol)))
                          .begin(),
                      std::back_inserter(thisTrace.calcData),
                      std::minus<double>());
                  thisTrace.pointer = false;
                  traces.push_back(thisTrace);
                }
                break;
              case RowDescriptor::Type::VoltageCapacitor:
                if (mObj.components.voltCap.at(dev.index).posNCol == -1) {
                  thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                      mObj.relXInd.begin(),
                      std::find(
                          mObj.relXInd.begin(), mObj.relXInd.end(),
                          mObj.components.voltCap.at(dev.index).negNCol)));
                  traces.push_back(thisTrace);
                } else if (mObj.components.voltCap.at(dev.index).negNCol ==
                           -1) {
                  thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                      mObj.relXInd.begin(),
                      std::find(
                          mObj.relXInd.begin(), mObj.relXInd.end(),
                          mObj.components.voltCap.at(dev.index).posNCol)));
                  traces.push_back(thisTrace);
                } else {
                  std::transform(
                      sObj.results.xVect
                          .at(std::distance(
                              mObj.relXInd.begin(),
                              std::find(mObj.relXInd.begin(),
                                        mObj.relXInd.end(),
                                        mObj.components.voltCap.at(dev.index)
                                            .posNCol)))
                          .begin(),
                      sObj.results.xVect
                          .at(std::distance(
                              mObj.relXInd.begin(),
                              std::find(mObj.relXInd.begin(),
                                        mObj.relXInd.end(),
                                        mObj.components.voltCap.at(dev.index)
                                            .posNCol)))
                          .end(),
                      sObj.results.xVect
                          .at(std::distance(
                              mObj.relXInd.begin(),
                              std::find(mObj.relXInd.begin(),
                                        mObj.relXInd.end(),
                                        mObj.components.voltCap.at(dev.index)
                                            .negNCol)))
                          .begin(),
                      std::back_inserter(thisTrace.calcData),
                      std::minus<double>());
                  thisTrace.pointer = false;
                  traces.push_back(thisTrace);
                }
                break;
              case RowDescriptor::Type::PhaseCapacitor:
                Errors::control_errors(VOLT_WHEN_PHASE, label);
                if (mObj.components.phaseCap.at(dev.index).posNCol == -1) {
                  thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                      mObj.relXInd.begin(),
                      std::find(
                          mObj.relXInd.begin(), mObj.relXInd.end(),
                          mObj.components.phaseCap.at(dev.index).negNCol)));
                  traces.push_back(thisTrace);
                } else if (mObj.components.phaseCap.at(dev.index).negNCol ==
                           -1) {
                  thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                      mObj.relXInd.begin(),
                      std::find(
                          mObj.relXInd.begin(), mObj.relXInd.end(),
                          mObj.components.phaseCap.at(dev.index).posNCol)));
                  traces.push_back(thisTrace);
                } else {
                  std::transform(
                      sObj.results.xVect
                          .at(std::distance(
                              mObj.relXInd.begin(),
                              std::find(mObj.relXInd.begin(),
                                        mObj.relXInd.end(),
                                        mObj.components.phaseCap.at(dev.index)
                                            .posNCol)))
                          .begin(),
                      sObj.results.xVect
                          .at(std::distance(
                              mObj.relXInd.begin(),
                              std::find(mObj.relXInd.begin(),
                                        mObj.relXInd.end(),
                                        mObj.components.phaseCap.at(dev.index)
                                            .posNCol)))
                          .end(),
                      sObj.results.xVect
                          .at(std::distance(
                              mObj.relXInd.begin(),
                              std::find(mObj.relXInd.begin(),
                                        mObj.relXInd.end(),
                                        mObj.components.phaseCap.at(dev.index)
                                            .negNCol)))
                          .begin(),
                      std::back_inserter(thisTrace.calcData),
                      std::minus<double>());
                  thisTrace.pointer = false;
                  traces.push_back(thisTrace);
                }
                break;
              case RowDescriptor::Type::VoltageJJ:
                if (mObj.components.voltJJ.at(dev.index).posNCol == -1) {
                  thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                      mObj.relXInd.begin(),
                      std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                                mObj.components.voltJJ.at(dev.index).negNCol)));
                  traces.push_back(thisTrace);
                } else if (mObj.components.voltJJ.at(dev.index).negNCol == -1) {
                  thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                      mObj.relXInd.begin(),
                      std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                                mObj.components.voltJJ.at(dev.index).posNCol)));
                  traces.push_back(thisTrace);
                } else {
                  std::transform(
                      sObj.results.xVect
                          .at(std::distance(
                              mObj.relXInd.begin(),
                              std::find(mObj.relXInd.begin(),
                                        mObj.relXInd.end(),
                                        mObj.components.voltJJ.at(dev.index)
                                            .posNCol)))
                          .begin(),
                      sObj.results.xVect
                          .at(std::distance(
                              mObj.relXInd.begin(),
                              std::find(mObj.relXInd.begin(),
                                        mObj.relXInd.end(),
                                        mObj.components.voltJJ.at(dev.index)
                                            .posNCol)))
                          .end(),
                      sObj.results.xVect
                          .at(std::distance(
                              mObj.relXInd.begin(),
                              std::find(mObj.relXInd.begin(),
                                        mObj.relXInd.end(),
                                        mObj.components.voltJJ.at(dev.index)
                                            .negNCol)))
                          .begin(),
                      std::back_inserter(thisTrace.calcData),
                      std::minus<double>());
                  thisTrace.pointer = false;
                  traces.push_back(thisTrace);
                }
                break;
              case RowDescriptor::Type::PhaseJJ:
                Errors::control_errors(VOLT_WHEN_PHASE, label);
                if (mObj.components.phaseJJ.at(dev.index).posNCol == -1) {
                  thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                      mObj.relXInd.begin(),
                      std::find(
                          mObj.relXInd.begin(), mObj.relXInd.end(),
                          mObj.components.phaseJJ.at(dev.index).negNCol)));
                  traces.push_back(thisTrace);
                } else if (mObj.components.phaseJJ.at(dev.index).negNCol ==
                           -1) {
                  thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                      mObj.relXInd.begin(),
                      std::find(
                          mObj.relXInd.begin(), mObj.relXInd.end(),
                          mObj.components.phaseJJ.at(dev.index).posNCol)));
                  traces.push_back(thisTrace);
                } else {
                  std::transform(
                      sObj.results.xVect
                          .at(std::distance(
                              mObj.relXInd.begin(),
                              std::find(mObj.relXInd.begin(),
                                        mObj.relXInd.end(),
                                        mObj.components.phaseJJ.at(dev.index)
                                            .posNCol)))
                          .begin(),
                      sObj.results.xVect
                          .at(std::distance(
                              mObj.relXInd.begin(),
                              std::find(mObj.relXInd.begin(),
                                        mObj.relXInd.end(),
                                        mObj.components.phaseJJ.at(dev.index)
                                            .posNCol)))
                          .end(),
                      sObj.results.xVect
                          .at(std::distance(
                              mObj.relXInd.begin(),
                              std::find(mObj.relXInd.begin(),
                                        mObj.relXInd.end(),
                                        mObj.components.phaseJJ.at(dev.index)
                                            .negNCol)))
                          .begin(),
                      std::back_inserter(thisTrace.calcData),
                      std::minus<double>());
                  thisTrace.pointer = false;
                  traces.push_back(thisTrace);
                }
                break;
              case RowDescriptor::Type::VoltageVS:
                thisTrace.traceData = &mObj.sources.at(dev.index);
                traces.push_back(thisTrace);
                break;
              case RowDescriptor::Type::PhaseVS:
                thisTrace.traceData = &mObj.sources.at(dev.index);
                traces.push_back(thisTrace);
                break;
              case RowDescriptor::Type::VoltageCS:
                Errors::control_errors(VOLT_ACROSS_CURRENT, i);
                break;
              case RowDescriptor::Type::PhaseCS:
                Errors::control_errors(VOLT_ACROSS_CURRENT, i);
                break;
              default:
                if (mObj.deviceLabelIndex.count(label) != 0) {
                  index1 = mObj.deviceLabelIndex.at(label).index;
                  if (iObj.argAnal == JoSIM::AnalysisType::Phase) {
                    Errors::control_errors(NODEPHASE_WHEN_VOLT, label);
                    thisTrace.type = 'P';
                    thisTrace.name = "NP_" + label;
                  } else
                    thisTrace.name = "NV_" + label;
                  thisTrace.traceData = &sObj.results.xVect.at(
                      std::distance(mObj.relXInd.begin(),
                                    std::find(mObj.relXInd.begin(),
                                              mObj.relXInd.end(), index1)));
                  traces.push_back(thisTrace);
                } else
                  Errors::control_errors(UNKNOWN_NODE, label);
                break;
              }
            } else
              Errors::control_errors(UNKNOWN_DEVICE, label);
          } else {
            label = tokens2.at(0);
            label2 = tokens2.at(1);
            if (label.find('_') != std::string::npos) {
              tokens2 = Misc::tokenize_delimeter(label, "_");
              label = tokens2.back();
              for (int k = 0; k < tokens2.size() - 1; k++)
                label += "|" + tokens2.at(k);
            } else if (label.find('.') != std::string::npos) {
              std::replace(label.begin(), label.end(), '.', '|');
            }
            if (label2.find('_') != std::string::npos) {
              tokens2 = Misc::tokenize_delimeter(label2, "_");
              label2 = tokens2.back();
              for (int k = 0; k < tokens2.size() - 1; k++)
                label2 = label + "|" + tokens2.at(k);
            } else if (label2.find('.') != std::string::npos) {
              std::replace(label2.begin(), label2.end(), '.', '|');
            }
            if (label == "0" || label == "GND") {
              if (mObj.deviceLabelIndex.count(label2) != 0) {
                index2 = mObj.deviceLabelIndex.at(label2).index;
                if (iObj.argAnal == JoSIM::AnalysisType::Phase) {
                  Errors::control_errors(NODEVOLT_WHEN_PHASE, label2);
                  thisTrace.type = 'P';
                  thisTrace.name = "NP_" + label2;
                } else
                  thisTrace.name = "NV_" + label2;
                thisTrace.traceData = &sObj.results.xVect.at(
                    std::distance(mObj.relXInd.begin(),
                                  std::find(mObj.relXInd.begin(),
                                            mObj.relXInd.end(), index2)));
                traces.push_back(thisTrace);
              } else
                Errors::control_errors(UNKNOWN_NODE, label2);
            } else if (label2 == "0" || label2 == "GND") {
              if (mObj.deviceLabelIndex.count(label) != 0) {
                index1 = mObj.deviceLabelIndex.at(label).index;
                if (iObj.argAnal == JoSIM::AnalysisType::Phase) {
                  Errors::control_errors(NODEVOLT_WHEN_PHASE, label);
                  thisTrace.type = 'P';
                  thisTrace.name = "NP_" + label;
                } else
                  thisTrace.name = "NV_" + label;
                thisTrace.traceData = &sObj.results.xVect.at(
                    std::distance(mObj.relXInd.begin(),
                                  std::find(mObj.relXInd.begin(),
                                            mObj.relXInd.end(), index1)));
                traces.push_back(thisTrace);
              } else
                Errors::control_errors(UNKNOWN_NODE, label);
            } else {
              if (mObj.deviceLabelIndex.count(label2) != 0) {
                if (mObj.deviceLabelIndex.count(label) != 0) {
                  if (iObj.argAnal == JoSIM::AnalysisType::Phase) {
                    Errors::control_errors(NODEVOLT_WHEN_PHASE, label);
                    thisTrace.type = 'P';
                    thisTrace.name = "NP_";
                    thisTrace.name += label;
                    thisTrace.name += "_";
                    thisTrace.name += label2;
                  } else
                    thisTrace.name = "NV_";
                    thisTrace.name += label;
                    thisTrace.name += "_";
                    thisTrace.name += label2;
                  std::transform(sObj.results.xVect
                                     .at(std::distance(
                                         mObj.relXInd.begin(),
                                         std::find(mObj.relXInd.begin(),
                                                   mObj.relXInd.end(), index1)))
                                     .begin(),
                                 sObj.results.xVect
                                     .at(std::distance(
                                         mObj.relXInd.begin(),
                                         std::find(mObj.relXInd.begin(),
                                                   mObj.relXInd.end(), index1)))
                                     .end(),
                                 sObj.results.xVect
                                     .at(std::distance(
                                         mObj.relXInd.begin(),
                                         std::find(mObj.relXInd.begin(),
                                                   mObj.relXInd.end(), index2)))
                                     .begin(),
                                 std::back_inserter(thisTrace.calcData),
                                 std::minus<double>());
                  thisTrace.pointer = false;
                  traces.push_back(thisTrace);
                } else
                  Errors::control_errors(UNKNOWN_NODE, label);
              } else
                Errors::control_errors(UNKNOWN_NODE, label2);
            }
          }
        } else if (tokens.at(j)[0] == 'C') {
          thisTrace.type = 'C';
          tokens2 = Misc::tokenize_delimeter(tokens.at(j), "C() ,");
          if (tokens2.size() == 1) {
            label = tokens2.at(0);
            if (label.find('_') != std::string::npos) {
              tokens2 = Misc::tokenize_delimeter(label, "_");
              label = tokens2.back();
              for (int k = 0; k < tokens2.size() - 1; k++)
                label += "|" + tokens2.at(k);
            } else if (label.find('.') != std::string::npos) {
              std::replace(label.begin(), label.end(), '.', '|');
            }
            thisTrace.name = label;
            thisTrace.type = 'C';
            if (mObj.deviceLabelIndex.count(label) != 0) {
              const auto &dev = mObj.deviceLabelIndex.at(label);
              switch (dev.type) {
              case RowDescriptor::Type::VoltageResistor:
                if (mObj.components.voltRes.at(dev.index).posNCol == -1) {
                  std::transform(
                      sObj.results.xVect
                          .at(std::distance(
                              mObj.relXInd.begin(),
                              std::find(mObj.relXInd.begin(),
                                        mObj.relXInd.end(),
                                        mObj.components.voltRes.at(dev.index)
                                            .negNCol)))
                          .begin(),
                      sObj.results.xVect
                          .at(std::distance(
                              mObj.relXInd.begin(),
                              std::find(mObj.relXInd.begin(),
                                        mObj.relXInd.end(),
                                        mObj.components.voltRes.at(dev.index)
                                            .negNCol)))
                          .end(),
                      std::back_inserter(thisTrace.calcData),
                      std::bind(
                          std::multiplies<double>(), std::placeholders::_1,
                          (1 / mObj.components.voltRes.at(dev.index).value)));
                } else if (mObj.components.voltRes.at(dev.index).negNCol ==
                           -1) {
                  std::transform(
                      sObj.results.xVect
                          .at(std::distance(
                              mObj.relXInd.begin(),
                              std::find(mObj.relXInd.begin(),
                                        mObj.relXInd.end(),
                                        mObj.components.voltRes.at(dev.index)
                                            .posNCol)))
                          .begin(),
                      sObj.results.xVect
                          .at(std::distance(
                              mObj.relXInd.begin(),
                              std::find(mObj.relXInd.begin(),
                                        mObj.relXInd.end(),
                                        mObj.components.voltRes.at(dev.index)
                                            .posNCol)))
                          .end(),
                      std::back_inserter(thisTrace.calcData),
                      std::bind(
                          std::multiplies<double>(), std::placeholders::_1,
                          (1 / mObj.components.voltRes.at(dev.index).value)));
                } else {
                  std::transform(
                      sObj.results.xVect
                          .at(std::distance(
                              mObj.relXInd.begin(),
                              std::find(mObj.relXInd.begin(),
                                        mObj.relXInd.end(),
                                        mObj.components.voltRes.at(dev.index)
                                            .posNCol)))
                          .begin(),
                      sObj.results.xVect
                          .at(std::distance(
                              mObj.relXInd.begin(),
                              std::find(mObj.relXInd.begin(),
                                        mObj.relXInd.end(),
                                        mObj.components.voltRes.at(dev.index)
                                            .posNCol)))
                          .end(),
                      sObj.results.xVect
                          .at(std::distance(
                              mObj.relXInd.begin(),
                              std::find(mObj.relXInd.begin(),
                                        mObj.relXInd.end(),
                                        mObj.components.voltRes.at(dev.index)
                                            .negNCol)))
                          .begin(),
                      std::back_inserter(thisTrace.calcData),
                      std::minus<double>());
                  std::transform(
                      thisTrace.calcData.begin(), thisTrace.calcData.end(),
                      thisTrace.calcData.begin(),
                      std::bind(
                          std::multiplies<double>(), std::placeholders::_1,
                          (1 / mObj.components.voltRes.at(dev.index).value)));
                }
                thisTrace.pointer = false;
                traces.push_back(thisTrace);
                break;
              case RowDescriptor::Type::PhaseResistor:
                thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                    mObj.relXInd.begin(),
                    std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                              mObj.components.phaseRes.at(dev.index).curNCol)));
                traces.push_back(thisTrace);
                break;
              case RowDescriptor::Type::VoltageInductor:
                thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                    mObj.relXInd.begin(),
                    std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                              mObj.components.voltInd.at(dev.index).curNCol)));
                traces.push_back(thisTrace);
                break;
              case RowDescriptor::Type::PhaseInductor:
                thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                    mObj.relXInd.begin(),
                    std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                              mObj.components.phaseInd.at(dev.index).curNCol)));
                traces.push_back(thisTrace);
                break;
              case RowDescriptor::Type::VoltageCapacitor:
                thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                    mObj.relXInd.begin(),
                    std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                              mObj.components.voltCap.at(dev.index).curNCol)));
                traces.push_back(thisTrace);
                break;
              case RowDescriptor::Type::PhaseCapacitor:
                thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                    mObj.relXInd.begin(),
                    std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                              mObj.components.phaseCap.at(dev.index).curNCol)));
                traces.push_back(thisTrace);
                break;
              case RowDescriptor::Type::VoltageJJ:
                thisTrace.traceData =
                    &mObj.components.voltJJ.at(dev.index).jjCur;
                traces.push_back(thisTrace);
                break;
              case RowDescriptor::Type::PhaseJJ:
                thisTrace.traceData =
                    &mObj.components.phaseJJ.at(dev.index).jjCur;
                traces.push_back(thisTrace);
                break;
              case RowDescriptor::Type::VoltageVS:
                Errors::control_errors(CURRENT_THROUGH_VOLT, i);
                break;
              case RowDescriptor::Type::PhaseVS:
                Errors::control_errors(CURRENT_THROUGH_VOLT, i);
                break;
              case RowDescriptor::Type::VoltageCS:
                thisTrace.traceData = &mObj.sources.at(dev.index);
                traces.push_back(thisTrace);
                break;
              case RowDescriptor::Type::PhaseCS:
                thisTrace.traceData = &mObj.sources.at(dev.index);
                traces.push_back(thisTrace);
                break;
              default:
                Errors::control_errors(UNKNOWN_DEVICE, label);
                break;
              }
            } else
              Errors::control_errors(UNKNOWN_DEVICE, label);
          } else
            Errors::control_errors(INVALID_CURRENT, i);
        } else if (tokens.at(j).find("#BRANCH") != std::string::npos) {
          thisTrace.type = 'C';
          tokens2 = Misc::tokenize_delimeter(tokens.at(j), " #");
          label = tokens2.at(0);
          if (label.find('_') != std::string::npos) {
            tokens2 = Misc::tokenize_delimeter(label, "_");
            label = tokens2.back();
            for (int k = 0; k < tokens2.size() - 1; k++)
              label += "|" + tokens2.at(k);
          } else if (label.find('.') != std::string::npos) {
            std::replace(label.begin(), label.end(), '.', '|');
          }
          thisTrace.name = label;
          if (mObj.deviceLabelIndex.count(label) != 0) {
            const auto &dev = mObj.deviceLabelIndex.at(label);
            switch (dev.type) {
            case RowDescriptor::Type::VoltageResistor:
              if (mObj.components.voltRes.at(dev.index).posNCol == -1) {
                std::transform(
                    sObj.results.xVect
                        .at(std::distance(
                            mObj.relXInd.begin(),
                            std::find(
                                mObj.relXInd.begin(), mObj.relXInd.end(),
                                mObj.components.voltRes.at(dev.index).negNCol)))
                        .begin(),
                    sObj.results.xVect
                        .at(std::distance(
                            mObj.relXInd.begin(),
                            std::find(
                                mObj.relXInd.begin(), mObj.relXInd.end(),
                                mObj.components.voltRes.at(dev.index).negNCol)))
                        .end(),
                    std::back_inserter(thisTrace.calcData),
                    std::bind(
                        std::multiplies<double>(), std::placeholders::_1,
                        (1 / mObj.components.voltRes.at(dev.index).value)));
              } else if (mObj.components.voltRes.at(dev.index).negNCol == -1) {
                std::transform(
                    sObj.results.xVect
                        .at(std::distance(
                            mObj.relXInd.begin(),
                            std::find(
                                mObj.relXInd.begin(), mObj.relXInd.end(),
                                mObj.components.voltRes.at(dev.index).posNCol)))
                        .begin(),
                    sObj.results.xVect
                        .at(std::distance(
                            mObj.relXInd.begin(),
                            std::find(
                                mObj.relXInd.begin(), mObj.relXInd.end(),
                                mObj.components.voltRes.at(dev.index).posNCol)))
                        .end(),
                    std::back_inserter(thisTrace.calcData),
                    std::bind(
                        std::multiplies<double>(), std::placeholders::_1,
                        (1 / mObj.components.voltRes.at(dev.index).value)));
              } else {
                std::transform(
                    sObj.results.xVect
                        .at(std::distance(
                            mObj.relXInd.begin(),
                            std::find(
                                mObj.relXInd.begin(), mObj.relXInd.end(),
                                mObj.components.voltRes.at(dev.index).posNCol)))
                        .begin(),
                    sObj.results.xVect
                        .at(std::distance(
                            mObj.relXInd.begin(),
                            std::find(
                                mObj.relXInd.begin(), mObj.relXInd.end(),
                                mObj.components.voltRes.at(dev.index).posNCol)))
                        .end(),
                    sObj.results.xVect
                        .at(std::distance(
                            mObj.relXInd.begin(),
                            std::find(
                                mObj.relXInd.begin(), mObj.relXInd.end(),
                                mObj.components.voltRes.at(dev.index).negNCol)))
                        .begin(),
                    std::back_inserter(thisTrace.calcData),
                    std::minus<double>());
                std::transform(
                    thisTrace.calcData.begin(), thisTrace.calcData.end(),
                    thisTrace.calcData.begin(),
                    std::bind(
                        std::multiplies<double>(), std::placeholders::_1,
                        (1 / mObj.components.voltRes.at(dev.index).value)));
              }
              thisTrace.pointer = false;
              traces.push_back(thisTrace);
              break;
            case RowDescriptor::Type::PhaseResistor:
              thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                  mObj.relXInd.begin(),
                  std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                            mObj.components.phaseRes.at(dev.index).curNCol)));
              traces.push_back(thisTrace);
              break;
            case RowDescriptor::Type::VoltageInductor:
              thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                  mObj.relXInd.begin(),
                  std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                            mObj.components.voltInd.at(dev.index).curNCol)));
              traces.push_back(thisTrace);
              break;
            case RowDescriptor::Type::PhaseInductor:
              thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                  mObj.relXInd.begin(),
                  std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                            mObj.components.phaseInd.at(dev.index).curNCol)));
              traces.push_back(thisTrace);
              break;
            case RowDescriptor::Type::VoltageCapacitor:
              thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                  mObj.relXInd.begin(),
                  std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                            mObj.components.voltCap.at(dev.index).curNCol)));
              traces.push_back(thisTrace);
              break;
            case RowDescriptor::Type::PhaseCapacitor:
              thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                  mObj.relXInd.begin(),
                  std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                            mObj.components.phaseCap.at(dev.index).curNCol)));
              traces.push_back(thisTrace);
              break;
            case RowDescriptor::Type::VoltageJJ:
              thisTrace.traceData = &mObj.components.voltJJ.at(dev.index).jjCur;
              traces.push_back(thisTrace);
              break;
            case RowDescriptor::Type::PhaseJJ:
              thisTrace.traceData =
                  &mObj.components.phaseJJ.at(dev.index).jjCur;
              traces.push_back(thisTrace);
              break;
            case RowDescriptor::Type::VoltageVS:
              Errors::control_errors(CURRENT_THROUGH_VOLT, i);
              break;
            case RowDescriptor::Type::PhaseVS:
              Errors::control_errors(CURRENT_THROUGH_VOLT, i);
              break;
            case RowDescriptor::Type::VoltageCS:
              thisTrace.traceData = &mObj.sources.at(dev.index);
              traces.push_back(thisTrace);
              break;
            case RowDescriptor::Type::PhaseCS:
              thisTrace.traceData = &mObj.sources.at(dev.index);
              traces.push_back(thisTrace);
              break;
            default:
              Errors::control_errors(UNKNOWN_DEVICE, label);
              break;
            }
          } else
            Errors::control_errors(UNKNOWN_DEVICE, label);
        } else if (tokens.at(j)[0] == 'P') {
          thisTrace.type = 'P';
          tokens2 = Misc::tokenize_delimeter(tokens.at(j), "P() ,");
          if (tokens2.size() == 1) {
            label = tokens2.at(0);
            if (label.find('_') != std::string::npos) {
              tokens2 = Misc::tokenize_delimeter(label, "_");
              label = tokens2.back();
              for (int k = 0; k < tokens2.size() - 1; k++)
                label += "|" + tokens2.at(k);
            } else if (label.find('.') != std::string::npos) {
              std::replace(label.begin(), label.end(), '.', '|');
            }
            thisTrace.name = label;
            if (mObj.deviceLabelIndex.count(label) != 0) {
              const auto &dev = mObj.deviceLabelIndex.at(label);
              switch (dev.type) {
              case RowDescriptor::Type::VoltageResistor:
                Errors::control_errors(PHASE_WHEN_VOLT, i);
                break;
              case RowDescriptor::Type::PhaseResistor:
                if (mObj.components.phaseRes.at(dev.index).posNCol == -1) {
                  thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                      mObj.relXInd.begin(),
                      std::find(
                          mObj.relXInd.begin(), mObj.relXInd.end(),
                          mObj.components.phaseRes.at(dev.index).negNCol)));
                  traces.push_back(thisTrace);
                } else if (mObj.components.phaseRes.at(dev.index).negNCol ==
                           -1) {
                  thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                      mObj.relXInd.begin(),
                      std::find(
                          mObj.relXInd.begin(), mObj.relXInd.end(),
                          mObj.components.phaseRes.at(dev.index).posNCol)));
                  traces.push_back(thisTrace);
                } else {
                  std::transform(
                      sObj.results.xVect
                          .at(std::distance(
                              mObj.relXInd.begin(),
                              std::find(mObj.relXInd.begin(),
                                        mObj.relXInd.end(),
                                        mObj.components.phaseRes.at(dev.index)
                                            .posNCol)))
                          .begin(),
                      sObj.results.xVect
                          .at(std::distance(
                              mObj.relXInd.begin(),
                              std::find(mObj.relXInd.begin(),
                                        mObj.relXInd.end(),
                                        mObj.components.phaseRes.at(dev.index)
                                            .posNCol)))
                          .end(),
                      sObj.results.xVect
                          .at(std::distance(
                              mObj.relXInd.begin(),
                              std::find(mObj.relXInd.begin(),
                                        mObj.relXInd.end(),
                                        mObj.components.phaseRes.at(dev.index)
                                            .negNCol)))
                          .begin(),
                      std::back_inserter(thisTrace.calcData),
                      std::minus<double>());
                  thisTrace.pointer = false;
                  traces.push_back(thisTrace);
                }
                break;
              case RowDescriptor::Type::VoltageInductor:
                Errors::control_errors(PHASE_WHEN_VOLT, i);
                break;
              case RowDescriptor::Type::PhaseInductor:
                if (mObj.components.phaseInd.at(dev.index).posNCol == -1) {
                  thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                      mObj.relXInd.begin(),
                      std::find(
                          mObj.relXInd.begin(), mObj.relXInd.end(),
                          mObj.components.phaseInd.at(dev.index).negNCol)));
                  traces.push_back(thisTrace);
                } else if (mObj.components.phaseInd.at(dev.index).negNCol ==
                           -1) {
                  thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                      mObj.relXInd.begin(),
                      std::find(
                          mObj.relXInd.begin(), mObj.relXInd.end(),
                          mObj.components.phaseInd.at(dev.index).posNCol)));
                  traces.push_back(thisTrace);
                } else {
                  std::transform(
                      sObj.results.xVect
                          .at(std::distance(
                              mObj.relXInd.begin(),
                              std::find(mObj.relXInd.begin(),
                                        mObj.relXInd.end(),
                                        mObj.components.phaseInd.at(dev.index)
                                            .posNCol)))
                          .begin(),
                      sObj.results.xVect
                          .at(std::distance(
                              mObj.relXInd.begin(),
                              std::find(mObj.relXInd.begin(),
                                        mObj.relXInd.end(),
                                        mObj.components.phaseInd.at(dev.index)
                                            .posNCol)))
                          .end(),
                      sObj.results.xVect
                          .at(std::distance(
                              mObj.relXInd.begin(),
                              std::find(mObj.relXInd.begin(),
                                        mObj.relXInd.end(),
                                        mObj.components.phaseInd.at(dev.index)
                                            .negNCol)))
                          .begin(),
                      std::back_inserter(thisTrace.calcData),
                      std::minus<double>());
                  thisTrace.pointer = false;
                  traces.push_back(thisTrace);
                }
                break;
              case RowDescriptor::Type::VoltageCapacitor:
                Errors::control_errors(PHASE_WHEN_VOLT, i);
                break;
              case RowDescriptor::Type::PhaseCapacitor:
                if (mObj.components.phaseCap.at(dev.index).posNCol == -1) {
                  thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                      mObj.relXInd.begin(),
                      std::find(
                          mObj.relXInd.begin(), mObj.relXInd.end(),
                          mObj.components.phaseCap.at(dev.index).negNCol)));
                  traces.push_back(thisTrace);
                } else if (mObj.components.phaseCap.at(dev.index).negNCol ==
                           -1) {
                  thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                      mObj.relXInd.begin(),
                      std::find(
                          mObj.relXInd.begin(), mObj.relXInd.end(),
                          mObj.components.phaseCap.at(dev.index).posNCol)));
                  traces.push_back(thisTrace);
                } else {
                  std::transform(
                      sObj.results.xVect
                          .at(std::distance(
                              mObj.relXInd.begin(),
                              std::find(mObj.relXInd.begin(),
                                        mObj.relXInd.end(),
                                        mObj.components.phaseCap.at(dev.index)
                                            .posNCol)))
                          .begin(),
                      sObj.results.xVect
                          .at(std::distance(
                              mObj.relXInd.begin(),
                              std::find(mObj.relXInd.begin(),
                                        mObj.relXInd.end(),
                                        mObj.components.phaseCap.at(dev.index)
                                            .posNCol)))
                          .end(),
                      sObj.results.xVect
                          .at(std::distance(
                              mObj.relXInd.begin(),
                              std::find(mObj.relXInd.begin(),
                                        mObj.relXInd.end(),
                                        mObj.components.phaseCap.at(dev.index)
                                            .negNCol)))
                          .begin(),
                      std::back_inserter(thisTrace.calcData),
                      std::minus<double>());
                  thisTrace.pointer = false;
                  traces.push_back(thisTrace);
                }
                break;
              case RowDescriptor::Type::VoltageJJ:
                thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                    mObj.relXInd.begin(),
                    std::find(mObj.relXInd.begin(), mObj.relXInd.end(),
                              mObj.components.voltJJ.at(dev.index).phaseNCol)));
                traces.push_back(thisTrace);
                break;
              case RowDescriptor::Type::PhaseJJ:
                if (mObj.components.phaseJJ.at(dev.index).posNCol == -1) {
                  thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                      mObj.relXInd.begin(),
                      std::find(
                          mObj.relXInd.begin(), mObj.relXInd.end(),
                          mObj.components.phaseJJ.at(dev.index).negNCol)));
                  traces.push_back(thisTrace);
                } else if (mObj.components.phaseJJ.at(dev.index).negNCol ==
                           -1) {
                  thisTrace.traceData = &sObj.results.xVect.at(std::distance(
                      mObj.relXInd.begin(),
                      std::find(
                          mObj.relXInd.begin(), mObj.relXInd.end(),
                          mObj.components.phaseJJ.at(dev.index).posNCol)));
                  traces.push_back(thisTrace);
                } else {
                  std::transform(
                      sObj.results.xVect
                          .at(std::distance(
                              mObj.relXInd.begin(),
                              std::find(mObj.relXInd.begin(),
                                        mObj.relXInd.end(),
                                        mObj.components.phaseJJ.at(dev.index)
                                            .posNCol)))
                          .begin(),
                      sObj.results.xVect
                          .at(std::distance(
                              mObj.relXInd.begin(),
                              std::find(mObj.relXInd.begin(),
                                        mObj.relXInd.end(),
                                        mObj.components.phaseJJ.at(dev.index)
                                            .posNCol)))
                          .end(),
                      sObj.results.xVect
                          .at(std::distance(
                              mObj.relXInd.begin(),
                              std::find(mObj.relXInd.begin(),
                                        mObj.relXInd.end(),
                                        mObj.components.phaseJJ.at(dev.index)
                                            .negNCol)))
                          .begin(),
                      std::back_inserter(thisTrace.calcData),
                      std::minus<double>());
                  thisTrace.pointer = false;
                  traces.push_back(thisTrace);
                }
                break;
              case RowDescriptor::Type::VoltageVS:
                Errors::control_errors(PHASE_OF_VOLT, i);
                break;
              case RowDescriptor::Type::VoltageCS:
                Errors::control_errors(PHASE_OF_CURRENT, i);
                break;
              default:
                if (mObj.deviceLabelIndex.count(label) != 0) {
                  index1 = mObj.deviceLabelIndex.at(label).index;
                  if (iObj.argAnal == JoSIM::AnalysisType::Voltage) {
                    Errors::control_errors(NODEPHASE_WHEN_VOLT, label);
                    thisTrace.type = 'V';
                    thisTrace.name = "NV_" + label;
                  } else
                    thisTrace.name = "NP_" + label;
                  thisTrace.traceData = &sObj.results.xVect.at(
                      std::distance(mObj.relXInd.begin(),
                                    std::find(mObj.relXInd.begin(),
                                              mObj.relXInd.end(), index1)));
                  traces.push_back(thisTrace);
                } else
                  Errors::control_errors(UNKNOWN_NODE, label);
                break;
              }
            } else
              Errors::control_errors(UNKNOWN_DEVICE, label);
          } else {
            label = tokens2.at(0);
            label2 = tokens2.at(1);
            if (label.find('_') != std::string::npos) {
              tokens2 = Misc::tokenize_delimeter(label, "_");
              label = tokens2.back();
              for (int k = 0; k < tokens2.size() - 1; k++)
                label += "|" + tokens2.at(k);
            } else if (label.find('.') != std::string::npos) {
              std::replace(label.begin(), label.end(), '.', '|');
            }
            if (label2.find('_') != std::string::npos) {
              tokens2 = Misc::tokenize_delimeter(label2, "_");
              label2 = tokens2.back();
              for (int k = 0; k < tokens2.size() - 1; k++)
                label2 = label + "|" + tokens2.at(k);
            } else if (label2.find('.') != std::string::npos) {
              std::replace(label2.begin(), label2.end(), '.', '|');
            }
            if (label == "0" || label == "GND") {
              if (mObj.deviceLabelIndex.count(label2) != 0) {
                index2 = mObj.deviceLabelIndex.at(label2).index;
                if (iObj.argAnal == JoSIM::AnalysisType::Voltage) {
                  Errors::control_errors(NODEVOLT_WHEN_PHASE, label2);
                  thisTrace.type = 'V';
                  thisTrace.name = "NV_" + label2;
                } else
                  thisTrace.name = "NP_" + label2;
                thisTrace.traceData = &sObj.results.xVect.at(
                    std::distance(mObj.relXInd.begin(),
                                  std::find(mObj.relXInd.begin(),
                                            mObj.relXInd.end(), index2)));
                traces.push_back(thisTrace);
              } else
                Errors::control_errors(UNKNOWN_NODE, label2);
            } else if (label2 == "0" || label2 == "GND") {
              if (mObj.deviceLabelIndex.count(label) != 0) {
                index1 = mObj.deviceLabelIndex.at(label).index;
                if (iObj.argAnal == JoSIM::AnalysisType::Voltage) {
                  Errors::control_errors(NODEVOLT_WHEN_PHASE, label);
                  thisTrace.type = 'V';
                  thisTrace.name = "NV_" + label;
                } else
                  thisTrace.name = "NP_" + label;
                thisTrace.traceData = &sObj.results.xVect.at(
                    std::distance(mObj.relXInd.begin(),
                                  std::find(mObj.relXInd.begin(),
                                            mObj.relXInd.end(), index1)));
                traces.push_back(thisTrace);
              } else
                Errors::control_errors(UNKNOWN_NODE, label);
            } else {
              if (mObj.deviceLabelIndex.count(label2) != 0) {
                if (mObj.deviceLabelIndex.count(label) != 0) {
                  if (iObj.argAnal == JoSIM::AnalysisType::Voltage) {
                    Errors::control_errors(NODEVOLT_WHEN_PHASE, label);
                    thisTrace.type = 'V';
                    thisTrace.name = "NV_";
                    thisTrace.name += label;
                    thisTrace.name += "_";
                    thisTrace.name += label2;
                  } else
                    thisTrace.name = "NP_";
                    thisTrace.name += label;
                    thisTrace.name += "_";
                    thisTrace.name += label2;
                  std::transform(sObj.results.xVect
                                     .at(std::distance(
                                         mObj.relXInd.begin(),
                                         std::find(mObj.relXInd.begin(),
                                                   mObj.relXInd.end(), index1)))
                                     .begin(),
                                 sObj.results.xVect
                                     .at(std::distance(
                                         mObj.relXInd.begin(),
                                         std::find(mObj.relXInd.begin(),
                                                   mObj.relXInd.end(), index1)))
                                     .end(),
                                 sObj.results.xVect
                                     .at(std::distance(
                                         mObj.relXInd.begin(),
                                         std::find(mObj.relXInd.begin(),
                                                   mObj.relXInd.end(), index2)))
                                     .begin(),
                                 std::back_inserter(thisTrace.calcData),
                                 std::minus<double>());
                  thisTrace.pointer = false;
                  traces.push_back(thisTrace);
                } else
                  Errors::control_errors(UNKNOWN_NODE, label);
              } else
                Errors::control_errors(UNKNOWN_NODE, label2);
            }
          }
        } else if (tokens.at(j).find("TRAN") != std::string::npos) {
        } else if (tokens.at(j)[0] == '"')
          Errors::control_errors(MATHOPS, tokens.at(j));
        else
          Errors::control_errors(UNKNOWN_PLOT, i);
      }
    } else if (i.find("SAVE") != std::string::npos) {
      Trace thisTrace;
      tokens = Misc::tokenize_space(i);
      for (int k = 1; k < tokens.size(); k++) {
        index1 = tokens.at(k).find('@');
        if (index1 != std::string::npos)
          tokens.at(k) =
              tokens.at(k).substr(0, index1) + tokens.at(k).substr(index1 + 1);
        index1 = tokens.at(k).find(".at(");
        if (index1 != std::string::npos)
          tokens.at(k) = tokens.at(k).substr(0, index1);
        index1 = tokens.at(k).find('.');
        if (index1 != std::string::npos) {
          tokens.at(k) = tokens.at(k).substr(0, index1) + "|" +
                         tokens.at(k).substr(index1 + 1);
        }
        index1 = tokens.at(k).find('[');
        if (index1 != std::string::npos)
          tokens.at(k) = tokens.at(k).substr(0, index1);
        if (mObj.deviceLabelIndex.count(tokens.at(k)) != 0) {
          const auto &dev = mObj.deviceLabelIndex.at(tokens.at(k));
          thisTrace.type = 'C';
          thisTrace.name = tokens.at(k);
          thisTrace.traceData = &mObj.sources.at(dev.index);
          traces.push_back(thisTrace);
        }
      }
    }
  }
*/
  std::vector<std::string> tokens, tokens2;

  for(const auto &i : iObj.relevantX) {
    tokens = Misc::tokenize_space(i.substr(i.find_first_of(" \t") + 1));
    for (auto &j : tokens) {
      for (auto &k : tokens) {
        if (k.find('_') != std::string::npos) {
          tokens2 = Misc::tokenize_delimeter(k, "_");
          k = tokens2.back();
          for (int l = 0; l < tokens2.size() - 1; l++)
            k += "|" + tokens2.at(l);
        } else if (k.find('.') != std::string::npos) {
          std::replace(k.begin(), k.end(), '.', '|');
        }
      }
      if(j.at(0) == 'D' || (j.at(0) == 'P' && j.at(1) == 'H')) { //////////////////// DEVV/DEVI/PHASE /////////////////
          if(j.back() == 'V') { //////////////////// DEVV /////////////////
            traces.emplace_back(Trace());
            traces.back().calcData.reserve(iObj.transSim.simsize());
            tokens2.clear();
            std::copy(tokens.begin() + 1, tokens.end(), std::back_inserter(tokens2));
            handle_voltage(tokens2, traces.back(), iObj, mObj, sObj);
            break;
          } else if (j.back() == 'I') { //////////////////// DEVI /////////////////
            traces.emplace_back(Trace());
            traces.back().calcData.reserve(iObj.transSim.simsize());
            tokens2.clear();
            std::copy(tokens.begin() + 1, tokens.end(), std::back_inserter(tokens2));
            handle_current(tokens2, traces.back(), iObj, mObj, sObj);
            break;
          } else if (j.back() == 'E') {//////////////////// PHASE /////////////////
            traces.emplace_back(Trace());
            traces.back().calcData.reserve(iObj.transSim.simsize());
            tokens2.clear();
            std::copy(tokens.begin() + 1, tokens.end(), std::back_inserter(tokens2));
            handle_phase(tokens2, traces.back(), iObj, mObj, sObj);
            break;
          }
      } else if (j.at(0) == 'C' || j.at(0) == 'I') { //////////////////// CURRENT - C() /////////////////
        tokens2 = Misc::tokenize_delimeter(j.substr(1), "(),");
        traces.emplace_back(Trace());
        traces.back().calcData.reserve(iObj.transSim.simsize());
        handle_current(tokens2, traces.back(), iObj, mObj, sObj);
      } else if (j.at(0) == 'P') { //////////////////// PHASE - P() /////////////////
        tokens2 = Misc::tokenize_delimeter(j.substr(1), "(),");
        traces.emplace_back(Trace());
        traces.back().calcData.reserve(iObj.transSim.simsize());
        handle_phase(tokens2, traces.back(), iObj, mObj, sObj);
      } else if (j.at(0) == 'V') { //////////////////// VOLTAGE - P() /////////////////
        tokens2 = Misc::tokenize_delimeter(j.substr(1), "(),");
        traces.emplace_back(Trace());
        traces.back().calcData.reserve(iObj.transSim.simsize());
        handle_voltage(tokens2, traces.back(), iObj, mObj, sObj);
      } else if (j.at(0) == 'N') { //////////////////// NODEV - NODEP /////////////////
        if(j.back() == 'V') {
          traces.emplace_back(Trace());
          traces.back().calcData.reserve(iObj.transSim.simsize());
          tokens2.clear();
          std::copy(tokens.begin() + 1, tokens.end(), std::back_inserter(tokens2));
          handle_voltage(tokens2, traces.back(), iObj, mObj, sObj);
          break;
        } else if(j.back() == 'P') {
          traces.emplace_back(Trace());
          traces.back().calcData.reserve(iObj.transSim.simsize());
          tokens2.clear();
          std::copy(tokens.begin() + 1, tokens.end(), std::back_inserter(tokens2));
          handle_phase(tokens2, traces.back(), iObj, mObj, sObj);
          break;
        }
      } else if (j.at(0) == '@') {
        traces.emplace_back(Trace());
        traces.back().calcData.reserve(iObj.transSim.simsize());
        // tokens2.clear();
        // std::copy(tokens.begin() + 1, tokens.end(), std::back_inserter(tokens2));
        if(tokens.at(0).at(tokens.at(0).size() - 2) == 'C') {
          tokens.at(0) = tokens.at(0).substr(1, tokens.at(0).size() - 4);
          handle_current(tokens, traces.back(), iObj, mObj, sObj);
          break;
        } else if(tokens.at(0).at(tokens.at(0).size() - 2) == 'V') {
          tokens.at(0) = tokens.at(0).substr(1, tokens.at(0).size() - 4);
          handle_voltage(tokens, traces.back(), iObj, mObj, sObj);
          break;
        } else if(tokens.at(0).at(tokens.at(0).size() - 2) == 'P') {
          tokens.at(0) = tokens.at(0).substr(1, tokens.at(0).size() - 4);
          handle_phase(tokens, traces.back(), iObj, mObj, sObj);
          break;
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
        if(std::find(mObj.rowNames.begin(), mObj.rowNames.end(), devToHandle.at(0)) != mObj.rowNames.end()) {
          result.name = "NODEV_" + devToHandle.at(0);
          result.calcData = sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0))));
        } else {
          Errors::control_errors(UNKNOWN_DEVICE, devToHandle.at(0));
        }
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
          result.calcData.emplace_back((HBAR / (iObj.transSim.prstep * EV)) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).at(0));
          for (int i = 1; i < sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).size(); i++) {
            result.calcData.emplace_back((HBAR / (iObj.transSim.prstep * EV))
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).at(i) - sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).at(i-1))
              - result.calcData.back());
          }
        } else if(devToHandle.at(0) == "0" || devToHandle.at(0) == "GND") {
          result.name = "NODEV_0_" + devToHandle.at(1);
          result.calcData.emplace_back(-((HBAR / (iObj.transSim.prstep * EV)) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(1)))).at(0)));
          for (int i = 1; i < sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(1)))).size(); i++) {
            result.calcData.emplace_back(-((HBAR / (iObj.transSim.prstep * EV))
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(1)))).at(i) - sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(1)))).at(i-1))
              - result.calcData.back()));
          }
        } else {
          result.name = "NODEV_" + devToHandle.at(0) + "_" + devToHandle.at(1);
          result.calcData.emplace_back(((HBAR / (iObj.transSim.prstep * EV)) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).at(0))
            - ((HBAR / (iObj.transSim.prstep * EV)) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(1)))).at(0)));
          for (int i = 1; i < sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).size(); i++) {
            result.calcData.emplace_back(((HBAR / (iObj.transSim.prstep * EV))
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).at(i) - sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).at(i-1)))
              - ((HBAR / (iObj.transSim.prstep * EV))
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(1)))).at(i) - sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(1)))).at(i-1)))
              - result.calcData.back());
          }
        }
      } else {
        std::vector<double> devv1, devv2;
        if(mObj.labelNodes.at(devToHandle.at(0)).negNode == "0" || mObj.labelNodes.at(devToHandle.at(0)).negNode == "GND") {
          devv1.emplace_back((HBAR / (iObj.transSim.prstep * EV)) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).at(0));
          for (int i = 1; i < sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).size(); i++) {
            devv1.emplace_back((HBAR / (iObj.transSim.prstep * EV))
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).at(i) 
              - sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).at(i-1)) 
              - devv1.back());
          }
        } else if(mObj.labelNodes.at(devToHandle.at(0)).posNode == "0" || mObj.labelNodes.at(devToHandle.at(0)).posNode == "GND") {
          devv1.emplace_back(-((HBAR / (iObj.transSim.prstep * EV)) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).at(0)));
          for (int i = 1; i < sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).size(); i++) {
            devv1.emplace_back(-((HBAR / (iObj.transSim.prstep * EV))
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).at(i) 
              - sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).at(i-1)) 
              - devv1.back()));
          }
        } else {
          devv1.emplace_back(((HBAR / (iObj.transSim.prstep * EV)) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).at(0))
            - ((HBAR / (iObj.transSim.prstep * EV)) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).at(0)));
          for (int i = 1; i < sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).size(); i++) {
            devv1.emplace_back(((HBAR / (iObj.transSim.prstep * EV))
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).at(i) 
              - sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).at(i-1)))
              - ((HBAR / (iObj.transSim.prstep * EV))
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).at(i) 
              - sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).at(i-1)))
              - devv1.back());
          }
        }
        if(mObj.labelNodes.at(devToHandle.at(1)).negNode == "0" || mObj.labelNodes.at(devToHandle.at(1)).negNode == "GND") {
          devv2.emplace_back((HBAR / (iObj.transSim.prstep * EV)) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(1)).posNode))).at(0));
          for (int i = 1; i < sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(1)))).size(); i++) {
            devv2.emplace_back((HBAR / (iObj.transSim.prstep * EV))
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(1)).posNode))).at(i) 
              - sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(1)).posNode))).at(i-1)) 
              - devv2.back());
          }
        } else if(mObj.labelNodes.at(devToHandle.at(1)).posNode == "0" || mObj.labelNodes.at(devToHandle.at(1)).posNode == "GND") {
          devv2.emplace_back(-((HBAR / (iObj.transSim.prstep * EV)) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(1)).negNode))).at(0)));
          for (int i = 1; i < sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(1)))).size(); i++) {
            devv2.emplace_back(-((HBAR / (iObj.transSim.prstep * EV))
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(1)).negNode))).at(i) 
              - sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(1)).negNode))).at(i-1)) 
              - devv2.back()));
          }
        } else {
          devv2.emplace_back(((HBAR / (iObj.transSim.prstep * EV)) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(1)).posNode))).at(0))
            - ((HBAR / (iObj.transSim.prstep * EV)) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(1)).negNode))).at(0)));
          for (int i = 1; i < sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(1)))).size(); i++) {
            devv2.emplace_back(((HBAR / (iObj.transSim.prstep * EV))
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(1)).posNode))).at(i) 
              - sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(1)).posNode))).at(i-1)))
              - ((HBAR / (iObj.transSim.prstep * EV))
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
        result.calcData.emplace_back((HBAR / (iObj.transSim.prstep * EV)) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).at(0));
        for (int i = 1; i < sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).size(); i++) {
          result.calcData.emplace_back((HBAR / (iObj.transSim.prstep * EV)) 
            * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).at(i) 
            - sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).at(i-1))
            - result.calcData.back());
        }
      } else {
        if(mObj.deviceLabelIndex.at(devToHandle.at(0)).type == RowDescriptor::Type::PhaseVS) {
          result.calcData = mObj.sources.at(mObj.deviceLabelIndex.at(devToHandle.at(0)).index);
        } else if(mObj.deviceLabelIndex.at(devToHandle.at(0)).type == RowDescriptor::Type::PhasePS) {
          result.calcData.emplace_back((HBAR / (iObj.transSim.prstep * EV)) * mObj.sources.at(mObj.deviceLabelIndex.at(devToHandle.at(0)).index).at(0));
          for (int i = 1; i < mObj.sources.at(mObj.deviceLabelIndex.at(devToHandle.at(0)).index).size(); i++) {
            result.calcData.emplace_back((HBAR / (iObj.transSim.prstep * EV)) 
              * (mObj.sources.at(mObj.deviceLabelIndex.at(devToHandle.at(0)).index).at(i) 
              - mObj.sources.at(mObj.deviceLabelIndex.at(devToHandle.at(0)).index).at(i-1))
              - result.calcData.back());
          }
        } else {
          if(mObj.labelNodes.at(devToHandle.at(0)).negNode == "0" || mObj.labelNodes.at(devToHandle.at(0)).negNode == "GND") {
            result.calcData.emplace_back((HBAR / (iObj.transSim.prstep * EV)) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).at(0));
            for (int i = 1; i < sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).size(); i++) {
              result.calcData.emplace_back((HBAR / (iObj.transSim.prstep * EV)) 
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).at(i)
              - sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).at(i-1))
              - result.calcData.back());
            }
          } else if(mObj.labelNodes.at(devToHandle.at(0)).posNode == "0" || mObj.labelNodes.at(devToHandle.at(0)).posNode == "GND") {
            result.calcData.emplace_back(-((HBAR / (iObj.transSim.prstep * EV)) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).at(0)));
            for (int i = 1; i < sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).size(); i++) {
              result.calcData.emplace_back(-((HBAR / (iObj.transSim.prstep * EV)) 
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).at(i)
              - sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).at(i-1))
              - result.calcData.back()));
            }
          } else {
            result.calcData.emplace_back((HBAR / (iObj.transSim.prstep * EV)) 
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).at(0)
              - sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).at(0)));
            for (int i = 1; i < sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).size(); i++) {
              result.calcData.emplace_back((HBAR / (iObj.transSim.prstep * EV)) 
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
          if(std::find(mObj.rowNames.begin(), mObj.rowNames.end(), devToHandle.at(0)) != mObj.rowNames.end()) {
            result.name = "NODEP_" + devToHandle.at(0);
            result.calcData = sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0))));
          } else {
            Errors::control_errors(UNKNOWN_DEVICE, devToHandle.at(0));
          }
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
          result.calcData.emplace_back(((iObj.transSim.prstep * EV) / HBAR) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).at(0));
          for (int i = 1; i < sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).size(); i++) {
            result.calcData.emplace_back(((iObj.transSim.prstep * EV) / HBAR)
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).at(i) + sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).at(i-1))
              + result.calcData.back());
          }
        } else if(devToHandle.at(0) == "0" || devToHandle.at(0) == "GND") {
          result.name = "NODEP_0_" + devToHandle.at(1);
          result.calcData.emplace_back(-(((iObj.transSim.prstep * EV) / HBAR) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(1)))).at(0)));
          for (int i = 1; i < sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(1)))).size(); i++) {
            result.calcData.emplace_back(-(((iObj.transSim.prstep * EV) / HBAR)
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(1)))).at(i) + sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(1)))).at(i-1))
              + result.calcData.back()));
          }
        } else {
          result.name = "NODEP_" + devToHandle.at(0) + "_" + devToHandle.at(1);
          result.calcData.emplace_back((((iObj.transSim.prstep * EV) / HBAR) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).at(0))
            - (((iObj.transSim.prstep * EV) / HBAR) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(1)))).at(0)));
          for (int i = 1; i < sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).size(); i++) {
            result.calcData.emplace_back((((iObj.transSim.prstep * EV) / HBAR)
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).at(i) + sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).at(i-1)))
              - (((iObj.transSim.prstep * EV) / HBAR)
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(1)))).at(i) + sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(1)))).at(i-1)))
              + result.calcData.back());
          }
        }
      } else {
        std::vector<double> devv1, devv2;
        if(mObj.labelNodes.at(devToHandle.at(0)).negNode == "0" || mObj.labelNodes.at(devToHandle.at(0)).negNode == "GND") {
          devv1.emplace_back(((iObj.transSim.prstep * EV) / HBAR) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).at(0));
          for (int i = 1; i < sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).size(); i++) {
            devv1.emplace_back(((iObj.transSim.prstep * EV) / HBAR)
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).at(i) 
              + sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).at(i-1)) 
              + devv1.back());
          }
        } else if(mObj.labelNodes.at(devToHandle.at(0)).posNode == "0" || mObj.labelNodes.at(devToHandle.at(0)).posNode == "GND") {
          devv1.emplace_back(-(((iObj.transSim.prstep * EV) / HBAR) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).at(0)));
          for (int i = 1; i < sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).size(); i++) {
            devv1.emplace_back(-(((iObj.transSim.prstep * EV) / HBAR)
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).at(i) 
              + sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).at(i-1)) 
              + devv1.back()));
          }
        } else {
          devv1.emplace_back((((iObj.transSim.prstep * EV) / HBAR) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).at(0))
            - (((iObj.transSim.prstep * EV) / HBAR) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).at(0)));
          for (int i = 1; i < sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).size(); i++) {
            devv1.emplace_back((((iObj.transSim.prstep * EV) / HBAR)
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).at(i) 
              + sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).at(i-1)))
              - (((iObj.transSim.prstep * EV) / HBAR)
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).at(i) 
              + sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).at(i-1)))
              + devv1.back());
          }
        }
        if(mObj.labelNodes.at(devToHandle.at(1)).negNode == "0" || mObj.labelNodes.at(devToHandle.at(1)).negNode == "GND") {
          devv2.emplace_back(((iObj.transSim.prstep * EV) / HBAR) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(1)).posNode))).at(0));
          for (int i = 1; i < sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(1)))).size(); i++) {
            devv2.emplace_back(((iObj.transSim.prstep * EV) / HBAR)
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(1)).posNode))).at(i) 
              + sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(1)).posNode))).at(i-1)) 
              + devv2.back());
          }
        } else if(mObj.labelNodes.at(devToHandle.at(1)).posNode == "0" || mObj.labelNodes.at(devToHandle.at(1)).posNode == "GND") {
          devv2.emplace_back(-(((iObj.transSim.prstep * EV) / HBAR) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(1)).negNode))).at(0)));
          for (int i = 1; i < sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(1)))).size(); i++) {
            devv2.emplace_back(-(((iObj.transSim.prstep * EV) / HBAR)
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(1)).negNode))).at(i) 
              + sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(1)).negNode))).at(i-1)) 
              + devv2.back()));
          }
        } else {
          devv2.emplace_back((((iObj.transSim.prstep * EV) / HBAR) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(1)).posNode))).at(0))
            - (((iObj.transSim.prstep * EV) / HBAR) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(1)).negNode))).at(0)));
          for (int i = 1; i < sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(1)))).size(); i++) {
            devv2.emplace_back((((iObj.transSim.prstep * EV) / HBAR)
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(1)).posNode))).at(i) 
              + sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(1)).posNode))).at(i-1)))
              - (((iObj.transSim.prstep * EV) / HBAR)
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
        result.calcData.emplace_back(((iObj.transSim.prstep * EV) / HBAR) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).at(0));
        for (int i = 1; i < sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).size(); i++) {
          result.calcData.emplace_back(((iObj.transSim.prstep * EV) / HBAR)
            * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).at(i) 
            + sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(devToHandle.at(0)))).at(i-1))
            + result.calcData.back());
        }
      } else {
        if(mObj.deviceLabelIndex.at(devToHandle.at(0)).type == RowDescriptor::Type::VoltagePS) {
          result.calcData = mObj.sources.at(mObj.deviceLabelIndex.at(devToHandle.at(0)).index);
        } else {
          if(mObj.labelNodes.at(devToHandle.at(0)).negNode == "0" || mObj.labelNodes.at(devToHandle.at(0)).negNode == "GND") {
            result.calcData.emplace_back(((iObj.transSim.prstep * EV) / HBAR) * sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).at(0));
            for (int i = 1; i < sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).size(); i++) {
              result.calcData.emplace_back(((iObj.transSim.prstep * EV) / HBAR)
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).at(i)
              + sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).at(i-1))
              + result.calcData.back());
            }
          } else if(mObj.labelNodes.at(devToHandle.at(0)).posNode == "0" || mObj.labelNodes.at(devToHandle.at(0)).posNode == "GND") {
            result.calcData.emplace_back(-(((iObj.transSim.prstep * EV) / HBAR)* sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).at(0)));
            for (int i = 1; i < sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).size(); i++) {
              result.calcData.emplace_back(-(((iObj.transSim.prstep * EV) / HBAR)
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).at(i)
              + sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).at(i-1))
              + result.calcData.back()));
            }
          } else {
            result.calcData.emplace_back(((iObj.transSim.prstep * EV) / HBAR)
              * (sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).at(0)
              - sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).negNode))).at(0)));
            for (int i = 1; i < sObj.results.xVect.at(mObj.XtoTraceMap.at(mObj.relToXMap.at(mObj.labelNodes.at(devToHandle.at(0)).posNode))).size(); i++) {
              result.calcData.emplace_back(((iObj.transSim.prstep * EV) / HBAR)
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
          if (traces.at(i).name.substr(0, 2) == "NV") {
            tokens = Misc::tokenize_delimeter(traces.at(i).name, "_");
            if (tokens.size() > 2)
              outfile << " " << i + 1 << " v(" << tokens[1] << "," << tokens[2]
                      << ") V\n";
            else
              outfile << " " << i + 1 << " v(" << tokens[1] << ") V\n";
          } else
            outfile << " " << i + 1 << " " << traces.at(i).name << " V\n";
        } else if (traces.at(i).type == 'P') {
          if (traces.at(i).name.substr(0, 2) == "NP") {
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
                  << "@" << label << "[c] C\n";
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
