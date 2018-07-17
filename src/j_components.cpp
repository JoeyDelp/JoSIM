// Copyright (c) 2018 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#include "j_components.hpp"

/*
  Return JJ parameters from tokens
*/
void
jj_comp(std::vector<std::string> tokens,
        InputFile& iFile,
        int& jj_type,
        double& jj_cap,
        double& jj_rn,
        double& jj_rzero,
        double& jj_icrit,
        double& jj_rtype,
        double& jj_vgap)
{
  /* Assume tokens 0-2 are label, pnode, nnode so they can be ignored */
  // However not the case when Junction is a 3 terminal device
  std::string label, modname, maybemodname, subcktName = "";
  double area = 1.0;
  // Identify the subcircuit name (if any) and the model name to identify the model better
  // This gets the model name regardless if it is the 3rd or the 4th token
  if(tokens.at(3).find_first_of("|") != std::string::npos) {
    if(iFile.subcircuitSegments.count(tokens.at(3).substr(0, tokens.at(3).find_first_of("|"))) != 0) {
      subcktName = tokens.at(3).substr(0, tokens.at(3).find_first_of("|"));
      modname = tokens.at(3).substr(tokens.at(3).find_first_of("|") + 1);
      if(!subcktName.empty() &&
        iFile.subcircuitSegments.at(subcktName).parVal.count(substring_after(tokens.at(4), "AREA=")) != 0)
        area = iFile.subcircuitSegments.at(subcktName).parVal.at(substring_after(tokens.at(4), "AREA="));
      else area = modifier(substring_after(tokens.at(4), "AREA="));
    }
    else if(iFile.subcircuitSegments.count(tokens.at(4).substr(0, tokens.at(4).find_first_of("|"))) != 0) {
      subcktName = tokens.at(4).substr(0, tokens.at(4).find_first_of("|"));
      modname = tokens.at(4).substr(tokens.at(4).find_first_of("|") + 1);
      if(!subcktName.empty() &&
        iFile.subcircuitSegments.at(subcktName).parVal.count(substring_after(tokens.at(5), "AREA=")) != 0)
        area = iFile.subcircuitSegments.at(subcktName).parVal.at(substring_after(tokens.at(5), "AREA="));
      else area = modifier(substring_after(tokens.at(5), "AREA="));
    }
    else invalid_component_errors(MISSING_JJMODEL, label);
  }
  else {
    if(iFile.mainModels.count(tokens.at(3)) != 0) {
      modname = tokens.at(3);
      if(iFile.parVal.count(substring_after(tokens.at(4), "AREA=")) != 0)
        area = iFile.parVal.at(substring_after(tokens.at(4), "AREA="));
      else area = modifier(substring_after(tokens.at(4), "AREA="));
    }
    else if(iFile.mainModels.count(tokens.at(4)) != 0) {
      modname = tokens.at(4);
      if(iFile.parVal.count(substring_after(tokens.at(5), "AREA=")) != 0)
        area = iFile.parVal.at(substring_after(tokens.at(5), "AREA="));
      else area = modifier(substring_after(tokens.at(5), "AREA="));
    }
    else invalid_component_errors(MISSING_JJMODEL, label);
  }
  if (area <= std::numeric_limits<double>::epsilon()) area = 1;
  if(!subcktName.empty()) { 
    if(iFile.subcircuitSegments.at(subcktName).subcktModels.count(modname) != 0) jj_rtype = iFile.subcircuitSegments.at(subcktName).subcktModels.at(modname).modelType;
    else if(iFile.mainModels.count(modname) != 0) jj_rtype = iFile.mainModels.at(modname).modelType;
  }
  else {
    if(iFile.mainModels.count(modname) != 0) jj_rtype = iFile.mainModels.at(modname).modelType;
    else invalid_component_errors(MODEL_NOT_DEFINED, modname);
  }
  switch(jj_type) {
  case RCSJ:
    if(!subcktName.empty()) { 
      if(iFile.subcircuitSegments.at(subcktName).subcktModels.count(modname) != 0) {
        if(iFile.subcircuitSegments.at(subcktName).parVal.count(iFile.subcircuitSegments.at(subcktName).subcktModels.at(modname).jj.vg) != 0)
          jj_vgap = iFile.subcircuitSegments.at(subcktName).parVal.at(iFile.subcircuitSegments.at(subcktName).subcktModels.at(modname).jj.vg);
        else jj_vgap = modifier(iFile.subcircuitSegments.at(subcktName).subcktModels.at(modname).jj.vg);
        if(iFile.subcircuitSegments.at(subcktName).parVal.count(iFile.subcircuitSegments.at(subcktName).subcktModels.at(modname).jj.rtype) != 0)
          jj_rtype = iFile.subcircuitSegments.at(subcktName).parVal.at(iFile.subcircuitSegments.at(subcktName).subcktModels.at(modname).jj.rtype);
        else jj_rtype = modifier(iFile.subcircuitSegments.at(subcktName).subcktModels.at(modname).jj.rtype);
        if(iFile.subcircuitSegments.at(subcktName).parVal.count(iFile.subcircuitSegments.at(subcktName).subcktModels.at(modname).jj.cap) != 0)
          jj_cap = iFile.subcircuitSegments.at(subcktName).parVal.at(iFile.subcircuitSegments.at(subcktName).subcktModels.at(modname).jj.cap);
        else jj_cap = modifier(iFile.subcircuitSegments.at(subcktName).subcktModels.at(modname).jj.cap);
        if(iFile.subcircuitSegments.at(subcktName).parVal.count(iFile.subcircuitSegments.at(subcktName).subcktModels.at(modname).jj.rnormal) != 0)
          jj_rn = iFile.subcircuitSegments.at(subcktName).parVal.at(iFile.subcircuitSegments.at(subcktName).subcktModels.at(modname).jj.rnormal);
        else jj_rn = modifier(iFile.subcircuitSegments.at(subcktName).subcktModels.at(modname).jj.rnormal);
        if(iFile.subcircuitSegments.at(subcktName).parVal.count(iFile.subcircuitSegments.at(subcktName).subcktModels.at(modname).jj.rzero) != 0)
          jj_rzero = iFile.subcircuitSegments.at(subcktName).parVal.at(iFile.subcircuitSegments.at(subcktName).subcktModels.at(modname).jj.rzero);
        else jj_rzero = modifier(iFile.subcircuitSegments.at(subcktName).subcktModels.at(modname).jj.rzero);
        if(iFile.subcircuitSegments.at(subcktName).parVal.count(iFile.subcircuitSegments.at(subcktName).subcktModels.at(modname).jj.icrit) != 0)
          jj_icrit = iFile.subcircuitSegments.at(subcktName).parVal.at(iFile.subcircuitSegments.at(subcktName).subcktModels.at(modname).jj.icrit);
        else jj_icrit = modifier(iFile.subcircuitSegments.at(subcktName).subcktModels.at(modname).jj.icrit);
      }
      else if(iFile.mainModels.count(modname) != 0) {
        if(iFile.parVal.count(iFile.mainModels.at(modname).jj.vg) != 0)
          jj_vgap = iFile.parVal.at(iFile.mainModels.at(modname).jj.vg);
        else jj_vgap = modifier(iFile.mainModels.at(modname).jj.vg);
        if(iFile.parVal.count(iFile.mainModels.at(modname).jj.rtype) != 0)
          jj_rtype = iFile.parVal.at(iFile.mainModels.at(modname).jj.rtype);
        else jj_rtype = modifier(iFile.mainModels.at(modname).jj.rtype);
        if(iFile.parVal.count(iFile.mainModels.at(modname).jj.cap) != 0)
          jj_cap = iFile.parVal.at(iFile.mainModels.at(modname).jj.cap);
        else jj_cap = modifier(iFile.mainModels.at(modname).jj.cap);
        if(iFile.parVal.count(iFile.mainModels.at(modname).jj.rnormal) != 0)
          jj_rn = iFile.parVal.at(iFile.mainModels.at(modname).jj.rnormal);
        else jj_rn = modifier(iFile.mainModels.at(modname).jj.rnormal);
        if(iFile.parVal.count(iFile.mainModels.at(modname).jj.rzero) != 0)
          jj_rzero = iFile.parVal.at(iFile.mainModels.at(modname).jj.rzero);
        else jj_rzero = modifier(iFile.mainModels.at(modname).jj.rzero);
        if(iFile.parVal.count(iFile.mainModels.at(modname).jj.icrit) != 0)
          jj_icrit = iFile.parVal.at(iFile.mainModels.at(modname).jj.icrit);
        else jj_icrit = modifier(iFile.mainModels.at(modname).jj.icrit);
      } 
      else invalid_component_errors(MODEL_NOT_DEFINED, modname);
    }
    else {
      if(iFile.mainModels.count(modname) != 0) {
        if(iFile.parVal.count(iFile.mainModels.at(modname).jj.vg) != 0)
          jj_vgap = iFile.parVal.at(iFile.mainModels.at(modname).jj.vg);
        else jj_vgap = modifier(iFile.mainModels.at(modname).jj.vg);
        if(iFile.parVal.count(iFile.mainModels.at(modname).jj.rtype) != 0)
          jj_rtype = iFile.parVal.at(iFile.mainModels.at(modname).jj.rtype);
        else jj_rtype = modifier(iFile.mainModels.at(modname).jj.rtype);
        if(iFile.parVal.count(iFile.mainModels.at(modname).jj.cap) != 0)
          jj_cap = iFile.parVal.at(iFile.mainModels.at(modname).jj.cap);
        else jj_cap = modifier(iFile.mainModels.at(modname).jj.cap);
        if(iFile.parVal.count(iFile.mainModels.at(modname).jj.rnormal) != 0)
          jj_rn = iFile.parVal.at(iFile.mainModels.at(modname).jj.rnormal);
        else jj_rn = modifier(iFile.mainModels.at(modname).jj.rnormal);
        if(iFile.parVal.count(iFile.mainModels.at(modname).jj.rzero) != 0)
          jj_rzero = iFile.parVal.at(iFile.mainModels.at(modname).jj.rzero);
        else jj_rzero = modifier(iFile.mainModels.at(modname).jj.rzero);
        if(iFile.parVal.count(iFile.mainModels.at(modname).jj.icrit) != 0)
          jj_icrit = iFile.parVal.at(iFile.mainModels.at(modname).jj.icrit);
        else jj_icrit = modifier(iFile.mainModels.at(modname).jj.icrit);
      }
      else invalid_component_errors(MODEL_NOT_DEFINED, modname);
    }
    jj_cap = jj_cap * area;
    jj_rn = jj_rn / area;
    jj_rzero = jj_rzero / area;
    jj_icrit = jj_icrit * area;
    /* Awaiting implementation: CONDEV & IC*/
    break;
  case MTJ:
    break;
  }
}
