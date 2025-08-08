// RUN: %cladclang %s -I%S/../../include -Xclang -verify -fsyntax-only

// Self-contained reproduction of assert-like behavior without libc headers.
// Ensures diagnostics are anchored in this file and stable across platforms.

#undef NDEBUG
extern "C" void __assert_fail(const char*, const char*, unsigned, const char*)
  __attribute__((noreturn));

// -------- Portable source-location helpers --------
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
// --------------------------------------------------

#define MY_ASSERT(expr) \
  ((expr) ? (void)0 : __assert_fail(#expr, MY_FILE(), MY_LINE(), MY_FUNCTION()))

#include "clad/Differentiator/Differentiator.h"

void calcViscFluxSide(int x, bool flag) {
  MY_ASSERT(x >= 0); // expected-warning {{attempted to differentiate unsupported statement}}
}

void a(bool c) { calcViscFluxSide(5, c); }

void drive() { (void)clad::gradient(a); } // should not crash
