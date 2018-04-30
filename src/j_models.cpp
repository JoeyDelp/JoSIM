// Copyright (c) 2018 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifdef WIN32
#include "include/j_models.hpp"
#else
#include "j_models.hpp"
#endif

std::unordered_map<std::string, model_rcsj> models;
/* 
  Identify the models in both the subcircuits as well as the main design 
*/
void model_rcsj_functions::identify_models(InputFile& iFile, std::unordered_map<std::string, model_rcsj> &models) {
	std::string modelname, identifier, value;
	/* First the subcircuit models */
	for (auto i : iFile.subcircuitModels) {
		for (auto j : i.second) {
			std::vector<std::string> modeltokens = tokenize_space(j);
			modelname = i.first + "_" + modeltokens[1];
			models[modelname].modelname = modeltokens[1];
			/* Identify string parrameter part of the string */
			unsigned first = j.find('(') + 1;
			unsigned last = j.find(')');
			std::string modelParams = j.substr(first, last - first);
			modeltokens.clear();
			/* Tokenize the parameter part of the string into seperate parameters */
			modeltokens = tokenize_delimeter(modelParams, " ,");
			for (int k = 1; k < modeltokens.size(); k++) {
				if (modeltokens[k - 1].at(modeltokens[k-1].size() - 1) == '=') {
					modeltokens[k - 1] = modeltokens[k - 1] + modeltokens[k];
					modeltokens[k] = modeltokens[k - 1] + modeltokens[k];
				}
			}
			for (auto k : modeltokens) {
				/* Isolate the identifier and value */
				first = k.find('=') + 1;
				identifier = k.substr(0, first - 1);
				value = k.substr(first);
				/* Apply value to model based on identifier */
				if (identifier == "RTYPE") models[modelname].rtype = atoi(value.c_str());
				else if (identifier == "CCT") models[modelname].cct = atoi(value.c_str());
				else if (identifier == "VG") models[modelname].vg = modifier(value);
				else if (identifier == "DELV") models[modelname].delv = modifier(value);
				else if (identifier == "ICON") models[modelname].icon = modifier(value);
				else if (identifier == "R0") models[modelname].rzero = modifier(value);
				else if (identifier == "RN") models[modelname].rnormal = modifier(value);
				else if (identifier == "CAP") models[modelname].cap = modifier(value);
				else if (identifier == "ICRIT") models[modelname].icrit = modifier(value);
				/* Error if unknown parameter was identfied */
				else model_errors(PARAM_TYPE_ERROR, identifier);
			}
		}
	}

	/* Second the main design */
	for (auto i : iFile.maincircuitModels) {
		std::vector<std::string> modeltokens = tokenize_space(i);
		modelname = modeltokens[1];
		models[modelname].modelname = modeltokens[1];
		/* Identify string parrameter part of the string */
		unsigned first = i.find('(') + 1;
		unsigned last = i.find(')');
		std::string modelParams = i.substr(first, last - first);
		modeltokens.clear();
		/* Tokenize the parameter part of the string into seperate parameters */
		modeltokens = tokenize_delimeter(modelParams, " ,");
		for (auto j : modeltokens) {
			/* Isolate the identifier and value */
			first = j.find('=') + 1;
			identifier = j.substr(0, first-1);
			value = j.substr(first);
			/* Apply value to model based on identifier */
			if (identifier == "RTYPE") models[modelname].rtype = atoi(value.c_str());
			else if (identifier == "CCT") models[modelname].cct = atoi(value.c_str());
			else if (identifier == "VG") models[modelname].vg = modifier(value);
			else if (identifier == "DELV") models[modelname].delv = modifier(value);
			else if (identifier == "ICON") models[modelname].icon = modifier(value);
			else if (identifier == "R0") models[modelname].rzero = modifier(value);
			else if (identifier == "RN") models[modelname].rnormal = modifier(value);
			else if (identifier == "CAP") models[modelname].cap = modifier(value);
			else if (identifier == "ICRIT") models[modelname].icrit = modifier(value);
			/* Error if unknown parameter was identfied */
			else model_errors(PARAM_TYPE_ERROR, identifier);
		}
	}
}
