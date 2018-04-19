#pragma once
#include "j_plot.hpp"

/*
Determine traces to plot from the control part of the main circuit
*/
void traces_to_plot(std::vector<std::string> controlPart, std::vector<std::string>& traceLabel, std::vector<std::vector<double>>& traceData) {
	std::vector<std::string> tokens;
	std::map<std::string, std::vector<double>> traces;
	std::string columnLabel, label;
	int index = -1;
	for (auto string : controlPart) {
		if (string.find(".PRINT") != std::string::npos) {
			tokens = tokenize_space(string);
			/* Print the identified node voltage */
			if (tokens[1] == "NODEV") {
				label = "NODE VOLTAGE " + tokens[2];
				columnLabel = "C_NV" + tokens[2];
				if (std::find(columnNames.begin(), columnNames.end(), columnLabel) != columnNames.end()) {
					index = index_of(columnNames, columnLabel);
					traceLabel.push_back(label);
					traceData.push_back(x[index]);
				}
				else {
					/* Error this node was not found and can therefore not be printed */
				}
			}
			/* Print the identified junction phase */
			else if (tokens[1] == "PHASE") {
				label = "PHASE " + tokens[2];
				columnLabel = "C_P" + tokens[2];
				if (std::find(columnNames.begin(), columnNames.end(), columnLabel) != columnNames.end()) {
					index = index_of(columnNames, columnLabel);
					traceLabel.push_back(label);
					traceData.push_back(x[index]);
				}
				else {
					/* Error this node was not found and can therefore not be printed */
				}
			}
			/* Print the identified device voltage */
			else if (tokens[1] == "DEVV") {

			}
			/* Print the identified device current */
			else if (tokens[1] == "DEVI") {

			}
			/* No such print command error thrown */
			else {
				plotting_errors(NO_SUCH_PLOT_TYPE, tokens[1]);
			}
		}
	}
}
/*
	Function that creates a plotting window with all available traces to plot
*/
int plot_all_traces() {
	Fl_Window * win = new Fl_Window(1240, 768);
	Fl_Scroll * scroll = new Fl_Scroll(0, 0, win->w(), win->h());
	std::vector<Fl_Chart *> Charts;
	std::string label;
	int counter = 0;
	for (auto i : columnNames) {
		label = substring_after(i, "C_");
		Charts.push_back(new Fl_Chart(20, 20 + (counter * (scroll->h() / 3)), scroll->w() - 40, (scroll->h()/3 - 20)));
		Charts[counter]->type(FL_LINE_CHART);
		for (int j = 0; j < x[counter].size(); j++) {
			Charts[counter]->add(x[counter][j]);

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
}

/*
	Function that creates a plotting window only for the specified plots in the simulation
*/
int plot_traces(InputFile& iFile) {
	std::vector<std::string> traceLabel;
	std::vector<std::vector<double>> traceData;
	traces_to_plot(iFile.controlPart, traceLabel, traceData);
	Fl_Window * win = new Fl_Window(1240, 768);
	Fl_Scroll * scroll = new Fl_Scroll(0, 0, win->w(), win->h());
	std::vector<Fl_Chart *> Charts;
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
	win->resizable(win);
	win->label(INPUT_FILE.c_str());
	win->show();
	return(Fl::run());
}