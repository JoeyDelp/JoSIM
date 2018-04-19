#pragma once
#include "j_plot.hpp"

/*
Callback function to resize the scroll widget to fit the window it is in
*/
void resize_children(Fl_Widget* w, void* data) {
	w->redraw();
}
/*
Function that creates a plotting window with all available traces to plot, enabled through command-line
*/
int plot_all_traces() {
	Fl_Window *win = new Fl_Window(1000, 660);
	Fl_Scroll *scroll = new Fl_Scroll(0, 0, win->w(), win->h());
	int counter = 0;
	int chartHeight = (win->h() / 3) - 20;
	for (auto i : columnNames) {
		const char * label = (substring_after(i, "C_")).c_str();
		Fl_Chart *newChart = new Fl_Chart(20, 20 + (counter * (win->h()/3)), win->w() - 40, chartHeight);
		newChart->type(FL_LINE_CHART);
		for (int j = 0; j < x[counter].size(); j++) {
			newChart->add(x[counter][j]);
		}
		counter++;
		newChart->label(label);
		newChart->align(FL_ALIGN_INSIDE||FL_ALIGN_CENTER||FL_ALIGN_TOP);
	}
	scroll->color(FL_WHITE);
	win->callback(resize_children);
	win->color(FL_WHITE);
	win->resizable(win);
	win->label(INPUT_FILE.c_str());
	win->show();
	return(Fl::run());
}