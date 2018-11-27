// Copyright (c) 2018 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef J_MATRIX_H_
#define J_MATRIX_H_
#include "j_std_include.h"

class Matrix {
  public:
    /*
      Systematically create A matrix
    */
    static
    void
    matrix_A(InputFile& iFile);
    /*
    Identify each non zero matrix element in voltage form
    */
    static
    void
    create_A_matrix_volt(InputFile& iFile);
    /*
    Identify each non zero matrix element in phase form
    */
    static
    void
    create_A_matrix_phase(InputFile& iFile);
    /*
      Create A matrix in CSR format
    */
    static
    void 
    csr_A_matrix(InputFile& iFile);
};
#endif