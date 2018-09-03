// Copyright (c) 2018 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef J_MATRIX_H
#define J_MATRIX_H
#include "j_std_include.hpp"

/*
  Systematically create A matrix
*/
void
matrix_A(InputFile& iFile);
/*
 Identify each non zero matrix element
*/
void
create_A_matrix(InputFile& iFile);
/*
  Create A matrix in CSR format
*/
void csr_A_matrix(InputFile& iFile);
#endif