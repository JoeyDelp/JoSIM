// Copyright (c) 2018 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef J_INPUT_H
#define J_INPUT_H
#include "j_std_include.hpp"

#define UTYPE -1
#define RCSJ 0
#define MTJ 1
#define NTRON 2
#define CSHE 3

/*
  Matrix element class
*/
class matrix_element
{
public:
	std::string label;
	bool junctionEntry;
	char junctionDirection;
	std::unordered_map<std::string, double> tokens;
	int rowIndex;
	int columnIndex;
	double value;
	/* Default matrix element constructor */
	matrix_element()
	{
		label = "NOTHING";
		junctionDirection = ' ';
		junctionEntry = false;
		rowIndex = -1;
		columnIndex = -1;
		value = 0.0;
	}
};

class element
{
public:
	std::string label;
	int VPindex;
	int VNindex;
	int CURindex;
	double value;
	element()
	{
		label = "NOTHING";
		VPindex = -1;
		VNindex = -1;
		CURindex = -1;
		value = 0.0;
	}
};

class model_xline
{
public:
	std::string label;
	double TD;
	double Z0;
	int pNode1, pNode2, nNode1, nNode2, iNode1, iNode2;
	/* Default xline constructor */
	model_xline()
	{
		pNode1 = -1;
		pNode2 = -1;
		nNode1 = -1;
		nNode2 = -1;
		iNode1 = -1;
		iNode2 = -1;
		TD = 0;
		Z0 = 1;
		label = "NOTHING";
	}
};

/* Inductor Branch Relation Class */
class inductor_br
{
public:
	int index, current1RowIndex, current1ColumnIndex;
	std::vector<int> current2RowIndex, current2ColumnIndex;
	double inductance;
	std::vector<double> mutualInductance;
	std::string mutI1;
	std::vector<std::string> mutI2;
};

/* A Matrix Class */
class A_matrix
{
public:
	std::unordered_map<std::string, std::unordered_map<std::string, double>>
		bMatrixConductanceMap;
	std::unordered_map<std::string, double> inductanceMap;
	std::unordered_map<std::string, std::vector<std::string>>
		nodeConnections;
	std::vector<matrix_element> mElements;
	std::vector<element> elements;
	std::vector<std::string> rowNames, columnNames;
	std::vector<double> nzval;
	std::vector<int> colind, rowptr;
	std::unordered_map<std::string, std::vector<double>> sources;
	std::unordered_map<std::string, model_xline> xlines;
	std::unordered_map<std::string, inductor_br> branchRelations;
	int Nsize, Msize;
	A_matrix() {
		bMatrixConductanceMap.clear();
		inductanceMap.clear();
		nodeConnections.clear();
		mElements.clear();
		elements.clear();
		rowNames.clear();
		columnNames.clear();
		nzval.clear();
		colind.clear();
		rowptr.clear();
		sources.clear();
		xlines.clear();
	};
};

/* RCSJ Model */
class model_rcsj
{
public:
	std::string rtype, cct, vg, delv, icon, rzero, rnormal, cap, icrit;
	// Default RCSJ model constructor
	model_rcsj()
	{
		rtype = "0";
		cct = "0.0";
		vg = "2.8E-3";
		delv = "0.1E-3";
		icon = "1E-3";
		rzero = "30";
		rnormal = "5";
		cap = "2.5E-12";
		icrit = "1E-3";
	}
};

/* MTJ Model */
class model_mtj
{
public:
	std::string modelname;
};

/* NTRON Model */
class model_ntron
{
public:
	std::string modelname;
};

/* NTRON Model */
class model_cshe
{
public:
	std::string modelname;
};

/* Model object */
class Model
{
public:
	std::string modelname;
	int modelType = UTYPE;
	model_rcsj jj;
	model_mtj mtj;
	model_ntron ntron;
	model_cshe cshe;
};

/* Subcircuit object */
class Subcircuit
{
public:
	std::string name;
	std::vector<std::string> io;
	std::vector<std::string> lines;
	std::vector<std::string> subckts;
	int componentCount = 0,
		jjCount = 0;
	std::unordered_map<std::string, double> parVal;
	std::unordered_map<std::string, Model> subcktModels;
	bool containsSubckt = false;
};

/* Transient analysis simulation object*/
class trans_sim
{
public:
	double prstep;
	double tstop;
	double tstart;
	double maxtstep;
	trans_sim()
	{
		tstart = 0.0;
		tstop = 0.0;
		prstep = 1E-12;
		maxtstep = 1E-12;
	}
	double simsize() { return (tstop - tstart) / prstep; }
};

/*
  Model parsing. Split a model line into parameters
*/
void parse_model(std::string s, std::unordered_map<std::string, Model>& m);
/*
  Recursive function, for subcircuit traversal
*/
std::vector<std::string> recurseSubckt(std::unordered_map<std::string, Subcircuit> subckts, std::string part);

/* Input File Object Class */
class InputFile
{
	std::vector<std::string> lines;

public:
	trans_sim tsim;
	std::vector<std::string> maincircuitSegment, controlPart, subckts;
	std::unordered_map<std::string, std::vector<std::string>> subcircuitModels;
	std::unordered_map<std::string, Subcircuit> subcircuitSegments;
	std::unordered_map<std::string, Model> mainModels;
	std::unordered_map<std::string, int> subCircuitComponentCount,
		subCircuitJJCount, subCircuitContainsSubCircuit;
	std::unordered_map<std::string, double> parVal;
	std::unordered_map<std::string, std::string> subcircuitNameMap;
	A_matrix matA;
	std::vector<std::vector<double>> xVect;
	std::vector<double> timeAxis;
	std::unordered_map<std::string, std::vector<double>> junctionCurrents;
	std::vector<std::string> mutualInductanceLines;
	int subCircuitCount = 0,
		jjCount = 0,
		componentCount = 0,
		allCounted = 0,
		circuitComponentCount = 0,
		circuitJJCount = 0,
		mainJJs = 0,
		mainComponents = 0;
	int simulationType = 4;
	/*
	  Input File Constructor
	*/
	InputFile(std::string iFileName);
	/*
	  Split the circuit into subcircuits, main circuit and identify circuit stats
	*/
	void circuit_to_segments(InputFile& iFile);
	/*
		  Substitute subcircuits into the main circuit to create a full main
	   circuit
	*/
	void sub_in_subcircuits(InputFile& iFile,
		std::vector<std::string>& segment,
		std::string label = "");
};
#endif