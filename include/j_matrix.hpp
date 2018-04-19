#pragma once
#include "j_std_include.hpp"

/*
  Matrix element class
*/
class matrix_element {
public:
	std::string label;
	int rowIndex;
	int columnIndex;
	double value;
	/* Default matrix element constructor */
	matrix_element() {
		label = "NOTHING";
		rowIndex = -1;
		columnIndex = -1;
		value = 0.0;
	}
};

class element {
public:
	std::string label;
	int VPindex;
	int VNindex;
	int CURindex;
	double value;
	element() {
		label = "NOTHING";
		VPindex = -1;
		VNindex = -1;
		CURindex = -1;
		value = 0.0;
	}
};

class model_xline {
public:
    std::string label;
    double TD;
    double Z0;
    int pNode1, pNode2, nNode1, nNode2, iNode1, iNode2;
    /* Default xline constructor */
    model_xline() {
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

extern std::unordered_map<std::string, std::unordered_map<std::string, double>> bMatrixConductanceMap;
extern std::unordered_map<std::string, double> inductanceMap;
extern std::unordered_map<std::string, model_xline> xlines;
extern std::unordered_map<std::string, std::vector<std::string>> nodeConnections;
extern std::vector<matrix_element> mElements;
extern std::vector<element> elements;
extern std::vector<std::string> rowNames, columnNames;
extern std::vector<double> nzval;
extern std::vector<int> colind;
extern std::vector<int> rowptr;
extern std::unordered_map<std::string, std::vector<double>> sources;
extern int Nsize;
extern int Msize;
/*
  Systematically create A matrix
*/
void matrix_A(InputFile & iFile);
/*
Create the A matrix in Compressed Row Storage (CRS) format
*/
void create_A_matrix(InputFile& iFile);
/*
Print A matrix
*/
void print_A_matrix();
