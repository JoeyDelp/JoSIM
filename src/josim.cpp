// Copyright (c) 2018 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#include "j_std_include.h"
#include "j_input.h"
#include "j_parser.h"
#include "j_simulation.h"
#include "j_matrix.h"
#include "j_output.h"

//Args cArg;
//InputFile iFile;
/*
  JoSIM entry point
*/
int
main(int argc, char* argv[]) {

	Input iObj;
	Matrix mObj;
	Simulation sObj;
	Output oObj;
	iObj.parse_arguments(argc, argv);
	iObj.read_input_file(iObj.fileName, iObj.fileLines);
	iObj.split_netlist(iObj.fileLines, iObj.controls, iObj.parameters, iObj.netlist);
	if(iObj.parameters.unparsedParams.size() > 0) Parser::parse_parameters(iObj.parameters.unparsedParams, iObj.parameters.parsedParams);
	iObj.expand_subcircuits();
	iObj.expand_maindesign();
	sObj.identify_simulation(iObj.controls, iObj.transSim.prstep, iObj.transSim.tstop, iObj.transSim.tstart, iObj.transSim.maxtstep);
	mObj.create_matrix(iObj);
	if(iObj.argAnal == VANALYSIS) sObj.transient_voltage_simulation(iObj, mObj);
	else if (iObj.argAnal == PANALYSIS) sObj.transient_phase_simulation(iObj, mObj);
	oObj.relevant_traces(iObj, mObj, sObj);


	// int subcktDepth = 0, 
	// 	thisDepth = 0, 
	// 	overallDepth = 0;

	// cArg.parse_arguments(argc, argv);

	// iFile.read_input_file(cArg.inName);

	// iFile.circuit_to_segments();

	// //Simulation::identify_simulation(iFile);

	// subcktDepth =
	// 	Misc::subCircuitDepth(iFile.maincircuitSegment, iFile, thisDepth, overallDepth);

	// for (int i = 0; i < subcktDepth; i++) iFile.sub_in_subcircuits(iFile.maincircuitSegment);

	// if (cArg.verbose) Misc::circuit_stats(1, iFile);
	// else Misc::circuit_stats(0, iFile);

	// if (cArg.dev) {
	// 	for (const auto& i : iFile.maincircuitSegment) {
	// 		std::vector<std::string> tokens = Misc::tokenize_space(i);
	// 		for (const auto& j : tokens) {
	// 			if (j.length() < 15) std::cout << std::setw(15) << std::left << j;
	// 			else std::cout << std::setw(j.length() + (30 - j.length())) << std::left << j;
	// 		}
	// 		std::cout << std::endl;
	// 	}
	// }

	// Matrix::matrix_A(iFile);

	// if (iFile.simulationType == TRANSIENT) {
	// 	if(cArg.analysisT == VANALYSIS) Simulation::transient_voltage_simulation(iFile);
	// 	else if(cArg.analysisT== PANALYSIS) Simulation::transient_phase_simulation(iFile);
	// }

	// if (cArg.plotRes) {
	// 	if (cArg.verbose) Plot::plot_all_traces(iFile);
	// 	else Plot::plot_traces(iFile);
	// }

	// if(cArg.saveRes) {
	// 	if (cArg.saveType == CSV) Output::write_data(iFile);
	// 	else if (cArg.saveType == DAT) Output::write_legacy_data(iFile);	
	// 	else if (cArg.saveType == WR) Output::write_wr_data(iFile);	
	// }
	
	// if (!cArg.saveRes && !cArg.plotRes) {
	// 	Output::write_cout(iFile);
	// }
}