// RUN: %cladclang -I%S/../../include %s

#include "clad/Differentiator/Differentiator.h"

void calcViscFluxSide(int, bool) { 
  // Reproduce the issue using __builtin_FILE() directly  
  const char* file = __builtin_FILE();
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