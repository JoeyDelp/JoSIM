// Copyright (c) 2018 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#include "j_components.hpp"

/*
  Return JJ parameters from tokens
*/
std::unordered_map<std::string, double>
jj_comp(std::vector<std::string> tokens,
        InputFile& iFile,
        int& jj_type)
{
  /* Assume tokens 0-2 are label, pnode, nnode so they can be ignored */
  // However not the case when Junction is a 3 terminal device
  std::unordered_map<std::string, double> jj_tokens;
  std::string label, modname, maybemodname, subcktName = "";
  double area, jj_cap, jj_rn, jj_rzero, jj_icrit, jj_rtype, jj_vgap;
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
    if(iFile.subcircuitSegments[subcktName].subcktModels.count(modname) != 0) jj_rtype = iFile.subcircuitSegments[subcktName].subcktModels[modname].modelType;
    else if(iFile.mainModels.count(modname) != 0) jj_rtype = iFile.mainModels[modname].modelType;
  }
  else {
    if(iFile.mainModels.count(modname) != 0) jj_rtype = iFile.mainModels[modname].modelType;
    else invalid_component_errors(MODEL_NOT_DEFINED, modname);
  }
  switch(jj_type) {
  case RCSJ:
    if(!subcktName.empty()) { 
      if(iFile.subcircuitSegments[subcktName].subcktModels.count(modname) != 0) {
        if(iFile.subcircuitSegments[subcktName].parVal.count(iFile.subcircuitSegments[subcktName].subcktModels[modname].jj.vg) != 0)
          jj_vgap = iFile.subcircuitSegments[subcktName].parVal[iFile.subcircuitSegments[subcktName].subcktModels[modname].jj.vg];
        else jj_vgap = modifier(iFile.subcircuitSegments[subcktName].subcktModels[modname].jj.vg);
        if(iFile.subcircuitSegments[subcktName].parVal.count(iFile.subcircuitSegments[subcktName].subcktModels[modname].jj.rtype) != 0)
          jj_rtype = iFile.subcircuitSegments[subcktName].parVal[iFile.subcircuitSegments[subcktName].subcktModels[modname].jj.rtype];
        else jj_rtype = modifier(iFile.subcircuitSegments[subcktName].subcktModels[modname].jj.rtype);
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
        if(iFile.parVal.count(iFile.mainModels[modname].jj.vg) != 0)
          jj_vgap = iFile.parVal[iFile.mainModels[modname].jj.vg];
        else jj_vgap = modifier(iFile.mainModels[modname].jj.vg);
        if(iFile.parVal.count(iFile.mainModels[modname].jj.rtype) != 0)
          jj_rtype = iFile.parVal[iFile.mainModels[modname].jj.rtype];
        else jj_rtype = modifier(iFile.mainModels[modname].jj.rtype);
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
        if(iFile.parVal.count(iFile.mainModels[modname].jj.vg) != 0)
          jj_vgap = iFile.parVal[iFile.mainModels[modname].jj.vg];
        else jj_vgap = modifier(iFile.mainModels[modname].jj.vg);
        if(iFile.parVal.count(iFile.mainModels[modname].jj.rtype) != 0)
          jj_rtype = iFile.parVal[iFile.mainModels[modname].jj.rtype];
        else jj_rtype = modifier(iFile.mainModels[modname].jj.rtype);
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
    jj_tokens["RTYPE"] = jj_rtype;
    jj_tokens["VGAP"] = jj_vgap;
    jj_tokens["CAP"] = jj_cap * area;
    jj_tokens["RN"] = jj_rn / area;
    jj_tokens["R0"] = jj_rzero / area;
    jj_tokens["ICRIT"] = jj_icrit * area;
    /* Awaiting implementation: CONDEV & IC*/
    break;
  case MTJ:
    break;
  }
  return jj_tokens;
}
