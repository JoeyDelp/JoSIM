// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef J_MATRIX_H_
#define J_MATRIX_H_
#include "j_components.h"
#include "j_input.h"
#include "j_errors.h"

class matrix_element {
    public:
      std::string label;
      int rowIndex;
      int colIndex;
      double value;
      matrix_element() {
          label = "NOTHING";
          rowIndex = -1;
          colIndex = -1;
          value = 0.0;
      }
};

class Matrix {
    public:
      Components components;
      std::unordered_map<std::string, std::vector<std::string>> nodeConnections;
      std::vector<matrix_element> mElements;
      std::unordered_map<std::string, std::vector<double>> sources;
      std::vector<std::string> rowNames, columnNames;
      std::vector<double> nzval;
	    std::vector<int> colind, rowptr;
      int Nsize, Msize;

      Matrix() {};
      void
      create_matrix(Input &iObj);

      void
      create_A_volt(Input &iObj);

      void
      create_A_phase(Input &iObj);

      void
      create_CSR();
};
#endif