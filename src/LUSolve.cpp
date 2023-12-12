// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#ifdef SLU
#include "JoSIM/LUSolve.hpp"

using namespace JoSIM;

LUSolve::LUSolve() {
  lwork = 0;
  nrhs = 1;
  trans = TRANS;
  set_default_options(&options);
  options.Equil = NO;
  options.Trans = trans;
}

void LUSolve::create_matrix(int64_t shape, std::vector<double>& nz,
                            std::vector<long long>& ci,
                            std::vector<long long>& rp) {
  bool do_allocation =
      (allocated == false || m != shape || n != shape || nnz != nz.size());
  m = n = shape;
  nnz = nz.size();
  if (do_allocation) {
    dCreate_CompCol_Matrix(&A, m, n, nnz, &nz.front(), &ci.front(), &rp.front(),
                           SLU_NC, SLU_D, SLU_GE);
    if (!(rhsb = doubleMalloc(m * nrhs))) ABORT("Malloc fails for rhsb[].");
    if (!(rhsx = doubleMalloc(m * nrhs))) ABORT("Malloc fails for rhsx[].");
    dCreate_Dense_Matrix(&B, m, nrhs, rhsb, m, SLU_DN, SLU_D, SLU_GE);
    dCreate_Dense_Matrix(&X, m, nrhs, rhsx, m, SLU_DN, SLU_D, SLU_GE);
  }
  xact = doubleMalloc(n * nrhs);
  ldx = n;
  dGenXtrue(n, nrhs, xact, ldx);
  dFillRHS(trans, nrhs, xact, ldx, &A, &B);
  if (!(etree = intMalloc(n))) ABORT("Malloc fails for etree[].");
  if (!(perm_r = intMalloc(m))) ABORT("Malloc fails for perm_r[].");
  if (!(perm_c = intMalloc(n))) ABORT("Malloc fails for perm_c[].");
  if (!(R = (double*)SUPERLU_MALLOC(A.nrow * sizeof(double))))
    ABORT("SUPERLU_MALLOC fails for R[].");
  if (!(C = (double*)SUPERLU_MALLOC(A.ncol * sizeof(double))))
    ABORT("SUPERLU_MALLOC fails for C[].");
  if (!(ferr = (double*)SUPERLU_MALLOC(nrhs * sizeof(double))))
    ABORT("SUPERLU_MALLOC fails for ferr[].");
  if (!(berr = (double*)SUPERLU_MALLOC(nrhs * sizeof(double))))
    ABORT("SUPERLU_MALLOC fails for berr[].");
  StatInit(&stat);
  allocated = true;
}

bool LUSolve::is_stable() {
  return ((info > 0 || rcond < 1e-8 || rpg > 1e8) == false);
}

void LUSolve::factorize(bool symbolic) {
  options.Fact = symbolic ? SamePattern_SameRowPerm : DOFACT;
  dgssvx(&options, &A, perm_c, perm_r, etree, equed, R, C, &L, &U, work, lwork,
         &B, &X, &rpg, &rcond, ferr, berr, &Glu, &mem_usage, &stat, &info);
  options.Fact = FACTORED;
  constructed = true;
}

void LUSolve::solve(std::vector<double>& x) {
  if (!constructed) {
    ABORT("Preconditioner not constructed.");
  }
  DNformat LHSstore = {static_cast<int_t>(x.size()), &x.front()};
  SuperMatrix LHSmat = {SLU_DN, SLU_D,    SLU_GE, static_cast<int_t>(x.size()),
                        1,      &LHSstore};
  dgstrs(trans, &L, &U, perm_c, perm_r, &LHSmat, &stat, &info);
}

void LUSolve::free() {
  if (allocated) {
    SUPERLU_FREE(rhsb);
    SUPERLU_FREE(rhsx);
    SUPERLU_FREE(xact);
    SUPERLU_FREE(etree);
    SUPERLU_FREE(R);
    SUPERLU_FREE(C);
    SUPERLU_FREE(perm_c);
    SUPERLU_FREE(perm_r);
    Destroy_SuperMatrix_Store(&A);
    Destroy_SuperMatrix_Store(&B);
    Destroy_SuperMatrix_Store(&X);
    SUPERLU_FREE(ferr);
    SUPERLU_FREE(berr);
  }
  if (constructed) {
    constructed = false;
    Destroy_SuperNode_Matrix(&L);
    Destroy_CompCol_Matrix(&U);
  }
}
#endif