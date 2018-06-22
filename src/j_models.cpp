// Copyright (c) 2018 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#include "j_models.hpp"

//std::unordered_map<std::string, model_rcsj> models;
/*
  Identify the models in both the subcircuits as well as the main design
*/
// void
// identify_models(
//   InputFile& iFile,
//   std::unordered_map<std::string, model_rcsj>& models)
// {
//   std::string modelname, identifier, value;
//   /* First the subcircuit models */
//   for (auto i : iFile.subcircuitModels) {
//     for (const auto& j : i.second) {
//       std::vector<std::string> modeltokens = tokenize_space(j);
//       modelname = i.first + "_" + modeltokens[1];
//       models[modelname].modelname = modeltokens[1];
//       /* Identify string parameter part of the string */
//       auto first = j.find('(') + 1;
//       auto last = j.find(')');
//       std::string modelParams = j.substr(first, last - first);
//       modeltokens.clear();
//       /* Tokenize the parameter part of the string into separate parameters */
//       modeltokens = tokenize_delimeter(modelParams, " ,");
//       for (int k = 1; k < modeltokens.size(); k++) {
//         if (modeltokens[k - 1].at(modeltokens[k - 1].size() - 1) == '=') {
//           modeltokens[k - 1] = modeltokens[k - 1] + modeltokens[k];
//           modeltokens[k] = modeltokens[k - 1] + modeltokens[k];
//         }
//       }
//       for (const auto& k : modeltokens) {
//         /* Isolate the identifier and value */
//         first = k.find('=') + 1;
//         identifier = k.substr(0, first - 1);
//         value = k.substr(first);
//         /* Apply value to model based on identifier */
//         if (identifier == "RTYPE")
//           models[modelname].rtype = std::stoi(value);
//         else if (identifier == "CCT")
//           models[modelname].cct = std::stoi(value);
//         else if (identifier == "VG")
//           models[modelname].vg = modifier(value);
//         else if (identifier == "DELV")
//           models[modelname].delv = modifier(value);
//         else if (identifier == "ICON")
//           models[modelname].icon = modifier(value);
//         else if (identifier == "R0")
//           models[modelname].rzero = modifier(value);
//         else if (identifier == "RN")
//           models[modelname].rnormal = modifier(value);
//         else if (identifier == "CAP")
//           models[modelname].cap = modifier(value);
//         else if (identifier == "ICRIT")
//           models[modelname].icrit = modifier(value);
//         /* Error if unknown parameter was identified */
//         else
//           model_errors(PARAM_TYPE_ERROR, identifier);
//       }
//     }
//   }

//   /* Second the main design */
//   for (const auto& i : iFile.maincircuitModels) {
//     std::vector<std::string> modeltokens = tokenize_space(i);
//     modelname = modeltokens[1];
//     models[modelname].modelname = modeltokens[1];
//     /* Identify string parameter part of the string */
//     size_t first = i.find('(') + 1;
//     size_t last = i.find(')');
//     std::string modelParams = i.substr(first, last - first);
//     modeltokens.clear();
//     /* Tokenize the parameter part of the string into separate parameters */
//     modeltokens = tokenize_delimeter(modelParams, " ,");
//     for (const auto& j : modeltokens) {
//       /* Isolate the identifier and value */
//       first = j.find('=') + 1;
//       identifier = j.substr(0, first - 1);
//       value = j.substr(first);
//       /* Apply value to model based on identifier */
//       if (identifier == "RTYPE")
//         models[modelname].rtype = std::stoi(value);
//       else if (identifier == "CCT")
//         models[modelname].cct = std::stoi(value);
//       else if (identifier == "VG")
//         models[modelname].vg = modifier(value);
//       else if (identifier == "DELV")
//         models[modelname].delv = modifier(value);
//       else if (identifier == "ICON")
//         models[modelname].icon = modifier(value);
//       else if (identifier == "R0")
//         models[modelname].rzero = modifier(value);
//       else if (identifier == "RN")
//         models[modelname].rnormal = modifier(value);
//       else if (identifier == "CAP")
//         models[modelname].cap = modifier(value);
//       else if (identifier == "ICRIT")
//         models[modelname].icrit = modifier(value);
//       /* Error if unknown parameter was identified */
//       else
//         model_errors(PARAM_TYPE_ERROR, identifier);
//     }
//   }
// }
