// RUN: %cladclang -Xclang -plugin-arg-clad -Xclang -disable-tbr %s -I%S/../../include -oBracelessBranch.out 2>&1 | %filecheck %s

// A braceless branch whose body carries no derivative (`delete[] p;`) used to
// leave the generated IfStmt with a null `then`. Sema accepts such a node, but
// clang walks into it and crashes -- in CodeGen, and again when printing the
// derivative. Compiling all the way to an executable is the point of this
// test; it does not need to run.

#include "clad/Differentiator/Differentiator.h"

struct Buffer {
  bool    own_ = false;
  int     n_   = 0;
  double* d_   = nullptr;
  void    release(int k) {
    if (n_ != k) {
      if (own_)
        delete[] d_;
      n_ = k;
    }
  }
};

double fn(double x) {
  double storage[2] = {x * x, 0};
  Buffer b;
  b.d_ = storage;
  b.release(2);
  return b.d_[0];
}

// CHECK: inline void release_reverse_forw(int k, Buffer *_d_this, int _d_k, clad::restore_tracker &{{.*}}) {

int main() { clad::gradient<clad::opts::disable_tbr>(fn); }
