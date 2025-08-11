// RUN: %cladclang %s -I%S/../../include -fsyntax-only -Xclang -verify 2>&1 | FileCheck %s

// Test for segmentation fault fix when asserts and PredefinedExpr are used
// This addresses issue #1442

#include "clad/Differentiator/Differentiator.h"

// Simulate different assert implementations without including <cassert>
// to ensure platform-independent testing across different compilers and OS.
// This avoids relying on system headers which vary between Ubuntu and macOS.
extern "C" void __assert_fail(const char*, const char*, int, const char*) __attribute__((noreturn));

// Version 1: Using standard __FILE__, __LINE__, __func__
#define TEST_ASSERT_V1(expr) \
    ((expr) ? (void)0 : __assert_fail(#expr, __FILE__, __LINE__, __func__))

// Version 2: Using builtin versions (as used in some glibc implementations)
#define TEST_ASSERT_V2(expr) \
    ((expr) ? (void)0 : __assert_fail(#expr, __builtin_FILE(), __builtin_LINE(), __builtin_FUNCTION()))

void calcViscFluxSide(int x, bool flag) {
    TEST_ASSERT_V1(x >= 0);
    // expected-warning@-1 {{attempted to differentiate unsupported statement, no changes applied}}
}

void calcViscFluxSide2(int x, bool flag) {
    TEST_ASSERT_V2(x >= 0);
    // expected-warning@-1 {{attempted to differentiate unsupported statement, no changes applied}}
}

void testPredefinedExpr(double x) {
    const char* fname = __func__;
    // expected-warning@-1 {{attempted to differentiate unsupported statement, no changes applied}}
    const char* fname2 = __FUNCTION__;
    // expected-warning@-1 {{attempted to differentiate unsupported statement, no changes applied}}
    const char* fname3 = __PRETTY_FUNCTION__;
    // expected-warning@-1 {{attempted to differentiate unsupported statement, no changes applied}}
}

void testFunction(bool c) {
    calcViscFluxSide(5, c);
    calcViscFluxSide2(5, c);
}

void testFunctionWithPredefined(double x) {
    testPredefinedExpr(x);
}

int main() {
    auto grad = clad::gradient(testFunction);
    auto grad2 = clad::gradient(testFunctionWithPredefined);
    return 0;
}

// CHECK: void testFunction_grad(bool c, bool *_d_c) {
// CHECK-NEXT: calcViscFluxSide(5, c);
// CHECK-NEXT: calcViscFluxSide2(5, c);
// CHECK-NEXT: }