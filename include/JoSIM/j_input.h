// Copyright (c) 2018 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef J_INPUT_H_
#define J_INPUT_H_
#include "j_std_include.h"
#include "j_errors.h"
#include "j_misc.h"

#define UTYPE -1
#define RCSJ 0
#define MTJ 1
#define NTRON 2
#define CSHE 3

class Parameter {
	public:
		std::vector<std::pair<std::string, std::string>> unparsedParams;
		std::unordered_map<std::pair<std::string, std::string>, double, pair_hash> parsedParams;
		Parameter() { }
};

class Transient {
	public:
		double prstep;
		double tstop;
		double tstart;
		double maxtstep;
		Transient() {
			tstart = 0.0;
			tstop = 0.0;
			prstep = 1E-12;
			maxtstep = 1E-12;
		};
		double simsize() { 
			return (tstop - tstart) / prstep; 
		};
};

class Subcircuit {
	public:
		std::vector<std::string> io;
		std::vector<std::pair<std::string, std::string>> lines;
		std::vector<std::string> subckts;
		int jjCount, compCount;
		bool containsSubckt;
		Subcircuit() {
			jjCount = 0;
			compCount = 0;
			containsSubckt = false;
		};
};

class Netlist {
	public:
		std::unordered_map<std::pair<std::string, std::string>, std::string, pair_hash> models;
		std::unordered_map<std::string, Subcircuit> subcircuits;
		std::unordered_map<std::string, int> subcktLookup;
		std::vector<std::string> maindesign;
		std::vector<std::string> subckts;
		int jjCount, compCount, subcktCounter, nestedSubcktCount;
		bool containsSubckt;
		Netlist() {
			jjCount = 0;
			compCount = 0;
			subcktCounter = 0;
			nestedSubcktCount = 0;
			containsSubckt = false;
		};
};

class Input {
	public:
		Netlist netlist;
		Parameter parameters;
		Transient transSim;
		std::string fileName;
		std::vector<std::string> fileLines, controls;
		std::vector<std::pair<std::string, std::string>> expNetlist;
		int argAnal, argExt, argConv;
		bool argPlot, argSave, argVerb, argDev;
		std::string argOutname;

		Input(){
			fileName = "";
            argAnal = VANALYSIS; 
            argExt = CSV;
            argConv = LEFT;
            argPlot = false;
            argSave = false;
            argVerb = false;
            argDev = false;
			argOutname = "";
        };

		void parse_arguments(int argc, char* argv[]);
        void display_help();
        void version_info();
		void read_input_file(std::string &fileName, 
							std::vector<std::string> &fileLines);
		void split_netlist(std::vector<std::string> &fileLines, 
						std::vector<std::string> &controls, 
						Parameter &parameters, 
						Netlist &netlist);
		void expand_subcircuits();
		void expand_maindesign();
};

// class A_matrix {
// 	public:
// 	std::unordered_map<std::string, std::vector<std::string>> nodeConnections;
// 	std::vector<matrix_element> mElements;
// 	std::vector<std::string> rowNames, columnNames;
// 	std::vector<double> nzval;
// 	std::vector<int> colind, rowptr;
// 	std::unordered_map<std::string, std::vector<double>> sources;
// 	int Nsize, Msize;
// 	A_matrix() {
// 		nodeConnections.clear();
// 		mElements.clear();
// 		rowNames.clear();
// 		columnNames.clear();
// 		nzval.clear();
// 		colind.clear();
// 		rowptr.clear();
// 		sources.clear();
// 	};
// };

// class trans_sim {
// 	public:
// 	double prstep;
// 	double tstop;
// 	double tstart;
// 	double maxtstep;
// 	trans_sim() {
// 		tstart = 0.0;
// 		tstop = 0.0;
// 		prstep = 1E-12;
// 		maxtstep = 1E-12;
// 	}
// 	double simsize() { return (tstop - tstart) / prstep; }
// };

// class param_values {
// 	public:
// 		std::unordered_map<std::string, std::string> unparsedMap;
// 		std::unordered_map<std::string, double> paramMap;
// 		void insertUParam(std::string paramName, std::string paramExpression, 
// 			std::string subcktName = "");
// 		void insertParam(std::string paramName, double paramValue, 
// 			std::string subcktName = "");
// 		double returnParam(std::string paramName, std::string subcktName = "");
// 		param_values() { }
// };

// void 
// check_model(std::string s, std::string sbcktName = "");

// std::vector<std::string> 
// recurseSubckt(std::unordered_map<std::string, Subcircuit> subckts, std::string part);

// class InputFile {
// 	std::vector<std::string> lines;

// 	public:
// 	std::unordered_map<std::string, std::string> models;
// 	param_values paramValues;
// 	trans_sim tsim;
// 	std::vector<std::string> maincircuitSegment, controlPart, subckts;
// 	std::unordered_map<std::string, Subcircuit> subcircuitSegments;
// 	std::unordered_map<std::string, int> subCircuitComponentCount,
// 		subCircuitJJCount, subCircuitContainsSubCircuit;
// 	std::unordered_map<std::string, double> parVal;
// 	std::unordered_map<std::string, std::string> subcircuitNameMap;

// 	std::unordered_map<std::string, res_phase> phaseRes;
// 	std::unordered_map<std::string, ind_phase> phaseInd;
// 	std::unordered_map<std::string, cap_phase> phaseCap;
// 	std::unordered_map<std::string, jj_phase> phaseJJ;
// 	std::unordered_map<std::string, vs_phase> phaseVs;

// 	std::unordered_map<std::string, res_volt> voltRes;
// 	std::unordered_map<std::string, ind_volt> voltInd;
// 	std::unordered_map<std::string, cap_volt> voltCap;
// 	std::unordered_map<std::string, jj_volt> voltJJ;
// 	std::unordered_map<std::string, vs_volt> voltVs;

// 	std::unordered_map<std::string, tx_line> txLine;
// 	std::unordered_map<std::string, tx_phase> txPhase;

// 	A_matrix matA;
// 	std::vector<std::vector<double>> xVect;
// 	std::vector<double> timeAxis;
// 	std::unordered_map<std::string, std::vector<double>> junctionCurrents;
// 	std::vector<std::string> mutualInductanceLines;
// 	int subCircuitCount = 0,
// 		jjCount = 0,
// 		componentCount = 0,
// 		allCounted = 0,
// 		circuitComponentCount = 0,
// 		circuitJJCount = 0,
// 		mainJJs = 0,
// 		mainComponents = 0;
// 	int simulationType = 4;

// 	InputFile() {};

// 	void read_input_file(std::string iFileName);
// 	void circuit_to_segments();
// 	void sub_in_subcircuits(std::vector<std::string>& segment,
// 		std::string label = "");
// };

// extern InputFile iFile;

#endif