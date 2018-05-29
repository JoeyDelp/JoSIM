// Copyright (c) 2018 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#include "j_components.hpp"


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
      if(!isSubCkt.empty()) iFile.subCircuitContainsSubCircuit[isSubCkt] = 1;
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
          if(iFile.subCircuitContainsSubCircuit[isSubCkt] != 0) {
            if(iFile.subCircuitContainsSubCircuit[tokens[1]] == 0) {
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
  // However not the case when Junction is a 3 terminal device
	std::string label, modname, maybemodname;
	double area;
	model_rcsj jj;
	bool found = false;
	try { label = tokens.at(0); }
	catch (const std::out_of_range&) {
		invalid_component_errors(MISSING_LABEL, "unknown");
	}
  // Now we need to be careful as the model could be at either 3 or 4
	try { 
    // If tokens are greater than 5 then it is most likely a 3 terminal junction
    if(tokens.size() > 5) {
      // Try the model name at 5th token
      if(models.find(tokens[4]) != models.end()) {
        jj = models[tokens[4]];
        modname = tokens.at(4);
        found = true;
      }
      else {
        // Else there might be junk at the end of the device line
        // Therefore try the 4th token
        if(models.find(tokens[3]) != models.end()) {
          jj = models[tokens[3]];
          modname = tokens.at(3); 
          found = true;
        }
        // Else the model name might be prepended if using global modals
        else {
          maybemodname = tokens[3].substr(tokens[3].find_last_of("_") + 1);
          if(models.find(maybemodname) != models.end()) {
            jj = models[maybemodname];
            modname = maybemodname; 
            found = true;
          }
          else {
            maybemodname = tokens[4].substr(tokens[4].find_last_of("_") + 1);
            if(models.find(maybemodname) != models.end()) {
              jj = models[maybemodname];
              modname = maybemodname; 
              found = true;
            }
          }
        }
      }
    }
    // Else it is most likely a 2 terminal device and the model is in the correct place
    else { 
      // Try the 4th token
      if(models.find(tokens[3]) != models.end()) {
        jj = models[tokens[3]];
        modname = tokens.at(3); 
        found = true;
      }
      // Else the model name might be prepended if using global modals
      else {
        maybemodname = tokens[3].substr(tokens[3].find_last_of("_") + 1);
        if(models.find(maybemodname) != models.end()) {
          jj = models[maybemodname];
          modname = maybemodname; 
          found = true;
        }
      }
    }
  }
	catch (const std::out_of_range) {
		invalid_component_errors(MISSING_JJMODEL, label);
	}
	if (!found) {
		invalid_component_errors(MODEL_NOT_DEFINED, modname);
	}
	try { 
    if(tokens.size() > 5) {
      if(parVal.find(substring_after(tokens.at(5), "AREA=")) != parVal.end()) area = parVal[substring_after(tokens.at(5), "AREA=")];
      else area = modifier(substring_after(tokens.at(5), "AREA="));
    }
    else {
      if(parVal.find(substring_after(tokens.at(4), "AREA=")) != parVal.end()) area = parVal[substring_after(tokens.at(4), "AREA=")];
      else area = modifier(substring_after(tokens.at(4), "AREA="));
    }
	}
	catch (const std::out_of_range&) {
		area = 1.0;
		if (VERBOSE) {
			invalid_component_errors(MODEL_AREA_NOT_GIVEN, label);
		}
	}
	jj_cap = jj.cap * area;
	jj_rn = jj.rnormal / area;
	jj_rzero = jj.rzero / area;
	jj_icrit = jj.icrit * area;
	/* Awaiting implementation: CONDEV & IC*/
}
