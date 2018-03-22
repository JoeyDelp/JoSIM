#pragma once
#include "include/j_std_include.hpp"

/*
	Determine traces to plot from the control part of the main circuit
*/
void traces_to_plot(std::vector<std::string> controlPart, std::vector<std::string>& trace_label, std::vector<std::vector<double>>& trace_data);
/*
	Function that creates a plotting window with all available traces to plot, enabled through command-line
*/
int plot_all_traces();
/*
	Function that creates a plotting window only for the specified plots in the simulation
*/
int plot_traces(InputFile& iFile);