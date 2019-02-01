// Copyright (c) 2018 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef J_PLOT_H_
#define J_PLOT_H_
#include "j_std_include.h"

class Plot {
    public:
        Plot() {};
};

// class Plot {
// 	public:
// 		/*
// 			Determine traces to plot from the control part of the main circuit
// 		*/
// 		static
// 		void
// 		traces_to_plot(InputFile& iFile,
// 			std::vector<std::string> controlPart,
// 			std::vector<std::string>& trace_label,
// 			std::vector<std::vector<double>>& trace_data);
// 		/*
// 			Determine traces to plot from the control part of the main circuit
// 		*/
// 		static
// 		void
// 		phase_traces_to_plot(InputFile& iFile,
// 			std::vector<std::string> controlPart,
// 			std::vector<std::string>& trace_label,
// 			std::vector<std::vector<double>>& trace_data);
// 		/*
// 			Function that creates a plotting window with all available traces to plot, enabled through command-line
// 		*/
// 		static
// 		int
// 		plot_all_traces(InputFile& iFile);
// 		/*
// 			Function that creates a plotting window only for the specified plots in the simulation
// 		*/
// 		static
// 		int
// 		plot_traces(InputFile& iFile);
// };
#endif