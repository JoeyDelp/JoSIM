// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#include "j_std_include.h"
#include "j_input.h"
#include "j_parser.h"
#include "j_simulation.h"
#include "j_matrix.h"
#include "j_output.h"
#include "j_verbose.h"

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
	if(iObj.argVerb) Verbose::print_expanded_netlist(iObj.expNetlist);
	sObj.identify_simulation(iObj.controls, iObj.transSim.prstep, iObj.transSim.tstop, iObj.transSim.tstart, iObj.transSim.maxtstep);
	mObj.create_matrix(iObj);
	if(iObj.argAnal == VANALYSIS) sObj.transient_voltage_simulation(iObj, mObj);
	else if (iObj.argAnal == PANALYSIS) sObj.transient_phase_simulation(iObj, mObj);
	oObj.relevant_traces(iObj, mObj, sObj);
	if(iObj.argSave) {
		if(iObj.argExt == CSV) oObj.write_data(iObj.argOutname);
		else if(iObj.argExt == DAT) oObj.write_legacy_data(iObj.argOutname);
		else if(iObj.argExt == WR) oObj.write_wr_data(iObj.argOutname);
	}
	if(iObj.argPlot) {
		if(!oObj.traces.empty()) oObj.plot_traces(iObj.fileName);
		else oObj.plot_all(iObj.fileName, mObj, sObj);
	}
	if(!iObj.argPlot && !iObj.argSave) oObj.write_cout(mObj, sObj);
	return 0;
}
