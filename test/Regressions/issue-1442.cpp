// RUN: %cladnumdiffclang %s -I%S/../../include -std=c++17 -fsyntax-only 2>&1 | %filecheck %s
// Test for issue #1442: Segmentation fault when asserts are used
// Expected to produce warnings but not crash

#include <cassert>

// Test case from the original issue - should not cause segfault
void calcViscFluxSide(int, bool) { 
  assert(0); // This should not cause a segfault
}

void a(bool c) { 
  calcViscFluxSide(0, c); 
}

#include "clad/Differentiator/Differentiator.h"

void test_assert_issue() { 
  auto a_grad = clad::gradient(a);
  // CHECK: warning: attempted to differentiate unsupported statement, no changes applied
  // CHECK-NOT: Segmentation fault
  // CHECK-NOT: Assertion {{.*}} failed
}

// Additional test cases for various assert scenarios
double test_multiple_asserts(double x, double y) {
  if (x < 0) {
    assert(x >= 0 && "x must be non-negative");
  }
  if (y < 0) {
    assert(y >= 0 && "y must be non-negative"); 
  }
  // Some differentiable computation
  return x * y + x * x;
}

// Test with other builtin functions that might create SourceLocExpr
double test_builtin_file(double x) {
  const char* file = __builtin_FILE();
  return x * x; // Simple differentiable expression
}

void test_builtin_line(double x) {
  int line = __builtin_LINE(); 
  // More builtin tests
}

void test_all_cases() {
  auto grad_fn = clad::gradient(test_multiple_asserts);
  auto grad_builtin = clad::gradient(test_builtin_file);
  // Should handle all cases without crashing
}

// CHECK: warning: attempted to differentiate unsupported statement, no changes applied
// CHECK: warning: attempted to differentiate unsupported statement, no changes applied
// CHECK: warning: attempted to differentiate unsupported statement, no changes applied

int main() {
  test_assert_issue();
  test_all_cases();
  return 0;
}