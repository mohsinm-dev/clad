// RUN: %cladclang %s -I%S/../../include -fsyntax-only 2>&1 | FileCheck %s

// Ubuntu debugging for newer LLVM runtime versions (18+, 19+)
// These versions may have different PredefinedExpr and extension handling
// The main goal is preventing crashes, not specific derivative output

#include "clad/Differentiator/Differentiator.h"

// Mock assert functions for newer runtime testing
void __assert_fail_v19(const char* assertion, const char* file, 
                       unsigned int line, const char* function) {}

// Newer LLVM versions may handle __extension__ differently
#define newer_runtime_assert(expr) \
  (__extension__ ({ \
    if (__builtin_expect(!(expr), 0)) { \
      __assert_fail_v19(#expr, __FILE__, __LINE__, __extension__ __PRETTY_FUNCTION__); \
    } \
  }))

// Test advanced GCC extension patterns that may only appear in newer versions
#define advanced_extension_assert(expr) \
  (__extension__ ({ \
    _Bool __result = static_cast<_Bool>(expr); \
    if (!__result) { \
      const char* __func_name = __extension__ __PRETTY_FUNCTION__; \
      __assert_fail_v19(#expr, __FILE__, __LINE__, __func_name); \
    } \
    __result; \
  }))

// Test compound literal expressions with extensions
#define compound_assert(expr) \
  ((struct { const char* name; }){ __extension__ __PRETTY_FUNCTION__ }, \
   (expr) ? (void)0 : __assert_fail_v19(#expr, __FILE__, __LINE__, __func__))

void testNewerRuntimePatterns(int x, double y) {
    newer_runtime_assert(x > 0);  // Previously caused segfaults
    advanced_extension_assert(y != 0.0);  // Previously caused segfaults
}

void testCompoundExpressions(float z) {
    compound_assert(z > 0.0f);  // Previously caused segfaults
}

// Test various predefined expressions that may behave differently in newer runtimes
void testNewerPredefinedExpr(int a) {
    // Standard predefined expressions
    const char* f1 = __func__;
    const char* f2 = __FUNCTION__;
    const char* f3 = __PRETTY_FUNCTION__;
    
    // Extension variants that might cause issues in newer runtimes
    const char* f4 = __extension__ __PRETTY_FUNCTION__;
    
    // Nested extension expressions
    const char* f5 = __extension__ (__extension__ __PRETTY_FUNCTION__);
    
    // Complex expressions with predefined identifiers
    int line = __LINE__;
    const char* file = __FILE__;
}

// Test C++ specific constructs that may interact with extensions differently
class TestClass {
public:
    void memberFunction(double x) {
        newer_runtime_assert(x >= 0.0);
        
        // Test __PRETTY_FUNCTION__ in member function context
        const char* pretty = __extension__ __PRETTY_FUNCTION__;
    }
};

// Template function testing - newer runtimes may handle template instantiation differently
template<typename T>
void templateFunction(T value) {
    newer_runtime_assert(static_cast<bool>(value));
    
    const char* template_func = __extension__ __PRETTY_FUNCTION__;
}

// Main test function combining all problematic patterns
void newerRuntimeCompositeTest(int x, double y, float z) {
    testNewerRuntimePatterns(x, y);
    testCompoundExpressions(z);
    testNewerPredefinedExpr(x);
    
    TestClass obj;
    obj.memberFunction(y);
    
    templateFunction<int>(x);
    templateFunction<double>(y);
}

int main() {
    // This should work on newer runtime versions after the fix
    auto grad = clad::gradient(newerRuntimeCompositeTest);
    return 0;
}

// Focus on what matters: no crashes or fatal errors
// CHECK-NOT: Segmentation fault
// CHECK-NOT: PLEASE submit a bug report
// CHECK-NOT: Stack dump:
// CHECK-NOT: fatal error: