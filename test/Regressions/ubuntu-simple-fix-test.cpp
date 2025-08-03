// RUN: %cladclang %s -I%S/../../include -fsyntax-only 2>&1 | FileCheck %s

// Simple test to verify the Ubuntu segfault fix works
// This should compile without crashing (the main achievement of the fix)

#include "clad/Differentiator/Differentiator.h"

// Mock assert function - minimal reproduction
void test_assert_mock(const char* expr, const char* file, int line, const char* func) {}

// Ubuntu-style assert that previously caused segfaults
#define simple_ubuntu_assert(expr) \
  ((expr) ? (void)0 : test_assert_mock(#expr, __FILE__, __LINE__, __extension__ __PRETTY_FUNCTION__))

void simpleTestFunction(double x) {
    simple_ubuntu_assert(x > 0.0);  // Previously caused segfaults
    
    // Test predefined expressions that caused issues
    const char* fname = __func__;
    const char* pretty = __extension__ __PRETTY_FUNCTION__;
}

int main() {
    // This should compile without segfault - that's the success criteria
    auto grad = clad::gradient(simpleTestFunction);
    return 0;
}

// Focus on crash prevention
// CHECK-NOT: Segmentation fault
// CHECK-NOT: PLEASE submit a bug report
// CHECK-NOT: Stack dump:
// CHECK-NOT: fatal error: