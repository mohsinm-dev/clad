// RUN: %cladclang -I%S/../../include %s
// Test for issue #1442: Segmentation fault when assert macros are used in clad

#include "clad/Differentiator/Differentiator.h"

// Standalone assert-like macro that reproduces the original issue.
// Use builtins if available, otherwise fall back to __FILE__/__LINE__ so the
// test remains portable across older Clang/GCC versions used in CI.

#if defined(__has_builtin)
#  if __has_builtin(__builtin_FILE)
#    define CLAD_ISSUE1442_FILE_EXPR __builtin_FILE()
#  else
#    define CLAD_ISSUE1442_FILE_EXPR __FILE__
#  endif
#  if __has_builtin(__builtin_LINE)
#    define CLAD_ISSUE1442_LINE_EXPR __builtin_LINE()
#  else
#    define CLAD_ISSUE1442_LINE_EXPR __LINE__
#  endif
#else
#  define CLAD_ISSUE1442_FILE_EXPR __FILE__
#  define CLAD_ISSUE1442_LINE_EXPR __LINE__
#endif

#define ASSERT_LIKE(expr)                                                      \
  do {                                                                         \
    if (!(expr)) {                                                             \
      const char* file = CLAD_ISSUE1442_FILE_EXPR;                             \
      int line = CLAD_ISSUE1442_LINE_EXPR;                                     \
      (void)file;                                                               \
      (void)line;                                                               \
    }                                                                          \
  } while (0)

void calcViscFluxSide(int, bool) { 
  // Original issue: assert macro expands to __builtin_FILE() etc.
  // Using our standalone version to avoid system header dependencies
  ASSERT_LIKE(true);
  
  // Also test direct usage of FILE/LINE expressions. If builtins are not
  // available on this toolchain, fall back to macros above.
  const char* file = CLAD_ISSUE1442_FILE_EXPR;
  int line = CLAD_ISSUE1442_LINE_EXPR;
  (void)file; (void)line;
}

void a(bool c) { 
  calcViscFluxSide(0, c); 
}

int main() {
  // This used to cause a segmentation fault due to null QualType
  // from SourceLocExpr (__builtin_FILE(), __builtin_LINE()) in assert-like macro expansion  
  auto grad_a = clad::gradient(a);
  
  return 0;
}
