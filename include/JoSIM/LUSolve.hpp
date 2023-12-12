// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifdef SLU
#ifndef JOSIM_LUSOLVE_H
#define JOSIM_LUSOLVE_H

#include <superlu/slu_ddefs.h>

#include <vector>

namespace JoSIM {

class LUSolve {
 private:
  char equed[1] = {'N'};
  trans_t trans;
  SuperMatrix A, L, U, B, X;
  GlobalLU_t Glu;

  long long *perm_r, *perm_c;
  long long* etree;
  void* work = nullptr;
  long long info, lwork, nrhs, ldx;
  long long m, n, nnz;
  double *rhsb, *rhsx, *xact;
  double *R, *C;
  double *ferr, *berr;
  double rpg, rcond;
  mem_usage_t mem_usage;
  superlu_options_t options;
  SuperLUStat_t stat;
  bool allocated = false;
  bool constructed = false;

 public:
  LUSolve();
  void create_matrix(int64_t shape, std::vector<double>& nz,
                     std::vector<long long>& ci, std::vector<long long>& rp);
  void factorize(bool symbolic = false);
  bool is_stable();
  void solve(std::vector<double>& x);
  void free();
};

}  // namespace JoSIM

#endif
#endif