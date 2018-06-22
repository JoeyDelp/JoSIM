// Copyright (c) 2018 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#include "j_components.hpp"

/*
  Return JJ parameters from tokens
*/
void
jj_comp(std::vector<std::string> tokens,
        InputFile& iFile,
        double& jj_cap,
        double& jj_rn,
        double& jj_rzero,
        double& jj_icrit)
{
  /* Assume tokens 0-2 are label, pnode, nnode so they can be ignored */
  // However not the case when Junction is a 3 terminal device
  std::string label, modname, maybemodname, subcktName = "";
  double area;
  // Identify the subcircuit name (if any) and the model name to identify the model better
  // This gets the model name regardless if it is the 3rd or the 4th token
  if(tokens[3].find_first_of("|") != std::string::npos) {
    if(iFile.subcircuitSegments.count(tokens[3].substr(0, tokens[3].find_first_of("|"))) != 0) {
      subcktName = tokens[3].substr(0, tokens[3].find_first_of("|"));
      modname = tokens[3].substr(tokens[3].find_first_of("|") + 1);
      if(iFile.subcircuitSegments[subcktName].parVal.count(substring_after(tokens[4], "AREA=")) != 0)
        area = iFile.subcircuitSegments[subcktName].parVal[substring_after(tokens[4], "AREA=")];
      else area = modifier(substring_after(tokens[4], "AREA="));
    }
    else if(iFile.subcircuitSegments.count(tokens[4].substr(0, tokens[4].find_first_of("|"))) != 0) {
      subcktName = tokens[4].substr(0, tokens[4].find_first_of("|"));
      modname = tokens[4].substr(tokens[4].find_first_of("|") + 1);
      if(iFile.subcircuitSegments[subcktName].parVal.count(substring_after(tokens[5], "AREA=")) != 0)
        area = iFile.subcircuitSegments[subcktName].parVal[substring_after(tokens[5], "AREA=")];
      else area = modifier(substring_after(tokens[5], "AREA="));
    }
    else invalid_component_errors(MISSING_JJMODEL, label);
  }
  else {
    if(iFile.mainModels.count(tokens[3]) != 0) {
      modname = tokens[3];
      if(iFile.parVal.count(substring_after(tokens[4], "AREA=")) != 0)
        area = iFile.parVal[substring_after(tokens[4], "AREA=")];
      else area = modifier(substring_after(tokens[4], "AREA="));
    }
    else if(iFile.mainModels.count(tokens[4]) != 0) {
      modname = tokens[4];
      if(iFile.parVal.count(substring_after(tokens[5], "AREA=")) != 0)
        area = iFile.parVal[substring_after(tokens[5], "AREA=")];
      else area = modifier(substring_after(tokens[5], "AREA="));
    }
    else invalid_component_errors(MISSING_JJMODEL, label);
  }
  if(!subcktName.empty()) { 
    if(iFile.subcircuitSegments[subcktName].subcktModels.count(modname) != 0) {
      if(iFile.subcircuitSegments[subcktName].parVal.count(iFile.subcircuitSegments[subcktName].subcktModels[modname].jj.cap) != 0)
        jj_cap = iFile.subcircuitSegments[subcktName].parVal[iFile.subcircuitSegments[subcktName].subcktModels[modname].jj.cap];
      else jj_cap = modifier(iFile.subcircuitSegments[subcktName].subcktModels[modname].jj.cap);
      if(iFile.subcircuitSegments[subcktName].parVal.count(iFile.subcircuitSegments[subcktName].subcktModels[modname].jj.rnormal) != 0)
        jj_rn = iFile.subcircuitSegments[subcktName].parVal[iFile.subcircuitSegments[subcktName].subcktModels[modname].jj.rnormal];
      else jj_rn = modifier(iFile.subcircuitSegments[subcktName].subcktModels[modname].jj.rnormal);
      if(iFile.subcircuitSegments[subcktName].parVal.count(iFile.subcircuitSegments[subcktName].subcktModels[modname].jj.rzero) != 0)
        jj_rzero = iFile.subcircuitSegments[subcktName].parVal[iFile.subcircuitSegments[subcktName].subcktModels[modname].jj.rzero];
      else jj_rzero = modifier(iFile.subcircuitSegments[subcktName].subcktModels[modname].jj.rzero);
      if(iFile.subcircuitSegments[subcktName].parVal.count(iFile.subcircuitSegments[subcktName].subcktModels[modname].jj.icrit) != 0)
        jj_icrit = iFile.subcircuitSegments[subcktName].parVal[iFile.subcircuitSegments[subcktName].subcktModels[modname].jj.icrit];
      else jj_icrit = modifier(iFile.subcircuitSegments[subcktName].subcktModels[modname].jj.icrit);
    }
    else if(iFile.mainModels.count(modname) != 0) {
      if(iFile.parVal.count(iFile.mainModels[modname].jj.cap) != 0)
        jj_cap = iFile.parVal[iFile.mainModels[modname].jj.cap];
      else jj_cap = modifier(iFile.mainModels[modname].jj.cap);
      if(iFile.parVal.count(iFile.mainModels[modname].jj.rnormal) != 0)
        jj_rn = iFile.parVal[iFile.mainModels[modname].jj.rnormal];
      else jj_rn = modifier(iFile.mainModels[modname].jj.rnormal);
      if(iFile.parVal.count(iFile.mainModels[modname].jj.rzero) != 0)
        jj_rzero = iFile.parVal[iFile.mainModels[modname].jj.rzero];
      else jj_rzero = modifier(iFile.mainModels[modname].jj.rzero);
      if(iFile.parVal.count(iFile.mainModels[modname].jj.icrit) != 0)
        jj_icrit = iFile.parVal[iFile.mainModels[modname].jj.icrit];
      else jj_icrit = modifier(iFile.mainModels[modname].jj.icrit);
    } 
    else invalid_component_errors(MODEL_NOT_DEFINED, modname);
  }
  else {
    if(iFile.mainModels.count(modname) != 0) {
      if(iFile.parVal.count(iFile.mainModels[modname].jj.cap) != 0)
        jj_cap = iFile.parVal[iFile.mainModels[modname].jj.cap];
      else jj_cap = modifier(iFile.mainModels[modname].jj.cap);
      if(iFile.parVal.count(iFile.mainModels[modname].jj.rnormal) != 0)
        jj_rn = iFile.parVal[iFile.mainModels[modname].jj.rnormal];
      else jj_rn = modifier(iFile.mainModels[modname].jj.rnormal);
      if(iFile.parVal.count(iFile.mainModels[modname].jj.rzero) != 0)
        jj_rzero = iFile.parVal[iFile.mainModels[modname].jj.rzero];
      else jj_rzero = modifier(iFile.mainModels[modname].jj.rzero);
      if(iFile.parVal.count(iFile.mainModels[modname].jj.icrit) != 0)
        jj_icrit = iFile.parVal[iFile.mainModels[modname].jj.icrit];
      else jj_icrit = modifier(iFile.mainModels[modname].jj.icrit);
    }
    else invalid_component_errors(MODEL_NOT_DEFINED, modname);
  }
  jj_cap = jj_cap * area;
  jj_rn = jj_rn / area;
  jj_rzero = jj_rzero / area;
  jj_icrit = jj_icrit * area;
  /* Awaiting implementation: CONDEV & IC*/
}
