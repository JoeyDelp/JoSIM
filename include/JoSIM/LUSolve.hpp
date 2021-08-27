// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_LUSOLVE_H
#define JOSIM_LUSOLVE_H

#include <superlu/slu_ddefs.h>

#include <vector>

namespace JoSIM {

  class LUSolve {
    private:
    char equed[1] = { 'N' };
    yes_no_t equil;
    trans_t trans;
    SuperMatrix A, L, U, B, X;
    GlobalLU_t Glu;

    std::vector<int64_t> perm_r, perm_c;
    std::vector<int64_t> etree;
    void* work = nullptr;
    int64_t info, lwork, nrhs, ldx;
    int64_t i, m, n, nnz;
    std::vector<double> rhsb, rhsx, xact;
    std::vector<double> R, C;
    std::vector<double> ferr, berr;
    double u, rpg, rcond;
    mem_usage_t mem_usage;
    superlu_options_t options;
    SuperLUStat_t stat;
    bool allocated = false;
    bool constructed = false;
    
    public:
    LUSolve();
    void create_matrix(int shape, std::vector<double>& nz,
      std::vector<int64_t>& ci, std::vector<int64_t>& rp);
    void factorize(bool symbolic = false);
    bool is_stable();
    void solve(std::vector<double>& x);
    void free();
  };

}

#endif