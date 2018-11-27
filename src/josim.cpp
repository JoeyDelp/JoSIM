// Copyright (c) 2018 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#include "j_std_include.h"

Args cArg;
InputFile iFile;
/*
  JoSIM entry point
*/
int
main(int argc, char* argv[]) {

	int subcktDepth = 0, 
		thisDepth = 0, 
		overallDepth = 0;

	cArg.parse_arguments(argc, argv);

	iFile.read_input_file(cArg.inName);

	iFile.circuit_to_segments();

	Simulation::identify_simulation(iFile);

	subcktDepth =
		Misc::subCircuitDepth(iFile.maincircuitSegment, iFile, thisDepth, overallDepth);

	for (int i = 0; i < subcktDepth; i++) iFile.sub_in_subcircuits(iFile.maincircuitSegment);

	if (cArg.verbose) Misc::circuit_stats(1, iFile);
	else Misc::circuit_stats(0, iFile);

	if (cArg.dev) {
		for (const auto& i : iFile.maincircuitSegment) {
			std::vector<std::string> tokens = Misc::tokenize_space(i);
			for (const auto& j : tokens) {
				if (j.length() < 15) std::cout << std::setw(15) << std::left << j;
				else std::cout << std::setw(j.length() + (30 - j.length())) << std::left << j;
			}
			std::cout << std::endl;
		}
	}

	Matrix::matrix_A(iFile);

	if (iFile.simulationType == TRANSIENT) {
		if(cArg.analysisT == VANALYSIS) Simulation::transient_voltage_simulation(iFile);
		else if(cArg.analysisT== PANALYSIS) Simulation::transient_phase_simulation(iFile);
	}

	if (cArg.plotRes) {
		if (cArg.verbose) Plot::plot_all_traces(iFile);
		else Plot::plot_traces(iFile);
	}

	if(cArg.saveRes) {
		if (cArg.saveType == CSV) Output::write_data(iFile);
		else if (cArg.saveType == DAT) Output::write_legacy_data(iFile);	
		else if (cArg.saveType == WR) Output::write_wr_data(iFile);	
	}
	
	if (!cArg.saveRes && !cArg.plotRes) {
		Output::write_cout(iFile);
	}
}