// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/LUSolve.hpp"

using namespace JoSIM;

LUSolve::LUSolve() {
  lwork = 0;
  nrhs = 1;
  //equil = YES;
  //u = 1.0;
  trans = TRANS;
  set_default_options(&options);
  options.Equil = NO;
  options.Trans = trans;
}

void LUSolve::create_matrix(int shape, std::vector<double>& nz,
  std::vector<int>& ci, std::vector<int>& rp) {
  bool do_allocation = (allocated == false || m != shape ||
    n != shape || nnz != nz.size());
  m = n = shape;
  nnz = nz.size();
  if (do_allocation) {
    dCreate_CompCol_Matrix(&A, m, n, nnz, &nz.front(), &ci.front(), 
      &rp.front(), SLU_NC, SLU_D, SLU_GE);
    rhsb.resize(m * nrhs);
    rhsx.resize(m * nrhs);
    dCreate_Dense_Matrix(&B, m, nrhs, &rhsb.front(), m, SLU_DN, SLU_D, SLU_GE);
    dCreate_Dense_Matrix(&X, m, nrhs, &rhsx.front(), m, SLU_DN, SLU_D, SLU_GE);
  }
  xact.resize(n * nrhs);
  ldx = n;
  dGenXtrue(n, nrhs, &xact.front(), ldx);
  dFillRHS(trans, nrhs, &xact.front(), ldx, &A, &B);
  etree.resize(n);
  perm_r.resize(m);
  perm_c.resize(n);
  R.resize(A.nrow);
  C.resize(A.ncol);
  ferr.resize(nrhs);
  berr.resize(nrhs);
  StatInit(&stat);
  allocated = true;
}

bool LUSolve::is_stable() {
  return ((info > 0 || rcond < 1e-8 || rpg > 1e8) == false);
}

void LUSolve::factorize(bool symbolic) {
  options.Fact = symbolic ? SamePattern_SameRowPerm : DOFACT;
  dgssvx(&options, &A, &perm_c.front(), &perm_r.front(), &etree.front(), equed, 
    &R.front(), &C.front(), &L, &U, work, lwork, &B, &X, &rpg, &rcond, 
    &ferr.front(), &berr.front(), &Glu, &mem_usage, &stat, &info);
  options.Fact = FACTORED;
  constructed = true;
}

void LUSolve::solve(std::vector<double>& x) {
  if (!constructed) {
    ABORT("Preconditioner not constructed.");
  }
  DNformat LHSstore = { x.size(), &x.front() };
  SuperMatrix LHSmat = { SLU_DN, SLU_D, SLU_GE, x.size(), 1, &LHSstore };
  dgstrs(trans, &L, &U, &perm_c.front(), &perm_r.front(), &LHSmat, &stat, 
    &info);
}

void LUSolve::free() {
  if (allocated) {
    //SUPERLU_FREE(rhsb);
    //SUPERLU_FREE(rhsx);
    //SUPERLU_FREE(xact);
    //SUPERLU_FREE(etree);
    //SUPERLU_FREE(R);
    //SUPERLU_FREE(C);
    //SUPERLU_FREE(perm_c);
    //SUPERLU_FREE(perm_r);
    Destroy_SuperMatrix_Store(&A);
    Destroy_SuperMatrix_Store(&B);
    Destroy_SuperMatrix_Store(&X);
    //SUPERLU_FREE(ferr);
    //SUPERLU_FREE(berr);
  }
  if (constructed) {
    constructed = false;
    Destroy_SuperNode_Matrix(&L);
    Destroy_CompCol_Matrix(&U);
  }
}