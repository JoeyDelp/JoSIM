// Copyright (c) 2018 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#include "j_plot.h"
#ifdef USING_MATPLOTLIB
namespace plt = matplotlibcpp;
#endif
/*
	Determine traces to plot from the control part of the main circuit
*/
void 
Plot::traces_to_plot(InputFile& iFile, 
	std::vector<std::string> controlPart, 
	std::vector<std::string>& traceLabel, 
	std::vector<std::vector<double>>& traceData) {

	std::vector<std::string> tokens, labeltokens, nodesTokens;
	std::vector<double> trace;
	std::map<std::string, std::vector<double>> traces;
	std::string columnLabel1, columnLabel2, label, nodesToPlot;
	int index1 = -1;
	int index2 = -1;
	for (const auto &string : controlPart) {
		/****************************************************/
		/*						PRINT						*/
		/****************************************************/
		if (string.find("PRINT") != std::string::npos) {
			tokens = Misc::tokenize_space(string);
			/* Print the identified node voltage */
			/*****************************************************************************************************/
			if (tokens[1] == "NODEV") {
				/* If more than one node is specified */
				if (tokens.size() == 4) {
					/* If second node is ground */
					if (tokens[3] == "0" || tokens[3] == "GND") {
						label = "NODE VOLTAGE " + tokens[2];
						if (tokens[2][0] == 'X') {
							labeltokens = Misc::tokenize_delimeter(tokens[2], "_");
							std::rotate(labeltokens.begin(), labeltokens.end() - 1, labeltokens.end());
							tokens[2] = labeltokens[0];
							for (int n = 1; n < labeltokens.size(); n++) {
								tokens[2] = tokens[2] + "|" + labeltokens[n];
							}
						}
						columnLabel1 = "C_NV" + tokens[2];
						if (std::find(iFile.matA.columnNames.begin(), iFile.matA.columnNames.end(), columnLabel1) != iFile.matA.columnNames.end()) {
							index1 = Misc::index_of(iFile.matA.columnNames, columnLabel1);
							traceLabel.push_back(label);
							traceData.push_back(iFile.xVect[index1]);
						}
						else {
							/* Error this node was not found and can therefore not be printed */
							Errors::plotting_errors(NO_SUCH_NODE_FOUND, tokens[2]);
						}
					}
					/* If first node is ground */
					else if (tokens[2] == "0" || tokens[3] == "GND") {
						label = "NODE VOLTAGE " + tokens[3];
						if (tokens[3][0] == 'X') {
							labeltokens = Misc::tokenize_delimeter(tokens[3], "_");
							std::rotate(labeltokens.begin(), labeltokens.end() - 1, labeltokens.end());
							tokens[3] = labeltokens[0];
							for (int n = 1; n < labeltokens.size(); n++) {
								tokens[3] = tokens[3] + "|" + labeltokens[n];
							}
						}
						columnLabel1 = "C_NV" + tokens[3];
						if (std::find(iFile.matA.columnNames.begin(), iFile.matA.columnNames.end(), columnLabel1) != iFile.matA.columnNames.end()) {
							index1 = Misc::index_of(iFile.matA.columnNames, columnLabel1);
							trace.clear();
							trace = iFile.xVect[index1];
							std::fill(trace.begin(), trace.end(), 0.0);
							std::transform(trace.begin(), trace.end(), iFile.xVect[index1].begin(), trace.begin(), std::minus<double>());
							traceLabel.push_back(label);
							traceData.push_back(trace);
						}
						else {
							/* Error this node was not found and can therefore not be printed */
							Errors::plotting_errors(NO_SUCH_NODE_FOUND, tokens[3]);
						}
					}
					/* If neither are ground*/
					else {
						label = "NODE VOLTAGE " + tokens[2] + " to " + tokens[3];
						columnLabel1 = "C_NV" + tokens[2];
						columnLabel2 = "C_NV" + tokens[3];
						if (tokens[2][0] == 'X') {
							labeltokens = Misc::tokenize_delimeter(tokens[2], "_");
							std::rotate(labeltokens.begin(), labeltokens.end() - 1, labeltokens.end());
							tokens[2] = labeltokens[0];
							for (int n = 1; n < labeltokens.size(); n++) {
								tokens[2] = tokens[2] + "|" + labeltokens[n];
							}
						}
						if (tokens[3][0] == 'X') {
							labeltokens = Misc::tokenize_delimeter(tokens[3], "_");
							std::rotate(labeltokens.begin(), labeltokens.end() - 1, labeltokens.end());
							tokens[3] = labeltokens[0];
							for (int n = 1; n < labeltokens.size(); n++) {
								tokens[3] = tokens[3] + "|" + labeltokens[n];
							}
						}
						if (std::find(iFile.matA.columnNames.begin(), iFile.matA.columnNames.end(), columnLabel1) != iFile.matA.columnNames.end()) {
							index1 = Misc::index_of(iFile.matA.columnNames, columnLabel1);
							trace.clear();
							trace = iFile.xVect[index1];
							if (std::find(iFile.matA.columnNames.begin(), iFile.matA.columnNames.end(), columnLabel2) != iFile.matA.columnNames.end()) {
								index2 = Misc::index_of(iFile.matA.columnNames, columnLabel2);
								std::transform(iFile.xVect[index1].begin(), iFile.xVect[index1].end(), iFile.xVect[index2].begin(), trace.begin(), std::minus<double>());
								traceLabel.push_back(label);
								traceData.push_back(trace);
							}
							else {
								/* Error this node was not found and can therefore not be printed */
								Errors::plotting_errors(NO_SUCH_NODE_FOUND, tokens[3]);
							}
						}
						else {
							/* Error this node was not found and can therefore not be printed */
							Errors::plotting_errors(NO_SUCH_NODE_FOUND, tokens[2]);
						}
					}
				}
				/* If only one node is specified */
				else {
					label = "NODE VOLTAGE " + tokens[2];
					columnLabel1 = "C_NV" + tokens[2];
					if (std::find(iFile.matA.columnNames.begin(), iFile.matA.columnNames.end(), columnLabel1) != iFile.matA.columnNames.end()) {
						index1 = Misc::index_of(iFile.matA.columnNames, columnLabel1);
						traceLabel.push_back(label);
						traceData.push_back(iFile.xVect[index1]);
					}
					else {
						/* Error this node was not found and can therefore not be printed */
					}
				}
			}
			/* Print the identified junction phase */
			/*****************************************************************************************************/
			else if (tokens[1] == "PHASE") {
				label = "PHASE " + tokens[2];
				if (tokens[2][0] == 'X') {
					labeltokens = Misc::tokenize_delimeter(tokens[2], "_");
					std::rotate(labeltokens.begin(), labeltokens.end() - 1, labeltokens.end());
					tokens[2] = labeltokens[0];
					for (int n = 1; n < labeltokens.size(); n++) {
						tokens[2] = tokens[2] + "|" + labeltokens[n];
					}
				}
				columnLabel1 = "C_P" + tokens[2];
				if (std::find(iFile.matA.columnNames.begin(), iFile.matA.columnNames.end(), columnLabel1) != iFile.matA.columnNames.end()) {
					index1 = Misc::index_of(iFile.matA.columnNames, columnLabel1);
					traceLabel.push_back(label);
					traceData.push_back(iFile.xVect[index1]);
				}
				else {
					/* Error this node was not found and can therefore not be printed */
					Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, tokens[2]);
				}
			}
			/* Print the identified device voltage */
			/*****************************************************************************************************/
			else if (tokens[1] == "DEVV") {
				label = "NOTHING";
				if (tokens[2][0] == 'X') {
					labeltokens = Misc::tokenize_delimeter(tokens[2], "_");
					std::rotate(labeltokens.begin(), labeltokens.end() - 1, labeltokens.end());
					tokens[2] = labeltokens[0];
					for (int n = 1; n < labeltokens.size(); n++) {
						tokens[2] = tokens[2] + "|" + labeltokens[n];
					}
				}
				switch (tokens[2][0]) {
					case 'R': 
						if(iFile.voltRes.count(tokens[2]) != 0) {
							if (iFile.voltRes.at(tokens[2]).posNRow == -1) 
								trace = iFile.xVect.at(
									iFile.voltRes.at(tokens[2]).negNRow);
							else if (iFile.voltRes.at(tokens[2]).negNRow == -1)
								trace = iFile.xVect.at(
									iFile.voltRes.at(tokens[2]).posNRow);
							else {
								trace = iFile.xVect.at(
									iFile.voltRes.at(tokens[2]).posNRow);
								std::transform(iFile.xVect.at(iFile.voltRes.at(tokens[2]).posNRow).begin(),
									iFile.xVect.at(iFile.voltRes.at(tokens[2]).posNRow).end(),
									iFile.xVect.at(iFile.voltRes.at(tokens[2]).negNRow).begin(),
									trace.begin(), std::minus<double>());
							}
							label = "DEVICE VOLTAGE " + tokens[2];
							traceLabel.push_back(label);
							traceData.push_back(trace);
						}
						else Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, tokens[2]);
						break;
					case 'L':
						if(iFile.voltInd.count(tokens[2]) != 0) {
							if (iFile.voltInd.at(tokens[2]).posNRow == -1) 
								trace = iFile.xVect.at(
									iFile.voltInd.at(tokens[2]).negNRow);
							else if (iFile.voltInd.at(tokens[2]).negNRow == -1)
								trace = iFile.xVect.at(
									iFile.voltInd.at(tokens[2]).posNRow);
							else {
								trace = iFile.xVect.at(
									iFile.voltInd.at(tokens[2]).posNRow);
								std::transform(iFile.xVect.at(iFile.voltInd.at(tokens[2]).posNRow).begin(),
									iFile.xVect.at(iFile.voltInd.at(tokens[2]).posNRow).end(),
									iFile.xVect.at(iFile.voltInd.at(tokens[2]).negNRow).begin(),
									trace.begin(), std::minus<double>());
							}
							label = "DEVICE VOLTAGE " + tokens[2];
							traceLabel.push_back(label);
							traceData.push_back(trace);
						}
						else Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, tokens[2]);
						break;
					case 'B':
						if(iFile.voltJJ.count(tokens[2]) != 0) {
							if (iFile.voltJJ.at(tokens[2]).posNRow == -1) 
								trace = iFile.xVect.at(
									iFile.voltJJ.at(tokens[2]).negNRow);
							else if (iFile.voltJJ.at(tokens[2]).negNRow == -1)
								trace = iFile.xVect.at(
									iFile.voltJJ.at(tokens[2]).posNRow);
							else {
								trace = iFile.xVect.at(
									iFile.voltJJ.at(tokens[2]).posNRow);
								std::transform(iFile.xVect.at(iFile.voltJJ.at(tokens[2]).posNRow).begin(),
									iFile.xVect.at(iFile.voltJJ.at(tokens[2]).posNRow).end(),
									iFile.xVect.at(iFile.voltJJ.at(tokens[2]).negNRow).begin(),
									trace.begin(), std::minus<double>());
							}
							label = "DEVICE VOLTAGE " + tokens[2];
							traceLabel.push_back(label);
							traceData.push_back(trace);
						}
						else Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, tokens[2]);
						break;
					case 'C':
						if(iFile.voltCap.count(tokens[2]) != 0) {
							if (iFile.voltCap.at(tokens[2]).posNRow == -1) 
								trace = iFile.xVect.at(
									iFile.voltCap.at(tokens[2]).negNRow);
							else if (iFile.voltCap.at(tokens[2]).negNRow == -1)
								trace = iFile.xVect.at(
									iFile.voltCap.at(tokens[2]).posNRow);
							else {
								trace = iFile.xVect.at(
									iFile.voltCap.at(tokens[2]).posNRow);
								std::transform(iFile.xVect.at(iFile.voltCap.at(tokens[2]).posNRow).begin(),
									iFile.xVect.at(iFile.voltCap.at(tokens[2]).posNRow).end(),
									iFile.xVect.at(iFile.voltCap.at(tokens[2]).negNRow).begin(),
									trace.begin(), std::minus<double>());
							}
							label = "DEVICE VOLTAGE " + tokens[2];
							traceLabel.push_back(label);
							traceData.push_back(trace);
						}
						else Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, tokens[2]);
						break;
					case 'V':
						if(iFile.voltVs.count(tokens[2]) != 0) {
							if (iFile.voltVs.at(tokens[2]).posNRow == -1) 
								trace = iFile.xVect.at(
									iFile.voltVs.at(tokens[2]).negNRow);
							else if (iFile.voltVs.at(tokens[2]).negNRow == -1)
								trace = iFile.xVect.at(
									iFile.voltVs.at(tokens[2]).posNRow);
							else {
								trace = iFile.xVect.at(
									iFile.voltVs.at(tokens[2]).posNRow);
								std::transform(iFile.xVect.at(iFile.voltVs.at(tokens[2]).posNRow).begin(),
									iFile.xVect.at(iFile.voltVs.at(tokens[2]).posNRow).end(),
									iFile.xVect.at(iFile.voltVs.at(tokens[2]).negNRow).begin(),
									trace.begin(), std::minus<double>());
							}
							label = "DEVICE VOLTAGE " + tokens[2];
							traceLabel.push_back(label);
							traceData.push_back(trace);
						}
						else Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, tokens[2]);
						break;
					default:
						Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, tokens[2]);
						break;
				}
			}
			/* Print the identified device current */
			/*****************************************************************************************************/
			else if (tokens[1] == "DEVI") {
				label = "NOTHING";
				if (tokens[2][0] == 'X') {
					labeltokens = Misc::tokenize_delimeter(tokens[2], "_");
					std::rotate(labeltokens.begin(), labeltokens.end() - 1, labeltokens.end());
					tokens[2] = labeltokens[0];
					for (int n = 1; n < labeltokens.size(); n++) {
						tokens[2] = tokens[2] + "|" + labeltokens[n];
					}
				}
				switch (tokens[2][0]) {
					case 'R': 
						if(iFile.voltRes.count(tokens[2]) != 0) {
							if (iFile.voltRes.at(tokens[2]).posNRow == -1)
								trace = iFile.xVect.at(
									iFile.voltRes.at(tokens[2]).negNRow);
							else if (iFile.voltRes.at(tokens[2]).negNRow== -1)
								trace = iFile.xVect.at(
									iFile.voltRes.at(tokens[2]).posNRow);
							else std::transform(
								iFile.xVect.at(iFile.voltRes.at(tokens[2]).posNRow).begin(), 
								iFile.xVect.at(iFile.voltRes.at(tokens[2]).posNRow).end(), 
								iFile.xVect.at(iFile.voltRes.at(tokens[2]).negNRow).begin(), 
								trace.begin(), std::minus<double>());
							std::transform(trace.begin(), trace.end(), 
								trace.begin(), std::bind(std::multiplies<double>(), 
								std::placeholders::_1, 
								(1 / iFile.voltRes.at(tokens[2]).value)));
							label = "DEVICE CURRENT " + tokens[2];
							traceLabel.push_back(label);
							traceData.push_back(trace);
						}
						else Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, tokens[2]);
						break;
					case 'L':
						if(iFile.voltInd.count(tokens[2]) != 0) {
							if (iFile.voltInd.at(tokens[2]).curNRow == -1)
								Errors::simulation_errors(
									INDUCTOR_CURRENT_NOT_FOUND, tokens[2]);
							else trace = iFile.xVect.at(
								iFile.voltInd.at(tokens[2]).curNRow);
							label = "DEVICE CURRENT " + tokens[2];
							traceLabel.push_back(label);
							traceData.push_back(trace);
						}
						else Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, tokens[2]);
						break;
					case 'B':
						if(iFile.voltJJ.count(tokens[2]) != 0) {
							trace = iFile.voltJJ.at(tokens[2]).jjCur;
							label = "DEVICE CURRENT " + tokens[2];
							traceLabel.push_back(label);
							traceData.push_back(trace);
						}
						else Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, tokens[2]);
						break;
					case 'C':
						if(iFile.voltCap.count(tokens[2]) != 0) {
							if (iFile.voltCap.at(tokens[2]).curNRow == -1)
								Errors::simulation_errors(
									INDUCTOR_CURRENT_NOT_FOUND, tokens[2]);
							else trace = iFile.xVect.at(
								iFile.voltCap.at(tokens[2]).curNRow);
							label = "DEVICE CURRENT " + tokens[2];
							traceLabel.push_back(label);
							traceData.push_back(trace);
						}
						else Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, tokens[2]);
						break;
					case 'V':
						if(iFile.voltVs.count(tokens[2]) != 0) {
							Errors::plotting_errors(CURRENT_THROUGH_VOLTAGE_SOURCE, tokens[2]);
						}
						else Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, tokens[2]);
						break;
					case 'I':
						if(iFile.matA.sources.count(tokens[2]) != 0) {
							trace = iFile.matA.sources.at(tokens[2]);
							label = "DEVICE CURRENT " + tokens[2];
							traceLabel.push_back(label);
							traceData.push_back(trace);
						}
						else Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, tokens[2]);
						break;
					default:
						Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, tokens[2]);
						break;
				}
			}
			/* No such print command error thrown */
			else {
				if (cArg.verbose) Errors::plotting_errors(NO_SUCH_PLOT_TYPE, tokens[1]);
			}
		}
		/****************************************************/
		/*						PLOT						*/
		/****************************************************/
		else if (string.find("PLOT") != std::string::npos) {
			tokens = Misc::tokenize_space(string);
			for (int k = 1; k < tokens.size(); k++) {
				/* If plotting voltage */
				if (tokens[k][0] == 'V') {
					/* Identify part between brackets */
					nodesToPlot = tokens[k].substr(2);
					nodesToPlot = nodesToPlot.substr(0, nodesToPlot.size() - 1);
					/* If multiple arguments are specified for V */
					if (nodesToPlot.find(',') != std::string::npos) {
						nodesTokens = Misc::tokenize_delimeter(nodesToPlot, ",");
						if (nodesTokens.size() > 2) {
							Errors::plotting_errors(TOO_MANY_NODES, string);
						}
						/* Ensure node 1 is not ground */
						if (nodesTokens[0] == "0" || nodesTokens[0] == "GND") {
							if (nodesTokens[1] == "0" || nodesTokens[1] == "GND") {
								Errors::plotting_errors(BOTH_ZERO, string);
							}
							else {
								if (nodesTokens[1][0] == 'X') {
									labeltokens = Misc::tokenize_delimeter(tokens[1], "_");
									std::rotate(labeltokens.begin(), labeltokens.end() - 1, labeltokens.end());
									nodesTokens[1] = labeltokens[0];
									for (int n = 1; n < labeltokens.size(); n++) {
										nodesTokens[1] = nodesTokens[1] + "|" + labeltokens[n];
									}
								}
								columnLabel1 = "C_NV" + nodesTokens[1];
								/* If this is a node voltage */
								if (std::find(iFile.matA.columnNames.begin(), iFile.matA.columnNames.end(), columnLabel1) != iFile.matA.columnNames.end()) {
									index1 = Misc::index_of(iFile.matA.columnNames, columnLabel1);
									trace.clear();
									trace = iFile.xVect[index1];
									std::fill(trace.begin(), trace.end(), 0.0);
									std::transform(trace.begin(), trace.end(), iFile.xVect[index1].begin(), trace.begin(), std::minus<double>());
									traceLabel.push_back(label);
									traceData.push_back(trace);
								}
								/* Else node not found */
								else {
									Errors::plotting_errors(NO_SUCH_NODE_FOUND, string);
								}
							}
						}
						/* Check if node 2 is ground */
						else if (tokens[1] == "0" || tokens[1] == "GND") {
							if (tokens[0][0] == 'X') {
								labeltokens = Misc::tokenize_delimeter(tokens[0], "_");
								std::rotate(labeltokens.begin(), labeltokens.end() - 1, labeltokens.end());
								tokens[0] = labeltokens[0];
								for (int n = 1; n < labeltokens.size(); n++) {
									tokens[0] = tokens[0] + "|" + labeltokens[n];
								}
							}
							columnLabel1 = "C_NV" + tokens[0];
							if (std::find(iFile.matA.columnNames.begin(), iFile.matA.columnNames.end(), columnLabel1) != iFile.matA.columnNames.end()) {
								index1 = Misc::index_of(iFile.matA.columnNames, columnLabel1);
								traceLabel.push_back(label);
								traceData.push_back(iFile.xVect[index1]);
							}
							else {
								Errors::plotting_errors(NO_SUCH_NODE_FOUND, string);
							}
						}
						/* Neither nodes are ground */
						else {
							label = "NODE VOLTAGE " + nodesTokens[0] + " to " + nodesTokens[1];
							columnLabel1 = "C_NV" + nodesTokens[0];
							columnLabel2 = "C_NV" + nodesTokens[1];
							if (nodesTokens[0][0] == 'X') {
								labeltokens = Misc::tokenize_delimeter(nodesTokens[0], "_");
								std::rotate(labeltokens.begin(), labeltokens.end() - 1, labeltokens.end());
								nodesTokens[0] = labeltokens[0];
								for (int n = 1; n < labeltokens.size(); n++) {
									nodesTokens[0] = nodesTokens[0] + "|" + labeltokens[n];
								}
							}
							if (nodesTokens[1][0] == 'X') {
								labeltokens = Misc::tokenize_delimeter(nodesTokens[1], "_");
								std::rotate(labeltokens.begin(), labeltokens.end() - 1, labeltokens.end());
								nodesTokens[1] = labeltokens[0];
								for (int n = 1; n < labeltokens.size(); n++) {
									nodesTokens[1] = nodesTokens[1] + "|" + labeltokens[n];
								}
							}
							if (std::find(iFile.matA.columnNames.begin(), iFile.matA.columnNames.end(), columnLabel1) != iFile.matA.columnNames.end()) {
								index1 = Misc::index_of(iFile.matA.columnNames, columnLabel1);
								trace.clear();
								trace = iFile.xVect[index1];
								if (std::find(iFile.matA.columnNames.begin(), iFile.matA.columnNames.end(), columnLabel2) != iFile.matA.columnNames.end()) {
									index2 = Misc::index_of(iFile.matA.columnNames, columnLabel2);
									std::transform(iFile.xVect[index1].begin(), iFile.xVect[index1].end(), iFile.xVect[index2].begin(), trace.begin(), std::minus<double>());
									traceLabel.push_back(label);
									traceData.push_back(trace);
								}
								else {
									/* Error this node was not found and can therefore not be printed */
									Errors::plotting_errors(NO_SUCH_NODE_FOUND, string);
								}
							}
						}
					}
					/* If only one argument is specified for V */
					else {
						/* Ensure node is not ground */
						if (nodesToPlot != "0" || nodesToPlot != "GND") {
							if (nodesToPlot[0] == 'X') {
								labeltokens = Misc::tokenize_delimeter(nodesToPlot, "_");
								std::rotate(labeltokens.begin(), labeltokens.end() - 1, labeltokens.end());
								nodesToPlot = labeltokens[0];
								for (int n = 1; n < labeltokens.size(); n++) {
									nodesToPlot = nodesToPlot + "|" + labeltokens[n];
								}
							}
							label = "C_NV" + nodesToPlot;
							/* If this is a node voltage */
							if (std::find(iFile.matA.columnNames.begin(), iFile.matA.columnNames.end(), label) != iFile.matA.columnNames.end()) {
								index1 = Misc::index_of(iFile.matA.columnNames, label);
								label = "NODE VOLTAGE " + nodesToPlot;
								traceLabel.push_back(label);
								traceData.push_back(iFile.xVect[index1]);
							}
							/* Else it might be device voltage */
							else {
								switch (nodesToPlot[0]) {
								case 'R': 
									if(iFile.voltRes.count(nodesToPlot) != 0) {
										if (iFile.voltRes.at(nodesToPlot).posNRow == -1) 
											trace = iFile.xVect.at(
												iFile.voltRes.at(nodesToPlot).negNRow);
										else if (iFile.voltRes.at(nodesToPlot).negNRow == -1)
											trace = iFile.xVect.at(
												iFile.voltRes.at(nodesToPlot).posNRow);
										else {
											trace = iFile.xVect.at(
												iFile.voltRes.at(nodesToPlot).posNRow);
											std::transform(iFile.xVect.at(iFile.voltRes.at(nodesToPlot).posNRow).begin(),
												iFile.xVect.at(iFile.voltRes.at(nodesToPlot).posNRow).end(),
												iFile.xVect.at(iFile.voltRes.at(nodesToPlot).negNRow).begin(),
												trace.begin(), std::minus<double>());
										}
										label = "DEVICE VOLTAGE " + nodesToPlot;
										traceLabel.push_back(label);
										traceData.push_back(trace);
									}
									else Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, nodesToPlot);
									break;
								case 'L':
									if(iFile.voltInd.count(nodesToPlot) != 0) {
										if (iFile.voltInd.at(nodesToPlot).posNRow == -1) 
											trace = iFile.xVect.at(
												iFile.voltInd.at(nodesToPlot).negNRow);
										else if (iFile.voltInd.at(nodesToPlot).negNRow == -1)
											trace = iFile.xVect.at(
												iFile.voltInd.at(nodesToPlot).posNRow);
										else {
											trace = iFile.xVect.at(
												iFile.voltInd.at(nodesToPlot).posNRow);
											std::transform(iFile.xVect.at(iFile.voltInd.at(nodesToPlot).posNRow).begin(),
												iFile.xVect.at(iFile.voltInd.at(nodesToPlot).posNRow).end(),
												iFile.xVect.at(iFile.voltInd.at(nodesToPlot).negNRow).begin(),
												trace.begin(), std::minus<double>());
										}
										label = "DEVICE VOLTAGE " + nodesToPlot;
										traceLabel.push_back(label);
										traceData.push_back(trace);
									}
									else Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, nodesToPlot);
									break;
								case 'B':
									if(iFile.voltJJ.count(nodesToPlot) != 0) {
										if (iFile.voltJJ.at(nodesToPlot).posNRow == -1) 
											trace = iFile.xVect.at(
												iFile.voltJJ.at(nodesToPlot).negNRow);
										else if (iFile.voltJJ.at(nodesToPlot).negNRow == -1)
											trace = iFile.xVect.at(
												iFile.voltJJ.at(nodesToPlot).posNRow);
										else {
											trace = iFile.xVect.at(
												iFile.voltJJ.at(nodesToPlot).posNRow);
											std::transform(iFile.xVect.at(iFile.voltJJ.at(nodesToPlot).posNRow).begin(),
												iFile.xVect.at(iFile.voltJJ.at(nodesToPlot).posNRow).end(),
												iFile.xVect.at(iFile.voltJJ.at(nodesToPlot).negNRow).begin(),
												trace.begin(), std::minus<double>());
										}
										label = "DEVICE VOLTAGE " + nodesToPlot;
										traceLabel.push_back(label);
										traceData.push_back(trace);
									}
									else Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, nodesToPlot);
									break;
								case 'C':
									if(iFile.voltCap.count(nodesToPlot) != 0) {
										if (iFile.voltCap.at(nodesToPlot).posNRow == -1) 
											trace = iFile.xVect.at(
												iFile.voltCap.at(nodesToPlot).negNRow);
										else if (iFile.voltCap.at(nodesToPlot).negNRow == -1)
											trace = iFile.xVect.at(
												iFile.voltCap.at(nodesToPlot).posNRow);
										else {
											trace = iFile.xVect.at(
												iFile.voltCap.at(nodesToPlot).posNRow);
											std::transform(iFile.xVect.at(iFile.voltCap.at(nodesToPlot).posNRow).begin(),
												iFile.xVect.at(iFile.voltCap.at(nodesToPlot).posNRow).end(),
												iFile.xVect.at(iFile.voltCap.at(nodesToPlot).negNRow).begin(),
												trace.begin(), std::minus<double>());
										}
										label = "DEVICE VOLTAGE " + nodesToPlot;
										traceLabel.push_back(label);
										traceData.push_back(trace);
									}
									else Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, nodesToPlot);
									break;
								case 'V':
									if(iFile.voltVs.count(nodesToPlot) != 0) {
										if (iFile.voltVs.at(nodesToPlot).posNRow == -1) 
											trace = iFile.xVect.at(
												iFile.voltVs.at(nodesToPlot).negNRow);
										else if (iFile.voltVs.at(nodesToPlot).negNRow == -1)
											trace = iFile.xVect.at(
												iFile.voltVs.at(nodesToPlot).posNRow);
										else {
											trace = iFile.xVect.at(
												iFile.voltVs.at(nodesToPlot).posNRow);
											std::transform(iFile.xVect.at(iFile.voltVs.at(nodesToPlot).posNRow).begin(),
												iFile.xVect.at(iFile.voltVs.at(nodesToPlot).posNRow).end(),
												iFile.xVect.at(iFile.voltVs.at(nodesToPlot).negNRow).begin(),
												trace.begin(), std::minus<double>());
										}
										label = "DEVICE VOLTAGE " + nodesToPlot;
										traceLabel.push_back(label);
										traceData.push_back(trace);
									}
									else Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, nodesToPlot);
									break;
								default:
									Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, nodesToPlot);
									break;
								}
							}
						}
					}
				}
				else if (tokens[k][0] == 'I') {
					/* Identify part between brackets */
					nodesToPlot = tokens[k].substr(2);
					nodesToPlot = nodesToPlot.substr(0, nodesToPlot.size() - 1);
					label = "NOTHING";
					if (nodesToPlot[0] == 'X') {
						labeltokens = Misc::tokenize_delimeter(nodesToPlot, "_");
						std::rotate(labeltokens.begin(), labeltokens.end() - 1, labeltokens.end());
						nodesToPlot = labeltokens[0];
						for (int n = 1; n < labeltokens.size(); n++) {
							nodesToPlot = nodesToPlot + "|" + labeltokens[n];
						}
					}
					switch (nodesToPlot[0]) {
					case 'R': 
						if(iFile.voltRes.count(nodesToPlot) != 0) {
							if (iFile.voltRes.at(nodesToPlot).posNRow == -1)
								trace = iFile.xVect.at(
									iFile.voltRes.at(nodesToPlot).negNRow);
							else if (iFile.voltRes.at(nodesToPlot).negNRow== -1)
								trace = iFile.xVect.at(
									iFile.voltRes.at(nodesToPlot).posNRow);
							else std::transform(
								iFile.xVect.at(iFile.voltRes.at(nodesToPlot).posNRow).begin(), 
								iFile.xVect.at(iFile.voltRes.at(nodesToPlot).posNRow).end(), 
								iFile.xVect.at(iFile.voltRes.at(nodesToPlot).negNRow).begin(), 
								trace.begin(), std::minus<double>());
							std::transform(trace.begin(), trace.end(), 
								trace.begin(), std::bind(std::multiplies<double>(), 
								std::placeholders::_1, 
								(1 / iFile.voltRes.at(nodesToPlot).value)));
							label = "DEVICE CURRENT " + nodesToPlot;
							traceLabel.push_back(label);
							traceData.push_back(trace);
						}
						else Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, nodesToPlot);
						break;
					case 'L':
						if(iFile.voltInd.count(nodesToPlot) != 0) {
							if (iFile.voltInd.at(nodesToPlot).curNRow == -1)
								Errors::simulation_errors(
									INDUCTOR_CURRENT_NOT_FOUND, nodesToPlot);
							else trace = iFile.xVect.at(
								iFile.voltInd.at(nodesToPlot).curNRow);
							label = "DEVICE CURRENT " + nodesToPlot;
							traceLabel.push_back(label);
							traceData.push_back(trace);
						}
						else Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, nodesToPlot);
						break;
					case 'B':
						if(iFile.voltJJ.count(nodesToPlot) != 0) {
							trace = iFile.voltJJ.at(nodesToPlot).jjCur;
							label = "DEVICE CURRENT " + nodesToPlot;
							traceLabel.push_back(label);
							traceData.push_back(trace);
						}
						else Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, nodesToPlot);
						break;
					case 'C':
						if(iFile.voltCap.count(nodesToPlot) != 0) {
							if (iFile.voltCap.at(nodesToPlot).curNRow == -1)
								Errors::simulation_errors(
									INDUCTOR_CURRENT_NOT_FOUND, nodesToPlot);
							else trace = iFile.xVect.at(
								iFile.voltCap.at(nodesToPlot).curNRow);
							label = "DEVICE CURRENT " + nodesToPlot;
							traceLabel.push_back(label);
							traceData.push_back(trace);
						}
						else Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, nodesToPlot);
						break;
					case 'V':
						if(iFile.voltVs.count(nodesToPlot) != 0) {
							Errors::plotting_errors(CURRENT_THROUGH_VOLTAGE_SOURCE, nodesToPlot);
						}
						else Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, nodesToPlot);
						break;
					case 'I':
						if(iFile.matA.sources.count(nodesToPlot) != 0) {
							trace = iFile.matA.sources.at(nodesToPlot);
							label = "DEVICE CURRENT " + nodesToPlot;
							traceLabel.push_back(label);
							traceData.push_back(trace);
						}
						else Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, nodesToPlot);
						break;
					default:
						Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, nodesToPlot);
						break;
					}
				}
				else if (tokens[k][0] == 'P') {
					/* Identify part between brackets */
					nodesToPlot = tokens[k].substr(2);
					nodesToPlot = nodesToPlot.substr(0, nodesToPlot.size() - 1);
					label = "PHASE " + nodesToPlot;
					if (nodesToPlot[0] == 'X') {
						labeltokens = Misc::tokenize_delimeter(nodesToPlot, "_");
						std::rotate(labeltokens.begin(), labeltokens.end() - 1, labeltokens.end());
						nodesToPlot = labeltokens[0];
						for (int n = 1; n < labeltokens.size(); n++) {
							nodesToPlot = nodesToPlot + "|" + labeltokens[n];
						}
					}
					columnLabel1 = "C_P" + nodesToPlot;
					if (std::find(iFile.matA.columnNames.begin(), iFile.matA.columnNames.end(), columnLabel1) != iFile.matA.columnNames.end()) {
						index1 = Misc::index_of(iFile.matA.columnNames, columnLabel1);
						traceLabel.push_back(label);
						traceData.push_back(iFile.xVect[index1]);
					}
					else {
						/* Error this node was not found and can therefore not be printed */
						Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, nodesToPlot);
					}
				}
			}
		}
		/****************************************************/
		/*						SAVE						*/
		/****************************************************/
		else if (string.find("SAVE") != std::string::npos) {
			tokens = Misc::tokenize_space(string);
			for (int k = 1; k < tokens.size(); k++) {
				index1 = tokens[k].find("@");
				if (index1 != std::string::npos) tokens[k] = tokens[k].substr(0, index1) + tokens[k].substr(index1 + 1);
				index1 = tokens[k].find(".at(");
				if (index1 != std::string::npos) tokens[k] = tokens[k].substr(0, index1);
				index1 = tokens[k].find(".");
				if (index1 != std::string::npos) tokens[k] = tokens[k].substr(0, index1) + "|" + tokens[k].substr(index1 + 1);
				index1 = tokens[k].find("[");
				if (index1 != std::string::npos) tokens[k] = tokens[k].substr(0, index1);
				/* If this is a current source */
				if (iFile.matA.sources.find(tokens[k]) != iFile.matA.sources.end()) {
					label = "CURRENT " + tokens[k];
					traceLabel.push_back(label);
					traceData.push_back(iFile.matA.sources.at(tokens[k]));
				}
			}
		}
	}
}
/*
	Determine phase traces to plot from the control part of the main circuit
*/
void Plot::phase_traces_to_plot(InputFile& iFile, std::vector<std::string> controlPart, std::vector<std::string>& traceLabel, std::vector<std::vector<double>>& traceData) {
	std::vector<std::string> tokens, labeltokens, nodesTokens;
	std::vector<double> trace;
	std::map<std::string, std::vector<double>> traces;
	std::string columnLabel1, columnLabel2, label, nodesToPlot;
	double vn1 = 0.0;
	int index1 = -1;
	int index2 = -1;
	for (const auto &string : controlPart) {
		/****************************************************/
		/*						PRINT						*/
		/****************************************************/
		if (string.find("PRINT") != std::string::npos) {
			tokens = Misc::tokenize_space(string);
			/* Print the identified node phase */
			/*****************************************************************************************************/
			if (tokens[1] == "NODEP") {
				/* If more than one node is specified */
				if (tokens.size() == 4) {
					/* If second node is ground */
					if (tokens[3] == "0" || tokens[3] == "GND") {
						label = "NODE PHASE " + tokens[2];
						if (tokens[2][0] == 'X') {
							labeltokens = Misc::tokenize_delimeter(tokens[2], "_");
							std::rotate(labeltokens.begin(), labeltokens.end() - 1, labeltokens.end());
							tokens[2] = labeltokens[0];
							for (int n = 1; n < labeltokens.size(); n++) {
								tokens[2] = tokens[2] + "|" + labeltokens[n];
							}
						}
						columnLabel1 = "C_NP" + tokens[2];
						if (std::find(iFile.matA.columnNames.begin(), iFile.matA.columnNames.end(), columnLabel1) != iFile.matA.columnNames.end()) {
							index1 = Misc::index_of(iFile.matA.columnNames, columnLabel1);
							traceLabel.push_back(label);
							traceData.push_back(iFile.xVect[index1]);
						}
						else {
							/* Error this node was not found and can therefore not be printed */
							Errors::plotting_errors(NO_SUCH_NODE_FOUND, tokens[2]);
						}
					}
					/* If first node is ground */
					else if (tokens[2] == "0" || tokens[3] == "GND") {
						label = "NODE PHASE " + tokens[3];
						if (tokens[3][0] == 'X') {
							labeltokens = Misc::tokenize_delimeter(tokens[3], "_");
							std::rotate(labeltokens.begin(), labeltokens.end() - 1, labeltokens.end());
							tokens[3] = labeltokens[0];
							for (int n = 1; n < labeltokens.size(); n++) {
								tokens[3] = tokens[3] + "|" + labeltokens[n];
							}
						}
						columnLabel1 = "C_NP" + tokens[3];
						if (std::find(iFile.matA.columnNames.begin(), iFile.matA.columnNames.end(), columnLabel1) != iFile.matA.columnNames.end()) {
							index1 = Misc::index_of(iFile.matA.columnNames, columnLabel1);
							trace.clear();
							trace = iFile.xVect[index1];
							std::fill(trace.begin(), trace.end(), 0.0);
							std::transform(trace.begin(), trace.end(), iFile.xVect[index1].begin(), trace.begin(), std::minus<double>());
							traceLabel.push_back(label);
							traceData.push_back(trace);
						}
						else {
							/* Error this node was not found and can therefore not be printed */
							Errors::plotting_errors(NO_SUCH_NODE_FOUND, tokens[3]);
						}
					}
					/* If neither are ground*/
					else {
						label = "NODE PHASE " + tokens[2] + " to " + tokens[3];
						columnLabel1 = "C_NP" + tokens[2];
						columnLabel2 = "C_NP" + tokens[3];
						if (tokens[2][0] == 'X') {
							labeltokens = Misc::tokenize_delimeter(tokens[2], "_");
							std::rotate(labeltokens.begin(), labeltokens.end() - 1, labeltokens.end());
							tokens[2] = labeltokens[0];
							for (int n = 1; n < labeltokens.size(); n++) {
								tokens[2] = tokens[2] + "|" + labeltokens[n];
							}
						}
						if (tokens[3][0] == 'X') {
							labeltokens = Misc::tokenize_delimeter(tokens[3], "_");
							std::rotate(labeltokens.begin(), labeltokens.end() - 1, labeltokens.end());
							tokens[3] = labeltokens[0];
							for (int n = 1; n < labeltokens.size(); n++) {
								tokens[3] = tokens[3] + "|" + labeltokens[n];
							}
						}
						if (std::find(iFile.matA.columnNames.begin(), iFile.matA.columnNames.end(), columnLabel1) != iFile.matA.columnNames.end()) {
							index1 = Misc::index_of(iFile.matA.columnNames, columnLabel1);
							trace.clear();
							trace = iFile.xVect[index1];
							if (std::find(iFile.matA.columnNames.begin(), iFile.matA.columnNames.end(), columnLabel2) != iFile.matA.columnNames.end()) {
								index2 = Misc::index_of(iFile.matA.columnNames, columnLabel2);
								std::transform(iFile.xVect[index1].begin(), iFile.xVect[index1].end(), iFile.xVect[index2].begin(), trace.begin(), std::minus<double>());
								traceLabel.push_back(label);
								traceData.push_back(trace);
							}
							else {
								/* Error this node was not found and can therefore not be printed */
								Errors::plotting_errors(NO_SUCH_NODE_FOUND, tokens[3]);
							}
						}
						else {
							/* Error this node was not found and can therefore not be printed */
							Errors::plotting_errors(NO_SUCH_NODE_FOUND, tokens[2]);
						}
					}
				}
				/* If only one node is specified */
				else {
					label = "NODE PHASE " + tokens[2];
					columnLabel1 = "C_NP" + tokens[2];
					if (std::find(iFile.matA.columnNames.begin(), iFile.matA.columnNames.end(), columnLabel1) != iFile.matA.columnNames.end()) {
						index1 = Misc::index_of(iFile.matA.columnNames, columnLabel1);
						traceLabel.push_back(label);
						traceData.push_back(iFile.xVect[index1]);
					}
					else {
						/* Error this node was not found and can therefore not be printed */
					}
				}
			}
			/* Complain about nodal voltage */
			/*****************************************************************************************************/
			else if (tokens[1] == "NODEV") {
				/* If more than one node is specified */
				if (tokens.size() == 4) {
					/* If second node is ground */
					if (tokens[3] == "0" || tokens[3] == "GND") {
						label = "NODE VOLTAGE " + tokens[2];
						if (tokens[2][0] == 'X') {
							labeltokens = Misc::tokenize_delimeter(tokens[2], "_");
							std::rotate(labeltokens.begin(), labeltokens.end() - 1, labeltokens.end());
							tokens[2] = labeltokens[0];
							for (int n = 1; n < labeltokens.size(); n++) {
								tokens[2] = tokens[2] + "|" + labeltokens[n];
							}
						}
						columnLabel1 = "C_NP" + tokens[2];
						if (std::find(iFile.matA.columnNames.begin(), iFile.matA.columnNames.end(), columnLabel1) != iFile.matA.columnNames.end()) {
							index1 = Misc::index_of(iFile.matA.columnNames, columnLabel1);
							traceLabel.push_back(label);
							trace.clear();
							vn1 = (PHI_ZERO) / (M_PI * iFile.tsim.prstep) * iFile.xVect[index1][0];
							trace.push_back(vn1);
							for (int i = 1; i < iFile.xVect[index1].size(); i++) {
								vn1 = (PHI_ZERO) / (M_PI * iFile.tsim.prstep) * (iFile.xVect[index1][i] -  iFile.xVect[index1][i-1]) - vn1;
								trace.push_back(vn1);
							}
							traceData.push_back(trace);
						}
						else {
							/* Error this node was not found and can therefore not be printed */
							Errors::plotting_errors(NO_SUCH_NODE_FOUND, tokens[2]);
						}
					}
					/* If first node is ground */
					else if (tokens[2] == "0" || tokens[3] == "GND") {
						label = "NODE VOLTAGE " + tokens[3];
						if (tokens[3][0] == 'X') {
							labeltokens = Misc::tokenize_delimeter(tokens[3], "_");
							std::rotate(labeltokens.begin(), labeltokens.end() - 1, labeltokens.end());
							tokens[3] = labeltokens[0];
							for (int n = 1; n < labeltokens.size(); n++) {
								tokens[3] = tokens[3] + "|" + labeltokens[n];
							}
						}
						columnLabel1 = "C_NP" + tokens[3];
						if (std::find(iFile.matA.columnNames.begin(), iFile.matA.columnNames.end(), columnLabel1) != iFile.matA.columnNames.end()) {
							index1 = Misc::index_of(iFile.matA.columnNames, columnLabel1);
							traceLabel.push_back(label);
							trace.clear();
							vn1 = (PHI_ZERO) / (M_PI * iFile.tsim.prstep) * iFile.xVect[index1][0];
							trace.push_back(-vn1);
							for (int i = 1; i < iFile.xVect[index1].size(); i++) {
								vn1 = (PHI_ZERO) / (M_PI * iFile.tsim.prstep) * (iFile.xVect[index1][i] -  iFile.xVect[index1][i-1]) - vn1;
								trace.push_back(-vn1);
							}
							traceData.push_back(trace);
						}
						else {
							/* Error this node was not found and can therefore not be printed */
							Errors::plotting_errors(NO_SUCH_NODE_FOUND, tokens[3]);
						}
					}
					/* If neither are ground*/
					else {
						label = "NODE VOLTAGE " + tokens[2] + " to " + tokens[3];
						columnLabel1 = "C_NP" + tokens[2];
						columnLabel2 = "C_NP" + tokens[3];
						if (tokens[2][0] == 'X') {
							labeltokens = Misc::tokenize_delimeter(tokens[2], "_");
							std::rotate(labeltokens.begin(), labeltokens.end() - 1, labeltokens.end());
							tokens[2] = labeltokens[0];
							for (int n = 1; n < labeltokens.size(); n++) {
								tokens[2] = tokens[2] + "|" + labeltokens[n];
							}
						}
						if (tokens[3][0] == 'X') {
							labeltokens = Misc::tokenize_delimeter(tokens[3], "_");
							std::rotate(labeltokens.begin(), labeltokens.end() - 1, labeltokens.end());
							tokens[3] = labeltokens[0];
							for (int n = 1; n < labeltokens.size(); n++) {
								tokens[3] = tokens[3] + "|" + labeltokens[n];
							}
						}
						if (std::find(iFile.matA.columnNames.begin(), iFile.matA.columnNames.end(), columnLabel1) != iFile.matA.columnNames.end()) {
							index1 = Misc::index_of(iFile.matA.columnNames, columnLabel1);
							trace.clear();
							if (std::find(iFile.matA.columnNames.begin(), iFile.matA.columnNames.end(), columnLabel2) != iFile.matA.columnNames.end()) {
								index2 = Misc::index_of(iFile.matA.columnNames, columnLabel2);
								traceLabel.push_back(label);
								trace.clear();
								vn1 = (PHI_ZERO) / (M_PI * iFile.tsim.prstep) * (iFile.xVect[index1][0] - iFile.xVect[index2][0]);
								trace.push_back(vn1);
								for (int i = 1; i < iFile.xVect[index1].size(); i++) {
									vn1 = (PHI_ZERO) / (M_PI * iFile.tsim.prstep) * ((iFile.xVect[index1][i] - iFile.xVect[index2][i]) -  (iFile.xVect[index1][i-1] - iFile.xVect[index2][i-1])) - vn1;
									trace.push_back(vn1);
								}
								traceData.push_back(trace);
								traceData.push_back(trace);
							}
							else {
								/* Error this node was not found and can therefore not be printed */
								Errors::plotting_errors(NO_SUCH_NODE_FOUND, tokens[3]);
							}
						}
						else {
							/* Error this node was not found and can therefore not be printed */
							Errors::plotting_errors(NO_SUCH_NODE_FOUND, tokens[2]);
						}
					}
				}
				/* If only one node is specified */
				else {
					if (tokens[2][0] == 'X') {
						labeltokens = Misc::tokenize_delimeter(tokens[2], "_");
						std::rotate(labeltokens.begin(), labeltokens.end() - 1, labeltokens.end());
						tokens[2] = labeltokens[0];
						for (int n = 1; n < labeltokens.size(); n++) {
							tokens[2] = tokens[2] + "|" + labeltokens[n];
						}
					}
					label = "NODE VOLTAGE " + tokens[2];
					columnLabel1 = "C_NP" + tokens[2];
					if (std::find(iFile.matA.columnNames.begin(), iFile.matA.columnNames.end(), columnLabel1) != iFile.matA.columnNames.end()) {
						index1 = Misc::index_of(iFile.matA.columnNames, columnLabel1);
						traceLabel.push_back(label);
						trace.clear();
							vn1 = (PHI_ZERO) / (M_PI * iFile.tsim.prstep) * iFile.xVect[index1][0];
							trace.push_back(vn1);
							for (int i = 1; i < iFile.xVect[index1].size(); i++) {
								vn1 = (PHI_ZERO) / (M_PI * iFile.tsim.prstep) * (iFile.xVect[index1][i] -  iFile.xVect[index1][i-1]) - vn1;
								trace.push_back(vn1);
							}
							traceData.push_back(trace);
					}
					else {
						/* Error this node was not found and can therefore not be printed */
					}
				}
			}
			/* Print the device phase */
			/*****************************************************************************************************/
			else if (tokens[1] == "PHASE") {
				label = "PHASE " + tokens[2];
				if (tokens[2][0] == 'X') {
					labeltokens = Misc::tokenize_delimeter(tokens[2], "_");
					std::rotate(labeltokens.begin(), labeltokens.end() - 1, labeltokens.end());
					tokens[2] = labeltokens[0];
					for (int n = 1; n < labeltokens.size(); n++) {
						tokens[2] = tokens[2] + "|" + labeltokens[n];
					}
				}
				switch(tokens[2][0]) {
					case 'B':
						if(iFile.phaseJJ.count(tokens[2]) != 0){
							trace.clear();
							if(iFile.phaseJJ[tokens[2]].posNRow == -1) {
								trace = iFile.xVect[iFile.phaseJJ[tokens[2]].negNRow];
								std::fill(trace.begin(), trace.end(), 0.0);
								std::transform(trace.begin(), trace.end(), iFile.xVect[iFile.phaseJJ[tokens[2]].negNRow].begin(), trace.begin(), std::minus<double>());
							}
							else if(iFile.phaseJJ[tokens[2]].negNRow == -1) trace = iFile.xVect[iFile.phaseJJ[tokens[2]].posNRow];
							else {
								trace = iFile.xVect[iFile.phaseJJ[tokens[2]].posNRow];
								std::transform(trace.begin(), trace.end(), iFile.xVect[iFile.phaseJJ[tokens[2]].negNRow].begin(), trace.begin(), std::minus<double>());
							}
							traceLabel.push_back(label);
							traceData.push_back(trace);
						}
						else {
							/* Error this device was not found and can therefore not be printed */
							Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, tokens[2]);
						}
						break;
					case 'L':
						if(iFile.phaseInd.count(tokens[2]) != 0){
							trace.clear();
							if(iFile.phaseInd[tokens[2]].posNRow == -1) {
								trace = iFile.xVect[iFile.phaseInd[tokens[2]].negNRow];
								std::fill(trace.begin(), trace.end(), 0.0);
								std::transform(trace.begin(), trace.end(), iFile.xVect[iFile.phaseInd[tokens[2]].negNRow].begin(), trace.begin(), std::minus<double>());
							}
							else if(iFile.phaseInd[tokens[2]].negNRow == -1) trace = iFile.xVect[iFile.phaseInd[tokens[2]].posNRow];
							else {
								trace = iFile.xVect[iFile.phaseInd[tokens[2]].posNRow];
								std::transform(trace.begin(), trace.end(), iFile.xVect[iFile.phaseInd[tokens[2]].negNRow].begin(), trace.begin(), std::minus<double>());
							}
							traceLabel.push_back(label);
							traceData.push_back(trace);
						}
						else {
							/* Error this device was not found and can therefore not be printed */
							Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, tokens[2]);
						}
						break;
					case 'R':
						if(iFile.phaseRes.count(tokens[2]) != 0){
							trace.clear();
							if(iFile.phaseRes[tokens[2]].posNRow == -1) {
								trace = iFile.xVect[iFile.phaseRes[tokens[2]].negNRow];
								std::fill(trace.begin(), trace.end(), 0.0);
								std::transform(trace.begin(), trace.end(), iFile.xVect[iFile.phaseRes[tokens[2]].negNRow].begin(), trace.begin(), std::minus<double>());
							}
							else if(iFile.phaseRes[tokens[2]].negNRow == -1) trace = iFile.xVect[iFile.phaseRes[tokens[2]].posNRow];
							else {
								trace = iFile.xVect[iFile.phaseRes[tokens[2]].posNRow];
								std::transform(trace.begin(), trace.end(), iFile.xVect[iFile.phaseRes[tokens[2]].negNRow].begin(), trace.begin(), std::minus<double>());
							}
							traceLabel.push_back(label);
							traceData.push_back(trace);
						}
						else {
							/* Error this device was not found and can therefore not be printed */
							Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, tokens[2]);
						}
						break;
					case 'C':
						if(iFile.phaseCap.count(tokens[2]) != 0){
							trace.clear();
							if(iFile.phaseCap[tokens[2]].posNRow == -1) {
								trace = iFile.xVect[iFile.phaseCap[tokens[2]].negNRow];
								std::fill(trace.begin(), trace.end(), 0.0);
								std::transform(trace.begin(), trace.end(), iFile.xVect[iFile.phaseCap[tokens[2]].negNRow].begin(), trace.begin(), std::minus<double>());
							}
							else if(iFile.phaseCap[tokens[2]].negNRow == -1) trace = iFile.xVect[iFile.phaseCap[tokens[2]].posNRow];
							else {
								trace = iFile.xVect[iFile.phaseCap[tokens[2]].posNRow];
								std::transform(trace.begin(), trace.end(), iFile.xVect[iFile.phaseCap[tokens[2]].negNRow].begin(), trace.begin(), std::minus<double>());
							}
							traceLabel.push_back(label);
							traceData.push_back(trace);
						}
						else {
							/* Error this device was not found and can therefore not be printed */
							Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, tokens[2]);
						}
						break;
					case 'V':
						if(iFile.phaseVs.count(tokens[2]) != 0){
							trace.clear();
							if(iFile.phaseVs[tokens[2]].posNRow == -1) {
								trace = iFile.xVect[iFile.phaseVs[tokens[2]].negNRow];
								std::fill(trace.begin(), trace.end(), 0.0);
								std::transform(trace.begin(), trace.end(), iFile.xVect[iFile.phaseVs[tokens[2]].negNRow].begin(), trace.begin(), std::minus<double>());
							}
							else if(iFile.phaseVs[tokens[2]].negNRow == -1) trace = iFile.xVect[iFile.phaseVs[tokens[2]].posNRow];
							else {
								trace = iFile.xVect[iFile.phaseVs[tokens[2]].posNRow];
								std::transform(trace.begin(), trace.end(), iFile.xVect[iFile.phaseVs[tokens[2]].negNRow].begin(), trace.begin(), std::minus<double>());
							}
							traceLabel.push_back(label);
							traceData.push_back(trace);
						}
						else {
							/* Error this device was not found and can therefore not be printed */
							Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, tokens[2]);
						}
						break;
					default:
						Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, tokens[2]);
						break;
				}
			}
			/* Complain about device voltage */
			/*****************************************************************************************************/
			else if (tokens[1] == "DEVV") {
				label = "DEVICE VOLTAGE " + tokens[2];
				if (tokens[2][0] == 'X') {
					labeltokens = Misc::tokenize_delimeter(tokens[2], "_");
					std::rotate(labeltokens.begin(), labeltokens.end() - 1, labeltokens.end());
					tokens[2] = labeltokens[0];
					for (int n = 1; n < labeltokens.size(); n++) {
						tokens[2] = tokens[2] + "|" + labeltokens[n];
					}
				}
				switch(tokens[2][0]) {
					case 'B':
						if(iFile.phaseJJ.count(tokens[2]) != 0){
							trace.clear();
							if(iFile.phaseJJ[tokens[2]].posNRow == -1) {
								vn1 = (PHI_ZERO) / (M_PI * iFile.tsim.prstep) * iFile.xVect[iFile.phaseJJ[tokens[2]].negNRow][0];
								trace.push_back(vn1);
								for (int i = 1; i < iFile.xVect[iFile.phaseJJ[tokens[2]].negNRow].size(); i++) {
									vn1 = (PHI_ZERO) / (M_PI * iFile.tsim.prstep) * (iFile.xVect[iFile.phaseJJ[tokens[2]].negNRow][i] -  iFile.xVect[iFile.phaseJJ[tokens[2]].negNRow][i-1]) - vn1;
									trace.push_back(vn1);
								}
							}
							else if(iFile.phaseJJ[tokens[2]].negNRow == -1) {
								vn1 = (PHI_ZERO) / (M_PI * iFile.tsim.prstep) * iFile.xVect[iFile.phaseJJ[tokens[2]].posNRow][0];
								trace.push_back(vn1);
								for (int i = 1; i < iFile.xVect[iFile.phaseJJ[tokens[2]].posNRow].size(); i++) {
									vn1 = (PHI_ZERO) / (M_PI * iFile.tsim.prstep) * (iFile.xVect[iFile.phaseJJ[tokens[2]].posNRow][i] -  iFile.xVect[iFile.phaseJJ[tokens[2]].posNRow][i-1]) - vn1;
									trace.push_back(vn1);
								}
							}
							else {
								vn1 = (PHI_ZERO) / (M_PI * iFile.tsim.prstep) * (iFile.xVect[iFile.phaseJJ[tokens[2]].posNRow][0] - iFile.xVect[iFile.phaseJJ[tokens[2]].negNRow][0]);
								trace.push_back(vn1);
								for (int i = 1; i < iFile.xVect[iFile.phaseJJ[tokens[2]].posNRow].size(); i++) {
									vn1 = (PHI_ZERO) / (M_PI * iFile.tsim.prstep) * ((iFile.xVect[iFile.phaseJJ[tokens[2]].posNRow][i] - iFile.xVect[iFile.phaseJJ[tokens[2]].negNRow][i]) -  (iFile.xVect[iFile.phaseJJ[tokens[2]].posNRow][i-1] - iFile.xVect[iFile.phaseJJ[tokens[2]].negNRow][i-1])) - vn1;
									trace.push_back(vn1);
								}
							}
							traceLabel.push_back(label);
							traceData.push_back(trace);
						}
						else {
							/* Error this device was not found and can therefore not be printed */
							Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, tokens[2]);
						}
						break;
					case 'L':
						if(iFile.phaseInd.count(tokens[2]) != 0){
							trace.clear();
							if(iFile.phaseInd[tokens[2]].posNRow == -1) {
								vn1 = (PHI_ZERO) / (M_PI * iFile.tsim.prstep) * iFile.xVect[iFile.phaseInd[tokens[2]].negNRow][0];
								trace.push_back(vn1);
								for (int i = 1; i < iFile.xVect[iFile.phaseInd[tokens[2]].negNRow].size(); i++) {
									vn1 = (PHI_ZERO) / (M_PI * iFile.tsim.prstep) * (iFile.xVect[iFile.phaseInd[tokens[2]].negNRow][i] -  iFile.xVect[iFile.phaseInd[tokens[2]].negNRow][i-1]) - vn1;
									trace.push_back(vn1);
								}
							}
							else if(iFile.phaseInd[tokens[2]].negNRow == -1) {
								vn1 = (PHI_ZERO) / (M_PI * iFile.tsim.prstep) * iFile.xVect[iFile.phaseInd[tokens[2]].posNRow][0];
								trace.push_back(vn1);
								for (int i = 1; i < iFile.xVect[iFile.phaseInd[tokens[2]].posNRow].size(); i++) {
									vn1 = (PHI_ZERO) / (M_PI * iFile.tsim.prstep) * (iFile.xVect[iFile.phaseInd[tokens[2]].posNRow][i] -  iFile.xVect[iFile.phaseInd[tokens[2]].posNRow][i-1]) - vn1;
									trace.push_back(vn1);
								}
							}
							else {
								vn1 = (PHI_ZERO) / (M_PI * iFile.tsim.prstep) * (iFile.xVect[iFile.phaseInd[tokens[2]].posNRow][0] - iFile.xVect[iFile.phaseInd[tokens[2]].negNRow][0]);
								trace.push_back(vn1);
								for (int i = 1; i < iFile.xVect[iFile.phaseInd[tokens[2]].posNRow].size(); i++) {
									vn1 = (PHI_ZERO) / (M_PI * iFile.tsim.prstep) * ((iFile.xVect[iFile.phaseInd[tokens[2]].posNRow][i] - iFile.xVect[iFile.phaseInd[tokens[2]].negNRow][i]) -  (iFile.xVect[iFile.phaseInd[tokens[2]].posNRow][i-1] - iFile.xVect[iFile.phaseInd[tokens[2]].negNRow][i-1])) - vn1;
									trace.push_back(vn1);
								}
							}
							traceLabel.push_back(label);
							traceData.push_back(trace);
						}
						else {
							/* Error this device was not found and can therefore not be printed */
							Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, tokens[2]);
						}
						break;
					case 'R':
						if(iFile.phaseRes.count(tokens[2]) != 0){
							trace.clear();
							if(iFile.phaseRes[tokens[2]].posNRow == -1) {
								vn1 = (PHI_ZERO) / (M_PI * iFile.tsim.prstep) * iFile.xVect[iFile.phaseRes[tokens[2]].negNRow][0];
								trace.push_back(vn1);
								for (int i = 1; i < iFile.xVect[iFile.phaseRes[tokens[2]].negNRow].size(); i++) {
									vn1 = (PHI_ZERO) / (M_PI * iFile.tsim.prstep) * (iFile.xVect[iFile.phaseRes[tokens[2]].negNRow][i] -  iFile.xVect[iFile.phaseRes[tokens[2]].negNRow][i-1]) - vn1;
									trace.push_back(vn1);
								}
							}
							else if(iFile.phaseRes[tokens[2]].negNRow == -1) {
								vn1 = (PHI_ZERO) / (M_PI * iFile.tsim.prstep) * iFile.xVect[iFile.phaseRes[tokens[2]].posNRow][0];
								trace.push_back(vn1);
								for (int i = 1; i < iFile.xVect[iFile.phaseRes[tokens[2]].posNRow].size(); i++) {
									vn1 = (PHI_ZERO) / (M_PI * iFile.tsim.prstep) * (iFile.xVect[iFile.phaseRes[tokens[2]].posNRow][i] -  iFile.xVect[iFile.phaseRes[tokens[2]].posNRow][i-1]) - vn1;
									trace.push_back(vn1);
								}
							}
							else {
								vn1 = (PHI_ZERO) / (M_PI * iFile.tsim.prstep) * (iFile.xVect[iFile.phaseRes[tokens[2]].posNRow][0] - iFile.xVect[iFile.phaseRes[tokens[2]].negNRow][0]);
								trace.push_back(vn1);
								for (int i = 1; i < iFile.xVect[iFile.phaseRes[tokens[2]].posNRow].size(); i++) {
									vn1 = (PHI_ZERO) / (M_PI * iFile.tsim.prstep) * ((iFile.xVect[iFile.phaseRes[tokens[2]].posNRow][i] - iFile.xVect[iFile.phaseRes[tokens[2]].negNRow][i]) -  (iFile.xVect[iFile.phaseRes[tokens[2]].posNRow][i-1] - iFile.xVect[iFile.phaseRes[tokens[2]].negNRow][i-1])) - vn1;
									trace.push_back(vn1);
								}
							}
							traceLabel.push_back(label);
							traceData.push_back(trace);
						}
						else {
							/* Error this device was not found and can therefore not be printed */
							Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, tokens[2]);
						}
						break;
					case 'C':
						if(iFile.phaseCap.count(tokens[2]) != 0){
							trace.clear();
							if(iFile.phaseCap[tokens[2]].posNRow == -1) {
								vn1 = (PHI_ZERO) / (M_PI * iFile.tsim.prstep) * iFile.xVect[iFile.phaseCap[tokens[2]].negNRow][0];
								trace.push_back(vn1);
								for (int i = 1; i < iFile.xVect[iFile.phaseCap[tokens[2]].negNRow].size(); i++) {
									vn1 = (PHI_ZERO) / (M_PI * iFile.tsim.prstep) * (iFile.xVect[iFile.phaseCap[tokens[2]].negNRow][i] -  iFile.xVect[iFile.phaseCap[tokens[2]].negNRow][i-1]) - vn1;
									trace.push_back(vn1);
								}
							}
							else if(iFile.phaseCap[tokens[2]].negNRow == -1) {
								vn1 = (PHI_ZERO) / (M_PI * iFile.tsim.prstep) * iFile.xVect[iFile.phaseCap[tokens[2]].posNRow][0];
								trace.push_back(vn1);
								for (int i = 1; i < iFile.xVect[iFile.phaseCap[tokens[2]].posNRow].size(); i++) {
									vn1 = (PHI_ZERO) / (M_PI * iFile.tsim.prstep) * (iFile.xVect[iFile.phaseCap[tokens[2]].posNRow][i] -  iFile.xVect[iFile.phaseCap[tokens[2]].posNRow][i-1]) - vn1;
									trace.push_back(vn1);
								}
							}
							else {
								vn1 = (PHI_ZERO) / (M_PI * iFile.tsim.prstep) * (iFile.xVect[iFile.phaseCap[tokens[2]].posNRow][0] - iFile.xVect[iFile.phaseCap[tokens[2]].negNRow][0]);
								trace.push_back(vn1);
								for (int i = 1; i < iFile.xVect[iFile.phaseCap[tokens[2]].posNRow].size(); i++) {
									vn1 = (PHI_ZERO) / (M_PI * iFile.tsim.prstep) * ((iFile.xVect[iFile.phaseCap[tokens[2]].posNRow][i] - iFile.xVect[iFile.phaseCap[tokens[2]].negNRow][i]) -  (iFile.xVect[iFile.phaseCap[tokens[2]].posNRow][i-1] - iFile.xVect[iFile.phaseCap[tokens[2]].negNRow][i-1])) - vn1;
									trace.push_back(vn1);
								}
							}
							traceLabel.push_back(label);
							traceData.push_back(trace);
						}
						else {
							/* Error this device was not found and can therefore not be printed */
							Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, tokens[2]);
						}
						break;
					case 'V':
						if(iFile.matA.sources.count(tokens[2]) != 0) {
							trace.clear();
							trace = iFile.matA.sources[tokens[2]];
							traceLabel.push_back(label);
							traceData.push_back(trace);
						}
						else {
							/* Error this device was not found and can therefore not be printed */
							Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, tokens[2]);
						}
						break;
					default:
						Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, tokens[2]);
						break;
				
				}
			}
			/* Print the identified device current */
			/*****************************************************************************************************/
			else if (tokens[1] == "DEVI") {
				label = "DEVICE CURRENT " + tokens[2];
				if (tokens[2][0] == 'X') {
					labeltokens = Misc::tokenize_delimeter(tokens[2], "_");
					std::rotate(labeltokens.begin(), labeltokens.end() - 1, labeltokens.end());
					tokens[2] = labeltokens[0];
					for (int n = 1; n < labeltokens.size(); n++) {
						tokens[2] = tokens[2] + "|" + labeltokens[n];
					}
				}
				switch(tokens[2][0]) {
					case 'B':
						if(iFile.phaseJJ.count(tokens[2]) != 0){
							trace.clear();
							std::transform(trace.begin(), trace.end(), iFile.phaseJJ[tokens[2]].jjCur.begin(), trace.begin(), std::plus<double>());
							traceLabel.push_back(label);
							traceData.push_back(trace);
						}
						else {
							/* Error this device was not found and can therefore not be printed */
							Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, tokens[2]);
						}
						break;
					case 'L':
						if(iFile.phaseInd.count(tokens[2]) != 0){
							trace.clear();
							trace = iFile.xVect[iFile.phaseInd[tokens[2]].curNRow];
							traceLabel.push_back(label);
							traceData.push_back(trace);
						}
						else {
							/* Error this device was not found and can therefore not be printed */
							Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, tokens[2]);
						}
						break;
					case 'R':
						if(iFile.phaseRes.count(tokens[2]) != 0){
							trace.clear();
							trace = iFile.xVect[iFile.phaseRes[tokens[2]].curNRow];
							traceLabel.push_back(label);
							traceData.push_back(trace);
						}
						else {
							/* Error this device was not found and can therefore not be printed */
							Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, tokens[2]);
						}
						break;
					case 'C':
						if(iFile.phaseCap.count(tokens[2]) != 0){
							trace.clear();
							trace = iFile.xVect[iFile.phaseCap[tokens[2]].curNRow];
							traceLabel.push_back(label);
							traceData.push_back(trace);
						}
						else {
							/* Error this device was not found and can therefore not be printed */
							Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, tokens[2]);
						}
						break;
					case 'V':
						if(iFile.phaseVs.count(tokens[2]) != 0){
							trace.clear();
							trace = iFile.xVect[iFile.phaseVs[tokens[2]].curNRow];
							traceLabel.push_back(label);
							traceData.push_back(trace);
						}
						else {
							/* Error this device was not found and can therefore not be printed */
							Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, tokens[2]);
						}
						break;
					case 'I':
						if(iFile.matA.sources.count(tokens[2]) != 0) {
							trace.clear();
							trace = iFile.matA.sources[tokens[2]];
							traceLabel.push_back(label);
							traceData.push_back(trace);
						}
						else {
							/* Error this device was not found and can therefore not be printed */
							Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, tokens[2]);
						}
						break;
					default:
						Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, tokens[2]);
						break;
				}
			}
			/* No such print command error thrown */
			else {
				Errors::plotting_errors(NO_SUCH_PLOT_TYPE, tokens[1]);
			}
		}
		/****************************************************/
		/*						PLOT						*/
		/****************************************************/
		else if (string.find("PLOT") != std::string::npos) {
			tokens = Misc::tokenize_space(string);
			for (int k = 1; k < tokens.size(); k++) {
				/* If plotting voltage */
				if (tokens[k][0] == 'V') {
					/* Identify part between brackets */
					nodesToPlot = tokens[k].substr(2);
					nodesToPlot = nodesToPlot.substr(0, nodesToPlot.size() - 1);
					/* If multiple arguments are specified for V */
					if (nodesToPlot.find(',') != std::string::npos) {
						nodesTokens = Misc::tokenize_delimeter(nodesToPlot, ",");
						if (nodesTokens.size() > 2) {
							Errors::plotting_errors(TOO_MANY_NODES, string);
						}
						/* Ensure node 1 is not ground */
						if (nodesTokens[0] == "0" || nodesTokens[0] == "GND") {
							if (nodesTokens[1] == "0" || nodesTokens[1] == "GND") {
								Errors::plotting_errors(BOTH_ZERO, string);
							}
							else {
								if (nodesTokens[1][0] == 'X') {
									labeltokens = Misc::tokenize_delimeter(tokens[1], "_");
									std::rotate(labeltokens.begin(), labeltokens.end() - 1, labeltokens.end());
									nodesTokens[1] = labeltokens[0];
									for (int n = 1; n < labeltokens.size(); n++) {
										nodesTokens[1] = nodesTokens[1] + "|" + labeltokens[n];
									}
								}
								columnLabel1 = "C_NP" + nodesTokens[1];
								/* If this is a node voltage */
								if (std::find(iFile.matA.columnNames.begin(), iFile.matA.columnNames.end(), columnLabel1) != iFile.matA.columnNames.end()) {
									index1 = Misc::index_of(iFile.matA.columnNames, columnLabel1);
									traceLabel.push_back(label);
									trace.clear();
									vn1 = (PHI_ZERO) / (M_PI * iFile.tsim.prstep) * iFile.xVect[index1][0];
									trace.push_back(-vn1);
									for (int i = 1; i < iFile.xVect[index1].size(); i++) {
										vn1 = (PHI_ZERO) / (M_PI * iFile.tsim.prstep) * (iFile.xVect[index1][i] -  iFile.xVect[index1][i-1]) - vn1;
										trace.push_back(-vn1);
									}
									traceData.push_back(trace);
								}
								/* Else node not found */
								else {
									Errors::plotting_errors(NO_SUCH_NODE_FOUND, string);
								}
							}
						}
						/* Check if node 2 is ground */
						else if (tokens[1] == "0" || tokens[1] == "GND") {
							if (tokens[0][0] == 'X') {
								labeltokens = Misc::tokenize_delimeter(tokens[0], "_");
								std::rotate(labeltokens.begin(), labeltokens.end() - 1, labeltokens.end());
								tokens[0] = labeltokens[0];
								for (int n = 1; n < labeltokens.size(); n++) {
									tokens[0] = tokens[0] + "|" + labeltokens[n];
								}
							}
							columnLabel1 = "C_NP" + tokens[0];
							if (std::find(iFile.matA.columnNames.begin(), iFile.matA.columnNames.end(), columnLabel1) != iFile.matA.columnNames.end()) {
								index1 = Misc::index_of(iFile.matA.columnNames, columnLabel1);
								traceLabel.push_back(label);
								trace.clear();
								vn1 = (PHI_ZERO) / (M_PI * iFile.tsim.prstep) * iFile.xVect[index1][0];
								trace.push_back(vn1);
								for (int i = 1; i < iFile.xVect[index1].size(); i++) {
									vn1 = (PHI_ZERO) / (M_PI * iFile.tsim.prstep) * (iFile.xVect[index1][i] -  iFile.xVect[index1][i-1]) - vn1;
									trace.push_back(vn1);
								}
								traceData.push_back(trace);
							}
							else {
								Errors::plotting_errors(NO_SUCH_NODE_FOUND, string);
							}
						}
						/* Neither nodes are ground */
						else {
							label = "NODE VOLTAGE " + nodesTokens[0] + " to " + nodesTokens[1];
							columnLabel1 = "C_NP" + nodesTokens[0];
							columnLabel2 = "C_NP" + nodesTokens[1];
							if (nodesTokens[0][0] == 'X') {
								labeltokens = Misc::tokenize_delimeter(nodesTokens[0], "_");
								std::rotate(labeltokens.begin(), labeltokens.end() - 1, labeltokens.end());
								nodesTokens[0] = labeltokens[0];
								for (int n = 1; n < labeltokens.size(); n++) {
									nodesTokens[0] = nodesTokens[0] + "|" + labeltokens[n];
								}
							}
							if (nodesTokens[1][0] == 'X') {
								labeltokens = Misc::tokenize_delimeter(nodesTokens[1], "_");
								std::rotate(labeltokens.begin(), labeltokens.end() - 1, labeltokens.end());
								nodesTokens[1] = labeltokens[0];
								for (int n = 1; n < labeltokens.size(); n++) {
									nodesTokens[1] = nodesTokens[1] + "|" + labeltokens[n];
								}
							}
							if (std::find(iFile.matA.columnNames.begin(), iFile.matA.columnNames.end(), columnLabel1) != iFile.matA.columnNames.end()) {
								index1 = Misc::index_of(iFile.matA.columnNames, columnLabel1);
								trace.clear();
								if (std::find(iFile.matA.columnNames.begin(), iFile.matA.columnNames.end(), columnLabel2) != iFile.matA.columnNames.end()) {
									index2 = Misc::index_of(iFile.matA.columnNames, columnLabel2);
									traceLabel.push_back(label);
									trace.clear();
									vn1 = (PHI_ZERO) / (M_PI * iFile.tsim.prstep) * (iFile.xVect[index1][0] - iFile.xVect[index2][0]);
									trace.push_back(vn1);
									for (int i = 1; i < iFile.xVect[index1].size(); i++) {
										vn1 = (PHI_ZERO) / (M_PI * iFile.tsim.prstep) * ((iFile.xVect[index1][i] - iFile.xVect[index2][i]) -  (iFile.xVect[index1][i-1] - iFile.xVect[index2][i-1])) - vn1;
										trace.push_back(vn1);
									}
									traceData.push_back(trace);
								}
								else {
									/* Error this node was not found and can therefore not be printed */
									Errors::plotting_errors(NO_SUCH_NODE_FOUND, tokens[3]);
								}
							}
						}
					}
					/* If only one argument is specified for V */
					else {
						if (nodesToPlot[0] == 'X') {
							labeltokens = Misc::tokenize_delimeter(nodesToPlot, "_");
							std::rotate(labeltokens.begin(), labeltokens.end() - 1, labeltokens.end());
							nodesToPlot = labeltokens[0];
							for (int n = 1; n < labeltokens.size(); n++) {
								nodesToPlot = nodesToPlot + "|" + labeltokens[n];
							}
						}
						label = "NODE VOLTAGE " + nodesToPlot;
						columnLabel1 = "C_NP" + nodesToPlot;
						if (std::find(iFile.matA.columnNames.begin(), iFile.matA.columnNames.end(), columnLabel1) != iFile.matA.columnNames.end()) {
							index1 = Misc::index_of(iFile.matA.columnNames, columnLabel1);
							traceLabel.push_back(label);
							trace.clear();
								vn1 = (PHI_ZERO) / (M_PI * iFile.tsim.prstep) * iFile.xVect[index1][0];
								trace.push_back(vn1);
								for (int i = 1; i < iFile.xVect[index1].size(); i++) {
									vn1 = (PHI_ZERO) / (M_PI * iFile.tsim.prstep) * (iFile.xVect[index1][i] -  iFile.xVect[index1][i-1]) - vn1;
									trace.push_back(vn1);
								}
								traceData.push_back(trace);
						}
						else {
							/* Error this node was not found and can therefore not be printed */
						}
					}
				}
				else if (tokens[k][0] == 'I') {
					/* Identify part between brackets */
					nodesToPlot = tokens[k].substr(2);
					nodesToPlot = nodesToPlot.substr(0, nodesToPlot.size() - 1);
					label = "DEVICE CURRENT " + nodesToPlot;
					if (nodesToPlot[0] == 'X') {
						labeltokens = Misc::tokenize_delimeter(nodesToPlot, "_");
						std::rotate(labeltokens.begin(), labeltokens.end() - 1, labeltokens.end());
						nodesToPlot = labeltokens[0];
						for (int n = 1; n < labeltokens.size(); n++) {
							nodesToPlot = nodesToPlot + "|" + labeltokens[n];
						}
					}
					switch(nodesToPlot[0]) {
					case 'B':
						if(iFile.phaseJJ.count(nodesToPlot) != 0){
							trace.clear();
							std::transform(trace.begin(), trace.end(), iFile.phaseJJ[nodesToPlot].jjCur.begin(), trace.begin(), std::plus<double>());
							traceLabel.push_back(label);
							traceData.push_back(trace);
						}
						else {
							/* Error this device was not found and can therefore not be printed */
							Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, nodesToPlot);
						}
						break;
					case 'L':
						if(iFile.phaseInd.count(nodesToPlot) != 0){
							trace.clear();
							trace = iFile.xVect[iFile.phaseInd[nodesToPlot].curNRow];
							traceLabel.push_back(label);
							traceData.push_back(trace);
						}
						else {
							/* Error this device was not found and can therefore not be printed */
							Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, nodesToPlot);
						}
						break;
					case 'R':
						if(iFile.phaseRes.count(nodesToPlot) != 0){
							trace.clear();
							trace = iFile.xVect[iFile.phaseRes[nodesToPlot].curNRow];
							traceLabel.push_back(label);
							traceData.push_back(trace);
						}
						else {
							/* Error this device was not found and can therefore not be printed */
							Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, nodesToPlot);
						}
						break;
					case 'C':
						if(iFile.phaseCap.count(nodesToPlot) != 0){
							trace.clear();
							trace = iFile.xVect[iFile.phaseCap[nodesToPlot].curNRow];
							traceLabel.push_back(label);
							traceData.push_back(trace);
						}
						else {
							/* Error this device was not found and can therefore not be printed */
							Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, nodesToPlot);
						}
						break;
					case 'V':
						if(iFile.phaseVs.count(nodesToPlot) != 0){
							trace.clear();
							trace = iFile.xVect[iFile.phaseVs[nodesToPlot].curNRow];
							traceLabel.push_back(label);
							traceData.push_back(trace);
						}
						else {
							/* Error this device was not found and can therefore not be printed */
							Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, nodesToPlot);
						}
						break;
					case 'I':
						if(iFile.matA.sources.count(nodesToPlot) != 0) {
							trace.clear();
							trace = iFile.matA.sources[nodesToPlot];
							traceLabel.push_back(label);
							traceData.push_back(trace);
						}
						else {
							/* Error this device was not found and can therefore not be printed */
							Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, nodesToPlot);
						}
						break;
					default:
						Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, nodesToPlot);
						break;
					}
				}
				else if (tokens[k][0] == 'P') {
					/* Identify part between brackets */
					nodesToPlot = tokens[k].substr(2);
					nodesToPlot = nodesToPlot.substr(0, nodesToPlot.size() - 1);
					label = "PHASE " + nodesToPlot;
					if (nodesToPlot[0] == 'X') {
						labeltokens = Misc::tokenize_delimeter(nodesToPlot, "_");
						std::rotate(labeltokens.begin(), labeltokens.end() - 1, labeltokens.end());
						nodesToPlot = labeltokens[0];
						for (int n = 1; n < labeltokens.size(); n++) {
							nodesToPlot = nodesToPlot + "|" + labeltokens[n];
						}
					}
					switch(nodesToPlot[0]) {
						case 'B':
							if(iFile.phaseJJ.count(nodesToPlot) != 0){
								trace.clear();
								if(iFile.phaseJJ[nodesToPlot].posNRow == -1) {
									trace = iFile.xVect[iFile.phaseJJ[nodesToPlot].negNRow];
									std::fill(trace.begin(), trace.end(), 0.0);
									std::transform(trace.begin(), trace.end(), iFile.xVect[iFile.phaseJJ[nodesToPlot].negNRow].begin(), trace.begin(), std::minus<double>());
								}
								else if(iFile.phaseJJ[nodesToPlot].negNRow == -1) trace = iFile.xVect[iFile.phaseJJ[nodesToPlot].posNRow];
								else {
									trace = iFile.xVect[iFile.phaseJJ[nodesToPlot].posNRow];
									std::transform(trace.begin(), trace.end(), iFile.xVect[iFile.phaseJJ[nodesToPlot].negNRow].begin(), trace.begin(), std::minus<double>());
								}
								traceLabel.push_back(label);
								traceData.push_back(trace);
							}
							else {
								/* Error this device was not found and can therefore not be printed */
								Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, nodesToPlot);
							}
							break;
						case 'L':
							if(iFile.phaseInd.count(nodesToPlot) != 0){
								trace.clear();
								if(iFile.phaseInd[nodesToPlot].posNRow == -1) {
									trace = iFile.xVect[iFile.phaseInd[nodesToPlot].negNRow];
									std::fill(trace.begin(), trace.end(), 0.0);
									std::transform(trace.begin(), trace.end(), iFile.xVect[iFile.phaseInd[nodesToPlot].negNRow].begin(), trace.begin(), std::minus<double>());
								}
								else if(iFile.phaseInd[nodesToPlot].negNRow == -1) trace = iFile.xVect[iFile.phaseInd[nodesToPlot].posNRow];
								else {
									trace = iFile.xVect[iFile.phaseInd[nodesToPlot].posNRow];
									std::transform(trace.begin(), trace.end(), iFile.xVect[iFile.phaseInd[nodesToPlot].negNRow].begin(), trace.begin(), std::minus<double>());
								}
								traceLabel.push_back(label);
								traceData.push_back(trace);
							}
							else {
								/* Error this device was not found and can therefore not be printed */
								Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, nodesToPlot);
							}
							break;
						case 'R':
							if(iFile.phaseRes.count(nodesToPlot) != 0){
								trace.clear();
								if(iFile.phaseRes[nodesToPlot].posNRow == -1) {
									trace = iFile.xVect[iFile.phaseRes[nodesToPlot].negNRow];
									std::fill(trace.begin(), trace.end(), 0.0);
									std::transform(trace.begin(), trace.end(), iFile.xVect[iFile.phaseRes[nodesToPlot].negNRow].begin(), trace.begin(), std::minus<double>());
								}
								else if(iFile.phaseRes[nodesToPlot].negNRow == -1) trace = iFile.xVect[iFile.phaseRes[nodesToPlot].posNRow];
								else {
									trace = iFile.xVect[iFile.phaseRes[nodesToPlot].posNRow];
									std::transform(trace.begin(), trace.end(), iFile.xVect[iFile.phaseRes[nodesToPlot].negNRow].begin(), trace.begin(), std::minus<double>());
								}
								traceLabel.push_back(label);
								traceData.push_back(trace);
							}
							else {
								/* Error this device was not found and can therefore not be printed */
								Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, nodesToPlot);
							}
							break;
						case 'C':
							if(iFile.phaseCap.count(nodesToPlot) != 0){
								trace.clear();
								if(iFile.phaseCap[nodesToPlot].posNRow == -1) {
									trace = iFile.xVect[iFile.phaseCap[nodesToPlot].negNRow];
									std::fill(trace.begin(), trace.end(), 0.0);
									std::transform(trace.begin(), trace.end(), iFile.xVect[iFile.phaseCap[nodesToPlot].negNRow].begin(), trace.begin(), std::minus<double>());
								}
								else if(iFile.phaseCap[nodesToPlot].negNRow == -1) trace = iFile.xVect[iFile.phaseCap[nodesToPlot].posNRow];
								else {
									trace = iFile.xVect[iFile.phaseCap[nodesToPlot].posNRow];
									std::transform(trace.begin(), trace.end(), iFile.xVect[iFile.phaseCap[nodesToPlot].negNRow].begin(), trace.begin(), std::minus<double>());
								}
								traceLabel.push_back(label);
								traceData.push_back(trace);
							}
							else {
								/* Error this device was not found and can therefore not be printed */
								Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, nodesToPlot);
							}
							break;
						case 'V':
							if(iFile.phaseVs.count(nodesToPlot) != 0){
								trace.clear();
								if(iFile.phaseVs[nodesToPlot].posNRow == -1) {
									trace = iFile.xVect[iFile.phaseVs[nodesToPlot].negNRow];
									std::fill(trace.begin(), trace.end(), 0.0);
									std::transform(trace.begin(), trace.end(), iFile.xVect[iFile.phaseVs[nodesToPlot].negNRow].begin(), trace.begin(), std::minus<double>());
								}
								else if(iFile.phaseVs[nodesToPlot].negNRow == -1) trace = iFile.xVect[iFile.phaseVs[nodesToPlot].posNRow];
								else {
									trace = iFile.xVect[iFile.phaseVs[nodesToPlot].posNRow];
									std::transform(trace.begin(), trace.end(), iFile.xVect[iFile.phaseVs[nodesToPlot].negNRow].begin(), trace.begin(), std::minus<double>());
								}
								traceLabel.push_back(label);
								traceData.push_back(trace);
							}
							else {
								/* Error this device was not found and can therefore not be printed */
								Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, nodesToPlot);
							}
							break;
						default:
							Errors::plotting_errors(NO_SUCH_DEVICE_FOUND, nodesToPlot);
							break;
					}	
				}
			}
		}
		/****************************************************/
		/*						SAVE						*/
		/****************************************************/
		else if (string.find("SAVE") != std::string::npos) {
			tokens = Misc::tokenize_space(string);
			for (int k = 1; k < tokens.size(); k++) {
				index1 = tokens[k].find("@");
				if (index1 != std::string::npos) tokens[k] = tokens[k].substr(0, index1) + tokens[k].substr(index1 + 1);
				index1 = tokens[k].find(".at(");
				if (index1 != std::string::npos) tokens[k] = tokens[k].substr(0, index1);
				index1 = tokens[k].find(".");
				if (index1 != std::string::npos) {
					tokens[k] = tokens[k].substr(0, index1) + "|" + tokens[k].substr(index1 + 1);
				}
				index1 = tokens[k].find("[");
				if (index1 != std::string::npos) tokens[k] = tokens[k].substr(0, index1);
				/* If this is a current source */
				if (iFile.matA.sources.find(tokens[k]) != iFile.matA.sources.end()) {
					label = "CURRENT " + tokens[k];
					traceLabel.push_back(label);
					traceData.push_back(iFile.matA.sources.at(tokens[k]));
				}
			}
		}
	}
}
/*
	Function that creates a plotting window with all available traces to plot
*/
int Plot::plot_all_traces(InputFile& iFile) {
	#ifdef USING_FLTK
		Fl_Window * win = new Fl_Window(1240, 768);
		Fl_Scroll * scroll = new Fl_Scroll(0, 0, win->w(), win->h());
		std::vector<Fl_Chart *> Charts;
		std::string label;
		int counter = 0;
		for (auto i : iFile.matA.columnNames) {
			label = Misc::substring_after(i, "C_");
			Charts.push_back(new Fl_Chart(20, 20 + (counter * (scroll->h() / 3)), scroll->w() - 40, (scroll->h() / 3 - 20)));
			Charts[counter]->type(FL_LINE_CHART);
			for (int j = 0; j < iFile.xVect[counter].size(); j++) {
				Charts[counter]->add(iFile.xVect[counter][j]);
			}
			Charts[counter]->color(FL_WHITE);
			Charts[counter]->align(FL_ALIGN_INSIDE | FL_ALIGN_CENTER | FL_ALIGN_TOP);
			Charts[counter]->copy_label(label.c_str());
			counter++;
		}
		win->resizable(scroll);
		win->label(INPUT_FILE.c_str());
		win->show();
		return(Fl::run());
	#elif USING_MATPLOTLIB
		int counter = 0;
		if (iFile.matA.columnNames.size() <= 3) {
			plt::figure();
			//plt::figure_size(800, 600);
			for (auto i : iFile.matA.columnNames) {
				plt::subplot(iFile.matA.columnNames.size(), 1, counter + 1);
				plt::grid(true);
				plt::plot(iFile.timeAxis, iFile.xVect[counter]);
				plt::title(Misc::substring_after(i, "C_"));
				if (Misc::substring_after(i, "C_")[0] == 'N'){
					if (Misc::substring_after(i, "C_")[1] == 'P') plt::ylabel("Phase (rads)");
					else if (Misc::substring_after(i, "C_")[1] == 'V') plt::ylabel("Voltage (V)");
				}
				else if (Misc::substring_after(i, "C_")[0] == 'I') plt::ylabel("Current (A)");
				else if (Misc::substring_after(i, "C_")[0] == 'P') plt::ylabel("Phase (rads)");
				counter++;
			}
			plt::xlabel("Time (s)");
			plt::tight_layout();
			plt::show();
		}
		else {
			for (int j = 0; j < iFile.matA.columnNames.size(); j = j + 3) {
				counter = j;
				//plt::figure_size(800, 600);
				plt::figure();
				while ((counter < iFile.matA.columnNames.size()) && (counter < j + 3)) {
					plt::subplot(3, 1, (counter - j) + 1);
					plt::grid(true);
					plt::plot(iFile.timeAxis, iFile.xVect[counter]);
					plt::title(Misc::substring_after(iFile.matA.columnNames[counter], "C_"));
					if (Misc::substring_after(iFile.matA.columnNames[counter], "C_")[0] == 'N'){
						if (Misc::substring_after(iFile.matA.columnNames[counter], "C_")[1] == 'V') plt::ylabel("Voltage (V)");
						else if (Misc::substring_after(iFile.matA.columnNames[counter], "C_")[1] == 'P') plt::ylabel("Phase (rads)");
					}
					else if (Misc::substring_after(iFile.matA.columnNames[counter], "C_")[0] == 'I') plt::ylabel("Current (A)");
					else if (Misc::substring_after(iFile.matA.columnNames[counter], "C_")[0] == 'P') plt::ylabel("Phase (rads)");
					counter++;
				}
				plt::xlabel("Time (s)");
				plt::tight_layout();
				plt::show(false);
			}
			plt::show();
		}
		return 0;
	#endif
	return 0;
}
/*
	Function that creates a plotting window only for the specified plots in the simulation
*/
int Plot::plot_traces(InputFile& iFile) {
	#ifdef USING_FLTK
		std::vector<std::string> traceLabel;
		std::vector<std::vector<double>> traceData;
		if(analysis == VANALYSIS) traces_to_plot(iFile, iFile.controlPart, traceLabel, traceData);
		else if(analysis == PANALYSIS) phase_traces_to_plot(iFile, iFile.controlPart, traceLabel, traceData);
		Fl_Window * win = new Fl_Window(1240, 768);
		Fl_Scroll * scroll = new Fl_Scroll(0, 0, win->w(), win->h());
		std::vector<Fl_Chart *> Charts;
		if (traceLabel.size() > 0) {
			for (int i = 0; i < traceLabel.size(); i++) {
				Charts.push_back(new Fl_Chart(20, 20 + (i * (scroll->h() / 3)), scroll->w() - 40, (scroll->h() / 3 - 20)));
				Charts[i]->type(FL_LINE_CHART);
				for (int j = 0; j < traceData[i].size(); j++) {
					Charts[i]->add(traceData[i][j]);
				}
				Charts[i]->color(FL_WHITE);
				Charts[i]->align(FL_ALIGN_INSIDE | FL_ALIGN_CENTER | FL_ALIGN_TOP);
				Charts[i]->copy_label(traceLabel[i].c_str());
			}
		}
		else if (traceLabel.size() == 0) {
			std::cout << "W: Plotting requested but no plot/print/save commands found." << std::endl;
			std::cout << "W: Plotting all the node voltages by default." << std::endl;
			int j = 0;
			std::string label;
			for (int i = 0; i < iFile.matA.columnNames.size(); i++) {
				label = Misc::substring_after(iFile.matA.columnNames[i], "C_");
				if (label[0] == 'N') {
					Charts.push_back(new Fl_Chart(20, 20 + (j * (scroll->h() / 3)), scroll->w() - 40, (scroll->h() / 3 - 20)));
					Charts[j]->type(FL_LINE_CHART);
					for (int k = 0; k < iFile.xVect[i].size(); k++) {
						Charts[j]->add(iFile.xVect[i][k]);
					}
					Charts[j]->color(FL_WHITE);
					Charts[j]->align(FL_ALIGN_INSIDE | FL_ALIGN_CENTER | FL_ALIGN_TOP);
					Charts[j]->copy_label(label.c_str());
					j++;
				}
			}
		}
		win->resizable(win);
		win->label(INPUT_FILE.c_str());
		win->show();
		return(Fl::run());
	#elif USING_MATPLOTLIB
		std::vector<std::string> traceLabel;
		std::vector<std::vector<double>> traceData;
		if(cArg.analysisT == VANALYSIS) traces_to_plot(iFile, iFile.controlPart, traceLabel, traceData);
		else if(cArg.analysisT == PANALYSIS) phase_traces_to_plot(iFile, iFile.controlPart, traceLabel, traceData);
		if (traceLabel.size() > 0) {
			if (traceLabel.size() <= 4) {
				//plt::figure_size(800, 600);
				plt::figure();
				for (int i = 0; i < traceLabel.size(); i++) {
					plt::subplot(traceLabel.size(), 1, i + 1);
					plt::grid(true);
					plt::plot(iFile.timeAxis, traceData[i]);
					plt::title(traceLabel[i].c_str());
					if (traceLabel[i].find("VOLTAGE") != std::string::npos) plt::ylabel("Voltage (V)");
					else if (traceLabel[i].find("CURRENT") != std::string::npos) plt::ylabel("Current (A)");
					else if (traceLabel[i].find("PHASE") != std::string::npos) plt::ylabel("Phase (rads)");
				}
				plt::xlabel("Time (s)");
				plt::tight_layout();
				plt::show();
			}
			else {
				for (int j = 0; j < traceLabel.size(); j = j + 4) {
					int  i = j;
					//plt::figure_size(800, 600);
					plt::figure();
					while ((i < traceLabel.size()) && (i < j + 4)) {
						plt::subplot(4, 1, (i - j) + 1);
						plt::grid(true);
						plt::plot(iFile.timeAxis, traceData[i]);
						plt::title(traceLabel[i].c_str());
						if (traceLabel[i].find("VOLTAGE") != std::string::npos) { plt::ylabel("Voltage (V)"); }
						else if (traceLabel[i].find("CURRENT") != std::string::npos) { plt::ylabel("Current (A)"); }
						else if (traceLabel[i].find("PHASE") != std::string::npos) { plt::ylabel("Phase (rads)"); }
						i++;
					}
					plt::xlabel("Time (s)");
					plt::tight_layout();
					plt::show(false);
				}
				plt::show();
			}
		}
		else if (traceLabel.size() == 0) {
			std::cout << "W: Plotting requested but no plot/print/save commands found." << std::endl;
			std::cout << "W: Plotting all the node voltages by default." << std::endl;
			// Find all the N column indices
			std::vector<int> nvIndices;
			for (int i = 0; i < iFile.matA.columnNames.size(); i++) if (iFile.matA.columnNames[i][2] == 'N') nvIndices.push_back(i);
			for (int j = 0; j < nvIndices.size(); j = j + 4) {
				int  i = j;
				//plt::figure_size(800, 800);
				plt::figure();
				while ((i < nvIndices.size()) && (i < j + 4)) {
					plt::subplot(4, 1, (i - j) + 1);
					plt::grid(true);
					plt::plot(iFile.timeAxis, iFile.xVect.at(nvIndices[i]));
					plt::title(Misc::substring_after(iFile.matA.columnNames.at(nvIndices[i]), "C_").c_str());
					plt::ylabel("Voltage (V)");
					i++;
				}
				plt::xlabel("Time (s)");
				plt::tight_layout();
				plt::show(false);
			}
			plt::show();
		}
		return 0;
	#endif
	return 0;
}