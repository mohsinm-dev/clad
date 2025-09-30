// RUN: %cladclang -I%S/../../include %s
// XFAIL: valgrind

// Test that assert() macro usage doesn't cause segmentation fault in clad
// This addresses issue #1442 where SourceLocExpr from assert() macro expansion
// would create null QualTypes causing crashes in ReferencesUpdater::updateType

#include "clad/Differentiator/Differentiator.h"
#include <cassert>
#include <iostream>

// Function containing assert() - should not cause segfault during differentiation
void calcViscFluxSide(int x, bool flag) { 
  assert(x > 0);  // This creates SourceLocExpr nodes with potential null types
}

void targetFunction(bool c) { 
  calcViscFluxSide(1, c); 
}

int main() {
  // This should compile and run without segfault
  auto grad = clad::gradient(targetFunction);
  
  // Test execution - the main goal is that compilation doesn't segfault
  bool result = false;
  grad.execute(true, &result);
  
  std::cout << "Assert test passed - no segfault occurred\n";
  
  return 0;
}