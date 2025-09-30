#!/bin/bash

echo "=== Testing assert() segfault fix ==="

# Test that our specific fix works
echo "Testing fix for assert() segfault..."

# Create a comprehensive test case
cat > assert_test.cpp << 'EOF'
#include <cassert>
#include "clad/Differentiator/Differentiator.h"

void funcWithAssert(double x, bool flag) {
    assert(x > 0);  // This should not cause segfault
    assert(flag);   // Multiple asserts
    double y = x * x;
}

void target(double x) {
    funcWithAssert(x, true);
}

int main() {
    auto grad = clad::gradient(target);
    double dx = 0;
    grad.execute(2.0, &dx);
    printf("Test passed - no segfault with assert() macros\n");
    return 0;
}
EOF

# Test with the built plugin
if clang++ -fplugin=./build/lib/clad.dylib -I./include assert_test.cpp -o assert_test 2>&1; then
    echo "✅ Compilation successful"
    if ./assert_test; then
        echo "✅ Execution successful"
        echo "✅ Assert fix is working correctly"
    else
        echo "❌ Execution failed"
        exit 1
    fi
else
    echo "❌ Compilation failed"
    exit 1
fi

# Clean up
rm -f assert_test.cpp assert_test

echo "=== Fix verification completed successfully ==="