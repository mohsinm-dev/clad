// RUN: %cladclang %s -I%S/../../include -fsyntax-only 2>&1 | FileCheck %s

// Ubuntu-specific assert pattern debugging for issue #1442
// This test reproduces Ubuntu/GCC-specific assert implementations without system dependencies
// The main goal is preventing crashes, not specific derivative output

#include "clad/Differentiator/Differentiator.h"

// Mock functions to avoid linking issues - these simulate the actual assert behavior
void __assert_fail(const char* assertion, const char* file, 
                   unsigned int line, const char* function) {}

void __assert_perror_fail(int errnum, const char* file,
                          unsigned int line, const char* function) {}

// Ubuntu/GCC glibc-style assert patterns that cause segfaults
// Pattern 1: Standard glibc assert with __extension__ __PRETTY_FUNCTION__
#define ubuntu_assert_glibc(expr) \
  (static_cast<bool> (expr) \
   ? static_cast<void>(0) \
   : __assert_fail (#expr, __FILE__, __LINE__, __extension__ __PRETTY_FUNCTION__))

// Pattern 2: Alternative glibc pattern with different casting
#define ubuntu_assert_alt(expr) \
  ((expr) \
   ? static_cast<void>(0) \
   : __assert_fail (#expr, __FILE__, __LINE__, __func__))

// Pattern 3: GCC-specific extension pattern
#define ubuntu_assert_extension(expr) \
  (__extension__ ({ \
    if (!(expr)) \
      __assert_fail (#expr, __FILE__, __LINE__, __extension__ __PRETTY_FUNCTION__); \
  }))

// Pattern 4: Complex conditional expression that Ubuntu might use
#define ubuntu_assert_complex(expr) \
  do { \
    if (__builtin_expect(!(expr), 0)) \
      __assert_fail (#expr, __FILE__, __LINE__, __extension__ __PRETTY_FUNCTION__); \
  } while (0)

// Test functions using different Ubuntu assert patterns
void testGlibcAssert(int x, bool flag) {
    ubuntu_assert_glibc(x >= 0);  // Previously caused segfaults
    
    if (flag) {
        ubuntu_assert_glibc(x < 100);  // Previously caused segfaults
    }
}

void testAlternativeAssert(double y) {
    ubuntu_assert_alt(y > 0.0);  // Previously caused segfaults
}

void testExtensionAssert(float z) {
    ubuntu_assert_extension(z != 0.0f);  // Previously caused segfaults
}

void testComplexAssert(int a, int b) {
    ubuntu_assert_complex(a + b > 0);  // Previously caused segfaults
}

// Test predefined expressions that cause issues on Ubuntu
void testPredefinedExpressions(double x) {
    // These are the expressions that trigger null QualType issues
    const char* f1 = __func__;
    const char* f2 = __FUNCTION__;
    const char* f3 = __PRETTY_FUNCTION__;
    
    // Ubuntu-specific: __extension__ with __PRETTY_FUNCTION__
    const char* f4 = __extension__ __PRETTY_FUNCTION__;
}

// Composite test function that exercises all problematic patterns
void ubuntuCompositeTest(int x, double y, float z, bool flag) {
    testGlibcAssert(x, flag);
    testAlternativeAssert(y);
    testExtensionAssert(z);
    testComplexAssert(x, static_cast<int>(y));
    testPredefinedExpressions(y);
}

// Main test function for gradient computation
int main() {
    // This should not segfault on Ubuntu after the fix
    auto grad = clad::gradient(ubuntuCompositeTest);
    return 0;
}

// Focus on what matters: no crashes or fatal errors
// CHECK-NOT: Segmentation fault
// CHECK-NOT: PLEASE submit a bug report
// CHECK-NOT: Stack dump:
// CHECK-NOT: fatal error: