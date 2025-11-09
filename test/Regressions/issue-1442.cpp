// RUN: %cladclang -I%S/../../include %s
// Test for issue #1442: Segmentation fault when assert macros are used in clad

#include "clad/Differentiator/Differentiator.h"

// Standalone assert-like macro that reproduces the original issue
// This mimics the behavior of standard assert which uses __builtin_FILE() etc.
#define ASSERT_LIKE(expr) \
  do { \
    if (!(expr)) { \
      const char* file = __builtin_FILE(); \
      int line = __builtin_LINE(); \
    } \
  } while(0)

void calcViscFluxSide(int, bool) { 
  // Original issue: assert macro expands to __builtin_FILE() etc.
  // Using our standalone version to avoid system header dependencies
  ASSERT_LIKE(true);
  
  // Also test direct usage of __builtin_FILE() which caused secondary crash  
  const char* file = __builtin_FILE();
  int line = __builtin_LINE();
}

void a(bool c) { 
  calcViscFluxSide(0, c); 
}

int main() {
  // This used to cause a segmentation fault due to null QualType
  // from SourceLocExpr (__builtin_FILE(), __builtin_LINE()) in assert-like macro expansion  
  auto grad_a = clad::gradient(a);
  
  return 0;
}