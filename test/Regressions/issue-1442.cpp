// RUN: %cladclang %s -I%S/../../include -Xclang -verify -fsyntax-only

// Single, cross-platform regression for #1442.
// - Avoids <cassert> and libc/OS variability
// - Triggers the Clad warning once (assert-like macro)
// - Exercises PredefinedExpr forms (no extra diagnostics)
// - Ensures "no crash" across Clang 12–18

#undef NDEBUG
extern "C" void __assert_fail(const char*, const char*, unsigned, const char*)
  __attribute__((noreturn));

// Portable source-location helpers
#ifndef __has_builtin
#  define __has_builtin(x) 0
#endif

#if __has_builtin(__builtin_FILE) && __has_builtin(__builtin_LINE)
#  define MY_FILE() __builtin_FILE()
#  define MY_LINE() __builtin_LINE()
#else
#  define MY_FILE() __FILE__
#  define MY_LINE() __LINE__
#endif

#if __has_builtin(__builtin_FUNCTION)
#  define MY_FUNCTION() __builtin_FUNCTION()
#else
#  define MY_FUNCTION() __func__
#endif

#define MY_ASSERT(expr) \
  ((expr) ? (void)0 : __assert_fail(#expr, MY_FILE(), MY_LINE(), MY_FUNCTION()))

#include "clad/Differentiator/Differentiator.h"

// --- Part 1: trigger the Clad "unsupported statement" warning once ---
void calcViscFluxSide(int x, bool flag) {
  (void)flag;                     // silence unused
  MY_ASSERT(x >= 0);              // expected-warning {{attempted to differentiate unsupported statement}}
}

void a(bool c) {
  // use c so no extra warnings
  calcViscFluxSide(c ? 5 : 6, c);
}

// --- Part 2: exercise PredefinedExpr nodes (no diagnostics expected) ---
static void uses_predef() {
  (void)__func__;
  (void)__FUNCTION__;
  (void)__PRETTY_FUNCTION__;
}

void predef_wrapper() { uses_predef(); }

// --- Driver: ensure differentiation runs without crashing ---
void drive() {
  (void)clad::gradient(a);
  (void)clad::gradient(predef_wrapper);
}
