#pragma once
#include "j_plot.hpp"

Fl_Window *win;
Fl_Scroll *scroll;
/*
Function that creates a plotting window with all available traces to plot, enabled through command-line
*/
int plot_all_traces() {
	win = new Fl_Window(1000, 660);
	scroll = new Fl_Scroll(0, 0, win->w(), win->h());
	std::vector<Fl_Chart *> Charts;
	std::string label;
	int counter = 0;
	int chartHeight = (win->h() / 3) - 20;
	for (auto i : columnNames) {
		label = substring_after(i, "C_");
		Charts.push_back(new Fl_Chart(20, 20 + (counter * (win->h() / 3)), win->w() - 40, chartHeight));
		Charts[counter]->type(FL_LINE_CHART);
		for (int j = 0; j < x[counter].size(); j++) {
			Charts[counter]->add(x[counter][j]);
		}
		Charts[counter]->color(FL_WHITE);
		Charts[counter]->align(FL_ALIGN_INSIDE|FL_ALIGN_CENTER|FL_ALIGN_TOP);
		Charts[counter]->copy_label(label.c_str());
		counter++;
	}
	win->color(FL_WHITE);
	win->resizable(win);
	win->label(INPUT_FILE.c_str());
	win->show();
	return(Fl::run());
}