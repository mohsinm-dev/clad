// RUN: %cladclang %s -I%S/../../include -fsyntax-only 2>&1 | FileCheck %s

// Test for segmentation fault fix when asserts and PredefinedExpr are used
// This addresses issue #1442 - the main goal is preventing crashes, not specific derivative output

#include "clad/Differentiator/Differentiator.h"
#include <cassert>

void calcViscFluxSide(int x, bool flag) {
    assert(x >= 0);  // Previously caused segfaults
}

void testPredefinedExpr(double x) {
    const char* fname = __func__;
    const char* fname2 = __FUNCTION__;
    const char* fname3 = __PRETTY_FUNCTION__;
    // These predefined expressions previously caused segfaults
}

void testFunction(bool c) {
    calcViscFluxSide(5, c);
}

void testFunctionWithPredefined(double x) {
    testPredefinedExpr(x);
}

int main() {
    // The key test: compilation should succeed without segfaults
    auto grad = clad::gradient(testFunction);
    auto grad2 = clad::gradient(testFunctionWithPredefined);
    return 0;
}

// Focus on what matters: no crashes or fatal errors
// CHECK-NOT: Segmentation fault
// CHECK-NOT: PLEASE submit a bug report
// CHECK-NOT: Stack dump:
// CHECK-NOT: fatal error:
