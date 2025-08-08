// RUN: %cladclang %s -I%S/../../include -Xclang -verify -fsyntax-only

// Covers PredefinedExpr forms that used to produce null QualType on some Clang versions.
// No diagnostics are required; the important part is "no crash".

#include "clad/Differentiator/Differentiator.h"

static void uses_predef() {
  (void)__func__;
  (void)__FUNCTION__;
  (void)__PRETTY_FUNCTION__;
}

void caller() { uses_predef(); }

void drive() { (void)clad::gradient(caller); } // should not crash, no diagnostics
