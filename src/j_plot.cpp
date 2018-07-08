// Copyright (c) 2018 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#include "j_plot.hpp"
#ifdef USING_MATPLOTLIB
namespace plt = matplotlibcpp;
#endif
/*
Determine traces to plot from the control part of the main circuit
*/
void traces_to_plot(InputFile& iFile, std::vector<std::string> controlPart, std::vector<std::string>& traceLabel, std::vector<std::vector<double>>& traceData) {
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
			tokens = tokenize_space(string);
			/* Print the identified node voltage */
			/*****************************************************************************************************/
			if (tokens[1] == "NODEV") {
				/* If more than one node is specified */
				if (tokens.size() == 4) {
					/* If second node is ground */
					if(tokens[3] == "0" || tokens[3] == "GND") {
						label = "NODE VOLTAGE " + tokens[2];
						if (tokens[2][0] == 'X') { 
							labeltokens = tokenize_delimeter(tokens[2], "_");
							std::rotate(labeltokens.begin(), labeltokens.end() - 1, labeltokens.end());
							tokens[2] = labeltokens[0];
							for (int n = 1; n < labeltokens.size(); n++) {
								tokens[2] = tokens[2] + "_" + labeltokens[n];
							}
						}
						columnLabel1 = "C_NV" + tokens[2];
						if (std::find(iFile.matA.columnNames.begin(), iFile.matA.columnNames.end(), columnLabel1) != iFile.matA.columnNames.end()) {
							index1 = index_of(iFile.matA.columnNames, columnLabel1);
							traceLabel.push_back(label);
							traceData.push_back(xVect[index1]);
						}
						else {
							/* Error this node was not found and can therefore not be printed */
							plotting_errors(NO_SUCH_NODE_FOUND, tokens[2]);
						}
					}
					/* If first node is ground */
					else if (tokens[2] == "0" || tokens[3] == "GND") {
						label = "NODE VOLTAGE " + tokens[3];
						if (tokens[3][0] == 'X') {
							labeltokens = tokenize_delimeter(tokens[3], "_");
							std::rotate(labeltokens.begin(), labeltokens.end() - 1, labeltokens.end());
							tokens[3] = labeltokens[0];
							for (int n = 1; n < labeltokens.size(); n++) {
								tokens[3] = tokens[3] + "_" + labeltokens[n];
							}
						}
						columnLabel1 = "C_NV" + tokens[3];
						if (std::find(iFile.matA.columnNames.begin(), iFile.matA.columnNames.end(), columnLabel1) != iFile.matA.columnNames.end()) {
							index1 = index_of(iFile.matA.columnNames, columnLabel1);
							trace.clear();
							trace = xVect[index1];
							std::fill(trace.begin(), trace.end(), 0.0);
							std::transform(trace.begin(), trace.end(), xVect[index1].begin(), trace.begin(), std::minus<double>());
							traceLabel.push_back(label);
							traceData.push_back(trace);
						}
						else {
							/* Error this node was not found and can therefore not be printed */
							plotting_errors(NO_SUCH_NODE_FOUND, tokens[3]);
						}
					}
					/* If neither are ground*/
					else {
						label = "NODE VOLTAGE " + tokens[2] + " to " + tokens[3];
						columnLabel1 = "C_NV" + tokens[2];
						columnLabel2 = "C_NV" + tokens[3];
						if (tokens[2][0] == 'X') {
							labeltokens = tokenize_delimeter(tokens[2], "_");
							std::rotate(labeltokens.begin(), labeltokens.end() - 1, labeltokens.end());
							tokens[2] = labeltokens[0];
							for (int n = 1; n < labeltokens.size(); n++) {
								tokens[2] = tokens[2] + "_" + labeltokens[n];
							}
						}
						if (tokens[3][0] == 'X') {
							labeltokens = tokenize_delimeter(tokens[3], "_");
							std::rotate(labeltokens.begin(), labeltokens.end() - 1, labeltokens.end());
							tokens[3] = labeltokens[0];
							for (int n = 1; n < labeltokens.size(); n++) {
								tokens[3] = tokens[3] + "_" + labeltokens[n];
							}
						}
						if (std::find(iFile.matA.columnNames.begin(), iFile.matA.columnNames.end(), columnLabel1) != iFile.matA.columnNames.end()) {
							index1 = index_of(iFile.matA.columnNames, columnLabel1);
							trace.clear();
							trace = xVect[index1];
							if (std::find(iFile.matA.columnNames.begin(), iFile.matA.columnNames.end(), columnLabel2) != iFile.matA.columnNames.end()) {
								index2 = index_of(iFile.matA.columnNames, columnLabel2);
								std::transform(xVect[index1].begin(), xVect[index1].end(), xVect[index2].begin(), trace.begin(), std::minus<double>());
								traceLabel.push_back(label);
								traceData.push_back(trace);
							}
							else {
								/* Error this node was not found and can therefore not be printed */
								plotting_errors(NO_SUCH_NODE_FOUND, tokens[3]);
							}
						}
						else {
							/* Error this node was not found and can therefore not be printed */
							plotting_errors(NO_SUCH_NODE_FOUND, tokens[2]);
						}
					}
				}
				/* If only one node is specified */
				else {
					label = "NODE VOLTAGE " + tokens[2];
					columnLabel1 = "C_NV" + tokens[2];
					if (std::find(iFile.matA.columnNames.begin(), iFile.matA.columnNames.end(), columnLabel1) != iFile.matA.columnNames.end()) {
						index1 = index_of(iFile.matA.columnNames, columnLabel1);
						traceLabel.push_back(label);
						traceData.push_back(xVect[index1]);
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
					labeltokens = tokenize_delimeter(tokens[2], "_");
					std::rotate(labeltokens.begin(), labeltokens.end() - 1, labeltokens.end());
					tokens[2] = labeltokens[0];
					for (int n = 1; n < labeltokens.size(); n++) {
						tokens[2] = tokens[2] + "_" + labeltokens[n];
					}
				}
				columnLabel1 = "C_P" + tokens[2];
				if (std::find(iFile.matA.columnNames.begin(), iFile.matA.columnNames.end(), columnLabel1) != iFile.matA.columnNames.end()) {
					index1 = index_of(iFile.matA.columnNames, columnLabel1);
					traceLabel.push_back(label);
					traceData.push_back(xVect[index1]);
				}
				else {
					/* Error this node was not found and can therefore not be printed */
					plotting_errors(NO_SUCH_DEVICE_FOUND, tokens[2]);
				}
			}
			/* Print the identified device voltage */
			/*****************************************************************************************************/
			else if (tokens[1] == "DEVV") {
				label = "NOTHING";
				if (tokens[2][0] == 'X') {
					labeltokens = tokenize_delimeter(tokens[2], "_");
					std::rotate(labeltokens.begin(), labeltokens.end() - 1, labeltokens.end());
					tokens[2] = labeltokens[0];
					for (int n = 1; n < labeltokens.size(); n++) {
						tokens[2] = tokens[2] + "_" + labeltokens[n];
					}
				}
				for (auto i : iFile.matA.elements) {
					if (i.label == tokens[2]) {
						trace.clear();
						if (i.VPindex == -1) trace = xVect[i.VNindex];
						else if (i.VNindex == -1) trace = xVect[i.VPindex];
						else {
							trace = xVect[i.VPindex];
							std::transform(xVect[i.VPindex].begin(), xVect[i.VPindex].end(), xVect[i.VNindex].begin(), trace.begin(), std::minus<double>());
						}
						label = "DEVICE VOLTAGE " + i.label;
						traceLabel.push_back(label);
						traceData.push_back(trace);
					}
				}
				if (label == "NOTHING") {
					if (VERBOSE) plotting_errors(NO_SUCH_DEVICE_FOUND, tokens[2]);
				}
			}
			/* Print the identified device current */
			/*****************************************************************************************************/
			else if (tokens[1] == "DEVI") {
				label = "NOTHING";
				if (tokens[2][0] == 'X') {
					labeltokens = tokenize_delimeter(tokens[2], "_");
					std::rotate(labeltokens.begin(), labeltokens.end() - 1, labeltokens.end());
					tokens[2] = labeltokens[0];
					for (int n = 1; n < labeltokens.size(); n++) {
						tokens[2] = tokens[2] + "_" + labeltokens[n];
					}
				}
				std::vector<double> trace;
				for (auto i : iFile.matA.elements) {
					if (i.label == tokens[2]) {
						if (tokens[2][0] == 'R') {
							if (i.VPindex == -1) trace = xVect[i.VNindex];
							else if (i.VNindex == -1) trace = xVect[i.VPindex];
							else std::transform(xVect[i.VPindex].begin(), xVect[i.VPindex].end(), xVect[i.VNindex].begin(), trace.begin(), std::minus<double>());
							std::transform(trace.begin(), trace.end(), trace.begin(), std::bind(std::multiplies<double>(), std::placeholders::_1, (1/i.value)));
							label = "DEVICE CURRENT " + i.label;
							traceLabel.push_back(label);
							traceData.push_back(trace);
						}
						else if (tokens[2][0] == 'C') {

						}
						else if (tokens[2][0] == 'L') {
							if (i.CURindex == -1) simulation_errors(INDUCTOR_CURRENT_NOT_FOUND, i.label);
							else trace = xVect[i.CURindex];
							label = "DEVICE CURRENT " + i.label;
							traceLabel.push_back(label);
							traceData.push_back(trace);
						}
						else if (tokens[2][0] == 'I') {
							label = "DEVICE CURRENT " + i.label;
							traceLabel.push_back(label);
							traceData.push_back(iFile.matA.sources[i.label]);
						}
						else if (tokens[2][0] == 'V') {
							if (VERBOSE) simulation_errors(CURRENT_THROUGH_VOLTAGE_SOURCE, i.label);
						}
						else if (tokens[2][0] == 'B') {
							trace = junctionCurrents["R_" + i.label];
							label = "DEVICE CURRENT " + i.label;
							traceLabel.push_back(label);
							traceData.push_back(trace);
						}
						else plotting_errors(NO_SUCH_DEVICE_FOUND, tokens[2]);
					}
				}
				if (label == "NOTHING") {
					plotting_errors(NO_SUCH_DEVICE_FOUND, tokens[2]);
				}
			}
			/* No such print command error thrown */
			else {
				if (VERBOSE) plotting_errors(NO_SUCH_PLOT_TYPE, tokens[1]);
			}
		}
		/****************************************************/
		/*						PLOT						*/
		/****************************************************/
		else if (string.find("PLOT") != std::string::npos) {
			tokens = tokenize_space(string);
			for (int k = 1; k < tokens.size(); k++) {
				/* If plotting voltage */
				if(tokens[k][0] == 'V') {
					/* Identify part between brackets */
					nodesToPlot = tokens[k].substr(2);
					nodesToPlot = nodesToPlot.substr(0, nodesToPlot.size() - 1);
					/* If multiple arguments are specified for V */
					if (nodesToPlot.find(',') != std::string::npos) {
						nodesTokens = tokenize_delimeter(nodesToPlot, ",");
						if(nodesTokens.size() > 2) {
							plotting_errors(TOO_MANY_NODES, string);
						}
						/* Ensure node 1 is not ground */
						if(nodesTokens[0] == "0" || nodesTokens[0] == "GND") {
							if(nodesTokens[1] == "0" || nodesTokens[1] == "GND") {
								plotting_errors(BOTH_ZERO, string);
							}
							else {
								if (nodesTokens[1][0] == 'X') { 
									labeltokens = tokenize_delimeter(tokens[1], "_");
									std::rotate(labeltokens.begin(), labeltokens.end() - 1, labeltokens.end());
									nodesTokens[1] = labeltokens[0];
									for (int n = 1; n < labeltokens.size(); n++) {
										nodesTokens[1] = nodesTokens[1] + "_" + labeltokens[n];
									}
								}
								columnLabel1 = "C_NV" + nodesTokens[1];
								/* If this is a node voltage */
								if (std::find(iFile.matA.columnNames.begin(), iFile.matA.columnNames.end(), columnLabel1) != iFile.matA.columnNames.end()) {
									index1 = index_of(iFile.matA.columnNames, columnLabel1);
									trace.clear();
									trace = xVect[index1];
									std::fill(trace.begin(), trace.end(), 0.0);
									std::transform(trace.begin(), trace.end(), xVect[index1].begin(), trace.begin(), std::minus<double>());
									traceLabel.push_back(label);
									traceData.push_back(trace);
								}
								/* Else node not found */
								else {
									plotting_errors(NO_SUCH_NODE_FOUND, string);
								}
							}
						}
						/* Check if node 2 is ground */
						else {
							if(tokens[1] == "0" || tokens[1] == "GND") {
								if (tokens[0][0] == 'X') { 
									labeltokens = tokenize_delimeter(tokens[0], "_");
									std::rotate(labeltokens.begin(), labeltokens.end() - 1, labeltokens.end());
									tokens[0] = labeltokens[0];
									for (int n = 1; n < labeltokens.size(); n++) {
										tokens[0] = tokens[0] + "_" + labeltokens[n];
									}
								}
								columnLabel1 = "C_NV" + tokens[0];
								if (std::find(iFile.matA.columnNames.begin(), iFile.matA.columnNames.end(), columnLabel1) != iFile.matA.columnNames.end()) {
									index1 = index_of(iFile.matA.columnNames, columnLabel1);
									traceLabel.push_back(label);
									traceData.push_back(xVect[index1]);
								}
								else {
									plotting_errors(NO_SUCH_NODE_FOUND, string);
								}
							}
							/* Neither nodes are ground */
							else {
								label = "NODE VOLTAGE " + nodesTokens[0] + " to " + nodesTokens[1];
								columnLabel1 = "C_NV" + nodesTokens[0];
								columnLabel2 = "C_NV" + nodesTokens[1];
								if (nodesTokens[0][0] == 'X') {
									labeltokens = tokenize_delimeter(nodesTokens[0], "_");
									std::rotate(labeltokens.begin(), labeltokens.end() - 1, labeltokens.end());
									nodesTokens[0] = labeltokens[0];
									for (int n = 1; n < labeltokens.size(); n++) {
										nodesTokens[0] = nodesTokens[0] + "_" + labeltokens[n];
									}
								}
								if (nodesTokens[1][0] == 'X') {
									labeltokens = tokenize_delimeter(nodesTokens[1], "_");
									std::rotate(labeltokens.begin(), labeltokens.end() - 1, labeltokens.end());
									nodesTokens[1] = labeltokens[0];
									for (int n = 1; n < labeltokens.size(); n++) {
										nodesTokens[1] = nodesTokens[1] + "_" + labeltokens[n];
									}
								}
								if (std::find(iFile.matA.columnNames.begin(), iFile.matA.columnNames.end(), columnLabel1) != iFile.matA.columnNames.end()) {
									index1 = index_of(iFile.matA.columnNames, columnLabel1);
									trace.clear();
									trace = xVect[index1];
									if (std::find(iFile.matA.columnNames.begin(), iFile.matA.columnNames.end(), columnLabel2) != iFile.matA.columnNames.end()) {
										index2 = index_of(iFile.matA.columnNames, columnLabel2);
										std::transform(xVect[index1].begin(), xVect[index1].end(), xVect[index2].begin(), trace.begin(), std::minus<double>());
										traceLabel.push_back(label);
										traceData.push_back(trace);
									}
									else {
										/* Error this node was not found and can therefore not be printed */
										plotting_errors(NO_SUCH_NODE_FOUND, string);
									}
								}
							}
						}
					}
					/* If only one argument is specified for V */
					else {
						/* Ensure node is not ground */
						if(nodesToPlot != "0" || nodesToPlot != "GND") {
							if (nodesToPlot[0] == 'X') { 
									labeltokens = tokenize_delimeter(nodesToPlot, "_");
									std::rotate(labeltokens.begin(), labeltokens.end() - 1, labeltokens.end());
									nodesToPlot = labeltokens[0];
									for (int n = 1; n < labeltokens.size(); n++) {
										nodesToPlot = nodesToPlot + "_" + labeltokens[n];
									}
								}
							label = "C_NV" + nodesToPlot;
							/* If this is a node voltage */
							if (std::find(iFile.matA.columnNames.begin(), iFile.matA.columnNames.end(), label) != iFile.matA.columnNames.end()) {
								index1 = index_of(iFile.matA.columnNames, label);
								label = "NODE VOLTAGE " + nodesToPlot;
								traceLabel.push_back(label);
								traceData.push_back(xVect[index1]);
							}
							/* Else it might be device voltage */
							else {
								label = "NOTHING";
								for (auto i : iFile.matA.elements) {
									if (i.label == nodesToPlot) {
										trace.clear();
										if (i.VPindex == -1) trace = xVect[i.VNindex];
										else if (i.VNindex == -1) trace = xVect[i.VPindex];
										else {
											trace = xVect[i.VPindex];
											std::transform(xVect[i.VPindex].begin(), xVect[i.VPindex].end(), xVect[i.VNindex].begin(), trace.begin(), std::minus<double>());
										}
										label = "DEVICE VOLTAGE " + i.label;
										traceLabel.push_back(label);
										traceData.push_back(trace);
									}
								}
								if (label == "NOTHING") {
									if (VERBOSE) plotting_errors(NO_SUCH_DEVICE_FOUND, nodesToPlot);
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
						labeltokens = tokenize_delimeter(nodesToPlot, "_");
						std::rotate(labeltokens.begin(), labeltokens.end() - 1, labeltokens.end());
						nodesToPlot = labeltokens[0];
						for (int n = 1; n < labeltokens.size(); n++) {
							nodesToPlot = nodesToPlot + "_" + labeltokens[n];
						}
					}
					std::vector<double> trace;
					for (auto i : iFile.matA.elements) {
						if (i.label == nodesToPlot) {
							if (nodesToPlot[0] == 'R') {
								if (i.VPindex == -1) trace = xVect[i.VNindex];
								else if (i.VNindex == -1) trace = xVect[i.VPindex];
								else std::transform(xVect[i.VPindex].begin(), xVect[i.VPindex].end(), xVect[i.VNindex].begin(), trace.begin(), std::minus<double>());
								std::transform(trace.begin(), trace.end(), trace.begin(), std::bind(std::multiplies<double>(), std::placeholders::_1, (1/i.value)));
								label = "DEVICE CURRENT " + i.label;
								traceLabel.push_back(label);
								traceData.push_back(trace);
							}
							else if (nodesToPlot[0] == 'C') {

							}
							else if (nodesToPlot[0] == 'L') {
								if (i.CURindex == -1) simulation_errors(INDUCTOR_CURRENT_NOT_FOUND, i.label);
								else trace = xVect[i.CURindex];
								label = "DEVICE CURRENT " + i.label;
								traceLabel.push_back(label);
								traceData.push_back(trace);
							}
							else if (nodesToPlot[0] == 'I') {
								label = "DEVICE CURRENT " + i.label;
								traceLabel.push_back(label);
								traceData.push_back(iFile.matA.sources[i.label]);
							}
							else if (nodesToPlot[0] == 'V') {
								if (VERBOSE) simulation_errors(CURRENT_THROUGH_VOLTAGE_SOURCE, i.label);
							}
							else if (nodesToPlot[0] == 'B') {
								trace = junctionCurrents["R_" + i.label];
								label = "DEVICE CURRENT " + i.label;
								traceLabel.push_back(label);
								traceData.push_back(trace);
							}
							else plotting_errors(NO_SUCH_DEVICE_FOUND, string);
						}
					}
					if (label == "NOTHING") {
						plotting_errors(NO_SUCH_DEVICE_FOUND, string);
					}
				}
				else if (tokens[k][0] == 'P') {
					/* Identify part between brackets */
					nodesToPlot = tokens[k].substr(2);
					nodesToPlot = nodesToPlot.substr(0, nodesToPlot.size() - 1);
					label = "PHASE " + nodesToPlot;
					if (nodesToPlot[0] == 'X') {
						labeltokens = tokenize_delimeter(nodesToPlot, "_");
						std::rotate(labeltokens.begin(), labeltokens.end() - 1, labeltokens.end());
						nodesToPlot = labeltokens[0];
						for (int n = 1; n < labeltokens.size(); n++) {
							nodesToPlot = nodesToPlot + "_" + labeltokens[n];
						}
					}
					columnLabel1 = "C_P" + nodesToPlot;
					if (std::find(iFile.matA.columnNames.begin(), iFile.matA.columnNames.end(), columnLabel1) != iFile.matA.columnNames.end()) {
						index1 = index_of(iFile.matA.columnNames, columnLabel1);
						traceLabel.push_back(label);
						traceData.push_back(xVect[index1]);
					}
					else {
						/* Error this node was not found and can therefore not be printed */
						plotting_errors(NO_SUCH_DEVICE_FOUND, nodesToPlot);
					}
				}
			}
		}
		/****************************************************/
		/*						SAVE						*/
		/****************************************************/
		else if (string.find("SAVE") != std::string::npos) {
			tokens = tokenize_space(string);
			for (int k = 1; k < tokens.size(); k++) {
				index1 = tokens[k].find("@");
				if(index1 != std::string::npos) tokens[k] = tokens[k].substr(0, index1) + tokens[k].substr(index1+1);
				index1 = tokens[k].find("[");
				if(index1 != std::string::npos) tokens[k] = tokens[k].substr(0, index1);
				index1 = tokens[k].find(".");
				if(index1 != std::string::npos) {
					tokens[k] = tokens[k].substr(0, index1) + "_" + tokens[k].substr(index1+1);
				}
				/* If this is a current source */
				if (iFile.matA.sources.find(tokens[k]) != iFile.matA.sources.end()) {
					label = "CURRENT " + tokens[k];
					traceLabel.push_back(label);
					traceData.push_back(iFile.matA.sources[tokens[k]]);
				}
			}
		}
	}
}
/*
	Function that creates a plotting window with all available traces to plot
*/
int plot_all_traces(InputFile& iFile) {
	#ifdef USING_FLTK
		Fl_Window * win = new Fl_Window(1240, 768);
		Fl_Scroll * scroll = new Fl_Scroll(0, 0, win->w(), win->h());
		std::vector<Fl_Chart *> Charts;
		std::string label;
		int counter = 0;
		for (auto i : iFile.matA.columnNames) {
			label = substring_after(i, "C_");
			Charts.push_back(new Fl_Chart(20, 20 + (counter * (scroll->h() / 3)), scroll->w() - 40, (scroll->h()/3 - 20)));
			Charts[counter]->type(FL_LINE_CHART);
			for (int j = 0; j < xVect[counter].size(); j++) {
				Charts[counter]->add(xVect[counter][j]);

			}
			Charts[counter]->color(FL_WHITE);
			Charts[counter]->align(FL_ALIGN_INSIDE|FL_ALIGN_CENTER|FL_ALIGN_TOP);
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
				plt::plot(timeAxis, xVect[counter]);
				plt::title(substring_after(i, "C_"));
				if(substring_after(i, "C_")[0] == 'N') plt::ylabel("Voltage (V)");
				else if (substring_after(i, "C_")[0] == 'I') plt::ylabel("Current (A)");
				else if (substring_after(i, "C_")[0] == 'P') plt::ylabel("Phase (rads)");
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
				while((counter < iFile.matA.columnNames.size()) && (counter < j + 3)) {
					plt::subplot(3, 1, (counter - j) + 1);
					plt::grid(true);
					plt::plot(timeAxis, xVect[counter]);
					plt::title(substring_after(iFile.matA.columnNames[counter], "C_"));
					if(substring_after(iFile.matA.columnNames[counter], "C_")[0] == 'N') plt::ylabel("Voltage (V)");
					else if (substring_after(iFile.matA.columnNames[counter], "C_")[0] == 'I') plt::ylabel("Current (A)");
					else if (substring_after(iFile.matA.columnNames[counter], "C_")[0] == 'P') plt::ylabel("Phase (rads)");
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
int plot_traces(InputFile& iFile) {
	#ifdef USING_FLTK
		std::vector<std::string> traceLabel;
		std::vector<std::vector<double>> traceData;
		traces_to_plot(iFile.controlPart, traceLabel, traceData);
		Fl_Window * win = new Fl_Window(1240, 768);
		Fl_Scroll * scroll = new Fl_Scroll(0, 0, win->w(), win->h());
		std::vector<Fl_Chart *> Charts;
		if(traceLabel.size() > 0) {
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
				label = substring_after(iFile.matA.columnNames[i], "C_");
				if(label[0] == 'N') {
					Charts.push_back(new Fl_Chart(20, 20 + (j * (scroll->h() / 3)), scroll->w() - 40, (scroll->h() / 3 - 20)));
					Charts[j]->type(FL_LINE_CHART);
					for (int k = 0; k < xVect[i].size(); k++) {
						Charts[j]->add(xVect[i][k]);
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
		traces_to_plot(iFile, iFile.controlPart, traceLabel, traceData);
		if(traceLabel.size() > 0) {
			if (traceLabel.size() <= 3) {
				//plt::figure_size(800, 600);
				plt::figure();
				for (int i = 0; i < traceLabel.size(); i++) {
					plt::subplot(traceLabel.size(), 1, i + 1);
					plt::grid(true);
					plt::plot(timeAxis, traceData[i]);
					plt::title(traceLabel[i].c_str());
					if(traceLabel[i].find("VOLTAGE") != std::string::npos) plt::ylabel("Voltage (V)");
					else if (traceLabel[i].find("CURRENT") != std::string::npos) plt::ylabel("Current (A)");
					else if (traceLabel[i].find("PHASE") != std::string::npos) plt::ylabel("Phase (rads)");
				}
				plt::xlabel("Time (s)");
				plt::tight_layout();
				plt::show();
			}
			else {
				for (int j = 0; j < traceLabel.size(); j = j + 3) {
					int  i = j;
					//plt::figure_size(800, 600);
					plt::figure();
					while((i < traceLabel.size()) && (i < j + 3)) {
						plt::subplot(3, 1, (i - j) + 1);
						plt::grid(true);
						plt::plot(timeAxis, traceData[i]);
						plt::title(traceLabel[i].c_str());
						if(traceLabel[i].find("VOLTAGE") != std::string::npos) { plt::ylabel("Voltage (V)"); }
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
		else if(traceLabel.size() == 0) {
			std::cout << "W: Plotting requested but no plot/print/save commands found." << std::endl;
			std::cout << "W: Plotting all the node voltages by default." << std::endl;
			// Find all the NV column indices
			std::vector<int> nvIndices;
			for(int i = 0; i < iFile.matA.columnNames.size(); i++) if(iFile.matA.columnNames[i][2] == 'N') nvIndices.push_back(i);
			for (int j = 0; j < nvIndices.size(); j = j + 3) {
					int  i = j;
					plt::figure_size(800, 600);
					while((i < nvIndices.size()) && (i < j + 3)) {
						plt::subplot(3, 1, (i - j) + 1);
						plt::grid(true);
						plt::plot(timeAxis, xVect[nvIndices[i]]);
						plt::title(substring_after(iFile.matA.columnNames[nvIndices[i]], "C_").c_str());
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
