// RUN: %cladclang -I%S/../../include %s

#include "clad/Differentiator/Differentiator.h"
#include <cassert>

void calcViscFluxSide(int, bool) { 
  assert(0); 
}

void a(bool c) { 
  calcViscFluxSide(0, c); 
}

int main() {
  // This used to cause a segmentation fault due to null QualType
  // from SourceLocExpr (__builtin_FILE()) in assert macro expansion
  auto grad_a = clad::gradient(a);
  
  return 0;
}