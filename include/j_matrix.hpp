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


extern std::map<std::string, std::map<std::string, double>> bMatrixConductanceMap;
extern std::vector<matrix_element> mElements;
extern std::vector<element> elements;
extern std::vector<std::string> rowNames, columnNames;
extern std::vector<double> nzval;
extern std::vector<int> colind;
extern std::vector<int> rowptr;
extern std::map<std::string, std::vector<double>> sources;
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