// RUN: %cladclang -I%S/../../include %s
// Test for issue #1442: Segmentation fault when assert macros are used in clad

#include <cassert>
#include "clad/Differentiator/Differentiator.h"

void calcViscFluxSide(int, bool) { 
  // Original issue: assert macro expands to __builtin_FILE() etc.
  assert(true);
  
  // Also test direct usage of __builtin_FILE() which caused secondary crash  
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