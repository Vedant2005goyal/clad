// RUN: %cladclang -std=c++17 -I%S/../../include %s -o %t
// RUN: %t | %filecheck_exec %s
#include <cstdio>

#if defined(__clang__) && (__clang_major__ > 16)
#include "clad/Differentiator/Differentiator.h"

void f2(double i, double j){
    auto _f = [] () {
    {
      double a = 1;
    }
  };
}
#endif
int main(){
    #if defined(__clang__) && (__clang_major__ > 16)
    auto diff=clad::gradient(f2);
    double di=0,dj=0;
    double i=7,j=7;
    diff.execute(i,j,&di,&dj);
    #endif
    
    printf("Execution successful\n");
    // CHECK-EXEC: Execution successful
    return 0;
}