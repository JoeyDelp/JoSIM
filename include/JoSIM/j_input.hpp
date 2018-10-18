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

/* Phase Resistor Element */
class phase_Resistor {
	public: 
	// String identifiers for node/branch rows and columns
	std::string pPositiveR, pNegativeR, bIResistorR;
	std::string pPositiveC, pNegativeC, bIResistorC;
	// Index identifier for node/branch rows and columns
	int pPR, pNR, bIR;
	int pPC, pNC, bIC;
	// Resistor matrix entry index
	int Rmptr;
	// RHS variables
	double IRn1, Pn1;
	// Characteristics
	double R;
	phase_Resistor() {
		pPositiveR = pNegativeR = bIResistorR = "NONE";
		pPositiveC = pNegativeC = bIResistorC = "NONE";
		pPR = pNR = bIR = -1;
		pPC = pNC = bIC = -1;
		Rmptr = -1;
		IRn1 = Pn1 = 0;
		R = 1;
	}
};

/* Phase Inductor element */
class phase_Inductor {
	public:
	//String identifiers for node/branch rows and columns
	std::string pPositiveR, pNegativeR, bIInductorR;
	std::string pPositiveC, pNegativeC, bIInductorC;
	// Index identifier for node/branch rows and columns
	int pPR, pNR, bIR;
	int pPC, pNC, bIC;
	// Inductor and Mutual matrix entry index
	int Lmptr, Mmptr;
	// Characteristics
	double L;
	phase_Inductor() {
		pPositiveR = pNegativeR = bIInductorR = "NONE";
		pPositiveC = pNegativeC = bIInductorC = "NONE";
		pPR = pNR = bIR = -1;
		pPC = pNC = bIC = -1;
		Lmptr = Mmptr = -1;
		L = 1;
	}
};

/* Phase Capacitor element */
class phase_Capacitor {
	public:
	// String identifiers for node/branch rows and columns
	std::string pPositiveR, pNegativeR, bICapacitorR;
	std::string pPositiveC, pNegativeC, bICapacitorC;
	// Index identifier for node/branch rows and columns
	int pPR, pNR, bIR;
	int pPC, pNC, bIC;
	// Capacitor matrix entry index
	int Cmptr;
	// RHS variables
	double ICn1, Pn1, Pn2, dPn1, dPn2;
	// Characteristics
	double C;
	phase_Capacitor() {
		pPositiveR = pNegativeR = bICapacitorR = "NONE";
		pPositiveC = pNegativeC = bICapacitorC = "NONE";
		pPR = pNR = bIR = -1;
		pPC = pNC = bIC = -1;
		Cmptr = -1;
		ICn1 = Pn1 = Pn2 = dPn1 = dPn2 = 0.0;
		C = 1;
	}
};

/* Phase Junction Element */
class phase_JJ {
	public:
	// String identifiers for node/branch rows and columns
	std::string pPositiveR, pNegativeR, vRow, bIResistorR, bICapacitorR;
	std::string pPositiveC, pNegativeC, vColumn, bIResistorC, bICapacitorC;
	// Index identifier for node/branch rows and columns
	int pPR, pNR, vR, bIRR, bICR;
	int pPC, pNC, vC, bIRC, bICC;
	// Resistor matrix entry index
	int RmptrP, RmptrN;
	// RHS variables
	double Phi0, Phi0n1, Pn1, Pn2, dPn1, dPn2, V0, Vn1, Vn2, dVn1, dVn2;
	// Model type
	int Rtype;
	// Characteristics
	double R0, RN, C, Ic, IcFact, DelV, Vg, Is, It, gLarge, lower, upper, subCond, transCond, normalCond;
	// Ic sin pi0 storage vector
	std::vector<double> junctionCurrent;
	phase_JJ() {
		RmptrP = RmptrN = -1;
		pPositiveR = pNegativeR = vRow = bIResistorR = bICapacitorR = "NONE";
		pPositiveC = pNegativeC = vColumn = bIResistorC = bICapacitorC = "NONE";
		pPR = pNR = vR = bIRR = bICR = -1;
		pPC = pNC = vC = bIRC = bICC = -1;
		Phi0 = Phi0n1 = Pn1 = Pn2 = dPn1 = dPn2 = V0 = Vn1 = Vn2 = dVn1 = dVn2 = 0.0;
		Rtype = 0;
		Vg = 2.8E-3;
		DelV = 0.1E-3;
		R0 = 30;
		RN = 5;
		Is = 0.0;
		C = 2.5E-12;
		Ic = 1E-3;
		IcFact = M_PI/4;
		It = 0.0;
		gLarge = 0.0;
	}
};

/* Phase Transmission Line Element */
class phase_TL {
	public:
	// String identifiers for node/branch rows and columns
	std::string pPositiveR1, pPositiveR2, pNegativeR1, pNegativeR2, bCurrentR1, bCurrentR2;
	std::string pPositiveC1, pPositiveC2, pNegativeC1, pNegativeC2, bCurrentC1, bCurrentC2;
	// Index identifier for node/branch rows and columns
	int pPR1, pPR2, pNR1, pNR2, bIR1, bIR2;
	int pPC1, pPC2, pNC1, pNC2, bIC1, bIC2;
	// Variable identifiers
	double Z0, TD;
	int k;
	// Characteristics
	double P1n1, P1n2, P1nk, P1nk1, P1nk2, dP1n1, dP1n2, dP1nk, dP1nk1;
	double P2n1, P2n2, P2nk, P2nk1, P2nk2, dP2n1, dP2n2, dP2nk, dP2nk1;
	double I1nk, I2nk;
	phase_TL() {
		pPositiveR1 = pPositiveR2 = pNegativeR1 = pNegativeR2 = bCurrentR1 = bCurrentR2 = "NONE";
		pPositiveC1 = pPositiveC2 = pNegativeC1 = pNegativeC2 = bCurrentC1 = bCurrentC2 = "NONE";
		pPR1 = pPR2 = pNR1 = pNR2 = bIR1 = bIR2 = -1;
		pPC1 = pPC2 = pNC1 = pNC2 = bIC1 = bIC2 = -1;
		P1n1 = P1n2 = P1nk = P1nk1 = P1nk2 = dP1n1 = dP1n2 = dP1nk = dP1nk1 = 0.0;
		P2n1 = P2n2 = P2nk = P2nk1 = P2nk2 = dP2n1 = dP2n2 = dP2nk = dP2nk1 = 0.0;
		I1nk = I2nk = 0.0;
		Z0 = 50;
		TD = 1;
		k = 1000;
	}
};

/* Phase Voltage Source element */
class phase_VS {
	public:
	// String identifiers for node/branch rows and columns
	std::string pPositiveR, pNegativeR, bIVoltR;
	std::string pPositiveC, pNegativeC, bIVoltC;
	// Index identifier for node/branch rows and columns
	int pPR, pNR, bIR;
	int pPC, pNC, bIC;
	// Characteristics
	double Pn1;
	phase_VS() {
		pPositiveR = pNegativeR = bIVoltR = "NONE";
		pPositiveC = pNegativeC = bIVoltC = "NONE";
		pPR = pNR = bIR = -1;
		pPC = pNC = bIC = -1;
		Pn1 = 0.0;
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
	std::unordered_map<std::string, double> impedanceMap;
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
		impedanceMap.clear();
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
	std::string rtype, cct, vg, delv, icon, rzero, rnormal, cap, icrit, icfact;
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
		icfact = std::to_string(M_PI / 4);
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
	std::unordered_map<std::string, phase_JJ> pJJ;
	std::unordered_map<std::string, phase_TL> pTL;
	std::unordered_map<std::string, phase_Resistor> pRes;
	std::unordered_map<std::string, phase_Inductor> pInd;
	std::unordered_map<std::string, phase_Capacitor> pCap;
	std::unordered_map<std::string, phase_VS> pVS;
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