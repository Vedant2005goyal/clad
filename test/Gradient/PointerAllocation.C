// RUN: %cladclang -Xclang -plugin-arg-clad -Xclang -disable-tbr %s -I%S/../../include -oPointerAllocation.out
// RUN: ./PointerAllocation.out | %filecheck_exec %s

// Assigning a `new` expression to a pointer must allocate the adjoint buffer
// too. StmtClone had no case for CXXNewExpr and fabricated a type-less node,
// so the adjoint assignment was dropped and the adjoint pointer stayed null.

#include "clad/Differentiator/Differentiator.h"
#include <cstdio>

double allocInBody(double x) {
  double* p = nullptr;
  p = new double[2];
  double* owner = p;
  p[0] = x * x;
  double r = p[0];
  delete[] owner;
  return r;
}

struct Buffer {
  int     n_;
  double* d_;
  Buffer(int k) : n_(k) { d_ = new double[k]; }
};

double allocInCtor(double x) {
  Buffer b(2);
  b.d_[0] = x * x * x;
  double r = b.d_[0];
  delete[] b.d_;
  return r;
}

int main() {
  double dx = 0;
  clad::gradient<clad::opts::disable_tbr>(allocInBody).execute(3, &dx);
  printf("{%.2f}\n", dx); // CHECK-EXEC: {6.00}

  dx = 0;
  clad::gradient<clad::opts::disable_tbr>(allocInCtor).execute(2, &dx);
  printf("{%.2f}\n", dx); // CHECK-EXEC: {12.00}
}
