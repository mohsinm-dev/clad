// RUN: %cladclang %s -I%S/../../include -fsyntax-only 2>&1 | FileCheck %s

// Test for issue #1442: Prevent segfaults when processing assert statements and PredefinedExpr
// The main goal is compilation success without crashes, not specific derivative generation

#include "clad/Differentiator/Differentiator.h"
#include <cassert>

void functionWithAssert(int x, bool flag) {
    assert(x >= 0);  // This previously caused segfaults
    
    if (flag) {
        assert(x < 100);  // Multiple asserts to stress test
    }
}

void functionWithPredefinedExpr(double x) {
    const char* fname = __func__;
    const char* fname2 = __FUNCTION__;
    const char* fname3 = __PRETTY_FUNCTION__;
    
    // These predefined expressions previously caused segfaults during differentiation
}

void compositeFunction(int x, double y, bool flag) {
    functionWithAssert(x, flag);
    functionWithPredefinedExpr(y);
}

int main() {
    // The key test: this should compile and execute without segfaulting
    // We don't care about the exact derivative code generated, just that it doesn't crash
    auto grad = clad::gradient(compositeFunction);
    
    // If we reach this point without segfaulting, the fix is working
    return 0;
}

// CHECK-NOT: Segmentation fault
// CHECK-NOT: PLEASE submit a bug report
// CHECK-NOT: Stack dump: