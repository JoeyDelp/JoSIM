// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/Components.hpp"

void Components::jj_model(std::string &modelstring, std::string &area,
                          const int &jjIndex, Input &iObj,
                          const std::string &subckt) {
  std::string params;
  std::vector<std::string> paramTokens, itemToken, tempToken;
  jj_volt jj = voltJJ.at(jjIndex);
  double value = 0.0;
  params = modelstring;
  params = params.substr(params.find_first_of('('), params.size());
  paramTokens = Misc::tokenize_delimeter(params, "(), ");
  for (int i = 0; i < paramTokens.size(); i++) {
    itemToken = Misc::tokenize_delimeter(paramTokens.at(i), "=");
    if ((itemToken.size() == 1) && (i != paramTokens.size() - 1)) {
      tempToken = Misc::tokenize_delimeter(paramTokens.at(i + 1), "=");
      if (tempToken.size() == 1)
        itemToken.push_back(tempToken.at(0));
      paramTokens.erase(paramTokens.begin() + i + 1);
    } else if ((itemToken.size() == 1) && (i != paramTokens.size() - 1))
      Errors::model_errors(static_cast<int>(ModelErrors::BAD_MODEL_DEFINITION), modelstring);
    value = Parameters::parse_param(itemToken.at(1), iObj.parameters.parameters,
                                subckt);
    if (itemToken.at(0) == "VG" || itemToken.at(0) == "VGAP")
      jj.vG = value;
    else if (itemToken.at(0) == "IC" || itemToken.at(0) == "ICRIT")
      jj.iC = value;
    else if (itemToken.at(0) == "RTYPE")
      jj.rType = (int)value;
    else if (itemToken.at(0) == "RN")
      jj.rN = value;
    else if (itemToken.at(0) == "R0")
      jj.r0 = value;
    else if (itemToken.at(0) == "CAP" || itemToken.at(0) == "C")
      jj.C = value;
    else if (itemToken.at(0) == "T")
      jj.T = value;
    else if (itemToken.at(0) == "TC")
      jj.tC = value;
    else if (itemToken.at(0) == "DELV")
      jj.delV = value;
    else if (itemToken.at(0) == "D")
      jj.D = value;
    else if (itemToken.at(0) == "ICFACT" || itemToken.at(0) == "ICFCT")
      jj.iCFact = value;
    else if (itemToken.at(0) == "PHI")
      jj.phi0 = jj.pn1 = value;
  }
  if (area == "")
    value = 1.0;
  else
    value = Parameters::parse_param(area, iObj.parameters.parameters, subckt);
  jj.C = jj.C * value;
  jj.rN = jj.rN / value;
  jj.r0 = jj.r0 / value;
  jj.iC = jj.iC * value;
  voltJJ.at(jjIndex) = jj;
}

void Components::jj_model_phase(std::string &modelstring, std::string &area,
                                const int &jjIndex, Input &iObj,
                                const std::string &subckt) {
  std::string params;
  std::vector<std::string> paramTokens, itemToken, tempToken;
  jj_phase jj = phaseJJ.at(jjIndex);
  double value = 0.0;
  params = modelstring;
  params = params.substr(params.find_first_of('('), params.size());
  paramTokens = Misc::tokenize_delimeter(params, "(), ");
  for (int i = 0; i < paramTokens.size(); i++) {
    itemToken = Misc::tokenize_delimeter(paramTokens.at(i), "=");
    if ((itemToken.size() == 1) && (i != paramTokens.size() - 1)) {
      tempToken = Misc::tokenize_delimeter(paramTokens.at(i + 1), "=");
      if (tempToken.size() == 1)
        itemToken.push_back(tempToken.at(0));
      paramTokens.erase(paramTokens.begin() + i + 1);
    } else if ((itemToken.size() == 1) && (i != paramTokens.size() - 1))
      Errors::model_errors(static_cast<int>(ModelErrors::BAD_MODEL_DEFINITION), modelstring);
    value = Parameters::parse_param(itemToken.at(1), iObj.parameters.parameters,
                                subckt);
    if (itemToken.at(0) == "VG" || itemToken.at(0) == "VGAP")
      jj.vG = value;
    else if (itemToken.at(0) == "IC" || itemToken.at(0) == "ICRIT")
      jj.iC = value;
    else if (itemToken.at(0) == "RTYPE")
      jj.rType = (int)value;
    else if (itemToken.at(0) == "RN")
      jj.rN = value;
    else if (itemToken.at(0) == "R0")
      jj.r0 = value;
    else if (itemToken.at(0) == "CAP" || itemToken.at(0) == "C")
      jj.C = value;
    else if (itemToken.at(0) == "T")
      jj.T = value;
    else if (itemToken.at(0) == "TC")
      jj.tC = value;
    else if (itemToken.at(0) == "DELV")
      jj.delV = value;
    else if (itemToken.at(0) == "D")
      jj.D = value;
    else if (itemToken.at(0) == "ICFACT" || itemToken.at(0) == "ICFCT")
      jj.iCFact = value;
    else if (itemToken.at(0) == "PHI")
      jj.phi0 = jj.pn1 = value;
  }
  if (area == "")
    value = 1.0;
  else
    value = Parameters::parse_param(area, iObj.parameters.parameters, subckt);
  jj.C = jj.C * value;
  jj.rN = jj.rN / value;
  jj.r0 = jj.r0 / value;
  jj.iC = jj.iC * value;
  phaseJJ.at(jjIndex) = jj;
}
