// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#include "JoSIM/j_matrix.h"

#include <string>

using namespace JoSIM;

void Matrix::create_matrix(Input &iObj)
{
  if (iObj.argAnal == AnalysisType::Voltage)
    Matrix::create_A_volt(iObj);
  else if (iObj.argAnal == AnalysisType::Phase)
    Matrix::create_A_phase(iObj);
  Matrix::create_CSR();
}

void Matrix::create_A_volt(Input &iObj)
{
  std::string cName, rName, cNameP, rNameP, cNameN, rNameN;
  std::vector<std::string> devicetokens, componentLabels;
  devicetokens.clear();
  componentLabels.clear();
  std::string label, nodeP, nodeN, subckt = "";
  std::unordered_map<std::string, int> rowMap, columnMap;
  rowMap.clear();
  columnMap.clear();
  int rowCounter, colCounter, expStart, expEnd, nodeCounter;
  bool pGND, nGND;
  rowCounter = colCounter = nodeCounter = 0;
  for (auto i : iObj.expNetlist) {
    devicetokens = Misc::tokenize_space(i.first);
    // Parse {} expressions
    expStart = expEnd = -1;
    for (int t = 0; t < devicetokens.size(); t++) {
      if (devicetokens.at(t).find('{') != std::string::npos)
        expStart = t;
      if (devicetokens.at(t).find('}') != std::string::npos)
        expEnd = t;
    }
    if (expStart == -1 && expEnd != -1)
      Errors::invalid_component_errors(INVALID_EXPR, i.first);
    else if (expStart != -1 && expEnd == -1)
      Errors::invalid_component_errors(INVALID_EXPR, i.first);
    if (expStart != -1 && expStart == expEnd) {
      devicetokens.at(expStart) = devicetokens.at(expStart).substr(
          devicetokens.at(expStart).find('{') + 1,
          devicetokens.at(expStart).size() - 1);
      devicetokens.at(expStart) = devicetokens.at(expStart).substr(
          0, devicetokens.at(expStart).find('}'));
      devicetokens.at(expStart) = Misc::precise_to_string(
          Parser::parse_param(devicetokens.at(expStart),
                              iObj.parameters.parsedParams, i.second),
          25);
    } else if (expStart != -1 && expEnd != -1) {
      int d = expStart + 1;
      while (expStart != expEnd) {
        devicetokens.at(expStart) += devicetokens.at(d);
        devicetokens.erase(devicetokens.begin() + d);
        expEnd--;
      }
      devicetokens.at(expStart) = devicetokens.at(expStart).substr(
          devicetokens.at(expStart).find('{') + 1,
          devicetokens.at(expStart).size() - 1);
      devicetokens.at(expStart) = devicetokens.at(expStart).substr(
          0, devicetokens.at(expStart).find('}'));
      devicetokens.at(expStart) = Misc::precise_to_string(
          Parser::parse_param(devicetokens.at(expStart),
                              iObj.parameters.parsedParams, i.second),
          25);
    }
    // End of parse {} expressions
    double value = 0.0;
    try {
      label = devicetokens.at(0);
      if (std::find(componentLabels.begin(), componentLabels.end(), label) ==
          componentLabels.end()) {
        if (label.find_first_of("_*!@#$\\/%^&*()") != std::string::npos) {
          std::cerr << "W: The use of special characters in label names is not "
                       "advised."
                    << std::endl;
          std::cerr << "W: This might produce unexpected results." << std::endl;
          std::cerr << "W: Continuing operation." << std::endl;
        }
        componentLabels.push_back(label);
      } else {
        Errors::invalid_component_errors(DUPLICATE_LABEL, label);
      }
    } catch (std::exception &e) {
      Errors::invalid_component_errors(MISSING_LABEL, i.first);
    }
    try {
      nodeP = devicetokens[1];
    } catch (std::exception &e) {
      Errors::invalid_component_errors(MISSING_PNODE, i.first);
    }
    try {
      nodeN = devicetokens[2];
    } catch (std::exception &e) {
      Errors::invalid_component_errors(MISSING_NNODE, i.first);
    }
    /**************/
    /** RESISTOR **/
    /**************/
    if (i.first[0] == 'R') {
      matrix_element e;
      components.voltRes.emplace_back(res_volt());
      deviceLabelIndex[label].type = RowDescriptor::Type::VoltageResistor;
      deviceLabelIndex[label].index = components.voltRes.size() - 1;
      try {
        auto parameter_name = ParameterName(devicetokens[3], i.second);
        if (iObj.parameters.parsedParams.count(parameter_name) != 0) {
          value = iObj.parameters.parsedParams.at(parameter_name);
        } else {
          parameter_name = ParameterName(devicetokens[3], "");
          if (iObj.parameters.parsedParams.count(parameter_name) != 0)
            value = iObj.parameters.parsedParams.at(parameter_name);
          else
            value = Misc::modifier(devicetokens[3]);
        }
      } catch (std::exception &e) {
        Errors::invalid_component_errors(RES_ERROR, i.first);
      }
      components.voltRes.back().label = label;
      components.voltRes.back().value = value;
      if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
        cNameP = "C_NV" + nodeP;
        rNameP = "R_N" + nodeP;
        components.voltRes.back().posNodeR = nodeP;
        components.voltRes.back().posNodeC = nodeP;
        if (rowMap.count(nodeP) == 0) {
          if (nodeMap.count(nodeP) == 0) {
            nodeMap[nodeP] = nodeCounter++;
            nodeConnections.emplace_back(NodeConnections());
            nodeConnections.back().name = nodeP;
            nodeConnections.back().connections.emplace_back(
                ComponentConnections());
            nodeConnections.back().connections.back().type =
                ComponentConnections::Type::ResistorP;
            nodeConnections.back().connections.back().index =
                components.voltRes.size() - 1;
          }
          rowDesc.emplace_back(RowDescriptor());
          rowDesc.back().type = RowDescriptor::Type::VoltageNode;
          rowDesc.back().index = nodeMap.at(nodeP);
          deviceLabelIndex[nodeP].type = RowDescriptor::Type::VoltageNode;
          deviceLabelIndex[nodeP].index = rowCounter;
          rowMap[nodeP] = rowCounter++;
        } else {
          nodeConnections.at(nodeMap.at(nodeP))
              .connections.emplace_back(ComponentConnections());
          nodeConnections.at(nodeMap.at(nodeP)).connections.back().type =
              ComponentConnections::Type::ResistorP;
          nodeConnections.at(nodeMap.at(nodeP)).connections.back().index =
              components.voltRes.size() - 1;
        }
        if (columnMap.count(cNameP) == 0) {
          columnMap[cNameP] = colCounter;
          colCounter++;
        }
        pGND = false;
      } else
        pGND = true;
      if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
        cNameN = "C_NV" + nodeN;
        rNameN = "R_N" + nodeN;
        components.voltRes.back().negNodeR = nodeN;
        components.voltRes.back().negNodeC = nodeN;
        if (rowMap.count(nodeN) == 0) {
          if (nodeMap.count(nodeN) == 0) {
            nodeMap[nodeN] = nodeCounter++;
            nodeConnections.emplace_back(NodeConnections());
            nodeConnections.back().name = nodeN;
            nodeConnections.back().connections.emplace_back(
                ComponentConnections());
            nodeConnections.back().connections.back().type =
                ComponentConnections::Type::ResistorN;
            nodeConnections.back().connections.back().index =
                components.voltRes.size() - 1;
          }
          rowDesc.emplace_back(RowDescriptor());
          rowDesc.back().type = RowDescriptor::Type::VoltageNode;
          rowDesc.back().index = nodeMap.at(nodeN);
          deviceLabelIndex[nodeN].type = RowDescriptor::Type::VoltageNode;
          deviceLabelIndex[nodeN].index = rowCounter;
          rowMap[nodeN] = rowCounter++;
        } else {
          nodeConnections.at(nodeMap.at(nodeN))
              .connections.emplace_back(ComponentConnections());
          nodeConnections.at(nodeMap.at(nodeN)).connections.back().type =
              ComponentConnections::Type::ResistorN;
          nodeConnections.at(nodeMap.at(nodeN)).connections.back().index =
              components.voltRes.size() - 1;
        }
        if (columnMap.count(cNameN) == 0) {
          columnMap[cNameN] = colCounter;
          colCounter++;
        }
        nGND = false;
      } else
        nGND = true;
      if (!pGND) {
        e.label = label;
        components.voltRes.back().posNCol = e.colIndex = columnMap.at(cNameP);
        components.voltRes.back().posNRow = e.rowIndex = rowMap.at(nodeP);
        e.value = 1 / value;
        components.voltRes.back().ppPtr = mElements.size();
        mElements.push_back(e);
        if (!nGND) {
          e.label = label;
          components.voltRes.back().negNCol = e.colIndex = columnMap.at(cNameN);
          components.voltRes.back().posNRow = e.rowIndex = rowMap.at(nodeP);
          e.value = -1 / value;
          components.voltRes.back().pnPtr = mElements.size();
          mElements.push_back(e);
          e.label = label;
          components.voltRes.back().posNCol = e.colIndex = columnMap.at(cNameP);
          components.voltRes.back().negNRow = e.rowIndex = rowMap.at(nodeN);
          e.value = -1 / value;
          components.voltRes.back().npPtr = mElements.size();
          mElements.push_back(e);
          e.label = label;
          components.voltRes.back().negNCol = e.colIndex = columnMap.at(cNameN);
          components.voltRes.back().negNRow = e.rowIndex = rowMap.at(nodeN);
          e.value = 1 / value;
          components.voltRes.back().nnPtr = mElements.size();
          mElements.push_back(e);
        }
      } else if (!nGND) {
        e.label = label;
        components.voltRes.back().negNCol = e.colIndex = columnMap.at(cNameN);
        components.voltRes.back().negNRow = e.rowIndex = rowMap.at(nodeN);
        e.value = 1 / value;
        components.voltRes.back().nnPtr = mElements.size();
        mElements.push_back(e);
      }
    }
    /***************/
    /** CAPACITOR **/
    /***************/
    else if (i.first[0] == 'C') {
      matrix_element e;
      components.voltCap.emplace_back(cap_volt());
      deviceLabelIndex[label].type = RowDescriptor::Type::VoltageCapacitor;
      deviceLabelIndex[label].index = components.voltCap.size() - 1;
      try {
        auto parameter_name = ParameterName(devicetokens[3], i.second);
        if (iObj.parameters.parsedParams.count(parameter_name) != 0) {
          value = iObj.parameters.parsedParams.at(parameter_name);
        } else {
          parameter_name = ParameterName(devicetokens[3], "");
          if (iObj.parameters.parsedParams.count(parameter_name) != 0)
            value = iObj.parameters.parsedParams.at(parameter_name);
          else
            value = Misc::modifier(devicetokens[3]);
        }
      } catch (std::exception &e) {
        Errors::invalid_component_errors(CAP_ERROR, i.first);
      }
      components.voltCap.back().label = devicetokens.at(0);
      components.voltCap.back().value = value;
      cName = "C_I" + devicetokens[0];
      rName = "R_" + devicetokens[0];
      if (rowMap.count(devicetokens.at(0)) == 0) {
        rowDesc.emplace_back(RowDescriptor());
        rowDesc.back().type = RowDescriptor::Type::VoltageCapacitor;
        rowDesc.back().index = components.voltCap.size() - 1;
        rowMap[devicetokens.at(0)] = rowCounter++;
      }
      if (columnMap.count(devicetokens.at(0)) == 0) {
        columnMap[cName] = colCounter;
        colCounter++;
      }
      components.voltCap.back().curNodeC = devicetokens.at(0);
      components.voltCap.back().curNodeR = devicetokens.at(0);
      if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
        cNameP = "C_NV" + nodeP;
        rNameP = "R_N" + nodeP;
        components.voltCap.back().posNodeC = nodeP;
        components.voltCap.back().posNodeR = nodeP;
        if (rowMap.count(nodeP) == 0) {
          if (nodeMap.count(nodeP) == 0) {
            nodeMap[nodeP] = nodeCounter++;
            nodeConnections.emplace_back(NodeConnections());
            nodeConnections.back().name = nodeP;
            nodeConnections.back().connections.emplace_back(
                ComponentConnections());
            nodeConnections.back().connections.back().type =
                ComponentConnections::Type::CapacitorP;
            nodeConnections.back().connections.back().index =
                components.voltCap.size() - 1;
          }
          rowDesc.emplace_back(RowDescriptor());
          rowDesc.back().type = RowDescriptor::Type::VoltageNode;
          rowDesc.back().index = nodeMap.at(nodeP);
          deviceLabelIndex[nodeP].type = RowDescriptor::Type::VoltageNode;
          deviceLabelIndex[nodeP].index = rowCounter;
          rowMap[nodeP] = rowCounter++;
        } else {
          nodeConnections.at(nodeMap.at(nodeP))
              .connections.emplace_back(ComponentConnections());
          nodeConnections.at(nodeMap.at(nodeP)).connections.back().type =
              ComponentConnections::Type::CapacitorP;
          nodeConnections.at(nodeMap.at(nodeP)).connections.back().index =
              components.voltCap.size() - 1;
        }
        if (columnMap.count(cNameP) == 0) {
          columnMap[cNameP] = colCounter;
          colCounter++;
        }
        pGND = false;
      } else
        pGND = true;
      if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
        cNameN = "C_NV" + nodeN;
        rNameN = "R_N" + nodeN;
        components.voltCap.back().negNodeC = nodeN;
        components.voltCap.back().negNodeR = nodeN;
        if (rowMap.count(nodeN) == 0) {
          if (nodeMap.count(nodeN) == 0) {
            nodeMap[nodeN] = nodeCounter++;
            nodeConnections.emplace_back(NodeConnections());
            nodeConnections.back().name = nodeN;
            nodeConnections.back().connections.emplace_back(
                ComponentConnections());
            nodeConnections.back().connections.back().type =
                ComponentConnections::Type::CapacitorN;
            nodeConnections.back().connections.back().index =
                components.voltCap.size() - 1;
          }
          rowDesc.emplace_back(RowDescriptor());
          rowDesc.back().type = RowDescriptor::Type::VoltageNode;
          rowDesc.back().index = nodeMap.at(nodeN);
          deviceLabelIndex[nodeN].type = RowDescriptor::Type::VoltageNode;
          deviceLabelIndex[nodeN].index = rowCounter;
          rowMap[nodeN] = rowCounter++;
        } else {
          nodeConnections.at(nodeMap.at(nodeN))
              .connections.emplace_back(ComponentConnections());
          nodeConnections.at(nodeMap.at(nodeN)).connections.back().type =
              ComponentConnections::Type::CapacitorN;
          nodeConnections.at(nodeMap.at(nodeN)).connections.back().index =
              components.voltCap.size() - 1;
        }
        if (columnMap.count(cNameN) == 0) {
          columnMap[cNameN] = colCounter;
          colCounter++;
        }
        nGND = false;
      } else
        nGND = true;
      if (!pGND) {
        e.label = label;
        components.voltCap.back().posNCol = e.colIndex = columnMap.at(cNameP);
        components.voltCap.back().curNRow = e.rowIndex =
            rowMap.at(devicetokens.at(0));
        e.value = 1;
        mElements.push_back(e);
        e.label = label;
        components.voltCap.back().curNCol = e.colIndex = columnMap.at(cName);
        components.voltCap.back().posNRow = e.rowIndex = rowMap.at(nodeP);
        e.value = 1;
        mElements.push_back(e);
        if (!nGND) {
          e.label = label;
          components.voltCap.back().negNCol = e.colIndex = columnMap.at(cNameN);
          components.voltCap.back().curNRow = e.rowIndex =
              rowMap.at(devicetokens.at(0));
          e.value = -1;
          mElements.push_back(e);
          e.label = label;
          components.voltCap.back().curNCol = e.colIndex = columnMap.at(cName);
          components.voltCap.back().negNRow = e.rowIndex = rowMap.at(nodeN);
          e.value = -1;
          mElements.push_back(e);
        }
      } else if (!nGND) {
        e.label = label;
        components.voltCap.back().negNCol = e.colIndex = columnMap.at(cNameN);
        components.voltCap.back().curNRow = e.rowIndex =
            rowMap.at(devicetokens.at(0));
        e.value = -1;
        mElements.push_back(e);
        e.label = label;
        components.voltCap.back().curNCol = e.colIndex = columnMap.at(cName);
        components.voltCap.back().negNRow = e.rowIndex = rowMap.at(nodeN);
        e.value = -1;
        mElements.push_back(e);
      }
      e.label = label;
      components.voltCap.back().curNCol = e.colIndex = columnMap.at(cName);
      components.voltCap.back().curNRow = e.rowIndex =
          rowMap.at(devicetokens.at(0));
      e.value = iObj.transSim.prstep / (2 * value);
      components.voltCap.back().capPtr = mElements.size();
      mElements.push_back(e);
    }
    /**************/
    /** INDUCTOR **/
    /**************/
    else if (i.first[0] == 'L') {
      matrix_element e;
      components.voltInd.emplace_back(ind_volt());
      deviceLabelIndex[label].type = RowDescriptor::Type::VoltageInductor;
      deviceLabelIndex[label].index = components.voltInd.size() - 1;
      try {
        auto parameter_name = ParameterName(devicetokens[3], i.second);
        if (iObj.parameters.parsedParams.count(parameter_name) != 0) {
          value = iObj.parameters.parsedParams.at(parameter_name);
        } else {
          parameter_name = ParameterName(devicetokens[3], "");
          if (iObj.parameters.parsedParams.count(parameter_name) != 0)
            value = iObj.parameters.parsedParams.at(parameter_name);
          else
            value = Misc::modifier(devicetokens[3]);
        }
      } catch (std::exception &e) {
        Errors::invalid_component_errors(IND_ERROR, i.first);
      }
      components.voltInd.back().label = label;
      components.voltInd.back().value = value;
      cName = "C_I" + devicetokens[0];
      rName = "R_" + devicetokens[0];
      if (rowMap.count(devicetokens.at(0)) == 0) {
        rowDesc.emplace_back(RowDescriptor());
        rowDesc.back().type = RowDescriptor::Type::VoltageInductor;
        rowDesc.back().index = components.voltInd.size() - 1;
        rowMap[devicetokens.at(0)] = rowCounter++;
      }
      if (columnMap.count(devicetokens.at(0)) == 0) {
        columnMap[cName] = colCounter;
        colCounter++;
      }
      components.voltInd.back().curNodeC = devicetokens.at(0);
      components.voltInd.back().curNodeR = devicetokens.at(0);
      if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
        cNameP = "C_NV" + nodeP;
        rNameP = "R_N" + nodeP;
        components.voltInd.back().posNodeC = nodeP;
        components.voltInd.back().posNodeR = nodeP;
        if (rowMap.count(nodeP) == 0) {
          if (nodeMap.count(nodeP) == 0) {
            nodeMap[nodeP] = nodeCounter++;
            nodeConnections.emplace_back(NodeConnections());
            nodeConnections.back().name = nodeP;
            nodeConnections.back().connections.emplace_back(
                ComponentConnections());
            nodeConnections.back().connections.back().type =
                ComponentConnections::Type::InductorP;
            nodeConnections.back().connections.back().index =
                components.voltInd.size() - 1;
          }
          rowDesc.emplace_back(RowDescriptor());
          rowDesc.back().type = RowDescriptor::Type::VoltageNode;
          rowDesc.back().index = nodeMap.at(nodeP);
          deviceLabelIndex[nodeP].type = RowDescriptor::Type::VoltageNode;
          deviceLabelIndex[nodeP].index = rowCounter;
          rowMap[nodeP] = rowCounter++;
        } else {
          nodeConnections.at(nodeMap.at(nodeP))
              .connections.emplace_back(ComponentConnections());
          nodeConnections.at(nodeMap.at(nodeP)).connections.back().type =
              ComponentConnections::Type::InductorP;
          nodeConnections.at(nodeMap.at(nodeP)).connections.back().index =
              components.voltInd.size() - 1;
        }
        if (columnMap.count(cNameP) == 0) {
          columnMap[cNameP] = colCounter;
          colCounter++;
        }
        pGND = false;
      } else
        pGND = true;
      if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
        cNameN = "C_NV" + nodeN;
        rNameN = "R_N" + nodeN;
        components.voltInd.back().negNodeC = nodeN;
        components.voltInd.back().negNodeR = nodeN;
        if (rowMap.count(nodeN) == 0) {
          if (nodeMap.count(nodeN) == 0) {
            nodeMap[nodeN] = nodeCounter++;
            nodeConnections.emplace_back(NodeConnections());
            nodeConnections.back().name = nodeN;
            nodeConnections.back().connections.emplace_back(
                ComponentConnections());
            nodeConnections.back().connections.back().type =
                ComponentConnections::Type::InductorN;
            nodeConnections.back().connections.back().index =
                components.voltInd.size() - 1;
          }
          rowDesc.emplace_back(RowDescriptor());
          rowDesc.back().type = RowDescriptor::Type::VoltageNode;
          rowDesc.back().index = nodeMap.at(nodeN);
          deviceLabelIndex[nodeN].type = RowDescriptor::Type::VoltageNode;
          deviceLabelIndex[nodeN].index = rowCounter;
          rowMap[nodeN] = rowCounter++;
        } else {
          nodeConnections.at(nodeMap.at(nodeN))
              .connections.emplace_back(ComponentConnections());
          nodeConnections.at(nodeMap.at(nodeN)).connections.back().type =
              ComponentConnections::Type::InductorN;
          nodeConnections.at(nodeMap.at(nodeN)).connections.back().index =
              components.voltInd.size() - 1;
        }
        if (columnMap.count(cNameN) == 0) {
          columnMap[cNameN] = colCounter;
          colCounter++;
        }
        nGND = false;
      } else
        nGND = true;
      if (!pGND) {
        e.label = label;
        components.voltInd.back().curNCol = e.colIndex = columnMap.at(cName);
        components.voltInd.back().posNRow = e.rowIndex = rowMap.at(nodeP);
        e.value = 1;
        mElements.push_back(e);
        e.label = label;
        components.voltInd.back().posNCol = e.colIndex = columnMap.at(cNameP);
        components.voltInd.back().curNRow = e.rowIndex =
            rowMap.at(devicetokens.at(0));
        e.value = 1;
        mElements.push_back(e);
        if (!nGND) {
          e.label = label;
          components.voltInd.back().curNCol = e.colIndex = columnMap.at(cName);
          components.voltInd.back().negNRow = e.rowIndex = rowMap.at(nodeN);
          e.value = -1;
          mElements.push_back(e);
          e.label = label;
          components.voltInd.back().negNCol = e.colIndex = columnMap.at(cNameN);
          components.voltInd.back().curNRow = e.rowIndex =
              rowMap.at(devicetokens.at(0));
          e.value = -1;
          mElements.push_back(e);
        }
      } else if (!nGND) {
        e.label = label;
        components.voltInd.back().curNCol = e.colIndex = columnMap.at(cName);
        components.voltInd.back().negNRow = e.rowIndex = rowMap.at(nodeN);
        e.value = -1;
        mElements.push_back(e);
        e.label = label;
        components.voltInd.back().negNCol = e.colIndex = columnMap.at(cNameN);
        components.voltInd.back().curNRow = e.rowIndex =
            rowMap.at(devicetokens.at(0));
        e.value = -1;
        mElements.push_back(e);
      }
      e.label = label;
      components.voltInd.back().curNCol = e.colIndex = columnMap.at(cName);
      components.voltInd.back().curNRow = e.rowIndex =
          rowMap.at(devicetokens.at(0));
      e.value = (-2 * value) / iObj.transSim.prstep;
      components.voltInd.back().indPtr = mElements.size();
      mElements.push_back(e);
    }
    /********************/
    /** VOLTAGE SOURCE **/
    /********************/
    else if (i.first[0] == 'V') {
      matrix_element e;
      sources.emplace_back(Misc::parse_function(i.first, iObj, i.second));
      components.voltVs.emplace_back(vs_volt());
      deviceLabelIndex[label].type = RowDescriptor::Type::VoltageVS;
      deviceLabelIndex[label].index = sources.size() - 1;
      cName = "C_" + devicetokens[0];
      rName = "R_" + devicetokens[0];
      if (rowMap.count(devicetokens.at(0)) == 0) {
        rowDesc.emplace_back(RowDescriptor());
        rowDesc.back().type = RowDescriptor::Type::VoltageVS;
        rowDesc.back().index = sources.size() - 1;
        rowMap[devicetokens.at(0)] = rowCounter++;
      }
      if (columnMap.count(devicetokens.at(0)) == 0) {
        columnMap[cName] = colCounter;
        colCounter++;
      }
      components.voltVs.back().curNodeC = devicetokens.at(0);
      components.voltVs.back().curNodeR = devicetokens.at(0);
      if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
        cNameP = "C_NV" + nodeP;
        rNameP = "R_N" + nodeP;
        components.voltVs.back().posNodeC = nodeP;
        components.voltVs.back().posNodeR = nodeP;
        if (rowMap.count(nodeP) == 0) {
          if (nodeMap.count(nodeP) == 0) {
            nodeMap[nodeP] = nodeCounter++;
            nodeConnections.emplace_back(NodeConnections());
            nodeConnections.back().name = nodeP;
            nodeConnections.back().connections.emplace_back(
                ComponentConnections());
            nodeConnections.back().connections.back().type =
                ComponentConnections::Type::VSP;
            nodeConnections.back().connections.back().index =
                sources.size() - 1;
          }
          rowDesc.emplace_back(RowDescriptor());
          rowDesc.back().type = RowDescriptor::Type::VoltageNode;
          rowDesc.back().index = nodeMap.at(nodeP);
          deviceLabelIndex[nodeP].type = RowDescriptor::Type::VoltageNode;
          deviceLabelIndex[nodeP].index = rowCounter;
          rowMap[nodeP] = rowCounter++;
        } else {
          nodeConnections.at(nodeMap.at(nodeP))
              .connections.emplace_back(ComponentConnections());
          nodeConnections.at(nodeMap.at(nodeP)).connections.back().type =
              ComponentConnections::Type::VSP;
          nodeConnections.at(nodeMap.at(nodeP)).connections.back().index =
              sources.size() - 1;
        }
        if (columnMap.count(cNameP) == 0) {
          columnMap[cNameP] = colCounter;
          colCounter++;
        }
        pGND = false;
      } else
        pGND = true;
      if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
        cNameN = "C_NV" + nodeN;
        rNameN = "R_N" + nodeN;
        components.voltVs.back().negNodeC = nodeN;
        components.voltVs.back().negNodeR = nodeN;
        if (rowMap.count(nodeN) == 0) {
          if (nodeMap.count(nodeN) == 0) {
            nodeMap[nodeN] = nodeCounter++;
            nodeConnections.emplace_back(NodeConnections());
            nodeConnections.back().name = nodeN;
            nodeConnections.back().connections.emplace_back(
                ComponentConnections());
            nodeConnections.back().connections.back().type =
                ComponentConnections::Type::VSN;
            nodeConnections.back().connections.back().index =
                sources.size() - 1;
          }
          rowDesc.emplace_back(RowDescriptor());
          rowDesc.back().type = RowDescriptor::Type::VoltageNode;
          rowDesc.back().index = nodeMap.at(nodeN);
          deviceLabelIndex[nodeN].type = RowDescriptor::Type::VoltageNode;
          deviceLabelIndex[nodeN].index = rowCounter;
          rowMap[nodeN] = rowCounter++;
        } else {
          nodeConnections.at(nodeMap.at(nodeN))
              .connections.emplace_back(ComponentConnections());
          nodeConnections.at(nodeMap.at(nodeN)).connections.back().type =
              ComponentConnections::Type::VSN;
          nodeConnections.at(nodeMap.at(nodeN)).connections.back().index =
              sources.size() - 1;
        }
        if (columnMap.count(cNameN) == 0) {
          columnMap[cNameN] = colCounter;
          colCounter++;
        }
        nGND = false;
      } else
        nGND = true;
      if (!pGND) {
        e.label = label;
        components.voltVs.back().curNCol = e.colIndex = columnMap.at(cName);
        components.voltVs.back().posNRow = e.rowIndex = rowMap.at(nodeP);
        e.value = 1;
        mElements.push_back(e);
        e.label = label;
        components.voltVs.back().posNCol = e.colIndex = columnMap.at(cNameP);
        components.voltVs.back().curNRow = e.rowIndex =
            rowMap.at(devicetokens.at(0));
        e.value = 1;
        mElements.push_back(e);
        if (!nGND) {
          e.label = label;
          components.voltVs.back().curNCol = e.colIndex = columnMap.at(cName);
          components.voltVs.back().negNRow = e.rowIndex = rowMap.at(nodeN);
          e.value = -1;
          mElements.push_back(e);
          e.label = label;
          components.voltVs.back().negNCol = e.colIndex = columnMap.at(cNameN);
          components.voltVs.back().curNRow = e.rowIndex =
              rowMap.at(devicetokens.at(0));
          e.value = -1;
          mElements.push_back(e);
        }
      } else if (!nGND) {
        e.label = label;
        components.voltVs.back().curNCol = e.colIndex = columnMap.at(cName);
        components.voltVs.back().negNRow = e.rowIndex = rowMap.at(nodeN);
        e.value = -1;
        mElements.push_back(e);
        e.label = label;
        components.voltVs.back().negNCol = e.colIndex = columnMap.at(cNameN);
        components.voltVs.back().curNRow = e.rowIndex =
            rowMap.at(devicetokens.at(0));
        e.value = -1;
        mElements.push_back(e);
      }
    }
    /********************/
    /** CURRENT SOURCE **/
    /********************/
    else if (i.first[0] == 'I') {
      sources.emplace_back(Misc::parse_function(i.first, iObj, i.second));
      deviceLabelIndex[label].type = RowDescriptor::Type::VoltageCS;
      deviceLabelIndex[label].index = sources.size() - 1;
      if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
        cNameP = "C_NV" + nodeP;
        rNameP = "R_N" + nodeP;
        if (rowMap.count(nodeP) == 0) {
          if (nodeMap.count(nodeP) == 0) {
            nodeMap[nodeP] = nodeCounter++;
            nodeConnections.emplace_back(NodeConnections());
            nodeConnections.back().name = nodeP;
            nodeConnections.back().connections.emplace_back(
                ComponentConnections());
            nodeConnections.back().connections.back().type =
                ComponentConnections::Type::CSP;
            nodeConnections.back().connections.back().index =
                sources.size() - 1;
          }
          rowDesc.emplace_back(RowDescriptor());
          rowDesc.back().type = RowDescriptor::Type::VoltageNode;
          rowDesc.back().index = nodeMap.at(nodeP);
          deviceLabelIndex[nodeP].type = RowDescriptor::Type::VoltageNode;
          deviceLabelIndex[nodeP].index = rowCounter;
          rowMap[nodeP] = rowCounter++;
        } else {
          nodeConnections.at(nodeMap.at(nodeP))
              .connections.emplace_back(ComponentConnections());
          nodeConnections.at(nodeMap.at(nodeP)).connections.back().type =
              ComponentConnections::Type::CSP;
          nodeConnections.at(nodeMap.at(nodeP)).connections.back().index =
              sources.size() - 1;
        }
        if (columnMap.count(cNameP) == 0) {
          columnMap[cNameP] = colCounter;
          colCounter++;
        }
      }
      if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
        cNameN = "C_NV" + nodeN;
        rNameN = "R_N" + nodeN;
        if (rowMap.count(nodeN) == 0) {
          if (nodeMap.count(nodeN) == 0) {
            nodeMap[nodeN] = nodeCounter++;
            nodeConnections.emplace_back(NodeConnections());
            nodeConnections.back().name = nodeN;
            nodeConnections.back().connections.emplace_back(
                ComponentConnections());
            nodeConnections.back().connections.back().type =
                ComponentConnections::Type::CSN;
            nodeConnections.back().connections.back().index =
                sources.size() - 1;
          }
          rowDesc.emplace_back(RowDescriptor());
          rowDesc.back().type = RowDescriptor::Type::VoltageNode;
          rowDesc.back().index = nodeMap.at(nodeN);
          deviceLabelIndex[nodeN].type = RowDescriptor::Type::VoltageNode;
          deviceLabelIndex[nodeN].index = rowCounter;
          rowMap[nodeN] = rowCounter++;
        } else {
          nodeConnections.at(nodeMap.at(nodeN))
              .connections.emplace_back(ComponentConnections());
          nodeConnections.at(nodeMap.at(nodeN)).connections.back().type =
              ComponentConnections::Type::CSN;
          nodeConnections.at(nodeMap.at(nodeN)).connections.back().index =
              sources.size() - 1;
        }
        if (columnMap.count(cNameN) == 0) {
          columnMap[cNameN] = colCounter;
          colCounter++;
        }
      }
    }
    /************************/
    /** JOSEPHSON JUNCTION **/
    /************************/
    else if (i.first[0] == 'B') {
      matrix_element e;
      components.voltJJ.emplace_back(jj_volt());
      deviceLabelIndex[label].type = RowDescriptor::Type::VoltageJJ;
      deviceLabelIndex[label].index = components.voltJJ.size() - 1;
      std::string modelstring = "", area = "";
      for (int t = devicetokens.size() - 1; t > 2; t--) {
        if (devicetokens[t].find('=') == std::string::npos) {
          if (iObj.netlist.models.count(
                  std::make_pair(devicetokens[t], i.second)) != 0) {
            modelstring = iObj.netlist.models.at(
                std::make_pair(devicetokens[t], i.second));
            break;
          } else if (iObj.netlist.models.count(
                         std::make_pair(devicetokens[t], "")) != 0) {
            modelstring =
                iObj.netlist.models.at(std::make_pair(devicetokens[t], ""));
            break;
          } else {
            Errors::invalid_component_errors(MODEL_NOT_DEFINED,
                                             devicetokens[t]);
            break;
          }
        }
        if (devicetokens[t].find("AREA") != std::string::npos) {
          area = devicetokens[t].substr(devicetokens[t].find_first_of('=') + 1,
                                        devicetokens[t].size() - 1);
        }
      }
      if (area == "" && iObj.argVerb)
        Errors::invalid_component_errors(MODEL_AREA_NOT_GIVEN, label);
      components.jj_model(modelstring, area, components.voltJJ.size() - 1, iObj,
                          i.second);
      components.voltJJ.back().label = label;
      cName = "C_P" + devicetokens[0];
      rName = "R_" + devicetokens[0];
      components.voltJJ.back().phaseNodeC = devicetokens.at(0);
      components.voltJJ.back().phaseNodeR = devicetokens.at(0);
      if (rowMap.count(devicetokens.at(0)) == 0) {
        rowDesc.emplace_back(RowDescriptor());
        rowDesc.back().type = RowDescriptor::Type::VoltageJJ;
        rowDesc.back().index = components.voltJJ.size() - 1;
        rowMap[devicetokens.at(0)] = rowCounter++;
      }
      if (columnMap.count(devicetokens.at(0)) == 0) {
        columnMap[cName] = colCounter;
        colCounter++;
      }
      if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
        cNameP = "C_NV" + nodeP;
        rNameP = "R_N" + nodeP;
        components.voltJJ.back().posNodeC = nodeP;
        components.voltJJ.back().posNodeR = nodeP;
        if (rowMap.count(nodeP) == 0) {
          if (nodeMap.count(nodeP) == 0) {
            nodeMap[nodeP] = nodeCounter++;
            nodeConnections.emplace_back(NodeConnections());
            nodeConnections.back().name = nodeP;
            nodeConnections.back().connections.emplace_back(
                ComponentConnections());
            nodeConnections.back().connections.back().type =
                ComponentConnections::Type::JJP;
            nodeConnections.back().connections.back().index =
                components.voltJJ.size() - 1;
          }
          rowDesc.emplace_back(RowDescriptor());
          rowDesc.back().type = RowDescriptor::Type::VoltageNode;
          rowDesc.back().index = nodeMap.at(nodeP);
          deviceLabelIndex[nodeP].type = RowDescriptor::Type::VoltageNode;
          deviceLabelIndex[nodeP].index = rowCounter;
          rowMap[nodeP] = rowCounter++;
        } else {
          nodeConnections.at(nodeMap.at(nodeP))
              .connections.emplace_back(ComponentConnections());
          nodeConnections.at(nodeMap.at(nodeP)).connections.back().type =
              ComponentConnections::Type::JJP;
          nodeConnections.at(nodeMap.at(nodeP)).connections.back().index =
              components.voltJJ.size() - 1;
        }
        if (columnMap.count(cNameP) == 0) {
          columnMap[cNameP] = colCounter;
          colCounter++;
        }
        pGND = false;
      } else
        pGND = true;
      if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
        cNameN = "C_NV" + nodeN;
        rNameN = "R_N" + nodeN;
        components.voltJJ.back().negNodeC = nodeN;
        components.voltJJ.back().negNodeR = nodeN;
        if (rowMap.count(nodeN) == 0) {
          if (nodeMap.count(nodeN) == 0) {
            nodeMap[nodeN] = nodeCounter++;
            nodeConnections.emplace_back(NodeConnections());
            nodeConnections.back().name = nodeN;
            nodeConnections.back().connections.emplace_back(
                ComponentConnections());
            nodeConnections.back().connections.back().type =
                ComponentConnections::Type::JJN;
            nodeConnections.back().connections.back().index =
                components.voltJJ.size() - 1;
          }
          rowDesc.emplace_back(RowDescriptor());
          rowDesc.back().type = RowDescriptor::Type::VoltageNode;
          rowDesc.back().index = nodeMap.at(nodeN);
          deviceLabelIndex[nodeN].type = RowDescriptor::Type::VoltageNode;
          deviceLabelIndex[nodeN].index = rowCounter;
          rowMap[nodeN] = rowCounter++;
        } else {
          nodeConnections.at(nodeMap.at(nodeN))
              .connections.emplace_back(ComponentConnections());
          nodeConnections.at(nodeMap.at(nodeN)).connections.back().type =
              ComponentConnections::Type::JJN;
          nodeConnections.at(nodeMap.at(nodeN)).connections.back().index =
              components.voltJJ.size() - 1;
        }
        if (columnMap.count(cNameN) == 0) {
          columnMap[cNameN] = colCounter;
          colCounter++;
        }
        nGND = false;
      } else
        nGND = true;
      if (!pGND) {
        e.label = label;
        components.voltJJ.back().posNCol = e.colIndex = columnMap.at(cNameP);
        components.voltJJ.back().posNRow = e.rowIndex = rowMap.at(nodeP);
        e.value = ((2 * components.voltJJ.back().C) / iObj.transSim.prstep) +
                  (1 / components.voltJJ.back().r0);
        components.voltJJ.back().ppPtr = mElements.size();
        mElements.push_back(e);
        e.label = label;
        components.voltJJ.back().posNCol = e.colIndex = columnMap.at(cNameP);
        components.voltJJ.back().phaseNRow = e.rowIndex =
            rowMap.at(devicetokens.at(0));
        e.value = (-iObj.transSim.prstep / 2) * ((2 * M_PI) / PHI_ZERO);
        mElements.push_back(e);
        if (!nGND) {
          e.label = label;
          components.voltJJ.back().negNCol = e.colIndex = columnMap.at(cNameN);
          components.voltJJ.back().posNRow = e.rowIndex = rowMap.at(nodeP);
          e.value =
              -(((2 * components.voltJJ.back().C) / iObj.transSim.prstep) +
                (1 / components.voltJJ.back().r0));
          components.voltJJ.back().npPtr = mElements.size();
          mElements.push_back(e);
          e.label = label;
          components.voltJJ.back().posNCol = e.colIndex = columnMap.at(cNameP);
          components.voltJJ.back().negNRow = e.rowIndex = rowMap.at(nodeN);
          e.value =
              -(((2 * components.voltJJ.back().C) / iObj.transSim.prstep) +
                (1 / components.voltJJ.back().r0));
          components.voltJJ.back().pnPtr = mElements.size();
          mElements.push_back(e);
          e.label = label;
          components.voltJJ.back().negNCol = e.colIndex = columnMap.at(cNameN);
          components.voltJJ.back().negNRow = e.rowIndex = rowMap.at(nodeN);
          e.value = ((2 * components.voltJJ.back().C) / iObj.transSim.prstep) +
                    (1 / components.voltJJ.back().r0);
          components.voltJJ.back().nnPtr = mElements.size();
          mElements.push_back(e);
          e.label = label;
          components.voltJJ.back().negNCol = e.colIndex = columnMap.at(cNameN);
          components.voltJJ.back().phaseNRow = e.rowIndex =
              rowMap.at(devicetokens.at(0));
          e.value = (iObj.transSim.prstep / 2) * ((2 * M_PI) / PHI_ZERO);
          mElements.push_back(e);
        }
      } else if (!nGND) {
        e.label = label;
        components.voltJJ.back().negNCol = e.colIndex = columnMap.at(cNameN);
        components.voltJJ.back().negNRow = e.rowIndex = rowMap.at(nodeN);
        e.value = ((2 * components.voltJJ.back().C) / iObj.transSim.prstep) +
                  (1 / components.voltJJ.back().r0);
        components.voltJJ.back().nnPtr = mElements.size();
        mElements.push_back(e);
        e.label = label;
        components.voltJJ.back().negNCol = e.colIndex = columnMap.at(cNameN);
        components.voltJJ.back().phaseNRow = e.rowIndex =
            rowMap.at(devicetokens.at(0));
        e.value = (iObj.transSim.prstep / 2) * ((2 * M_PI) / PHI_ZERO);
        mElements.push_back(e);
      }
      e.label = label;
      components.voltJJ.back().phaseNCol = e.colIndex = columnMap.at(cName);
      components.voltJJ.back().phaseNRow = e.rowIndex =
          rowMap.at(devicetokens.at(0));
      e.value = 1;
      mElements.push_back(e);
      components.voltJJ.back().gLarge =
          components.voltJJ.back().iC /
          (components.voltJJ.back().iCFact * components.voltJJ.back().delV);
      components.voltJJ.back().lowerB =
          components.voltJJ.back().vG - 0.5 * components.voltJJ.back().delV;
      components.voltJJ.back().upperB =
          components.voltJJ.back().vG + 0.5 * components.voltJJ.back().delV;
      components.voltJJ.back().subCond =
          1 / components.voltJJ.back().r0 +
          ((2 * components.voltJJ.back().C) / iObj.transSim.prstep);
      components.voltJJ.back().transCond =
          components.voltJJ.back().gLarge +
          ((2 * components.voltJJ.back().C) / iObj.transSim.prstep);
      components.voltJJ.back().normalCond =
          1 / components.voltJJ.back().rN +
          ((2 * components.voltJJ.back().C) / iObj.transSim.prstep);
      components.voltJJ.back().Del0 =
          1.76 * BOLTZMANN * components.voltJJ.back().tC;
      components.voltJJ.back().Del =
          components.voltJJ.back().Del0 *
          sqrt(cos((M_PI / 2) *
                   (components.voltJJ.back().T / components.voltJJ.back().tC) *
                   (components.voltJJ.back().T / components.voltJJ.back().tC)));
      components.voltJJ.back().rNCalc =
          ((M_PI * components.voltJJ.back().Del) /
           (2 * EV * components.voltJJ.back().iC)) *
          tanh(components.voltJJ.back().Del /
               (2 * BOLTZMANN * components.voltJJ.back().T));
      components.voltJJ.back().iS =
          -components.voltJJ.back().iC * sin(components.voltJJ.back().phi0);
    }
    /***********************/
    /** TRANSMISSION LINE **/
    /***********************/
    else if (i.first[0] == 'T') {
      std::string cName2, rName2, cNameP2, rNameP2, cNameN2,
                  rNameN2, nodeP2, nodeN2;
      bool pGND2, nGND2;
      matrix_element e;
      components.txLine.emplace_back(tx_line());
      deviceLabelIndex[label].type = RowDescriptor::Type::VoltageTX;
      deviceLabelIndex[label].index = components.txLine.size() - 1;
      double z0 = 10, tD = 0.0;
      for (int t = 5; t < devicetokens.size(); t++) {
        if (devicetokens[t].find("TD") != std::string::npos)
          tD = Parser::parse_param(
              devicetokens[t].substr(devicetokens[t].find("TD=") + 3,
                                     devicetokens[t].size() - 1),
              iObj.parameters.parsedParams, i.second);
        else if (devicetokens[t].find("Z0") != std::string::npos)
          z0 = Parser::parse_param(
              devicetokens[t].substr(devicetokens[t].find("Z0=") + 3,
                                     devicetokens[t].size() - 1),
              iObj.parameters.parsedParams, i.second);
        else if (devicetokens[t].find("LOSSLESS") != std::string::npos) {
        } else
          Errors::invalid_component_errors(INVALID_TX_DEFINED, i.first);
      }
      components.txLine.back().label = label;
      components.txLine.back().k = tD / iObj.transSim.prstep;
      components.txLine.back().value = z0;
      cName = "C_I1" + devicetokens[0];
      rName = devicetokens[0] + "-I1";
      cName2 = "C_I2" + devicetokens[0];
      rName2 = devicetokens[0] + "-I2";
      if (rowMap.count(rName) == 0) {
        rowDesc.emplace_back(RowDescriptor());
        rowDesc.back().type = RowDescriptor::Type::VoltageTX1;
        rowDesc.back().index = components.txLine.size() - 1;
        rowMap[rName] = rowCounter++;
      }
      if (columnMap.count(devicetokens.at(0)) == 0) {
        columnMap[cName] = colCounter;
        colCounter++;
      }
      if (rowMap.count(rName2) == 0) {
        rowDesc.emplace_back(RowDescriptor());
        rowDesc.back().type = RowDescriptor::Type::VoltageTX2;
        rowDesc.back().index = components.txLine.size() - 1;
        rowMap[rName2] = rowCounter++;
      }
      if (columnMap.count(cName2) == 0) {
        columnMap[cName2] = colCounter;
        colCounter++;
      }
      components.txLine.back().curNode1C = rName;
      components.txLine.back().curNode1R = cName;
      components.txLine.back().curNode2C = cName2;
      components.txLine.back().curNode2R = rName2;
      if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
        cNameP = "C_NV" + nodeP;
        rNameP = "R_N" + nodeP;
        components.txLine.back().posNodeC = nodeP;
        components.txLine.back().posNodeR = nodeP;
        if (rowMap.count(nodeP) == 0) {
          if (nodeMap.count(nodeP) == 0) {
            nodeMap[nodeP] = nodeCounter++;
            nodeConnections.emplace_back(NodeConnections());
            nodeConnections.back().name = nodeP;
            nodeConnections.back().connections.emplace_back(
                ComponentConnections());
            nodeConnections.back().connections.back().type =
                ComponentConnections::Type::TXP1;
            nodeConnections.back().connections.back().index =
                components.txLine.size() - 1;
          }
          rowDesc.emplace_back(RowDescriptor());
          rowDesc.back().type = RowDescriptor::Type::VoltageNode;
          rowDesc.back().index = nodeMap.at(nodeP);
          deviceLabelIndex[nodeP].type = RowDescriptor::Type::VoltageNode;
          deviceLabelIndex[nodeP].index = rowCounter;
          rowMap[nodeP] = rowCounter++;
        } else {
          nodeConnections.at(nodeMap.at(nodeP))
              .connections.emplace_back(ComponentConnections());
          nodeConnections.at(nodeMap.at(nodeP)).connections.back().type =
              ComponentConnections::Type::TXP1;
          nodeConnections.at(nodeMap.at(nodeP)).connections.back().index =
              components.txLine.size() - 1;
        }
        if (columnMap.count(cNameP) == 0) {
          columnMap[cNameP] = colCounter;
          colCounter++;
        }
        pGND = false;
      } else
        pGND = true;
      if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
        cNameN = "C_NV" + nodeN;
        rNameN = "R_N" + nodeN;
        components.txLine.back().negNodeC = nodeN;
        components.txLine.back().negNodeR = nodeN;
        if (rowMap.count(nodeN) == 0) {
          if (nodeMap.count(nodeN) == 0) {
            nodeMap[nodeN] = nodeCounter++;
            nodeConnections.emplace_back(NodeConnections());
            nodeConnections.back().name = nodeN;
            nodeConnections.back().connections.emplace_back(
                ComponentConnections());
            nodeConnections.back().connections.back().type =
                ComponentConnections::Type::TXN1;
            nodeConnections.back().connections.back().index =
                components.txLine.size() - 1;
          }
          rowDesc.emplace_back(RowDescriptor());
          rowDesc.back().type = RowDescriptor::Type::VoltageNode;
          rowDesc.back().index = nodeMap.at(nodeN);
          deviceLabelIndex[nodeN].type = RowDescriptor::Type::VoltageNode;
          deviceLabelIndex[nodeN].index = rowCounter;
          rowMap[nodeN] = rowCounter++;
        } else {
          nodeConnections.at(nodeMap.at(nodeN))
              .connections.emplace_back(ComponentConnections());
          nodeConnections.at(nodeMap.at(nodeN)).connections.back().type =
              ComponentConnections::Type::TXN1;
          nodeConnections.at(nodeMap.at(nodeN)).connections.back().index =
              components.txLine.size() - 1;
        }
        if (columnMap.count(cNameN) == 0) {
          columnMap[cNameN] = colCounter;
          colCounter++;
        }
        nGND = false;
      } else
        nGND = true;
      try {
        nodeP2 = devicetokens[3];
      } catch (std::exception &e) {
        Errors::invalid_component_errors(MISSING_PNODE, i.first);
      }
      /* Check if negative node exists, if not it's a bad device line definition
       */
      try {
        nodeN2 = devicetokens[4];
      } catch (std::exception &e) {
        Errors::invalid_component_errors(MISSING_NNODE, i.first);
      }
      if (nodeP2 != "0" && nodeP2.find("GND") == std::string::npos) {
        cNameP2 = "C_NV" + nodeP2;
        rNameP2 = "R_N" + nodeP2;
        components.txLine.back().posNode2C = nodeP2;
        components.txLine.back().posNode2R = nodeP2;
        if (rowMap.count(nodeP2) == 0) {
          if (nodeMap.count(nodeP2) == 0) {
            nodeMap[nodeP2] = nodeCounter++;
            nodeConnections.emplace_back(NodeConnections());
            nodeConnections.back().name = nodeP2;
            nodeConnections.back().connections.emplace_back(
                ComponentConnections());
            nodeConnections.back().connections.back().type =
                ComponentConnections::Type::TXP2;
            nodeConnections.back().connections.back().index =
                components.txLine.size() - 1;
          }
          rowDesc.emplace_back(RowDescriptor());
          rowDesc.back().type = RowDescriptor::Type::VoltageNode;
          rowDesc.back().index = nodeMap.at(nodeP2);
          deviceLabelIndex[nodeP2].type = RowDescriptor::Type::VoltageNode;
          deviceLabelIndex[nodeP2].index = rowCounter;
          rowMap[nodeP2] = rowCounter++;
        } else {
          nodeConnections.at(nodeMap.at(nodeP2))
              .connections.emplace_back(ComponentConnections());
          nodeConnections.at(nodeMap.at(nodeP2)).connections.back().type =
              ComponentConnections::Type::TXP2;
          nodeConnections.at(nodeMap.at(nodeP2)).connections.back().index =
              components.txLine.size() - 1;
        }
        if (columnMap.count(cNameP2) == 0) {
          columnMap[cNameP2] = colCounter;
          colCounter++;
        }
        pGND2 = false;
      } else
        pGND2 = true;
      if (nodeN2 != "0" && nodeN2.find("GND") == std::string::npos) {
        cNameN2 = "C_NV" + nodeN2;
        rNameN2 = "R_N" + nodeN2;
        components.txLine.back().negNode2C = nodeN2;
        components.txLine.back().negNode2R = nodeN2;
        if (rowMap.count(nodeN2) == 0) {
          if (nodeMap.count(nodeN2) == 0) {
            nodeMap[nodeN2] = nodeCounter++;
            nodeConnections.emplace_back(NodeConnections());
            nodeConnections.back().name = nodeN2;
            nodeConnections.back().connections.emplace_back(
                ComponentConnections());
            nodeConnections.back().connections.back().type =
                ComponentConnections::Type::TXN2;
            nodeConnections.back().connections.back().index =
                components.txLine.size() - 1;
          }
          rowDesc.emplace_back(RowDescriptor());
          rowDesc.back().type = RowDescriptor::Type::VoltageNode;
          rowDesc.back().index = nodeMap.at(nodeN2);
          deviceLabelIndex[nodeN2].type = RowDescriptor::Type::VoltageNode;
          deviceLabelIndex[nodeN2].index = rowCounter;
          rowMap[nodeN2] = rowCounter++;
        } else {
          nodeConnections.at(nodeMap.at(nodeN2))
              .connections.emplace_back(ComponentConnections());
          nodeConnections.at(nodeMap.at(nodeN2)).connections.back().type =
              ComponentConnections::Type::TXN2;
          nodeConnections.at(nodeMap.at(nodeN2)).connections.back().index =
              components.txLine.size() - 1;
        }
        if (columnMap.count(cNameN2) == 0) {
          columnMap[cNameN2] = colCounter;
          colCounter++;
        }
        nGND2 = false;
      } else
        nGND2 = true;
      if (!pGND) {
        e.label = label;
        components.txLine.back().curN1Col = e.colIndex = columnMap.at(cName);
        components.txLine.back().posNRow = e.rowIndex = rowMap.at(nodeP);
        if (std::find(relXInd.begin(), relXInd.end(), e.rowIndex) ==
            relXInd.end())
          relXInd.push_back(e.rowIndex);
        e.value = 1;
        mElements.push_back(e);
        e.label = label;
        components.txLine.back().posNCol = e.colIndex = columnMap.at(cNameP);
        components.txLine.back().curN1Row = e.rowIndex = rowMap.at(rName);
        if (std::find(relXInd.begin(), relXInd.end(), e.rowIndex) ==
            relXInd.end())
          relXInd.push_back(e.rowIndex);
        e.value = 1;
        mElements.push_back(e);
        if (!nGND) {
          e.label = label;
          components.txLine.back().curN1Col = e.colIndex = columnMap.at(cName);
          components.txLine.back().negNRow = e.rowIndex = rowMap.at(nodeN);
          if (std::find(relXInd.begin(), relXInd.end(), e.rowIndex) ==
              relXInd.end())
            relXInd.push_back(e.rowIndex);
          e.value = -1;
          mElements.push_back(e);
          e.label = label;
          components.txLine.back().negNCol = e.colIndex = columnMap.at(cNameN);
          components.txLine.back().curN1Row = e.rowIndex = rowMap.at(rName);
          if (std::find(relXInd.begin(), relXInd.end(), e.rowIndex) ==
              relXInd.end())
            relXInd.push_back(e.rowIndex);
          e.value = -1;
          mElements.push_back(e);
        }
      } else if (!nGND) {
        e.label = label;
        components.txLine.back().curN1Col = e.colIndex = columnMap.at(cName);
        components.txLine.back().negNRow = e.rowIndex = rowMap.at(nodeN);
        if (std::find(relXInd.begin(), relXInd.end(), e.rowIndex) ==
            relXInd.end())
          relXInd.push_back(e.rowIndex);
        e.value = -1;
        mElements.push_back(e);
        e.label = label;
        components.txLine.back().negNCol = e.colIndex = columnMap.at(cNameN);
        components.txLine.back().curN1Row = e.rowIndex =
            rowMap.at(devicetokens.at(0));
        if (std::find(relXInd.begin(), relXInd.end(), e.rowIndex) ==
            relXInd.end())
          relXInd.push_back(e.rowIndex);
        e.value = -1;
        mElements.push_back(e);
      }
      e.label = label;
      components.txLine.back().curN1Col = e.colIndex = columnMap.at(cName);
      components.txLine.back().curN1Row = e.rowIndex = rowMap.at(rName);
      if (std::find(relXInd.begin(), relXInd.end(), e.rowIndex) ==
          relXInd.end())
        relXInd.push_back(e.rowIndex);
      e.value = -z0;
      mElements.push_back(e);
      if (!pGND2) {
        e.label = label;
        components.txLine.back().curN2Col = e.colIndex = columnMap.at(cName2);
        components.txLine.back().posN2Row = e.rowIndex = rowMap.at(nodeP2);
        if (std::find(relXInd.begin(), relXInd.end(), e.rowIndex) ==
            relXInd.end())
          relXInd.push_back(e.rowIndex);
        e.value = 1;
        mElements.push_back(e);
        e.label = label;
        components.txLine.back().posN2Col = e.colIndex = columnMap.at(cNameP2);
        components.txLine.back().curN2Row = e.rowIndex = rowMap.at(rName2);
        if (std::find(relXInd.begin(), relXInd.end(), e.rowIndex) ==
            relXInd.end())
          relXInd.push_back(e.rowIndex);
        e.value = 1;
        mElements.push_back(e);
        if (!nGND2) {
          e.label = label;
          components.txLine.back().curN2Col = e.colIndex = columnMap.at(cName2);
          components.txLine.back().negN2Row = e.rowIndex = rowMap.at(nodeN2);
          if (std::find(relXInd.begin(), relXInd.end(), e.rowIndex) ==
              relXInd.end())
            relXInd.push_back(e.rowIndex);
          e.value = -1;
          mElements.push_back(e);
          e.label = label;
          components.txLine.back().negN2Col = e.colIndex =
              columnMap.at(cNameN2);
          components.txLine.back().curN2Row = e.rowIndex = rowMap.at(rName2);
          if (std::find(relXInd.begin(), relXInd.end(), e.rowIndex) ==
              relXInd.end())
            relXInd.push_back(e.rowIndex);
          e.value = -1;
          mElements.push_back(e);
        }
      } else if (!nGND2) {
        e.label = label;
        components.txLine.back().curN2Col = e.colIndex = columnMap.at(cName2);
        components.txLine.back().negN2Row = e.rowIndex = rowMap.at(nodeN2);
        if (std::find(relXInd.begin(), relXInd.end(), e.rowIndex) ==
            relXInd.end())
          relXInd.push_back(e.rowIndex);
        e.value = -1;
        mElements.push_back(e);
        e.label = label;
        components.txLine.back().negN2Col = e.colIndex = columnMap.at(cNameN2);
        components.txLine.back().curN2Row = e.rowIndex = rowMap.at(rName2);
        if (std::find(relXInd.begin(), relXInd.end(), e.rowIndex) ==
            relXInd.end())
          relXInd.push_back(e.rowIndex);
        e.value = -1;
        mElements.push_back(e);
      }
      e.label = label;
      components.txLine.back().curN2Col = e.colIndex = columnMap.at(cName2);
      components.txLine.back().curN2Row = e.rowIndex = rowMap.at(rName2);
      if (std::find(relXInd.begin(), relXInd.end(), e.rowIndex) ==
          relXInd.end())
        relXInd.push_back(e.rowIndex);
      e.value = -z0;
      mElements.push_back(e);
    }
    /******************/
    /** PHASE SOURCE **/
    /******************/
    else if (i.first[0] == 'P') {
      std::cerr << "E: Phase source not allowed in voltage analysis."
                << std::endl;
      std::cerr << "E: Infringing line: " << i.first << std::endl;
      std::cerr << "E: Simulation will now terminate." << std::endl;
      std::cerr << std::endl;
      exit(-1);
    }
    /*********************/
    /** MUTUAL COUPLING **/
    /*********************/
    else if (i.first[0] == 'K') {
      components.mutualInductanceLines.push_back(i);
    }
    /************************/
    /*  UNKNOWN DEVICE TYPE */
    /************************/
    else 
      Errors::invalid_component_errors(UNKNOWN_DEVICE_TYPE, label);
  }
  double mutualL = 0.0, cf = 0.0;
  for (const auto &i : components.mutualInductanceLines) {
    devicetokens = Misc::tokenize_space(i.first);
    try {
      label = devicetokens.at(0);
    } catch (std::exception &e) {
      Errors::invalid_component_errors(MISSING_LABEL, i.first);
    }
    try {
      auto parameter_name = ParameterName(devicetokens[3], i.second);
      if (iObj.parameters.parsedParams.count(parameter_name) != 0)
        cf = iObj.parameters.parsedParams.at(parameter_name);
      else
        cf = Misc::modifier(devicetokens[3]);
    } catch (std::exception &e) {
      Errors::invalid_component_errors(MUT_ERROR, i.first);
    }
    std::string ind1, ind2;
    int index1, index2;
    index1 = index2 = -1;
    ind1 = devicetokens[1];
    ind2 = devicetokens[2];
    if (deviceLabelIndex.count(ind1) == 0)
      Errors::invalid_component_errors(MISSING_INDUCTOR, ind1);
    else
      index1 = deviceLabelIndex.at(ind1).index;
    if (deviceLabelIndex.count(ind2) == 0)
      Errors::invalid_component_errors(MISSING_INDUCTOR, ind2);
    else
      index2 = deviceLabelIndex.at(ind2).index;
    cf = cf * sqrt(components.voltInd.at(index1).value *
                   components.voltInd.at(index2).value);
    mutualL = ((2 * cf) / iObj.transSim.prstep);
    components.voltInd.at(index1).mut[index2] = mutualL;
    components.voltInd.at(index2).mut[index1] = mutualL;
    matrix_element e;
    e.label = label;
    e.colIndex = components.voltInd.at(index1).curNCol;
    e.rowIndex = components.voltInd.at(index2).curNRow;
    e.value = -mutualL;
    components.voltInd.at(index1).mutPtr[ind2] = mElements.size();
    mElements.push_back(e);
    e.label = label;
    e.colIndex = components.voltInd.at(index2).curNCol;
    e.rowIndex = components.voltInd.at(index1).curNRow;
    e.value = -mutualL;
    components.voltInd.at(index2).mutPtr[ind1] = mElements.size();
    mElements.push_back(e);
  }
  std::map<int, std::string> rowMapFlip = Misc::flip_map(rowMap);
  std::map<int, std::string> columnMapFlip = Misc::flip_map(columnMap);
  std::transform(
      rowMapFlip.begin(), rowMapFlip.end(), back_inserter(rowNames),
      [](std::pair<int, std::string> const &pair) { return pair.second; });
  std::transform(
      columnMapFlip.begin(), columnMapFlip.end(), back_inserter(columnNames),
      [](std::pair<int, std::string> const &pair) { return pair.second; });
}

void Matrix::create_A_phase(Input &iObj)
{
  std::string cName, rName, cNameP, rNameP, cNameN, rNameN;
  std::vector<std::string> devicetokens, componentLabels;
  devicetokens.clear();
  componentLabels.clear();
  std::string label, nodeP, nodeN, subckt;
  std::unordered_map<std::string, int> rowMap, columnMap;
  rowMap.clear();
  columnMap.clear();
  int rowCounter, colCounter, expStart, expEnd, nodeCounter;
  bool pGND, nGND;
  rowCounter = colCounter = nodeCounter = 0;
  /* Main circuit node identification */
  for (auto i : iObj.expNetlist) {
    expStart = expEnd = -1;
    devicetokens = Misc::tokenize_space(i.first);
    for (int t = 0; t < devicetokens.size(); t++) {
      if (devicetokens[t].find('{') != std::string::npos)
        expStart = t;
      if (devicetokens[t].find('}') != std::string::npos)
        expEnd = t;
    }
    if (expStart == -1 && expEnd != -1)
      Errors::invalid_component_errors(INVALID_EXPR, i.first);
    else if (expStart != -1 && expEnd == -1)
      Errors::invalid_component_errors(INVALID_EXPR, i.first);
    if (expStart != -1 && expStart == expEnd) {
      devicetokens[expStart] =
          devicetokens[expStart].substr(devicetokens[expStart].find('{') + 1,
                                        devicetokens[expStart].size() - 1);
      devicetokens[expStart] =
          devicetokens[expStart].substr(0, devicetokens[expStart].find('}'));
      devicetokens[expStart] = Misc::precise_to_string(
          Parser::parse_param(devicetokens[expStart],
                              iObj.parameters.parsedParams, i.second),
          25);
    } else if (expStart != -1 && expEnd != -1) {
      int d = expStart + 1;
      while (expStart != expEnd) {
        devicetokens[expStart] += devicetokens[d];
        devicetokens.erase(devicetokens.begin() + d);
        expEnd--;
      }
      devicetokens[expStart] =
          devicetokens[expStart].substr(devicetokens[expStart].find('{') + 1,
                                        devicetokens[expStart].size() - 1);
      devicetokens[expStart] =
          devicetokens[expStart].substr(0, devicetokens[expStart].find('}'));
      devicetokens[expStart] = Misc::precise_to_string(
          Parser::parse_param(devicetokens[expStart],
                              iObj.parameters.parsedParams, i.second),
          25);
    }
    double value = 0.0;
    /* Check if label exists, if not there is a bug in the program */
    try {
      label = devicetokens.at(0);
      if (std::find(componentLabels.begin(), componentLabels.end(), label) ==
          componentLabels.end()) {
        if (label.find_first_of("_*!@#$\\/%^&*()") != std::string::npos) {
          std::cerr << "W: The use of special characters in label names is not "
                       "advised."
                    << std::endl;
          std::cerr << "W: This might produce unexpected results." << std::endl;
          std::cerr << "W: Continuing operation." << std::endl;
        }
        componentLabels.push_back(label);
      } else
        Errors::invalid_component_errors(DUPLICATE_LABEL, label);
    } catch (std::exception &e) {
      Errors::invalid_component_errors(MISSING_LABEL, i.first);
    }
    try {
      nodeP = devicetokens[1];
    } catch (std::exception &e) {
      Errors::invalid_component_errors(MISSING_PNODE, i.first);
    }
    /* Check if negative node exists, if not it's a bad device line definition
     */
    try {
      nodeN = devicetokens[2];
    } catch (std::exception &e) {
      Errors::invalid_component_errors(MISSING_NNODE, i.first);
    }
    /********************/
    /** PHASE RESISTOR **/
    /********************/
    if (i.first[0] == 'R') {
      matrix_element e;
      components.phaseRes.emplace_back(res_phase());
      deviceLabelIndex[label].type = RowDescriptor::Type::PhaseResistor;
      deviceLabelIndex[label].index = components.phaseRes.size() - 1;
      try {
        auto parameter_name = ParameterName(devicetokens[3], i.second);
        if (iObj.parameters.parsedParams.count(parameter_name) != 0) {
          value = iObj.parameters.parsedParams.at(parameter_name);
        } else {
          parameter_name = ParameterName(devicetokens[3], "");
          if (iObj.parameters.parsedParams.count(parameter_name) != 0)
            value = iObj.parameters.parsedParams.at(parameter_name);
          else
            value = Misc::modifier(devicetokens[3]);
        }
      } catch (std::exception &e) {
        Errors::invalid_component_errors(RES_ERROR, i.first);
      }
      cName = "C_I" + devicetokens[0];
      rName = "R_" + devicetokens[0];
      components.phaseRes.back().curNodeC = devicetokens.at(0);
      components.phaseRes.back().curNodeR = devicetokens.at(0);
      components.phaseRes.back().value = value;
      components.phaseRes.back().label = label;
      if (rowMap.count(devicetokens.at(0)) == 0) {
        rowDesc.emplace_back(RowDescriptor());
        rowDesc.back().type = RowDescriptor::Type::PhaseResistor;
        rowDesc.back().index = components.phaseRes.size() - 1;
        rowMap[devicetokens.at(0)] = rowCounter++;
      }
      if (columnMap.count(devicetokens.at(0)) == 0) {
        columnMap[cName] = colCounter;
        colCounter++;
      }
      if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
        cNameP = "C_NP" + nodeP;
        rNameP = "R_N" + nodeP;
        components.phaseRes.back().posNodeC = nodeP;
        components.phaseRes.back().posNodeR = nodeP;
        if (rowMap.count(nodeP) == 0) {
          if (nodeMap.count(nodeP) == 0) {
            nodeMap[nodeP] = nodeCounter++;
            nodeConnections.emplace_back(NodeConnections());
            nodeConnections.back().name = nodeP;
            nodeConnections.back().connections.emplace_back(
                ComponentConnections());
            nodeConnections.back().connections.back().type =
                ComponentConnections::Type::ResistorP;
            nodeConnections.back().connections.back().index =
                components.phaseRes.size() - 1;
          }
          rowDesc.emplace_back(RowDescriptor());
          rowDesc.back().type = RowDescriptor::Type::PhaseNode;
          rowDesc.back().index = nodeMap.at(nodeP);
          deviceLabelIndex[nodeP].type = RowDescriptor::Type::PhaseNode;
          deviceLabelIndex[nodeP].index = rowCounter;
          rowMap[nodeP] = rowCounter++;
        } else {
          nodeConnections.at(nodeMap.at(nodeP))
              .connections.emplace_back(ComponentConnections());
          nodeConnections.at(nodeMap.at(nodeP)).connections.back().type =
              ComponentConnections::Type::ResistorP;
          nodeConnections.at(nodeMap.at(nodeP)).connections.back().index =
              components.phaseRes.size() - 1;
        }
        if (columnMap.count(cNameP) == 0) {
          columnMap[cNameP] = colCounter;
          colCounter++;
        }
        pGND = false;
      } else
        pGND = true;
      if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
        cNameN = "C_NP" + nodeN;
        rNameN = "R_N" + nodeN;
        components.phaseRes.back().negNodeC = nodeN;
        components.phaseRes.back().negNodeR = nodeN;
        if (rowMap.count(nodeN) == 0) {
          if (nodeMap.count(nodeN) == 0) {
            nodeMap[nodeN] = nodeCounter++;
            nodeConnections.emplace_back(NodeConnections());
            nodeConnections.back().name = nodeN;
            nodeConnections.back().connections.emplace_back(
                ComponentConnections());
            nodeConnections.back().connections.back().type =
                ComponentConnections::Type::ResistorN;
            nodeConnections.back().connections.back().index =
                components.phaseRes.size() - 1;
          }
          rowDesc.emplace_back(RowDescriptor());
          rowDesc.back().type = RowDescriptor::Type::PhaseNode;
          rowDesc.back().index = nodeMap.at(nodeN);
          deviceLabelIndex[nodeN].type = RowDescriptor::Type::PhaseNode;
          deviceLabelIndex[nodeN].index = rowCounter;
          rowMap[nodeN] = rowCounter++;
        } else {
          nodeConnections.at(nodeMap.at(nodeN))
              .connections.emplace_back(ComponentConnections());
          nodeConnections.at(nodeMap.at(nodeN)).connections.back().type =
              ComponentConnections::Type::ResistorN;
          nodeConnections.at(nodeMap.at(nodeN)).connections.back().index =
              components.phaseRes.size() - 1;
        }
        if (columnMap.count(cNameN) == 0) {
          columnMap[cNameN] = colCounter;
          colCounter++;
        }
        nGND = false;
      } else
        nGND = true;
      if (!pGND) {
        e.label = label;
        e.colIndex = columnMap.at(cName);
        components.phaseRes.back().posNRow = e.rowIndex = rowMap.at(nodeP);
        e.value = 1;
        mElements.push_back(e);
        e.label = label;
        components.phaseRes.back().posNCol = e.colIndex = columnMap.at(cNameP);
        e.rowIndex = rowMap.at(devicetokens.at(0));
        e.value = 1;
        mElements.push_back(e);
      }
      if (!nGND) {
        e.label = label;
        e.colIndex = columnMap.at(cName);
        components.phaseRes.back().negNRow = e.rowIndex = rowMap.at(nodeN);
        e.value = -1;
        mElements.push_back(e);
        e.label = label;
        components.phaseRes.back().negNCol = e.colIndex = columnMap.at(cNameN);
        e.rowIndex = rowMap.at(devicetokens.at(0));
        e.value = -1;
        mElements.push_back(e);
      }
      e.label = label;
      components.phaseRes.back().curNCol = e.colIndex = columnMap.at(cName);
      components.phaseRes.back().curNRow = e.rowIndex =
          rowMap.at(devicetokens.at(0));
      e.value =
          -(M_PI * components.phaseRes.back().value * iObj.transSim.prstep) /
          PHI_ZERO;
      components.phaseRes.back().resPtr = mElements.size();
      mElements.push_back(e);
    }
    /*********************/
    /** PHASE CAPACITOR **/
    /*********************/
    else if (i.first[0] == 'C') {
      matrix_element e;
      components.phaseCap.emplace_back(cap_phase());
      deviceLabelIndex[label].type = RowDescriptor::Type::PhaseCapacitor;
      deviceLabelIndex[label].index = components.phaseCap.size() - 1;
      try {
        auto parameter_name = ParameterName(devicetokens[3], i.second);
        if (iObj.parameters.parsedParams.count(parameter_name) != 0) {
          value = iObj.parameters.parsedParams.at(parameter_name);
        } else {
          parameter_name = ParameterName(devicetokens[3], "");
          if (iObj.parameters.parsedParams.count(parameter_name) != 0)
            value = iObj.parameters.parsedParams.at(parameter_name);
          else
            value = Misc::modifier(devicetokens[3]);
        }
      } catch (std::exception &e) {
        Errors::invalid_component_errors(CAP_ERROR, i.first);
      }
      cName = "C_I" + devicetokens[0];
      rName = "R_" + devicetokens[0];
      components.phaseCap.back().curNodeC = devicetokens.at(0);
      components.phaseCap.back().curNodeR = devicetokens.at(0);
      components.phaseCap.back().value = value;
      components.phaseCap.back().label = label;
      if (rowMap.count(devicetokens.at(0)) == 0) {
        rowDesc.emplace_back(RowDescriptor());
        rowDesc.back().type = RowDescriptor::Type::PhaseCapacitor;
        rowDesc.back().index = components.phaseCap.size() - 1;
        rowMap[devicetokens.at(0)] = rowCounter++;
      }
      if (columnMap.count(devicetokens.at(0)) == 0) {
        columnMap[cName] = colCounter;
        colCounter++;
      }
      if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
        cNameP = "C_NP" + nodeP;
        rNameP = "R_N" + nodeP;
        components.phaseCap.back().posNodeC = nodeP;
        components.phaseCap.back().posNodeR = nodeP;
        if (rowMap.count(nodeP) == 0) {
          if (nodeMap.count(nodeP) == 0) {
            nodeMap[nodeP] = nodeCounter++;
            nodeConnections.emplace_back(NodeConnections());
            nodeConnections.back().name = nodeP;
            nodeConnections.back().connections.emplace_back(
                ComponentConnections());
            nodeConnections.back().connections.back().type =
                ComponentConnections::Type::CapacitorP;
            nodeConnections.back().connections.back().index =
                components.phaseCap.size() - 1;
          }
          rowDesc.emplace_back(RowDescriptor());
          rowDesc.back().type = RowDescriptor::Type::PhaseNode;
          rowDesc.back().index = nodeMap.at(nodeP);
          deviceLabelIndex[nodeP].type = RowDescriptor::Type::PhaseNode;
          deviceLabelIndex[nodeP].index = rowCounter;
          rowMap[nodeP] = rowCounter++;
        } else {
          nodeConnections.at(nodeMap.at(nodeP))
              .connections.emplace_back(ComponentConnections());
          nodeConnections.at(nodeMap.at(nodeP)).connections.back().type =
              ComponentConnections::Type::CapacitorP;
          nodeConnections.at(nodeMap.at(nodeP)).connections.back().index =
              components.phaseCap.size() - 1;
        }
        if (columnMap.count(cNameP) == 0) {
          columnMap[cNameP] = colCounter;
          colCounter++;
        }
        pGND = false;
      } else
        pGND = true;
      if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
        cNameN = "C_NP" + nodeN;
        rNameN = "R_N" + nodeN;
        components.phaseCap.back().negNodeC = nodeN;
        components.phaseCap.back().negNodeR = nodeN;
        if (rowMap.count(nodeN) == 0) {
          if (nodeMap.count(nodeN) == 0) {
            nodeMap[nodeN] = nodeCounter++;
            nodeConnections.emplace_back(NodeConnections());
            nodeConnections.back().name = nodeN;
            nodeConnections.back().connections.emplace_back(
                ComponentConnections());
            nodeConnections.back().connections.back().type =
                ComponentConnections::Type::CapacitorN;
            nodeConnections.back().connections.back().index =
                components.phaseCap.size() - 1;
          }
          rowDesc.emplace_back(RowDescriptor());
          rowDesc.back().type = RowDescriptor::Type::PhaseNode;
          rowDesc.back().index = nodeMap.at(nodeN);
          deviceLabelIndex[nodeN].type = RowDescriptor::Type::PhaseNode;
          deviceLabelIndex[nodeN].index = rowCounter;
          rowMap[nodeN] = rowCounter++;
        } else {
          nodeConnections.at(nodeMap.at(nodeN))
              .connections.emplace_back(ComponentConnections());
          nodeConnections.at(nodeMap.at(nodeN)).connections.back().type =
              ComponentConnections::Type::CapacitorN;
          nodeConnections.at(nodeMap.at(nodeN)).connections.back().index =
              components.phaseCap.size() - 1;
        }
        if (columnMap.count(cNameN) == 0) {
          columnMap[cNameN] = colCounter;
          colCounter++;
        }
        nGND = false;
      } else
        nGND = true;
      if (!pGND) {
        e.label = label;
        e.colIndex = columnMap.at(cName);
        components.phaseCap.back().posNRow = e.rowIndex = rowMap.at(nodeP);
        e.value = 1;
        mElements.push_back(e);
        e.label = label;
        components.phaseCap.back().posNCol = e.colIndex = columnMap.at(cNameP);
        e.rowIndex = rowMap.at(devicetokens.at(0));
        e.value = 1;
        mElements.push_back(e);
      }
      if (!nGND) {
        e.label = label;
        e.colIndex = columnMap.at(cName);
        components.phaseCap.back().negNRow = e.rowIndex = rowMap.at(nodeN);
        e.value = -1;
        mElements.push_back(e);
        e.label = label;
        components.phaseCap.back().negNCol = e.colIndex = columnMap.at(cNameN);
        e.rowIndex = rowMap.at(devicetokens.at(0));
        e.value = -1;
        mElements.push_back(e);
      }
      e.label = label;
      components.phaseCap.back().curNCol = e.colIndex = columnMap.at(cName);
      components.phaseCap.back().curNRow = e.rowIndex =
          rowMap.at(devicetokens.at(0));
      e.value = (-2 * M_PI * iObj.transSim.prstep * iObj.transSim.prstep) /
                (PHI_ZERO * 4 * components.phaseCap.back().value);
      components.phaseCap.back().capPtr = mElements.size();
      mElements.push_back(e);
    }
    /********************/
    /** PHASE INDUCTOR **/
    /********************/
    else if (i.first[0] == 'L') {
      matrix_element e;
      components.phaseInd.emplace_back(ind_phase());
      deviceLabelIndex[label].type = RowDescriptor::Type::PhaseInductor;
      deviceLabelIndex[label].index = components.phaseInd.size() - 1;
      try {
        auto parameter_name = ParameterName(devicetokens[3], i.second);
        if (iObj.parameters.parsedParams.count(parameter_name) != 0) {
          value = iObj.parameters.parsedParams.at(parameter_name);
        } else {
          parameter_name = ParameterName(devicetokens[3], "");
          if (iObj.parameters.parsedParams.count(parameter_name) != 0)
            value = iObj.parameters.parsedParams.at(parameter_name);
          else
            value = Misc::modifier(devicetokens[3]);
        }
      } catch (std::exception &e) {
        Errors::invalid_component_errors(IND_ERROR, i.first);
      }
      cName = "C_I" + devicetokens[0];
      rName = "R_" + devicetokens[0];
      components.phaseInd.back().curNodeC = devicetokens.at(0);
      components.phaseInd.back().curNodeR = devicetokens.at(0);
      components.phaseInd.back().value = value;
      components.phaseInd.back().label = label;
      if (rowMap.count(devicetokens.at(0)) == 0) {
        rowDesc.emplace_back(RowDescriptor());
        rowDesc.back().type = RowDescriptor::Type::PhaseInductor;
        rowDesc.back().index = components.phaseInd.size() - 1;
        rowMap[devicetokens.at(0)] = rowCounter++;
      }
      if (columnMap.count(devicetokens.at(0)) == 0) {
        columnMap[cName] = colCounter;
        colCounter++;
      }
      if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
        cNameP = "C_NP" + nodeP;
        rNameP = "R_N" + nodeP;
        components.phaseInd.back().posNodeC = nodeP;
        components.phaseInd.back().posNodeR = nodeP;
        if (rowMap.count(nodeP) == 0) {
          if (nodeMap.count(nodeP) == 0) {
            nodeMap[nodeP] = nodeCounter++;
            nodeConnections.emplace_back(NodeConnections());
            nodeConnections.back().name = nodeP;
            nodeConnections.back().connections.emplace_back(
                ComponentConnections());
            nodeConnections.back().connections.back().type =
                ComponentConnections::Type::InductorP;
            nodeConnections.back().connections.back().index =
                components.phaseInd.size() - 1;
          }
          rowDesc.emplace_back(RowDescriptor());
          rowDesc.back().type = RowDescriptor::Type::PhaseNode;
          rowDesc.back().index = nodeMap.at(nodeP);
          deviceLabelIndex[nodeP].type = RowDescriptor::Type::PhaseNode;
          deviceLabelIndex[nodeP].index = rowCounter;
          rowMap[nodeP] = rowCounter++;
        } else {
          nodeConnections.at(nodeMap.at(nodeP))
              .connections.emplace_back(ComponentConnections());
          nodeConnections.at(nodeMap.at(nodeP)).connections.back().type =
              ComponentConnections::Type::InductorP;
          nodeConnections.at(nodeMap.at(nodeP)).connections.back().index =
              components.phaseInd.size() - 1;
        }
        if (columnMap.count(cNameP) == 0) {
          columnMap[cNameP] = colCounter;
          colCounter++;
        }
        pGND = false;
      } else
        pGND = true;
      if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
        cNameN = "C_NP" + nodeN;
        rNameN = "R_N" + nodeN;
        components.phaseInd.back().negNodeC = nodeN;
        components.phaseInd.back().negNodeR = nodeN;
        if (rowMap.count(nodeN) == 0) {
          if (nodeMap.count(nodeN) == 0) {
            nodeMap[nodeN] = nodeCounter++;
            nodeConnections.emplace_back(NodeConnections());
            nodeConnections.back().name = nodeN;
            nodeConnections.back().connections.emplace_back(
                ComponentConnections());
            nodeConnections.back().connections.back().type =
                ComponentConnections::Type::InductorN;
            nodeConnections.back().connections.back().index =
                components.phaseInd.size() - 1;
          }
          rowDesc.emplace_back(RowDescriptor());
          rowDesc.back().type = RowDescriptor::Type::PhaseNode;
          rowDesc.back().index = nodeMap.at(nodeN);
          deviceLabelIndex[nodeN].type = RowDescriptor::Type::PhaseNode;
          deviceLabelIndex[nodeN].index = rowCounter;
          rowMap[nodeN] = rowCounter++;
        } else {
          nodeConnections.at(nodeMap.at(nodeN))
              .connections.emplace_back(ComponentConnections());
          nodeConnections.at(nodeMap.at(nodeN)).connections.back().type =
              ComponentConnections::Type::InductorN;
          nodeConnections.at(nodeMap.at(nodeN)).connections.back().index =
              components.phaseInd.size() - 1;
        }
        if (columnMap.count(cNameN) == 0) {
          columnMap[cNameN] = colCounter;
          colCounter++;
        }
        nGND = false;
      } else
        nGND = true;
      if (!pGND) {
        e.label = label;
        e.colIndex = columnMap.at(cName);
        components.phaseInd.back().posNRow = e.rowIndex = rowMap.at(nodeP);
        e.value = 1;
        mElements.push_back(e);
        e.label = label;
        components.phaseInd.back().posNCol = e.colIndex = columnMap.at(cNameP);
        e.rowIndex = rowMap.at(devicetokens.at(0));
        e.value = 1;
        mElements.push_back(e);
      }
      if (!nGND) {
        e.label = label;
        e.colIndex = columnMap.at(cName);
        components.phaseInd.back().negNRow = e.rowIndex = rowMap.at(nodeN);
        e.value = -1;
        mElements.push_back(e);
        e.label = label;
        components.phaseInd.back().negNCol = e.colIndex = columnMap.at(cNameN);
        e.rowIndex = rowMap.at(devicetokens.at(0));
        e.value = -1;
        mElements.push_back(e);
      }
      e.label = label;
      components.phaseInd.back().curNCol = e.colIndex = columnMap.at(cName);
      components.phaseInd.back().curNRow = e.rowIndex =
          rowMap.at(devicetokens.at(0));
      e.value = -(components.phaseInd.back().value * 2 * M_PI) / PHI_ZERO;
      components.phaseInd.back().indPtr = mElements.size();
      mElements.push_back(e);
    }
    /**************************/
    /** PHASE VOLTAGE SOURCE **/
    /**************************/
    else if (i.first[0] == 'V') {
      matrix_element e;
      sources.emplace_back(Misc::parse_function(i.first, iObj, i.second));
      components.phaseVs.emplace_back(vs_phase());
      deviceLabelIndex[label].type = RowDescriptor::Type::PhaseVS;
      deviceLabelIndex[label].index = sources.size() - 1;
      cName = "C_" + devicetokens[0];
      rName = "R_" + devicetokens[0];
      components.phaseVs.back().curNodeC = devicetokens.at(0);
      components.phaseVs.back().curNodeR = devicetokens.at(0);
      components.phaseVs.back().label = label;
      if (rowMap.count(devicetokens.at(0)) == 0) {
        rowDesc.emplace_back(RowDescriptor());
        rowDesc.back().type = RowDescriptor::Type::PhasePS;
        rowDesc.back().index = sources.size() - 1;
        rowMap[devicetokens.at(0)] = rowCounter++;
      }
      if (columnMap.count(devicetokens.at(0)) == 0) {
        columnMap[cName] = colCounter;
        colCounter++;
      }
      if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
        cNameP = "C_NP" + nodeP;
        rNameP = "R_N" + nodeP;
        components.phaseVs.back().posNodeC = nodeP;
        components.phaseVs.back().posNodeR = nodeP;
        if (rowMap.count(nodeP) == 0) {
          if (nodeMap.count(nodeP) == 0) {
            nodeMap[nodeP] = nodeCounter++;
            nodeConnections.emplace_back(NodeConnections());
            nodeConnections.back().name = nodeP;
            nodeConnections.back().connections.emplace_back(
                ComponentConnections());
            nodeConnections.back().connections.back().type =
                ComponentConnections::Type::VSP;
            nodeConnections.back().connections.back().index =
                sources.size() - 1;
          }
          rowDesc.emplace_back(RowDescriptor());
          rowDesc.back().type = RowDescriptor::Type::PhaseNode;
          rowDesc.back().index = nodeMap.at(nodeP);
          deviceLabelIndex[nodeP].type = RowDescriptor::Type::PhaseNode;
          deviceLabelIndex[nodeP].index = rowCounter;
          rowMap[nodeP] = rowCounter++;
        } else {
          nodeConnections.at(nodeMap.at(nodeP))
              .connections.emplace_back(ComponentConnections());
          nodeConnections.at(nodeMap.at(nodeP)).connections.back().type =
              ComponentConnections::Type::VSP;
          nodeConnections.at(nodeMap.at(nodeP)).connections.back().index =
              sources.size() - 1;
        }
        if (columnMap.count(cNameP) == 0) {
          columnMap[cNameP] = colCounter;
          colCounter++;
        }
        pGND = false;
      } else
        pGND = true;
      if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
        cNameN = "C_NP" + nodeN;
        rNameN = "R_N" + nodeN;
        components.phaseVs.back().negNodeC = nodeN;
        components.phaseVs.back().negNodeR = nodeN;
        if (rowMap.count(nodeN) == 0) {
          if (nodeMap.count(nodeN) == 0) {
            nodeMap[nodeN] = nodeCounter++;
            nodeConnections.emplace_back(NodeConnections());
            nodeConnections.back().name = nodeN;
            nodeConnections.back().connections.emplace_back(
                ComponentConnections());
            nodeConnections.back().connections.back().type =
                ComponentConnections::Type::VSN;
            nodeConnections.back().connections.back().index =
                sources.size() - 1;
          }
          rowDesc.emplace_back(RowDescriptor());
          rowDesc.back().type = RowDescriptor::Type::PhaseNode;
          rowDesc.back().index = nodeMap.at(nodeN);
          deviceLabelIndex[nodeN].type = RowDescriptor::Type::PhaseNode;
          deviceLabelIndex[nodeN].index = rowCounter;
          rowMap[nodeN] = rowCounter++;
        } else {
          nodeConnections.at(nodeMap.at(nodeN))
              .connections.emplace_back(ComponentConnections());
          nodeConnections.at(nodeMap.at(nodeN)).connections.back().type =
              ComponentConnections::Type::VSN;
          nodeConnections.at(nodeMap.at(nodeN)).connections.back().index =
              sources.size() - 1;
        }
        if (columnMap.count(cNameN) == 0) {
          columnMap[cNameN] = colCounter;
          colCounter++;
        }
        nGND = false;
      } else
        nGND = true;
      if (!pGND) {
        e.label = label;
        components.phaseVs.back().curNCol = e.colIndex = columnMap.at(cName);
        components.phaseVs.back().posNRow = e.rowIndex = rowMap.at(nodeP);
        e.value = 1;
        mElements.push_back(e);
        e.label = label;
        components.phaseVs.back().posNCol = e.colIndex = columnMap.at(cNameP);
        components.phaseVs.back().curNRow = e.rowIndex =
            rowMap.at(devicetokens.at(0));
        e.value = 1;
        mElements.push_back(e);
      }
      if (!nGND) {
        e.label = label;
        components.phaseVs.back().curNCol = e.colIndex = columnMap.at(cName);
        components.phaseVs.back().negNRow = e.rowIndex = rowMap.at(nodeN);
        e.value = -1;
        mElements.push_back(e);
        e.label = label;
        components.phaseVs.back().negNCol = e.colIndex = columnMap.at(cNameN);
        components.phaseVs.back().curNRow = e.rowIndex =
            rowMap.at(devicetokens.at(0));
        e.value = -1;
        mElements.push_back(e);
      }
    }
    /********************/
    /** CURRENT SOURCE **/
    /********************/
    else if (i.first[0] == 'I') {
      sources.emplace_back(Misc::parse_function(i.first, iObj, i.second));
      deviceLabelIndex[label].type = RowDescriptor::Type::PhaseCS;
      deviceLabelIndex[label].index = sources.size() - 1;
      if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
        cNameP = "C_NP" + nodeP;
        rNameP = "R_N" + nodeP;
        if (rowMap.count(nodeP) == 0) {
          if (nodeMap.count(nodeP) == 0) {
            nodeMap[nodeP] = nodeCounter++;
            nodeConnections.emplace_back(NodeConnections());
            nodeConnections.back().name = nodeP;
            nodeConnections.back().connections.emplace_back(
                ComponentConnections());
            nodeConnections.back().connections.back().type =
                ComponentConnections::Type::CSP;
            nodeConnections.back().connections.back().index =
                sources.size() - 1;
          }
          rowDesc.emplace_back(RowDescriptor());
          rowDesc.back().type = RowDescriptor::Type::PhaseNode;
          rowDesc.back().index = nodeMap.at(nodeP);
          deviceLabelIndex[nodeP].type = RowDescriptor::Type::PhaseNode;
          deviceLabelIndex[nodeP].index = rowCounter;
          rowMap[nodeP] = rowCounter++;
        } else {
          nodeConnections.at(nodeMap.at(nodeP))
              .connections.emplace_back(ComponentConnections());
          nodeConnections.at(nodeMap.at(nodeP)).connections.back().type =
              ComponentConnections::Type::CSP;
          nodeConnections.at(nodeMap.at(nodeP)).connections.back().index =
              sources.size() - 1;
        }
        if (columnMap.count(cNameP) == 0) {
          columnMap[cNameP] = colCounter;
          colCounter++;
        }
      }
      if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
        cNameN = "C_NP" + nodeN;
        rNameN = "R_N" + nodeN;
        if (rowMap.count(nodeN) == 0) {
          if (nodeMap.count(nodeN) == 0) {
            nodeMap[nodeN] = nodeCounter++;
            nodeConnections.emplace_back(NodeConnections());
            nodeConnections.back().name = nodeN;
            nodeConnections.back().connections.emplace_back(
                ComponentConnections());
            nodeConnections.back().connections.back().type =
                ComponentConnections::Type::CSN;
            nodeConnections.back().connections.back().index =
                sources.size() - 1;
          }
          rowDesc.emplace_back(RowDescriptor());
          rowDesc.back().type = RowDescriptor::Type::PhaseNode;
          rowDesc.back().index = nodeMap.at(nodeN);
          deviceLabelIndex[nodeN].type = RowDescriptor::Type::PhaseNode;
          deviceLabelIndex[nodeN].index = rowCounter;
          rowMap[nodeN] = rowCounter++;
        } else {
          nodeConnections.at(nodeMap.at(nodeN))
              .connections.emplace_back(ComponentConnections());
          nodeConnections.at(nodeMap.at(nodeN)).connections.back().type =
              ComponentConnections::Type::CSN;
          nodeConnections.at(nodeMap.at(nodeN)).connections.back().index =
              sources.size() - 1;
        }
        if (columnMap.count(cNameN) == 0) {
          columnMap[cNameN] = colCounter;
          colCounter++;
        }
      }
    }
    /******************************/
    /** PHASE JOSEPHSON JUNCTION **/
    /******************************/
    else if (i.first[0] == 'B') {
      std::string cVolt, rVolt, jj;
      jj = devicetokens.at(0);
      matrix_element e;
      components.phaseJJ.emplace_back(jj_phase());
      deviceLabelIndex[label].type = RowDescriptor::Type::PhaseJJ;
      deviceLabelIndex[label].index = components.phaseJJ.size() - 1;
      std::string modelstring = "", area = "";
      for (int t = devicetokens.size() - 1; t > 2; t--) {
        if (devicetokens[t].find('=') == std::string::npos) {
          if (iObj.netlist.models.count(
                  std::make_pair(devicetokens[t], i.second)) != 0) {
            modelstring = iObj.netlist.models.at(
                std::make_pair(devicetokens[t], i.second));
            break;
          } else if (iObj.netlist.models.count(
                         std::make_pair(devicetokens[t], "")) != 0) {
            modelstring =
                iObj.netlist.models.at(std::make_pair(devicetokens[t], ""));
            break;
          } else {
            Errors::invalid_component_errors(MODEL_NOT_DEFINED,
                                             devicetokens[t]);
            break;
          }
        }
        if (devicetokens[t].find("AREA") != std::string::npos) {
          area = devicetokens[t].substr(devicetokens[t].find_first_of('=') + 1,
                                        devicetokens[t].size() - 1);
        }
      }
      if (area == "")
        Errors::invalid_component_errors(MODEL_AREA_NOT_GIVEN, label);
      components.phaseJJ.back().label = jj;
      components.jj_model_phase(modelstring, area,
                                components.phaseJJ.size() - 1, iObj, subckt);
      cVolt = "C_V" + devicetokens[0];
      rVolt = "R_" + devicetokens[0];
      components.phaseJJ.back().voltNodeC = devicetokens.at(0);
      components.phaseJJ.back().voltNodeR = devicetokens.at(0);
      if (rowMap.count(devicetokens.at(0)) == 0) {
        rowDesc.emplace_back(RowDescriptor());
        rowDesc.back().type = RowDescriptor::Type::PhaseJJ;
        rowDesc.back().index = components.phaseJJ.size() - 1;
        rowMap[devicetokens.at(0)] = rowCounter++;
      }
      if (columnMap.count(cVolt) == 0) {
        columnMap[cVolt] = colCounter;
        colCounter++;
      }
      if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
        cNameP = "C_NP" + nodeP;
        rNameP = "R_N" + nodeP;
        components.phaseJJ.back().posNodeC = nodeP;
        components.phaseJJ.back().posNodeR = nodeP;
        if (rowMap.count(nodeP) == 0) {
          if (nodeMap.count(nodeP) == 0) {
            nodeMap[nodeP] = nodeCounter++;
            nodeConnections.emplace_back(NodeConnections());
            nodeConnections.back().name = nodeP;
            nodeConnections.back().connections.emplace_back(
                ComponentConnections());
            nodeConnections.back().connections.back().type =
                ComponentConnections::Type::JJP;
            nodeConnections.back().connections.back().index =
                components.phaseJJ.size() - 1;
          }
          rowDesc.emplace_back(RowDescriptor());
          rowDesc.back().type = RowDescriptor::Type::PhaseNode;
          rowDesc.back().index = nodeMap.at(nodeP);
          deviceLabelIndex[nodeP].type = RowDescriptor::Type::PhaseNode;
          deviceLabelIndex[nodeP].index = rowCounter;
          rowMap[nodeP] = rowCounter++;
        } else {
          nodeConnections.at(nodeMap.at(nodeP))
              .connections.emplace_back(ComponentConnections());
          nodeConnections.at(nodeMap.at(nodeP)).connections.back().type =
              ComponentConnections::Type::JJP;
          nodeConnections.at(nodeMap.at(nodeP)).connections.back().index =
              components.phaseJJ.size() - 1;
        }
        if (columnMap.count(cNameP) == 0) {
          columnMap[cNameP] = colCounter;
          colCounter++;
        }
        pGND = false;
      } else
        pGND = true;
      if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
        cNameN = "C_NP" + nodeN;
        rNameN = "R_N" + nodeN;
        components.phaseJJ.back().negNodeC = nodeN;
        components.phaseJJ.back().negNodeR = nodeN;
        if (rowMap.count(nodeN) == 0) {
          if (nodeMap.count(nodeN) == 0) {
            nodeMap[nodeN] = nodeCounter++;
            nodeConnections.emplace_back(NodeConnections());
            nodeConnections.back().name = nodeN;
            nodeConnections.back().connections.emplace_back(
                ComponentConnections());
            nodeConnections.back().connections.back().type =
                ComponentConnections::Type::JJN;
            nodeConnections.back().connections.back().index =
                components.phaseJJ.size() - 1;
          }
          rowDesc.emplace_back(RowDescriptor());
          rowDesc.back().type = RowDescriptor::Type::PhaseNode;
          rowDesc.back().index = nodeMap.at(nodeN);
          deviceLabelIndex[nodeN].type = RowDescriptor::Type::PhaseNode;
          deviceLabelIndex[nodeN].index = rowCounter;
          rowMap[nodeN] = rowCounter++;
        } else {
          nodeConnections.at(nodeMap.at(nodeN))
              .connections.emplace_back(ComponentConnections());
          nodeConnections.at(nodeMap.at(nodeN)).connections.back().type =
              ComponentConnections::Type::JJN;
          nodeConnections.at(nodeMap.at(nodeN)).connections.back().index =
              components.phaseJJ.size() - 1;
        }
        if (columnMap.count(cNameN) == 0) {
          columnMap[cNameN] = colCounter;
          colCounter++;
        }
        nGND = false;
      } else
        nGND = true;
      if (!pGND) {
        e.label = label;
        e.colIndex = columnMap.at(cVolt);
        components.phaseJJ.back().posNRow = e.rowIndex = rowMap.at(nodeP);
        e.value = 1 / components.phaseJJ.back().r0 +
                  ((2 * components.phaseJJ.back().C) / iObj.transSim.prstep);
        components.phaseJJ.back().pPtr = mElements.size();
        mElements.push_back(e);
        e.label = label;
        components.phaseJJ.back().posNCol = e.colIndex = columnMap.at(cNameP);
        e.rowIndex = rowMap.at(devicetokens.at(0));
        e.value = 1;
        mElements.push_back(e);
      }
      if (!nGND) {
        e.label = label;
        e.colIndex = columnMap.at(cVolt);
        components.phaseJJ.back().negNRow = e.rowIndex = rowMap.at(nodeN);
        e.value = -1 / components.phaseJJ.back().r0 -
                  ((2 * components.phaseJJ.back().C) / iObj.transSim.prstep);
        components.phaseJJ.back().nPtr = mElements.size();
        mElements.push_back(e);
        e.label = label;
        components.phaseJJ.back().negNCol = e.colIndex = columnMap.at(cVolt);
        e.rowIndex = rowMap.at(devicetokens.at(0));
        e.value = -1;
        mElements.push_back(e);
      }
      e.label = label;
      components.phaseJJ.back().voltNCol = e.colIndex = columnMap.at(cVolt);
      components.phaseJJ.back().voltNRow = e.rowIndex =
          rowMap.at(devicetokens.at(0));
      e.value = -(iObj.transSim.prstep / 2) * ((2 * M_PI) / PHI_ZERO);
      mElements.push_back(e);
      components.phaseJJ.back().gLarge =
          components.phaseJJ.back().iC /
          (components.phaseJJ.back().iCFact * components.phaseJJ.back().delV);
      components.phaseJJ.back().lower =
          components.phaseJJ.back().vG - 0.5 * components.phaseJJ.back().delV;
      components.phaseJJ.back().upper =
          components.phaseJJ.back().vG + 0.5 * components.phaseJJ.back().delV;
      components.phaseJJ.back().subCond =
          1 / components.phaseJJ.back().r0 +
          ((2 * components.phaseJJ.back().C) / iObj.transSim.prstep);
      components.phaseJJ.back().transCond =
          components.phaseJJ.back().gLarge +
          ((2 * components.phaseJJ.back().C) / iObj.transSim.prstep);
      components.phaseJJ.back().normalCond =
          1 / components.phaseJJ.back().rN +
          ((2 * components.phaseJJ.back().C) / iObj.transSim.prstep);
      components.phaseJJ.back().Del0 =
          1.76 * BOLTZMANN * components.phaseJJ.back().tC;
      components.phaseJJ.back().Del =
          components.phaseJJ.back().Del0 *
          sqrt(cos(
              (M_PI / 2) *
              (components.phaseJJ.back().T / components.phaseJJ.back().tC) *
              (components.phaseJJ.back().T / components.phaseJJ.back().tC)));
      components.phaseJJ.back().rNCalc =
          ((M_PI * components.phaseJJ.back().Del) /
           (2 * EV * components.phaseJJ.back().iC)) *
          tanh(components.phaseJJ.back().Del /
               (2 * BOLTZMANN * components.phaseJJ.back().T));
      components.phaseJJ.back().iS =
          -components.phaseJJ.back().iC * sin(components.phaseJJ.back().phi0);
    }
    /*****************************/
    /** PHASE TRANSMISSION LINE **/
    /*****************************/
    else if (i.first[0] == 'T') {
      std::string nodeP2, nodeN2, cNameP2, rNameP2, cNameN2, rNameN2, cName1,
          rName1, cName2, rName2, tl;
      bool p2GND, n2GND;
      tl = devicetokens.at(0);
      try {
        nodeP2 = devicetokens[3];
      } catch (std::exception &e) {
        Errors::invalid_component_errors(MISSING_PNODE, i.first);
      }
      try {
        nodeN2 = devicetokens[4];
      } catch (std::exception &e) {
        Errors::invalid_component_errors(MISSING_NNODE, i.first);
      }
      matrix_element e;
      components.txPhase.emplace_back(tx_phase());
      deviceLabelIndex[label].type = RowDescriptor::Type::PhaseTX;
      deviceLabelIndex[label].index = components.txPhase.size() - 1;
      if (devicetokens.size() < 7) {
        Errors::invalid_component_errors(TIME_ERROR, i.first);
      }
      for (size_t l = 5; l < devicetokens.size(); l++) {
        if (devicetokens.back().find("TD") != std::string::npos)
          components.txPhase.back().tD =
              Misc::modifier((devicetokens.back()).substr(3));
        else if (devicetokens.back().find("Z0") != std::string::npos)
          components.txPhase.back().value =
              Misc::modifier((devicetokens.back()).substr(3));
      }
      components.txPhase.back().k =
          components.txPhase.back().tD / iObj.transSim.prstep;
      cName1 = "C_I1" + label;
      rName1 = label + "-I1";
      components.txPhase.back().curNode1C = cName1;
      components.txPhase.back().curNode1R = rName1;
      if (rowMap.count(rName1) == 0) {
        rowDesc.emplace_back(RowDescriptor());
        rowDesc.back().type = RowDescriptor::Type::PhaseTX1;
        rowDesc.back().index = components.txPhase.size() - 1;
        rowMap[rName1] = rowCounter++;
      }
      if (columnMap.count(cName1) == 0) {
        columnMap[cName1] = colCounter;
        colCounter++;
      }
      cName2 = "C_I2" + label;
      rName2 = label + "-I2";
      components.txPhase.back().curNode2C = cName2;
      components.txPhase.back().curNode2R = rName2;
      if (rowMap.count(rName2) == 0) {
        rowDesc.emplace_back(RowDescriptor());
        rowDesc.back().type = RowDescriptor::Type::PhaseTX2;
        rowDesc.back().index = components.txPhase.size() - 1;
        rowMap[rName2] = rowCounter++;
      }
      if (columnMap.count(cName2) == 0) {
        columnMap[cName2] = colCounter;
        colCounter++;
      }
      if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
        cNameP = "C_NP" + nodeP;
        rNameP = "R_N" + nodeP;
        components.txPhase.back().posNodeC = nodeP;
        components.txPhase.back().posNodeR = nodeP;
        if (rowMap.count(nodeP) == 0) {
          if (nodeMap.count(nodeP) == 0) {
            nodeMap[nodeP] = nodeCounter++;
            nodeConnections.emplace_back(NodeConnections());
            nodeConnections.back().name = nodeP;
            nodeConnections.back().connections.emplace_back(
                ComponentConnections());
            nodeConnections.back().connections.back().type =
                ComponentConnections::Type::TXP1;
            nodeConnections.back().connections.back().index =
                components.txPhase.size() - 1;
          }
          rowDesc.emplace_back(RowDescriptor());
          rowDesc.back().type = RowDescriptor::Type::PhaseNode;
          rowDesc.back().index = nodeMap.at(nodeP);
          deviceLabelIndex[nodeP].type = RowDescriptor::Type::PhaseNode;
          deviceLabelIndex[nodeP].index = rowCounter;
          rowMap[nodeP] = rowCounter++;
        } else {
          nodeConnections.at(nodeMap.at(nodeP))
              .connections.emplace_back(ComponentConnections());
          nodeConnections.at(nodeMap.at(nodeP)).connections.back().type =
              ComponentConnections::Type::TXP1;
          nodeConnections.at(nodeMap.at(nodeP)).connections.back().index =
              components.txPhase.size() - 1;
        }
        if (columnMap.count(cNameP) == 0) {
          columnMap[cNameP] = colCounter;
          colCounter++;
        }
        pGND = false;
      } else
        pGND = true;
      if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
        cNameN = "C_NP" + nodeN;
        rNameN = "R_N" + nodeN;
        components.txPhase.back().negNodeC = nodeN;
        components.txPhase.back().negNodeR = nodeN;
        if (rowMap.count(nodeN) == 0) {
          if (nodeMap.count(nodeN) == 0) {
            nodeMap[nodeN] = nodeCounter++;
            nodeConnections.emplace_back(NodeConnections());
            nodeConnections.back().name = nodeN;
            nodeConnections.back().connections.emplace_back(
                ComponentConnections());
            nodeConnections.back().connections.back().type =
                ComponentConnections::Type::TXN1;
            nodeConnections.back().connections.back().index =
                components.txPhase.size() - 1;
          }
          rowDesc.emplace_back(RowDescriptor());
          rowDesc.back().type = RowDescriptor::Type::PhaseNode;
          rowDesc.back().index = nodeMap.at(nodeN);
          deviceLabelIndex[nodeN].type = RowDescriptor::Type::PhaseNode;
          deviceLabelIndex[nodeN].index = rowCounter;
          rowMap[nodeN] = rowCounter++;
        } else {
          nodeConnections.at(nodeMap.at(nodeN))
              .connections.emplace_back(ComponentConnections());
          nodeConnections.at(nodeMap.at(nodeN)).connections.back().type =
              ComponentConnections::Type::TXN1;
          nodeConnections.at(nodeMap.at(nodeN)).connections.back().index =
              components.txPhase.size() - 1;
        }
        if (columnMap.count(cNameN) == 0) {
          columnMap[cNameN] = colCounter;
          colCounter++;
        }
        nGND = false;
      } else
        nGND = true;
      if (nodeP2 != "0" && nodeP2.find("GND") == std::string::npos) {
        cNameP2 = "C_NP" + nodeP2;
        rNameP2 = "R_N" + nodeP2;
        components.txPhase.back().posNode2C = nodeP2;
        components.txPhase.back().posNode2R = nodeP2;
        if (rowMap.count(nodeP2) == 0) {
          if (nodeMap.count(nodeP2) == 0) {
            nodeMap[nodeP2] = nodeCounter++;
            nodeConnections.emplace_back(NodeConnections());
            nodeConnections.back().name = nodeP2;
            nodeConnections.back().connections.emplace_back(
                ComponentConnections());
            nodeConnections.back().connections.back().type =
                ComponentConnections::Type::TXP2;
            nodeConnections.back().connections.back().index =
                components.txPhase.size() - 1;
          }
          rowDesc.emplace_back(RowDescriptor());
          rowDesc.back().type = RowDescriptor::Type::PhaseNode;
          rowDesc.back().index = nodeMap.at(nodeP2);
          deviceLabelIndex[nodeP2].type = RowDescriptor::Type::PhaseNode;
          deviceLabelIndex[nodeP2].index = rowCounter;
          rowMap[nodeP2] = rowCounter++;
        } else {
          nodeConnections.at(nodeMap.at(nodeP2))
              .connections.emplace_back(ComponentConnections());
          nodeConnections.at(nodeMap.at(nodeP2)).connections.back().type =
              ComponentConnections::Type::TXP2;
          nodeConnections.at(nodeMap.at(nodeP2)).connections.back().index =
              components.txPhase.size() - 1;
        }
        if (columnMap.count(cNameP2) == 0) {
          columnMap[cNameP2] = colCounter;
          colCounter++;
        }
        p2GND = false;
      } else
        p2GND = true;
      if (nodeN2 != "0" && nodeN2.find("GND") == std::string::npos) {
        cNameN2 = "C_NP" + nodeN2;
        rNameN2 = "R_N" + nodeN2;
        components.txPhase.back().negNode2C = nodeN2;
        components.txPhase.back().negNode2R = nodeN2;
        if (rowMap.count(nodeN2) == 0) {
          if (nodeMap.count(nodeN2) == 0) {
            nodeMap[nodeN2] = nodeCounter++;
            nodeConnections.emplace_back(NodeConnections());
            nodeConnections.back().name = nodeN2;
            nodeConnections.back().connections.emplace_back(
                ComponentConnections());
            nodeConnections.back().connections.back().type =
                ComponentConnections::Type::TXN2;
            nodeConnections.back().connections.back().index =
                components.txPhase.size() - 1;
          }
          rowDesc.emplace_back(RowDescriptor());
          rowDesc.back().type = RowDescriptor::Type::PhaseNode;
          rowDesc.back().index = nodeMap.at(nodeN2);
          deviceLabelIndex[nodeN2].type = RowDescriptor::Type::PhaseNode;
          deviceLabelIndex[nodeN2].index = rowCounter;
          rowMap[nodeN2] = rowCounter++;
        } else {
          nodeConnections.at(nodeMap.at(nodeN2))
              .connections.emplace_back(ComponentConnections());
          nodeConnections.at(nodeMap.at(nodeN2)).connections.back().type =
              ComponentConnections::Type::TXN2;
          nodeConnections.at(nodeMap.at(nodeN2)).connections.back().index =
              components.txPhase.size() - 1;
        }
        if (columnMap.count(cNameN2) == 0) {
          columnMap[cNameN2] = colCounter;
          colCounter++;
        }
        n2GND = false;
      } else
        n2GND = true;
      if (!pGND) {
        components.txPhase.back().posNCol = e.colIndex = columnMap.at(cNameP);
        e.rowIndex = rowMap.at(rName1);
        if (std::find(relXInd.begin(), relXInd.end(), e.rowIndex) ==
            relXInd.end())
          relXInd.push_back(e.rowIndex);
        e.value = 1;
        mElements.push_back(e);
        e.colIndex = columnMap.at(cName1);
        components.txPhase.back().posNRow = e.rowIndex = rowMap.at(nodeP);
        if (std::find(relXInd.begin(), relXInd.end(), e.rowIndex) ==
            relXInd.end())
          relXInd.push_back(e.rowIndex);
        e.value = 1;
        mElements.push_back(e);
      }
      if (!nGND) {
        components.txPhase.back().negNCol = e.colIndex = columnMap.at(cNameN);
        e.rowIndex = rowMap.at(rName1);
        if (std::find(relXInd.begin(), relXInd.end(), e.rowIndex) ==
            relXInd.end())
          relXInd.push_back(e.rowIndex);
        e.value = -1;
        mElements.push_back(e);
        e.colIndex = columnMap.at(cName1);
        components.txPhase.back().negNRow = e.rowIndex = rowMap.at(nodeN);
        if (std::find(relXInd.begin(), relXInd.end(), e.rowIndex) ==
            relXInd.end())
          relXInd.push_back(e.rowIndex);
        e.value = -1;
        mElements.push_back(e);
      }
      if (!p2GND) {
        components.txPhase.back().posN2Col = e.colIndex = columnMap.at(cNameP2);
        e.rowIndex = rowMap.at(rName2);
        if (std::find(relXInd.begin(), relXInd.end(), e.rowIndex) ==
            relXInd.end())
          relXInd.push_back(e.rowIndex);
        e.value = 1;
        mElements.push_back(e);
        e.colIndex = columnMap.at(cName2);
        components.txPhase.back().posN2Row = e.rowIndex = rowMap.at(nodeP2);
        if (std::find(relXInd.begin(), relXInd.end(), e.rowIndex) ==
            relXInd.end())
          relXInd.push_back(e.rowIndex);
        e.value = 1;
        mElements.push_back(e);
      }
      if (!n2GND) {
        components.txPhase.back().negN2Col = e.colIndex = columnMap.at(cNameN2);
        e.rowIndex = rowMap.at(rName2);
        if (std::find(relXInd.begin(), relXInd.end(), e.rowIndex) ==
            relXInd.end())
          relXInd.push_back(e.rowIndex);
        e.value = -1;
        mElements.push_back(e);
        e.colIndex = columnMap.at(cName2);
        components.txPhase.back().negN2Row = e.rowIndex = rowMap.at(nodeN2);
        if (std::find(relXInd.begin(), relXInd.end(), e.rowIndex) ==
            relXInd.end())
          relXInd.push_back(e.rowIndex);
        e.value = -1;
        mElements.push_back(e);
      }
      e.label = label;
      components.txPhase.back().curN1Col = e.colIndex = columnMap.at(cName1);
      components.txPhase.back().curN1Row = e.rowIndex = rowMap.at(rName1);
      if (std::find(relXInd.begin(), relXInd.end(), e.rowIndex) ==
          relXInd.end())
        relXInd.push_back(e.rowIndex);
      e.value =
          -(M_PI * iObj.transSim.prstep * components.txPhase.back().value) /
          (PHI_ZERO);
      mElements.push_back(e);
      e.label = label;
      components.txPhase.back().curN2Col = e.colIndex = columnMap.at(cName2);
      components.txPhase.back().curN2Row = e.rowIndex = rowMap.at(rName2);
      if (std::find(relXInd.begin(), relXInd.end(), e.rowIndex) ==
          relXInd.end())
        relXInd.push_back(e.rowIndex);
      e.value =
          -(M_PI * iObj.transSim.prstep * components.txPhase.back().value) /
          (PHI_ZERO);
      mElements.push_back(e);
    }
    /******************/
    /** PHASE SOURCE **/
    /******************/
    else if (i.first[0] == 'P') {
      matrix_element e;
      sources.emplace_back(Misc::parse_function(i.first, iObj, i.second));
      components.phasePs.emplace_back(ps_phase());
      deviceLabelIndex[label].type = RowDescriptor::Type::PhasePS;
      deviceLabelIndex[label].index = sources.size() - 1;
      cName = "C_" + devicetokens[0];
      rName = "R_" + devicetokens[0];
      components.phasePs.back().curNodeC = devicetokens.at(0);
      components.phasePs.back().curNodeR = devicetokens.at(0);
      components.phasePs.back().label = label;
      if (rowMap.count(devicetokens.at(0)) == 0) {
        rowDesc.emplace_back(RowDescriptor());
        rowDesc.back().type = RowDescriptor::Type::PhasePS;
        rowDesc.back().index = sources.size() - 1;
        rowMap[devicetokens.at(0)] = rowCounter++;
      }
      if (columnMap.count(devicetokens.at(0)) == 0) {
        columnMap[cName] = colCounter;
        colCounter++;
      }
      if (nodeP != "0" && nodeP.find("GND") == std::string::npos) {
        cNameP = "C_NP" + nodeP;
        rNameP = "R_N" + nodeP;
        components.phasePs.back().posNodeC = nodeP;
        components.phasePs.back().posNodeR = nodeP;
        if (rowMap.count(nodeP) == 0) {
          if (nodeMap.count(nodeP) == 0) {
            nodeMap[nodeP] = nodeCounter++;
            nodeConnections.emplace_back(NodeConnections());
            nodeConnections.back().name = nodeP;
            nodeConnections.back().connections.emplace_back(
                ComponentConnections());
            nodeConnections.back().connections.back().type =
                ComponentConnections::Type::PSP;
            nodeConnections.back().connections.back().index =
                sources.size() - 1;
          }
          rowDesc.emplace_back(RowDescriptor());
          rowDesc.back().type = RowDescriptor::Type::PhaseNode;
          rowDesc.back().index = nodeMap.at(nodeP);
          deviceLabelIndex[nodeP].type = RowDescriptor::Type::PhaseNode;
          deviceLabelIndex[nodeP].index = rowCounter;
          rowMap[nodeP] = rowCounter++;
        } else {
          nodeConnections.at(nodeMap.at(nodeP))
              .connections.emplace_back(ComponentConnections());
          nodeConnections.at(nodeMap.at(nodeP)).connections.back().type =
              ComponentConnections::Type::PSP;
          nodeConnections.at(nodeMap.at(nodeP)).connections.back().index =
              sources.size() - 1;
        }
        if (columnMap.count(cNameP) == 0) {
          columnMap[cNameP] = colCounter;
          colCounter++;
        }
        pGND = false;
      } else
        pGND = true;
      if (nodeN != "0" && nodeN.find("GND") == std::string::npos) {
        cNameN = "C_NP" + nodeN;
        rNameN = "R_N" + nodeN;
        components.phasePs.back().negNodeC = nodeN;
        components.phasePs.back().negNodeR = nodeN;
        if (rowMap.count(nodeN) == 0) {
          if (nodeMap.count(nodeN) == 0) {
            nodeMap[nodeN] = nodeCounter++;
            nodeConnections.emplace_back(NodeConnections());
            nodeConnections.back().name = nodeN;
            nodeConnections.back().connections.emplace_back(
                ComponentConnections());
            nodeConnections.back().connections.back().type =
                ComponentConnections::Type::PSN;
            nodeConnections.back().connections.back().index =
                sources.size() - 1;
          }
          rowDesc.emplace_back(RowDescriptor());
          rowDesc.back().type = RowDescriptor::Type::PhaseNode;
          rowDesc.back().index = nodeMap.at(nodeN);
          deviceLabelIndex[nodeN].type = RowDescriptor::Type::PhaseNode;
          deviceLabelIndex[nodeN].index = rowCounter;
          rowMap[nodeN] = rowCounter++;
        } else {
          nodeConnections.at(nodeMap.at(nodeN))
              .connections.emplace_back(ComponentConnections());
          nodeConnections.at(nodeMap.at(nodeN)).connections.back().type =
              ComponentConnections::Type::PSN;
          nodeConnections.at(nodeMap.at(nodeN)).connections.back().index =
              sources.size() - 1;
        }
        if (columnMap.count(cNameN) == 0) {
          columnMap[cNameN] = colCounter;
          colCounter++;
        }
        nGND = false;
      } else
        nGND = true;
      if (!pGND) {
        e.label = label;
        components.phasePs.back().curNCol = e.colIndex = columnMap.at(cName);
        components.phasePs.back().posNRow = e.rowIndex = rowMap.at(nodeP);
        e.value = 1;
        mElements.push_back(e);
        e.label = label;
        components.phasePs.back().posNCol = e.colIndex = columnMap.at(cNameP);
        components.phasePs.back().curNRow = e.rowIndex =
            rowMap.at(devicetokens.at(0));
        e.value = 1;
        mElements.push_back(e);
      }
      if (!nGND) {
        e.label = label;
        components.phasePs.back().curNCol = e.colIndex = columnMap.at(cName);
        components.phasePs.back().negNRow = e.rowIndex = rowMap.at(nodeN);
        e.value = -1;
        mElements.push_back(e);
        e.label = label;
        components.phasePs.back().negNCol = e.colIndex = columnMap.at(cNameN);
        components.phasePs.back().curNRow = e.rowIndex =
            rowMap.at(devicetokens.at(0));
        e.value = -1;
        mElements.push_back(e);
      }
    }
    /*********************/
    /** MUTUAL COUPLING **/
    /*********************/
    else if (i.first[0] == 'K') {
      components.mutualInductanceLines.push_back(i);
    }
  }
  double mutualL = 0.0, cf = 0.0;
  for (const auto &i : components.mutualInductanceLines) {
    devicetokens = Misc::tokenize_space(i.first);
    try {
      label = devicetokens.at(0);
    } catch (std::exception &e) {
      Errors::invalid_component_errors(MISSING_LABEL, i.first);
    }
    try {
      ParameterName parameter_name = ParameterName(devicetokens[3], i.second);
      if (iObj.parameters.parsedParams.count(parameter_name) != 0)
        cf = iObj.parameters.parsedParams.at(parameter_name);
      else
        cf = Misc::modifier(devicetokens[3]);
    } catch (std::exception &e) {
      Errors::invalid_component_errors(MUT_ERROR, i.first);
    }
    std::string ind1, ind2;
    int index1, index2;
    ind1 = devicetokens[1];
    ind2 = devicetokens[2];
    index1 = deviceLabelIndex.at(ind1).index;
    index2 = deviceLabelIndex.at(ind2).index;
    mutualL = cf * sqrt(components.phaseInd.at(index1).value *
                        components.phaseInd.at(index2).value);
    matrix_element e;
    e.label = label;
    e.colIndex = components.phaseInd.at(index2).curNCol;
    e.rowIndex = components.phaseInd.at(index1).curNRow;
    e.value = -(mutualL * 2 * M_PI) / PHI_ZERO;
    components.phaseInd.at(index1).mutPtr = mElements.size();
    mElements.push_back(e);
    e.label = label;
    e.colIndex = components.phaseInd.at(index1).curNCol;
    e.rowIndex = components.phaseInd.at(index2).curNRow;
    e.value = -(mutualL * 2 * M_PI) / PHI_ZERO;
    components.phaseInd.at(index2).mutPtr = mElements.size();
    mElements.push_back(e);
  }
  std::map<int, std::string> rowMapFlip = Misc::flip_map(rowMap);
  std::map<int, std::string> columnMapFlip = Misc::flip_map(columnMap);
  std::transform(
      rowMapFlip.begin(), rowMapFlip.end(), back_inserter(rowNames),
      [](std::pair<int, std::string> const &pair) { return pair.second; });
  std::transform(
      columnMapFlip.begin(), columnMapFlip.end(), back_inserter(columnNames),
      [](std::pair<int, std::string> const &pair) { return pair.second; });
}

void Matrix::create_CSR()
{
  Nsize = rowNames.size();
  Msize = columnNames.size();
  std::vector<std::map<int, double>> aMat(rowNames.size());
  colind.clear();
  nzval.clear();
  rowptr.clear();
  rowptr.push_back(0);
  for (const auto &i : mElements) {
    aMat[i.rowIndex][i.colIndex] += i.value;
  }
  for (const auto &i : aMat) {
    for (auto j : i) {
      nzval.push_back(j.second);
      colind.push_back(j.first);
    }
    rowptr.push_back(rowptr.back() + i.size());
  }
}

void Matrix::find_relevant_x(Input &iObj)
{
  std::vector<std::string> tokens, tokens2;
  std::string label, label2;

  int index1, index2;
  for (const auto &i : iObj.controls) {
    if (i.find("PRINT") != std::string::npos) {
      if (i.at(0) == '.')
        iObj.relevantX.push_back(i.substr(1));
      else
        iObj.relevantX.push_back(i);
    } else if (i.find("PLOT") != std::string::npos) {
      if (i.at(0) == '.')
        iObj.relevantX.push_back(i.substr(1));
      else
        iObj.relevantX.push_back(i);
    } else if (i.find("SAVE") != std::string::npos) {
      if (i.at(0) == '.')
        iObj.relevantX.push_back(i.substr(1));
      else
        iObj.relevantX.push_back(i);
    }
  }
  std::sort(iObj.relevantX.begin(), iObj.relevantX.end());
  auto last = std::unique(iObj.relevantX.begin(), iObj.relevantX.end());
  iObj.relevantX.erase(last, iObj.relevantX.end());
  if(iObj.relevantX.size() == 0) {
    for (int o = 0; o < rowNames.size(); o++) relXInd.emplace_back(o);
  } else {
    for (const auto &i : iObj.relevantX) {
      if (i.find("PRINT") != std::string::npos) {
        tokens = Misc::tokenize_space(i);
        if (tokens.at(1) == "DEVI") {
          label = tokens.at(2);
          if (label.find('_') != std::string::npos) {
            tokens = Misc::tokenize_delimeter(label, "_");
            label = tokens.back();
            for (int j = 0; j < tokens.size() - 1; j++)
              label += "|" + tokens.at(j);
          } else if (label.find('.') != std::string::npos) {
            std::replace(label.begin(), label.end(), '.', '|');
          }
          if (deviceLabelIndex.count(label) == 0) {
            Errors::control_errors(UNKNOWN_DEVICE, label);
          } else {
            const auto &dev = deviceLabelIndex.at(label);
            switch (dev.type) {
            case RowDescriptor::Type::VoltageResistor:
              if (components.voltRes.at(dev.index).posNCol == -1) {
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.voltRes.at(dev.index).negNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.voltRes.at(dev.index).negNCol);
                }
              } else if (components.voltRes.at(dev.index).negNCol == -1) {
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.voltRes.at(dev.index).posNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.voltRes.at(dev.index).posNCol);
                }
              } else {
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.voltRes.at(dev.index).posNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.voltRes.at(dev.index).posNCol);
                }
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.voltRes.at(dev.index).negNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.voltRes.at(dev.index).negNCol);
                }
              }
              break;
            case RowDescriptor::Type::PhaseResistor:
              if (std::find(relXInd.begin(), relXInd.end(),
                            components.phaseRes.at(dev.index).curNCol) ==
                  relXInd.end()) {
                relXInd.push_back(components.phaseRes.at(dev.index).curNCol);
              }
              break;
            case RowDescriptor::Type::VoltageInductor:
              if (std::find(relXInd.begin(), relXInd.end(),
                            components.voltInd.at(dev.index).curNCol) ==
                  relXInd.end()) {
                relXInd.push_back(components.voltInd.at(dev.index).curNCol);
              }
              break;
            case RowDescriptor::Type::PhaseInductor:
              if (std::find(relXInd.begin(), relXInd.end(),
                            components.phaseInd.at(dev.index).curNCol) ==
                  relXInd.end()) {
                relXInd.push_back(components.phaseInd.at(dev.index).curNCol);
              }
              break;
            case RowDescriptor::Type::VoltageCapacitor:
              if (std::find(relXInd.begin(), relXInd.end(),
                            components.voltCap.at(dev.index).curNCol) ==
                  relXInd.end()) {
                relXInd.push_back(components.voltCap.at(dev.index).curNCol);
              }
              break;
            case RowDescriptor::Type::PhaseCapacitor:
              if (std::find(relXInd.begin(), relXInd.end(),
                            components.phaseCap.at(dev.index).curNCol) ==
                  relXInd.end()) {
                relXInd.push_back(components.phaseCap.at(dev.index).curNCol);
              }
              break;
            default:
              break;
            }
          }
        } else if (tokens.at(1) == "DEVV") {
          label = tokens.at(2);
          if (label.find('_') != std::string::npos) {
            tokens = Misc::tokenize_delimeter(label, "_");
            label = tokens.back();
            for (int j = 0; j < tokens.size() - 1; j++)
              label += "|" + tokens.at(j);
          } else if (label.find('.') != std::string::npos)
            std::replace(label.begin(), label.end(), '.', '|');
          if (deviceLabelIndex.count(label) == 0) {
            Errors::control_errors(UNKNOWN_DEVICE, label);
          } else {
            const auto &dev = deviceLabelIndex.at(label);
            switch (dev.type) {
            case RowDescriptor::Type::VoltageResistor:
              if (components.voltRes.at(dev.index).posNCol == -1) {
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.voltRes.at(dev.index).negNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.voltRes.at(dev.index).negNCol);
                }
              } else if (components.voltRes.at(dev.index).negNCol == -1) {
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.voltRes.at(dev.index).posNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.voltRes.at(dev.index).posNCol);
                }
              } else {
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.voltRes.at(dev.index).posNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.voltRes.at(dev.index).posNCol);
                }
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.voltRes.at(dev.index).negNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.voltRes.at(dev.index).negNCol);
                }
              }
              break;
            case RowDescriptor::Type::PhaseResistor:
              if (components.phaseRes.at(dev.index).posNCol == -1) {
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.phaseRes.at(dev.index).negNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.phaseRes.at(dev.index).negNCol);
                }
              } else if (components.phaseRes.at(dev.index).negNCol == -1) {
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.phaseRes.at(dev.index).posNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.phaseRes.at(dev.index).posNCol);
                }
              } else {
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.phaseRes.at(dev.index).posNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.phaseRes.at(dev.index).posNCol);
                }
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.phaseRes.at(dev.index).negNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.phaseRes.at(dev.index).negNCol);
                }
              }
              break;
            case RowDescriptor::Type::VoltageInductor:
              if (components.voltInd.at(dev.index).posNCol == -1) {
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.voltInd.at(dev.index).negNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.voltInd.at(dev.index).negNCol);
                }
              } else if (components.voltInd.at(dev.index).negNCol == -1) {
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.voltInd.at(dev.index).posNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.voltInd.at(dev.index).posNCol);
                }
              } else {
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.voltInd.at(dev.index).posNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.voltInd.at(dev.index).posNCol);
                }
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.voltInd.at(dev.index).negNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.voltInd.at(dev.index).negNCol);
                }
              }
              break;
            case RowDescriptor::Type::PhaseInductor:
              if (components.phaseInd.at(dev.index).posNCol == -1) {
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.phaseInd.at(dev.index).negNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.phaseInd.at(dev.index).negNCol);
                }
              } else if (components.phaseInd.at(dev.index).negNCol == -1) {
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.phaseInd.at(dev.index).posNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.phaseInd.at(dev.index).posNCol);
                }
              } else {
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.phaseInd.at(dev.index).posNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.phaseInd.at(dev.index).posNCol);
                }
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.phaseInd.at(dev.index).negNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.phaseInd.at(dev.index).negNCol);
                }
              }
              break;
            case RowDescriptor::Type::VoltageCapacitor:
              if (components.voltCap.at(dev.index).posNCol == -1) {
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.voltCap.at(dev.index).negNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.voltCap.at(dev.index).negNCol);
                }
              } else if (components.voltCap.at(dev.index).negNCol == -1) {
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.voltCap.at(dev.index).posNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.voltCap.at(dev.index).posNCol);
                }
              } else {
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.voltCap.at(dev.index).posNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.voltCap.at(dev.index).posNCol);
                }
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.voltCap.at(dev.index).negNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.voltCap.at(dev.index).negNCol);
                }
              }
              break;
            case RowDescriptor::Type::PhaseCapacitor:
              if (components.phaseCap.at(dev.index).posNCol == -1) {
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.phaseCap.at(dev.index).negNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.phaseCap.at(dev.index).negNCol);
                }
              } else if (components.phaseCap.at(dev.index).negNCol == -1) {
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.phaseCap.at(dev.index).posNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.phaseCap.at(dev.index).posNCol);
                }
              } else {
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.phaseCap.at(dev.index).posNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.phaseCap.at(dev.index).posNCol);
                }
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.phaseCap.at(dev.index).negNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.phaseCap.at(dev.index).negNCol);
                }
              }
              break;
            case RowDescriptor::Type::VoltageJJ:
              if (components.voltJJ.at(dev.index).posNCol == -1) {
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.voltJJ.at(dev.index).negNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.voltJJ.at(dev.index).negNCol);
                }
              } else if (components.voltJJ.at(dev.index).negNCol == -1) {
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.voltJJ.at(dev.index).posNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.voltJJ.at(dev.index).posNCol);
                }
              } else {
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.voltJJ.at(dev.index).posNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.voltJJ.at(dev.index).posNCol);
                }
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.voltJJ.at(dev.index).negNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.voltJJ.at(dev.index).negNCol);
                }
              }
              break;
            case RowDescriptor::Type::PhaseJJ:
              if (components.phaseJJ.at(dev.index).posNCol == -1) {
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.phaseJJ.at(dev.index).negNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.phaseJJ.at(dev.index).negNCol);
                }
              } else if (components.phaseJJ.at(dev.index).negNCol == -1) {
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.phaseJJ.at(dev.index).posNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.phaseJJ.at(dev.index).posNCol);
                }
              } else {
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.phaseJJ.at(dev.index).posNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.phaseJJ.at(dev.index).posNCol);
                }
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.phaseJJ.at(dev.index).negNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.phaseJJ.at(dev.index).negNCol);
                }
              }
              break;
            default:
              break;
            }
          }
        } else if ((tokens.at(1) == "NODEV") || (tokens.at(1) == "NODEP")) {
          if (tokens.size() == 3) {
            label = tokens.at(2);
            if (label.find('_') != std::string::npos) {
              tokens = Misc::tokenize_delimeter(label, "_");
              label = tokens.back();
              for (int j = 0; j < tokens.size() - 1; j++)
                label += "|" + tokens.at(j);
            } else if (label.find('.') != std::string::npos) {
              std::replace(label.begin(), label.end(), '.', '|');
            }
            if (deviceLabelIndex.count(label) != 0) {
              index1 = deviceLabelIndex.at(label).index;
              if (std::find(relXInd.begin(), relXInd.end(), index1) ==
                  relXInd.end()) {
                relXInd.push_back(index1);
              }
            }
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
              if (deviceLabelIndex.count(label2) != 0) {
                index2 = deviceLabelIndex.at(label2).index;
                if (std::find(relXInd.begin(), relXInd.end(), index2) ==
                    relXInd.end()) {
                  relXInd.push_back(index2);
                }
              }
            } else if (label2 == "0" || label2 == "GND") {
              if (deviceLabelIndex.count(label) != 0) {
                index1 = deviceLabelIndex.at(label).index;
                if (std::find(relXInd.begin(), relXInd.end(), index1) ==
                    relXInd.end()) {
                  relXInd.push_back(index1);
                }
              }
            } else {
              if (deviceLabelIndex.count(label) != 0) {
                if (deviceLabelIndex.count(label2) != 0) {
                  index1 = deviceLabelIndex.at(label).index;
                  if (std::find(relXInd.begin(), relXInd.end(), index1) ==
                      relXInd.end()) {
                    relXInd.push_back(index1);
                  }
                  index2 = deviceLabelIndex.at(label2).index;
                  if (std::find(relXInd.begin(), relXInd.end(), index2) ==
                      relXInd.end()) {
                    relXInd.push_back(index2);
                  }
                }
              }
            }
          }
        } else if (tokens.at(1) == "PHASE"  || tokens.at(1) == "DEVP") {
          label = tokens.at(2);
          if (label.find('_') != std::string::npos) {
            tokens = Misc::tokenize_delimeter(label, "_");
            label = tokens.back();
            for (int j = 0; j < tokens.size() - 1; j++)
              label += "|" + tokens.at(j);
          } else if (label.find('.') != std::string::npos) {
            std::replace(label.begin(), label.end(), '.', '|');
          }
          if (deviceLabelIndex.count(label) == 0) {
            Errors::control_errors(UNKNOWN_DEVICE, label);
          } else {
            const auto &dev = deviceLabelIndex.at(label);
            switch (dev.type) {
            case RowDescriptor::Type::VoltageResistor:
              if (components.voltRes.at(dev.index).posNCol == -1) {
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.voltRes.at(dev.index).negNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.voltRes.at(dev.index).negNCol);
                }
              } else if (components.voltRes.at(dev.index).negNCol == -1) {
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.voltRes.at(dev.index).posNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.voltRes.at(dev.index).posNCol);
                }
              } else {
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.voltRes.at(dev.index).posNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.voltRes.at(dev.index).posNCol);
                }
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.voltRes.at(dev.index).negNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.voltRes.at(dev.index).negNCol);
                }
              }
              break;
            case RowDescriptor::Type::PhaseResistor:
              if (components.phaseRes.at(dev.index).posNCol == -1) {
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.phaseRes.at(dev.index).negNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.phaseRes.at(dev.index).negNCol);
                }
              } else if (components.phaseRes.at(dev.index).negNCol == -1) {
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.phaseRes.at(dev.index).posNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.phaseRes.at(dev.index).posNCol);
                }
              } else {
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.phaseRes.at(dev.index).posNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.phaseRes.at(dev.index).posNCol);
                }
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.phaseRes.at(dev.index).negNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.phaseRes.at(dev.index).negNCol);
                }
              }
              break;
            case RowDescriptor::Type::VoltageInductor:
              if (components.voltInd.at(dev.index).posNCol == -1) {
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.voltInd.at(dev.index).negNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.voltInd.at(dev.index).negNCol);
                }
              } else if (components.voltInd.at(dev.index).negNCol == -1) {
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.voltInd.at(dev.index).posNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.voltInd.at(dev.index).posNCol);
                }
              } else {
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.voltInd.at(dev.index).posNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.voltInd.at(dev.index).posNCol);
                }
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.voltInd.at(dev.index).negNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.voltInd.at(dev.index).negNCol);
                }
              }
              break;
            case RowDescriptor::Type::PhaseInductor:
              if (components.phaseInd.at(dev.index).posNCol == -1) {
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.phaseInd.at(dev.index).negNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.phaseInd.at(dev.index).negNCol);
                }
              } else if (components.phaseInd.at(dev.index).negNCol == -1) {
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.phaseInd.at(dev.index).posNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.phaseInd.at(dev.index).posNCol);
                }
              } else {
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.phaseInd.at(dev.index).posNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.phaseInd.at(dev.index).posNCol);
                }
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.phaseInd.at(dev.index).negNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.phaseInd.at(dev.index).negNCol);
                }
              }
              break;
            case RowDescriptor::Type::VoltageCapacitor:
              if (components.voltCap.at(dev.index).posNCol == -1) {
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.voltCap.at(dev.index).negNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.voltCap.at(dev.index).negNCol);
                }
              } else if (components.voltCap.at(dev.index).negNCol == -1) {
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.voltCap.at(dev.index).posNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.voltCap.at(dev.index).posNCol);
                }
              } else {
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.voltCap.at(dev.index).posNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.voltCap.at(dev.index).posNCol);
                }
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.voltCap.at(dev.index).negNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.voltCap.at(dev.index).negNCol);
                }
              }
              break;
            case RowDescriptor::Type::PhaseCapacitor:
              if (components.phaseCap.at(dev.index).posNCol == -1) {
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.phaseCap.at(dev.index).negNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.phaseCap.at(dev.index).negNCol);
                }
              } else if (components.phaseCap.at(dev.index).negNCol == -1) {
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.phaseCap.at(dev.index).posNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.phaseCap.at(dev.index).posNCol);
                }
              } else {
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.phaseCap.at(dev.index).posNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.phaseCap.at(dev.index).posNCol);
                }
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.phaseCap.at(dev.index).negNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.phaseCap.at(dev.index).negNCol);
                }
              }
              break;
            case RowDescriptor::Type::VoltageJJ:
              if (std::find(relXInd.begin(), relXInd.end(),
                            components.voltJJ.at(dev.index).phaseNCol) ==
                  relXInd.end()) {
                relXInd.push_back(components.voltJJ.at(dev.index).phaseNCol);
              }
              break;
            case RowDescriptor::Type::PhaseJJ:
              if (components.phaseJJ.at(dev.index).posNCol == -1) {
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.phaseJJ.at(dev.index).negNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.phaseJJ.at(dev.index).negNCol);
                }
              } else if (components.phaseJJ.at(dev.index).negNCol == -1) {
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.phaseJJ.at(dev.index).posNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.phaseJJ.at(dev.index).posNCol);
                }
              } else {
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.phaseJJ.at(dev.index).posNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.phaseJJ.at(dev.index).posNCol);
                }
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.phaseJJ.at(dev.index).negNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.phaseJJ.at(dev.index).negNCol);
                }
              }
              break;
            default:
              break;
            }
          }
        }
      } else if (i.find("PLOT") != std::string::npos) {
        tokens = Misc::tokenize_space(i);
        for (int j = 1; j < tokens.size(); j++) {
          if (tokens.at(j)[0] == 'V') {
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
              if (deviceLabelIndex.count(label) == 0) {
                Errors::control_errors(UNKNOWN_DEVICE, label);
              } else {
                const auto &dev = deviceLabelIndex.at(label);
                switch (dev.type) {
                case RowDescriptor::Type::VoltageResistor:
                  if (components.voltRes.at(dev.index).posNCol == -1) {
                    if (std::find(relXInd.begin(), relXInd.end(),
                                  components.voltRes.at(dev.index).negNCol) ==
                        relXInd.end()) {
                      relXInd.push_back(components.voltRes.at(dev.index).negNCol);
                    }
                  } else if (components.voltRes.at(dev.index).negNCol == -1) {
                    if (std::find(relXInd.begin(), relXInd.end(),
                                  components.voltRes.at(dev.index).posNCol) ==
                        relXInd.end()) {
                      relXInd.push_back(components.voltRes.at(dev.index).posNCol);
                    }
                  } else {
                    if (std::find(relXInd.begin(), relXInd.end(),
                                  components.voltRes.at(dev.index).posNCol) ==
                        relXInd.end()) {
                      relXInd.push_back(components.voltRes.at(dev.index).posNCol);
                    }
                    if (std::find(relXInd.begin(), relXInd.end(),
                                  components.voltRes.at(dev.index).negNCol) ==
                        relXInd.end()) {
                      relXInd.push_back(components.voltRes.at(dev.index).negNCol);
                    }
                  }
                  break;
                case RowDescriptor::Type::PhaseResistor:
                  if (components.phaseRes.at(dev.index).posNCol == -1) {
                    if (std::find(relXInd.begin(), relXInd.end(),
                                  components.phaseRes.at(dev.index).negNCol) ==
                        relXInd.end()) {
                      relXInd.push_back(
                          components.phaseRes.at(dev.index).negNCol);
                    }
                  } else if (components.phaseRes.at(dev.index).negNCol == -1) {
                    if (std::find(relXInd.begin(), relXInd.end(),
                                  components.phaseRes.at(dev.index).posNCol) ==
                        relXInd.end()) {
                      relXInd.push_back(
                          components.phaseRes.at(dev.index).posNCol);
                    }
                  } else {
                    if (std::find(relXInd.begin(), relXInd.end(),
                                  components.phaseRes.at(dev.index).posNCol) ==
                        relXInd.end()) {
                      relXInd.push_back(
                          components.phaseRes.at(dev.index).posNCol);
                    }
                    if (std::find(relXInd.begin(), relXInd.end(),
                                  components.phaseRes.at(dev.index).negNCol) ==
                        relXInd.end()) {
                      relXInd.push_back(
                          components.phaseRes.at(dev.index).negNCol);
                    }
                  }
                  break;
                case RowDescriptor::Type::VoltageInductor:
                  if (components.voltInd.at(dev.index).posNCol == -1) {
                    if (std::find(relXInd.begin(), relXInd.end(),
                                  components.voltInd.at(dev.index).negNCol) ==
                        relXInd.end()) {
                      relXInd.push_back(components.voltInd.at(dev.index).negNCol);
                    }
                  } else if (components.voltInd.at(dev.index).negNCol == -1) {
                    if (std::find(relXInd.begin(), relXInd.end(),
                                  components.voltInd.at(dev.index).posNCol) ==
                        relXInd.end()) {
                      relXInd.push_back(components.voltInd.at(dev.index).posNCol);
                    }
                  } else {
                    if (std::find(relXInd.begin(), relXInd.end(),
                                  components.voltInd.at(dev.index).posNCol) ==
                        relXInd.end()) {
                      relXInd.push_back(components.voltInd.at(dev.index).posNCol);
                    }
                    if (std::find(relXInd.begin(), relXInd.end(),
                                  components.voltInd.at(dev.index).negNCol) ==
                        relXInd.end()) {
                      relXInd.push_back(components.voltInd.at(dev.index).negNCol);
                    }
                  }
                  break;
                case RowDescriptor::Type::PhaseInductor:
                  if (components.phaseInd.at(dev.index).posNCol == -1) {
                    if (std::find(relXInd.begin(), relXInd.end(),
                                  components.phaseInd.at(dev.index).negNCol) ==
                        relXInd.end()) {
                      relXInd.push_back(
                          components.phaseInd.at(dev.index).negNCol);
                    }
                  } else if (components.phaseInd.at(dev.index).negNCol == -1) {
                    if (std::find(relXInd.begin(), relXInd.end(),
                                  components.phaseInd.at(dev.index).posNCol) ==
                        relXInd.end()) {
                      relXInd.push_back(
                          components.phaseInd.at(dev.index).posNCol);
                    }
                  } else {
                    if (std::find(relXInd.begin(), relXInd.end(),
                                  components.phaseInd.at(dev.index).posNCol) ==
                        relXInd.end()) {
                      relXInd.push_back(
                          components.phaseInd.at(dev.index).posNCol);
                    }
                    if (std::find(relXInd.begin(), relXInd.end(),
                                  components.phaseInd.at(dev.index).negNCol) ==
                        relXInd.end()) {
                      relXInd.push_back(
                          components.phaseInd.at(dev.index).negNCol);
                    }
                  }
                  break;
                case RowDescriptor::Type::VoltageCapacitor:
                  if (components.voltCap.at(dev.index).posNCol == -1) {
                    if (std::find(relXInd.begin(), relXInd.end(),
                                  components.voltCap.at(dev.index).negNCol) ==
                        relXInd.end()) {
                      relXInd.push_back(components.voltCap.at(dev.index).negNCol);
                    }
                  } else if (components.voltCap.at(dev.index).negNCol == -1) {
                    if (std::find(relXInd.begin(), relXInd.end(),
                                  components.voltCap.at(dev.index).posNCol) ==
                        relXInd.end()) {
                      relXInd.push_back(components.voltCap.at(dev.index).posNCol);
                    }
                  } else {
                    if (std::find(relXInd.begin(), relXInd.end(),
                                  components.voltCap.at(dev.index).posNCol) ==
                        relXInd.end()) {
                      relXInd.push_back(components.voltCap.at(dev.index).posNCol);
                    }
                    if (std::find(relXInd.begin(), relXInd.end(),
                                  components.voltCap.at(dev.index).negNCol) ==
                        relXInd.end()) {
                      relXInd.push_back(components.voltCap.at(dev.index).negNCol);
                    }
                  }
                  break;
                case RowDescriptor::Type::PhaseCapacitor:
                  if (components.phaseCap.at(dev.index).posNCol == -1) {
                    if (std::find(relXInd.begin(), relXInd.end(),
                                  components.phaseCap.at(dev.index).negNCol) ==
                        relXInd.end()) {
                      relXInd.push_back(
                          components.phaseCap.at(dev.index).negNCol);
                    }
                  } else if (components.phaseCap.at(dev.index).negNCol == -1) {
                    if (std::find(relXInd.begin(), relXInd.end(),
                                  components.phaseCap.at(dev.index).posNCol) ==
                        relXInd.end()) {
                      relXInd.push_back(
                          components.phaseCap.at(dev.index).posNCol);
                    }
                  } else {
                    if (std::find(relXInd.begin(), relXInd.end(),
                                  components.phaseCap.at(dev.index).posNCol) ==
                        relXInd.end()) {
                      relXInd.push_back(
                          components.phaseCap.at(dev.index).posNCol);
                    }
                    if (std::find(relXInd.begin(), relXInd.end(),
                                  components.phaseCap.at(dev.index).negNCol) ==
                        relXInd.end()) {
                      relXInd.push_back(
                          components.phaseCap.at(dev.index).negNCol);
                    }
                  }
                  break;
                case RowDescriptor::Type::VoltageJJ:
                  if (components.voltJJ.at(dev.index).posNCol == -1) {
                    if (std::find(relXInd.begin(), relXInd.end(),
                                  components.voltJJ.at(dev.index).negNCol) ==
                        relXInd.end()) {
                      relXInd.push_back(components.voltJJ.at(dev.index).negNCol);
                    }
                  } else if (components.voltJJ.at(dev.index).negNCol == -1) {
                    if (std::find(relXInd.begin(), relXInd.end(),
                                  components.voltJJ.at(dev.index).posNCol) ==
                        relXInd.end()) {
                      relXInd.push_back(components.voltJJ.at(dev.index).posNCol);
                    }
                  } else {
                    if (std::find(relXInd.begin(), relXInd.end(),
                                  components.voltJJ.at(dev.index).posNCol) ==
                        relXInd.end()) {
                      relXInd.push_back(components.voltJJ.at(dev.index).posNCol);
                    }
                    if (std::find(relXInd.begin(), relXInd.end(),
                                  components.voltJJ.at(dev.index).negNCol) ==
                        relXInd.end()) {
                      relXInd.push_back(components.voltJJ.at(dev.index).negNCol);
                    }
                  }
                  break;
                case RowDescriptor::Type::PhaseJJ:
                  if (components.phaseJJ.at(dev.index).posNCol == -1) {
                    if (std::find(relXInd.begin(), relXInd.end(),
                                  components.phaseJJ.at(dev.index).negNCol) ==
                        relXInd.end()) {
                      relXInd.push_back(components.phaseJJ.at(dev.index).negNCol);
                    }
                  } else if (components.phaseJJ.at(dev.index).negNCol == -1) {
                    if (std::find(relXInd.begin(), relXInd.end(),
                                  components.phaseJJ.at(dev.index).posNCol) ==
                        relXInd.end()) {
                      relXInd.push_back(components.phaseJJ.at(dev.index).posNCol);
                    }
                  } else {
                    if (std::find(relXInd.begin(), relXInd.end(),
                                  components.phaseJJ.at(dev.index).posNCol) ==
                        relXInd.end()) {
                      relXInd.push_back(components.phaseJJ.at(dev.index).posNCol);
                    }
                    if (std::find(relXInd.begin(), relXInd.end(),
                                  components.phaseJJ.at(dev.index).negNCol) ==
                        relXInd.end()) {
                      relXInd.push_back(components.phaseJJ.at(dev.index).negNCol);
                    }
                  }
                  break;
                default:
                  if (std::find(relXInd.begin(), relXInd.end(), dev.index) ==
                      relXInd.end()) {
                    relXInd.push_back(dev.index);
                  }
                  break;
                }
              }
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
                if (deviceLabelIndex.count(label2) != 0) {
                  index2 = deviceLabelIndex.at(label2).index;
                  if (std::find(relXInd.begin(), relXInd.end(), index2) ==
                      relXInd.end()) {
                    relXInd.push_back(index2);
                  }
                }
              } else if (label2 == "0" || label2 == "GND") {
                if (deviceLabelIndex.count(label) != 0) {
                  index1 = deviceLabelIndex.at(label).index;
                  if (std::find(relXInd.begin(), relXInd.end(), index1) ==
                      relXInd.end()) {
                    relXInd.push_back(index1);
                  }
                }
              } else {
                if (deviceLabelIndex.count(label) != 0) {
                  if (deviceLabelIndex.count(label2) != 0) {
                    index1 = deviceLabelIndex.at(label).index;
                    if (std::find(relXInd.begin(), relXInd.end(), index1) ==
                        relXInd.end()) {
                      relXInd.push_back(index1);
                    }
                    index2 = deviceLabelIndex.at(label2).index;
                    if (std::find(relXInd.begin(), relXInd.end(), index2) ==
                        relXInd.end()) {
                      relXInd.push_back(index2);
                    }
                  }
                }
              }
            }
          } else if (tokens.at(j)[0] == 'C') {
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
              if (deviceLabelIndex.count(label) == 0) {
              } else {
                const auto &dev = deviceLabelIndex.at(label);
                switch (dev.type) {
                case RowDescriptor::Type::VoltageResistor:
                  if (components.voltRes.at(dev.index).posNCol == -1) {
                    if (std::find(relXInd.begin(), relXInd.end(),
                                  components.voltRes.at(dev.index).negNCol) ==
                        relXInd.end()) {
                      relXInd.push_back(components.voltRes.at(dev.index).negNCol);
                    }
                  } else if (components.voltRes.at(dev.index).negNCol == -1) {
                    if (std::find(relXInd.begin(), relXInd.end(),
                                  components.voltRes.at(dev.index).posNCol) ==
                        relXInd.end()) {
                      relXInd.push_back(components.voltRes.at(dev.index).posNCol);
                    }
                  } else {
                    if (std::find(relXInd.begin(), relXInd.end(),
                                  components.voltRes.at(dev.index).posNCol) ==
                        relXInd.end()) {
                      relXInd.push_back(components.voltRes.at(dev.index).posNCol);
                    }
                    if (std::find(relXInd.begin(), relXInd.end(),
                                  components.voltRes.at(dev.index).negNCol) ==
                        relXInd.end()) {
                      relXInd.push_back(components.voltRes.at(dev.index).negNCol);
                    }
                  }
                  break;
                case RowDescriptor::Type::PhaseResistor:
                  if (std::find(relXInd.begin(), relXInd.end(),
                                components.phaseRes.at(dev.index).curNCol) ==
                      relXInd.end()) {
                    relXInd.push_back(components.phaseRes.at(dev.index).curNCol);
                  }
                  break;
                case RowDescriptor::Type::VoltageInductor:
                  if (std::find(relXInd.begin(), relXInd.end(),
                                components.voltInd.at(dev.index).curNCol) ==
                      relXInd.end()) {
                    relXInd.push_back(components.voltInd.at(dev.index).curNCol);
                  }
                  break;
                case RowDescriptor::Type::PhaseInductor:
                  if (std::find(relXInd.begin(), relXInd.end(),
                                components.phaseInd.at(dev.index).curNCol) ==
                      relXInd.end()) {
                    relXInd.push_back(components.phaseInd.at(dev.index).curNCol);
                  }
                  break;
                case RowDescriptor::Type::VoltageCapacitor:
                  if (std::find(relXInd.begin(), relXInd.end(),
                                components.voltCap.at(dev.index).curNCol) ==
                      relXInd.end()) {
                    relXInd.push_back(components.voltCap.at(dev.index).curNCol);
                  }
                  break;
                case RowDescriptor::Type::PhaseCapacitor:
                  if (std::find(relXInd.begin(), relXInd.end(),
                                components.phaseCap.at(dev.index).curNCol) ==
                      relXInd.end()) {
                    relXInd.push_back(components.phaseCap.at(dev.index).curNCol);
                  }
                  break;
                default:
                  break;
                }
              }
            }
          } else if (tokens.at(j).find("#BRANCH") != std::string::npos) {
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
            if (deviceLabelIndex.count(label) == 0) {
            } else {
              const auto &dev = deviceLabelIndex.at(label);
              switch (dev.type) {
              case RowDescriptor::Type::VoltageResistor:
                if (components.voltRes.at(dev.index).posNCol == -1) {
                  if (std::find(relXInd.begin(), relXInd.end(),
                                components.voltRes.at(dev.index).negNCol) ==
                      relXInd.end()) {
                    relXInd.push_back(components.voltRes.at(dev.index).negNCol);
                  }
                } else if (components.voltRes.at(dev.index).negNCol == -1) {
                  if (std::find(relXInd.begin(), relXInd.end(),
                                components.voltRes.at(dev.index).posNCol) ==
                      relXInd.end()) {
                    relXInd.push_back(components.voltRes.at(dev.index).posNCol);
                  }
                } else {
                  if (std::find(relXInd.begin(), relXInd.end(),
                                components.voltRes.at(dev.index).posNCol) ==
                      relXInd.end()) {
                    relXInd.push_back(components.voltRes.at(dev.index).posNCol);
                  }
                  if (std::find(relXInd.begin(), relXInd.end(),
                                components.voltRes.at(dev.index).negNCol) ==
                      relXInd.end()) {
                    relXInd.push_back(components.voltRes.at(dev.index).negNCol);
                  }
                }
                break;
              case RowDescriptor::Type::PhaseResistor:
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.phaseRes.at(dev.index).curNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.phaseRes.at(dev.index).curNCol);
                }
                break;
              case RowDescriptor::Type::VoltageInductor:
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.voltInd.at(dev.index).curNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.voltInd.at(dev.index).curNCol);
                }
                break;
              case RowDescriptor::Type::PhaseInductor:
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.phaseInd.at(dev.index).curNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.phaseInd.at(dev.index).curNCol);
                }
                break;
              case RowDescriptor::Type::VoltageCapacitor:
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.voltCap.at(dev.index).curNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.voltCap.at(dev.index).curNCol);
                }
                break;
              case RowDescriptor::Type::PhaseCapacitor:
                if (std::find(relXInd.begin(), relXInd.end(),
                              components.phaseCap.at(dev.index).curNCol) ==
                    relXInd.end()) {
                  relXInd.push_back(components.phaseCap.at(dev.index).curNCol);
                }
                break;
              default:
                break;
              }
            }
          } else if (tokens.at(j)[0] == 'P') {
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
              if (deviceLabelIndex.count(label) == 0) {
              } else {
                const auto &dev = deviceLabelIndex.at(label);
                switch (dev.type) {
                case RowDescriptor::Type::VoltageResistor:
                case RowDescriptor::Type::PhaseResistor:
                  if (components.phaseRes.at(dev.index).posNCol == -1) {
                    if (std::find(relXInd.begin(), relXInd.end(),
                                  components.phaseRes.at(dev.index).negNCol) ==
                        relXInd.end()) {
                      relXInd.push_back(
                          components.phaseRes.at(dev.index).negNCol);
                    }
                  } else if (components.phaseRes.at(dev.index).negNCol == -1) {
                    if (std::find(relXInd.begin(), relXInd.end(),
                                  components.phaseRes.at(dev.index).posNCol) ==
                        relXInd.end()) {
                      relXInd.push_back(
                          components.phaseRes.at(dev.index).posNCol);
                    }
                  } else {
                    if (std::find(relXInd.begin(), relXInd.end(),
                                  components.phaseRes.at(dev.index).posNCol) ==
                        relXInd.end()) {
                      relXInd.push_back(
                          components.phaseRes.at(dev.index).posNCol);
                    }
                    if (std::find(relXInd.begin(), relXInd.end(),
                                  components.phaseRes.at(dev.index).negNCol) ==
                        relXInd.end()) {
                      relXInd.push_back(
                          components.phaseRes.at(dev.index).negNCol);
                    }
                  }
                  break;
                case RowDescriptor::Type::VoltageInductor:
                case RowDescriptor::Type::PhaseInductor:
                  if (components.phaseInd.at(dev.index).posNCol == -1) {
                    if (std::find(relXInd.begin(), relXInd.end(),
                                  components.phaseInd.at(dev.index).negNCol) ==
                        relXInd.end()) {
                      relXInd.push_back(
                          components.phaseInd.at(dev.index).negNCol);
                    }
                  } else if (components.phaseInd.at(dev.index).negNCol == -1) {
                    if (std::find(relXInd.begin(), relXInd.end(),
                                  components.phaseInd.at(dev.index).posNCol) ==
                        relXInd.end()) {
                      relXInd.push_back(
                          components.phaseInd.at(dev.index).posNCol);
                    }
                  } else {
                    if (std::find(relXInd.begin(), relXInd.end(),
                                  components.phaseInd.at(dev.index).posNCol) ==
                        relXInd.end()) {
                      relXInd.push_back(
                          components.phaseInd.at(dev.index).posNCol);
                    }
                    if (std::find(relXInd.begin(), relXInd.end(),
                                  components.phaseInd.at(dev.index).negNCol) ==
                        relXInd.end()) {
                      relXInd.push_back(
                          components.phaseInd.at(dev.index).negNCol);
                    }
                  }
                  break;
                case RowDescriptor::Type::VoltageCapacitor:
                case RowDescriptor::Type::PhaseCapacitor:
                  if (components.phaseCap.at(dev.index).posNCol == -1) {
                    if (std::find(relXInd.begin(), relXInd.end(),
                                  components.phaseCap.at(dev.index).negNCol) ==
                        relXInd.end()) {
                      relXInd.push_back(
                          components.phaseCap.at(dev.index).negNCol);
                    }
                  } else if (components.phaseCap.at(dev.index).negNCol == -1) {
                    if (std::find(relXInd.begin(), relXInd.end(),
                                  components.phaseCap.at(dev.index).posNCol) ==
                        relXInd.end()) {
                      relXInd.push_back(
                          components.phaseCap.at(dev.index).posNCol);
                    }
                  } else {
                    if (std::find(relXInd.begin(), relXInd.end(),
                                  components.phaseCap.at(dev.index).posNCol) ==
                        relXInd.end()) {
                      relXInd.push_back(
                          components.phaseCap.at(dev.index).posNCol);
                    }
                    if (std::find(relXInd.begin(), relXInd.end(),
                                  components.phaseCap.at(dev.index).negNCol) ==
                        relXInd.end()) {
                      relXInd.push_back(
                          components.phaseCap.at(dev.index).negNCol);
                    }
                  }
                  break;
                case RowDescriptor::Type::VoltageJJ:
                  if (std::find(relXInd.begin(), relXInd.end(),
                                components.voltJJ.at(dev.index).phaseNCol) ==
                      relXInd.end()) {
                    relXInd.push_back(components.voltJJ.at(dev.index).phaseNCol);
                  }
                  break;
                case RowDescriptor::Type::PhaseJJ:
                  if (components.phaseJJ.at(dev.index).posNCol == -1) {
                    if (std::find(relXInd.begin(), relXInd.end(),
                                  components.phaseJJ.at(dev.index).negNCol) ==
                        relXInd.end()) {
                      relXInd.push_back(components.phaseJJ.at(dev.index).negNCol);
                    }
                  } else if (components.phaseJJ.at(dev.index).negNCol == -1) {
                    if (std::find(relXInd.begin(), relXInd.end(),
                                  components.phaseJJ.at(dev.index).posNCol) ==
                        relXInd.end()) {
                      relXInd.push_back(components.phaseJJ.at(dev.index).posNCol);
                    }
                  } else {
                    if (std::find(relXInd.begin(), relXInd.end(),
                                  components.phaseJJ.at(dev.index).posNCol) ==
                        relXInd.end()) {
                      relXInd.push_back(components.phaseJJ.at(dev.index).posNCol);
                    }
                    if (std::find(relXInd.begin(), relXInd.end(),
                                  components.phaseJJ.at(dev.index).negNCol) ==
                        relXInd.end()) {
                      relXInd.push_back(components.phaseJJ.at(dev.index).negNCol);
                    }
                  }
                  break;
                default:
                  if (deviceLabelIndex.count(label) != 0) {
                    if (std::find(relXInd.begin(), relXInd.end(), dev.index) ==
                        relXInd.end()) {
                      relXInd.push_back(dev.index);
                    }
                  }
                  break;
                }
              }
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
                if (deviceLabelIndex.count(label2) != 0) {
                  index2 = deviceLabelIndex.at(label2).index;
                  if (std::find(relXInd.begin(), relXInd.end(), index2) ==
                      relXInd.end()) {
                    relXInd.push_back(index2);
                  }
                }
              } else if (label2 == "0" || label2 == "GND") {
                if (deviceLabelIndex.count(label) != 0) {
                  index1 = deviceLabelIndex.at(label).index;
                  if (std::find(relXInd.begin(), relXInd.end(), index1) ==
                      relXInd.end()) {
                    relXInd.push_back(index1);
                  }
                }
              } else {
                if (deviceLabelIndex.count(label) != 0) {
                  if (deviceLabelIndex.count(label2) != 0) {
                    index1 = deviceLabelIndex.at(label).index;
                    if (std::find(relXInd.begin(), relXInd.end(), index1) ==
                        relXInd.end()) {
                      relXInd.push_back(index1);
                    }
                    index2 = deviceLabelIndex.at(label2).index;
                    if (std::find(relXInd.begin(), relXInd.end(), index2) ==
                        relXInd.end()) {
                      relXInd.push_back(index2);
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}
