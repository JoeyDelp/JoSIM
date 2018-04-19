#pragma once
#include "j_plot.hpp"

/*
Function that creates a plotting window with all available traces to plot, enabled through command-line
*/
int plot_all_traces() {
	Fl_Window *win = new Fl_Window(1000, 480);
	std::vector<Fl_Chart*> Charts;
	int counter = 0;
	Fl_Chart *newChart = new Fl_Chart(20, 20, win->w() - 40, win->h() - 40);
	for (auto i : columnNames) {
		newChart->label(i.c_str());
		Charts.push_back(newChart);
	}
	win->resizable(win);
	win->label(INPUT_FILE.c_str());
	win->show();
	return(Fl::run());
}