#pragma once
#include "include/j_components.hpp"

/*
  Count the components
*/
void count_component(std::string c, InputFile& iFile, std::string isSubCkt) {
  std::vector<std::string> tokens;
  switch(c[0]) {
    case 'R':
      if(!isSubCkt.empty()) iFile.subCircuitComponentCount[isSubCkt]++;
      else iFile.circuitComponentCount++;
      break;
    case 'L':
      if(!isSubCkt.empty()) iFile.subCircuitComponentCount[isSubCkt]++;
      else iFile.circuitComponentCount++;
      break;
    case 'C':
      if(!isSubCkt.empty()) iFile.subCircuitComponentCount[isSubCkt]++;
      else iFile.circuitComponentCount++;
      break;
    case 'B':
      if(!isSubCkt.empty()){
        iFile.subCircuitJJCount[isSubCkt]++;
        iFile.subCircuitComponentCount[isSubCkt]++;
      }
      else {
        iFile.circuitJJCount++;
        iFile.circuitComponentCount++;
      }
      break;
    case 'K':
      if(!isSubCkt.empty()) iFile.subCircuitComponentCount[isSubCkt]++;
      else iFile.circuitComponentCount++;
      break;
    case 'V':
      if(!isSubCkt.empty()) iFile.subCircuitComponentCount[isSubCkt]++;
      else iFile.circuitComponentCount++;
      break;
    case 'I':
      if(!isSubCkt.empty()) iFile.subCircuitComponentCount[isSubCkt]++;
      else iFile.circuitComponentCount++;
      break;
    case 'T':
      if(!isSubCkt.empty()) iFile.subCircuitComponentCount[isSubCkt]++;
      else iFile.circuitComponentCount++;
      break;
    case 'X':
      if(!isSubCkt.empty()) iFile.subCircuitContainsSubCicuit[isSubCkt] = 1;
      else {
        tokens = tokenize_space(c);
        iFile.circuitComponentCount += iFile.subCircuitComponentCount[tokens[1]];
        iFile.circuitJJCount += iFile.subCircuitJJCount[tokens[1]];
      }
      break;
    default:
      break;
  }
}
/*
  Count the components in each subcircuit and add them to the respective subcircuit component counts
*/
void count_subcircuit_component(std::vector<std::string> c, InputFile& iFile, std::string isSubCkt) {
  std::vector<std::string> tokens;
  int totalSubComponents = 0;
  int totalSubJJ = 0;
  bool triggered = false;
  for (auto i : c) {
    switch(i[0]) {
      case 'X':
        tokens = tokenize_space(i);
        if(!isSubCkt.empty()) {
          if(iFile.subCircuitContainsSubCicuit[isSubCkt] != 0) {
            if(iFile.subCircuitContainsSubCicuit[tokens[1]] == 0) {
              totalSubComponents += iFile.subCircuitComponentCount[tokens[1]];
              totalSubJJ += iFile.subCircuitJJCount[tokens[1]];
            }
            else {
               iFile.allCounted = 0;
               totalSubComponents = 0;
               totalSubJJ = 0;
               triggered = true;
               break;
             }
          }
         }
        break;
      default:
        break;
    }
    if (triggered) break;
  }
  iFile.subCircuitComponentCount[isSubCkt] += totalSubComponents;
  iFile.subCircuitJJCount[isSubCkt] += totalSubJJ;
}
/*
  Return JJ parameters from tokens
*/
void jj_comp(std::vector<std::string> tokens, double &jj_cap, double &jj_rn, double &jj_rzero, double &jj_icrit) {
	/* Assume tokens 0-2 are label, pnode, nnode so they can be ignored */
	std::string label, modname;
	double area;
	model_rcsj jj;
	bool found = false;
	try { label = tokens.at(0); }
	catch (std::out_of_range) {
		invalid_component_errors(MISSING_LABEL, "unknown");
	}
	try { modname = tokens.at(3); }
	catch (const std::out_of_range) {
		invalid_component_errors(MISSING_JJMODEL, label);
	}
	for (auto i : models) {
		if (i.first.find(modname) != std::string::npos) {
			jj = i.second;
			found = true;
			break;
		}
	}
	if (!found) {
		invalid_component_errors(MODEL_NOT_DEFINED, modname);
	}
	try { 
		area = modifier(substring_after(tokens.at(4), "AREA=")); 
	}
	catch (std::out_of_range) {
		area = 1.0;
		invalid_component_errors(MODEL_AREA_NOT_GIVEN, label);
	}
	jj_cap = jj.cap * area;
	jj_rn = jj.rnormal / area;
	jj_rzero = jj.rzero / area;
	jj_icrit = jj.icrit * area;
	/* Awaiting implementation: CONDEV & IC*/
}