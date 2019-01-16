// Copyright (c) 2018 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#include "j_components.h"

void
Component::jj_comp(std::string modName, std::string area, std::string jjLabel, std::string subckt) {
	std::string params;
	std::vector<std::string> paramTokens, itemToken, tempToken;
	jj_volt jj = iFile.voltJJ.at(jjLabel);
	double value = 0.0;
	if (modName != "") { 
		params = iFile.models.at(modName);
		params = params.substr(params.find_first_of("("), params.size());
		paramTokens = Misc::tokenize_delimeter(params, "(), ");
		for(int i = 0; i < paramTokens.size(); i++) {
			itemToken = Misc::tokenize_delimeter(paramTokens.at(i), "=");
			if((itemToken.size() == 1) && (i != paramTokens.size() - 1)) { 
				tempToken = Misc::tokenize_delimeter(paramTokens.at(i + 1), "=");
				if(tempToken.size() == 1) itemToken.push_back(tempToken.at(0));
				paramTokens.erase(paramTokens.begin() + i + 1);
			}
			else if((itemToken.size() == 1) && (i != paramTokens.size() - 1))
				Errors::model_errors(BAD_MODEL_DEFINITION, iFile.models.at(modName));
			value = Parser::parse_return_expression(itemToken.at(1), subckt);
			if(itemToken.at(0) == "VG" || itemToken.at(0) == "VGAP") jj.vG = value;
			else if(itemToken.at(0) == "IC" || itemToken.at(0) == "ICRIT") jj.iC = value;
			else if(itemToken.at(0) == "RTYPE") jj.rType = (int)value;
			else if(itemToken.at(0) == "RN") jj.rN = value;
			else if(itemToken.at(0) == "R0") jj.r0 = value;
			else if(itemToken.at(0) == "CAP" || itemToken.at(0) == "C") jj.C = value;
			else if(itemToken.at(0) == "T") jj.T = value;
			else if(itemToken.at(0) == "TC") jj.tC = value;
			else if(itemToken.at(0) == "DELV") jj.delV = value;
			else if(itemToken.at(0) == "D") jj.D = value;
			else if(itemToken.at(0) == "ICFACT" || 
					itemToken.at(0) == "ICFCT") jj.iCFact = value;
			else if(itemToken.at(0) == "PHI") jj.phi0 = jj.pn1 = value;
		}
		if (area == "") value = 1.0;
		else value = Parser::parse_return_expression(area, subckt);
		jj.C = jj.C * value;
		jj.rN = jj.rN / value;
		jj.r0 = jj.r0 / value;
		jj.iC = jj.iC * value;
	}
	else {
		Errors::invalid_component_errors(MODEL_NOT_DEFINED, modName);
		jj.C = jj.C * value;
		jj.rN = jj.rN / value;
		jj.r0 = jj.r0 / value;
		jj.iC = jj.iC * value;
	} 
	iFile.voltJJ.at(jjLabel) = jj;
}

void
Component::jj_comp_phase(std::string modName, std::string area, std::string jjLabel, std::string subckt) {
	std::string params;
	std::vector<std::string> paramTokens, itemToken, tempToken;
	jj_phase jj = iFile.phaseJJ.at(jjLabel);
	double value = 0.0;
	if (modName != "") { 
		params = iFile.models.at(modName);
		params = params.substr(params.find_first_of("("), params.size());
		paramTokens = Misc::tokenize_delimeter(params, "(), ");
		for(int i = 0; i < paramTokens.size(); i++) {
			itemToken = Misc::tokenize_delimeter(paramTokens.at(i), "=");
			if((itemToken.size() == 1) && (i != paramTokens.size() - 1)) { 
				tempToken = Misc::tokenize_delimeter(paramTokens.at(i + 1), "=");
				if(tempToken.size() == 1) itemToken.push_back(tempToken.at(0));
				paramTokens.erase(paramTokens.begin() + i + 1);
			}
			else if((itemToken.size() == 1) && (i != paramTokens.size() - 1))
				Errors::model_errors(BAD_MODEL_DEFINITION, iFile.models.at(modName));
			value = Parser::parse_return_expression(itemToken.at(1), subckt);
			if(itemToken.at(0) == "VG" || itemToken.at(0) == "VGAP") jj.vG = value;
			else if(itemToken.at(0) == "IC" || itemToken.at(0) == "ICRIT") jj.iC = value;
			else if(itemToken.at(0) == "RTYPE") jj.rType = (int)value;
			else if(itemToken.at(0) == "RN") jj.rN = value;
			else if(itemToken.at(0) == "R0") jj.r0 = value;
			else if(itemToken.at(0) == "CAP" || itemToken.at(0) == "C") jj.C = value;
			else if(itemToken.at(0) == "T") jj.T = value;
			else if(itemToken.at(0) == "TC") jj.tC = value;
			else if(itemToken.at(0) == "DELV") jj.delV = value;
			else if(itemToken.at(0) == "D") jj.D = value;
			else if(itemToken.at(0) == "ICFACT" || 
					itemToken.at(0) == "ICFCT") jj.iCFact = value;
			else if(itemToken.at(0) == "PHI") jj.phi0 = jj.pn1 = value;
		}
		if (area == "") value = 1.0;
		else value = Parser::parse_return_expression(area, subckt);
		jj.C = jj.C * value;
		jj.rN = jj.rN / value;
		jj.r0 = jj.r0 / value;
		jj.iC = jj.iC * value;
	}
	else {
		Errors::invalid_component_errors(MODEL_NOT_DEFINED, modName);
		jj.C = jj.C * value;
		jj.rN = jj.rN / value;
		jj.r0 = jj.r0 / value;
		jj.iC = jj.iC * value;
	} 
	iFile.phaseJJ.at(jjLabel) = jj;
}