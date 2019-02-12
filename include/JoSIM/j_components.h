// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef J_COMPONENTS_H
#define J_COMPONENTS_H
#include "j_std_include.h"
#include "j_input.h"
#include "j_misc.h"
#include "j_errors.h"
#include "j_parser.h"

class device {
	public:
		std::string label;
		std::string posNodeR, posNodeC;
		std::string negNodeR, negNodeC;
		int posNRow, posNCol;
		int negNRow, negNCol;
		int ppPtr, nnPtr, pnPtr, npPtr;
		double value;
		device() {
			label = "NOTHING";
			posNodeR = posNodeC = "NONE";
			negNodeR = negNodeC = "NONE";
			posNRow = posNCol = -1;
			negNRow = negNCol = -1;
			ppPtr = nnPtr = pnPtr = npPtr = -1;
			value = 0;
		}
};

class res_volt : public device {
};

class ind_volt : public device {
	public:
		std::string curNodeR, curNodeC;
		int curNRow, curNCol;
		int indPtr;
		std::unordered_map<std::string, double> mut;
		std::unordered_map<std::string, int> mutPtr;
		ind_volt() {
			curNodeR = curNodeC = "NONE";
			curNRow = curNCol = -1;
			indPtr = -1;
		}
};

class cap_volt : public device {
	public:
		std::string curNodeR, curNodeC;
		int curNRow, curNCol;
		int capPtr;
		cap_volt() {
			curNodeR = curNodeC = "NONE";
			curNRow = curNCol = -1;
			capPtr = -1;
		}
};

class jj_volt : public device {
	public:
		std::string phaseNodeR, phaseNodeC;
		int phaseNRow, phaseNCol;
		double phi0, pn1, pn2, dPn1, dPn2, v0, vn1, vn2, dVn1, dVn2;
		double r0, rN, C, iC, iCFact, delV, vG, iS, iT, gLarge, lowerB, 
		   upperB, subCond, transCond, normalCond, T, tC, Del0, Del, D, rNCalc;
		int rType;
		std::vector<double> jjCur;
		jj_volt() {
			phi0 = pn1 = pn2 = dPn1 = dPn2 = 0.0;
			v0 = vn1 = vn2 = dVn1 = dVn2 = 0.0;
			rType = 1;
			vG = 2.8E-3;
			delV = 0.1E-3;
			r0 = 30;
			rN = 5;
			C = 2.5E-12;
			iC = 1E-3;
			iCFact = M_PI/4;
			iT = iS = gLarge = 0.0;
			T = 4.2;
			tC = 9.1;
			D = 0.0;
		}
};

class tx_line : public device {
	public:
		std::string posNode2R, posNode2C;
		std::string negNode2R, negNode2C;
		std::string curNode1R, curNode1C;
		std::string curNode2R, curNode2C;
		int posN2Row, posN2Col;
		int negN2Row, negN2Col;
		int curN1Row, curN1Col;
		int curN2Row, curN2Col;
		int k;
		double tD;
		tx_line() {
			posNode2R = posNode2C = "NONE";
			negNode2R = negNode2C = "NONE";
			curNode1R = curNode1C = "NONE";
			curNode2R = curNode2C = "NONE";
			posN2Row = posN2Col = -1;
			negN2Row = negN2Col = -1;
			curN1Row = curN1Col = -1;
			curN2Row = curN2Col = -1;
			k = 0;
			tD = 0;
		}
};

class vs_volt : public device {
	public:
		std::string curNodeR, curNodeC;
		int curNRow, curNCol;
		vs_volt() {
			curNodeR = curNodeC = "NONE";
			curNRow = curNCol = -1;
		}
};

class res_phase : public device {
	public: 
		std::string curNodeR, curNodeC;
		int curNRow, curNCol;
		int resPtr;
		double IRn1, pn1;
		res_phase() {
			curNodeR = curNodeC = "NONE";
			curNRow = curNCol = -1;
			resPtr = -1;
			IRn1 = pn1 = 0;
		}
};

class ind_phase : public device {
	public:
		std::string curNodeR, curNodeC;
		int curNRow, curNCol;
		int indPtr, mutPtr;
		ind_phase() {
			curNodeR = curNodeC = "NONE";
			curNRow = curNCol = -1;
			indPtr = mutPtr = -1;
		}
};

class cap_phase : public device {
	public:
		std::string curNodeR, curNodeC;
		int curNRow, curNCol;
		int capPtr;
		double ICn1, pn1, pn2, dPn1, dPn2;
		cap_phase() {
			curNodeR = curNodeC = "NONE";
			curNRow = curNCol = -1;
			capPtr = -1;
			ICn1 = pn1 = pn2 = dPn1 = dPn2 = 0.0;
		}
};

class jj_phase : public device {
	public:
		std::string voltNodeR, voltNodeC;
		int voltNRow, voltNCol;
		int pPtr, nPtr;
		double phi0, Phi0n1, pn1, pn2, dPn1, dPn2, v0, vn1, vn2, dVn1, dVn2;
		int rType;
		double r0, rN, C, iC, iCFact, delV, vG, iS, It, gLarge, 
			lower, upper, subCond, transCond, normalCond, 
			T, tC, Del0, Del, D, rNCalc;
		std::vector<double> jjCur;
		jj_phase() {
			voltNodeR = voltNodeC = "NONE";
			voltNRow = voltNCol = -1;
			pPtr = nPtr = -1;
			phi0 = Phi0n1 = pn1 = pn2 = dPn1 = dPn2 = v0 = vn1 = vn2 = dVn1 = dVn2 = 0.0;
			rType = 0;
			vG = 2.8E-3;
			delV = 0.1E-3;
			r0 = 30;
			rN = 5;
			iS = 0.0;
			C = 2.5E-12;
			iC = 1E-3;
			iCFact = M_PI/4;
			It = 0.0;
			gLarge = 0.0;
			T = 4.2;
			tC = 9.1;
			D = 0.0;
		}
};

class vs_phase : public device {
	public:
		std::string curNodeR, curNodeC;
		int curNRow, curNCol;
		double pn1;
		vs_phase() {
			curNodeR = curNodeC = "NONE";
			curNRow = curNCol = -1;
			pn1 = 0.0;
		}
};

class ps_phase : public device {
	public:
		std::string curNodeR, curNodeC;
		int curNRow, curNCol;
		ps_phase() {
			curNodeR = curNodeC = "NONE";
			curNRow = curNCol = -1;
		}
};

class tx_phase : public tx_line {
	public: 
		double p1n1, p1n2, p1nk, p1nk1, dP1n1, dP1n2, dP1nk, dP1nk1,
			p2n1, p2n2, p2nk, p2nk1, dP2n1, dP2n2, dP2nk, dP2nk1;
		tx_phase() {
			p1n1 = p1n2 = p1nk = p1nk1 = dP1n1 = dP1n2 = dP1nk = dP1nk1 = 0.0;
			p2n1 = p2n2 = p2nk = p2nk1 = dP2n1 = dP2n2 = dP2nk = dP2nk1 = 0.0;
		}
};

class Components {
	public:
		std::unordered_map<std::string, res_phase> phaseRes;
		std::unordered_map<std::string, ind_phase> phaseInd;
		std::unordered_map<std::string, cap_phase> phaseCap;
		std::unordered_map<std::string, jj_phase> phaseJJ;
		std::unordered_map<std::string, vs_phase> phaseVs;
		std::unordered_map<std::string, ps_phase> phasePs;

		std::unordered_map<std::string, res_volt> voltRes;
		std::unordered_map<std::string, ind_volt> voltInd;
		std::unordered_map<std::string, cap_volt> voltCap;
		std::unordered_map<std::string, jj_volt> voltJJ;
		std::unordered_map<std::string, vs_volt> voltVs;

		std::unordered_map<std::string, tx_line> txLine;
		std::unordered_map<std::string, tx_phase> txPhase;

		std::vector<std::pair<std::string, std::string>> mutualInductanceLines;

		Components() {};

		void
		jj_model(std::string &modelstring, std::string &area, std::string &jjLabel, Input &iObj, std::string subckt = "");

		void
		jj_model_phase(std::string &modelstring, std::string &area, std::string &jjLabel, Input &iObj, std::string subckt = "");
};
#endif