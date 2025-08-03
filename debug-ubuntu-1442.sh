#!/bin/bash

# Ubuntu Issue #1442 Debugging Script
# This script applies debug logging and runs comprehensive testing

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/obj-debug"

echo "=== Ubuntu Issue #1442 Debugging Script ==="
echo "Script directory: $SCRIPT_DIR"
echo "Build directory: $BUILD_DIR"

# Function to print with timestamp
log_msg() {
    echo "[$(date '+%H:%M:%S')] $*"
}

# Function to apply debug patch
apply_debug_patch() {
    log_msg "Applying debug logging patch..."
    
    if [ -f "$SCRIPT_DIR/lib/Differentiator/StmtClone-debug.cpp.patch" ]; then
        # Create backup of original file
        cp "$SCRIPT_DIR/lib/Differentiator/StmtClone.cpp" \
           "$SCRIPT_DIR/lib/Differentiator/StmtClone.cpp.backup"
        
        # Apply patch
        cd "$SCRIPT_DIR"
        patch -p1 < "$SCRIPT_DIR/lib/Differentiator/StmtClone-debug.cpp.patch"
        
        log_msg "Debug patch applied successfully"
    else
        log_msg "Debug patch file not found, skipping..."
    fi
}

# Function to restore original file
restore_original() {
    log_msg "Restoring original StmtClone.cpp..."
    
    if [ -f "$SCRIPT_DIR/lib/Differentiator/StmtClone.cpp.backup" ]; then
        mv "$SCRIPT_DIR/lib/Differentiator/StmtClone.cpp.backup" \
           "$SCRIPT_DIR/lib/Differentiator/StmtClone.cpp"
        log_msg "Original file restored"
    fi
}

# Function to setup debug build
setup_debug_build() {
    log_msg "Setting up debug build environment..."
    
    # Remove old build directory
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
    fi
    
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    # Find LLVM installation
    LLVM_CONFIG=$(which llvm-config 2>/dev/null || echo "")
    if [ -z "$LLVM_CONFIG" ]; then
        # Try common locations
        for version in 18 17 16 15 14 13 12 11; do
            if command -v llvm-config-$version >/dev/null 2>&1; then
                LLVM_CONFIG="llvm-config-$version"
                break
            fi
        done
    fi
    
    if [ -z "$LLVM_CONFIG" ]; then
        log_msg "ERROR: llvm-config not found. Please install LLVM development packages."
        exit 1
    fi
    
    LLVM_DIR=$(${LLVM_CONFIG} --prefix)
    log_msg "Using LLVM installation: $LLVM_DIR"
    
    # Configure with debug options
    cmake .. \
        -DCMAKE_BUILD_TYPE=Debug \
        -DLLVM_DIR="$LLVM_DIR" \
        -DClang_DIR="$LLVM_DIR" \
        -DLLVM_EXTERNAL_LIT="$(which lit)" \
        -DCMAKE_CXX_FLAGS="-g -O0 -fno-omit-frame-pointer -DCLAD_DEBUG_UBUNTU" \
        -DCMAKE_C_FLAGS="-g -O0 -fno-omit-frame-pointer" \
        -DCLAD_ENABLE_DOXYGEN=Off \
        -DCLAD_ENABLE_SPHINX=Off
    
    log_msg "Debug build configured successfully"
}

# Function to build with debug info
build_debug() {
    log_msg "Building Clad with debug information..."
    
    cd "$BUILD_DIR"
    make -j$(nproc) VERBOSE=1
    
    log_msg "Debug build completed"
}

# Function to run Ubuntu-specific tests
run_ubuntu_tests() {
    log_msg "Running Ubuntu-specific tests with debug logging..."
    
    cd "$BUILD_DIR"
    
    # Set debug environment variables
    export CLAD_DEBUG_UBUNTU=1
    export CLAD_DEBUG=1
    
    # Create debug log directory
    mkdir -p debug_logs
    
    log_msg "Running ubuntu-assert-patterns test..."
    
    # Run specific test with detailed logging
    timeout 120 python3 -m lit -v \
        "$SCRIPT_DIR/test/Regressions/ubuntu-assert-patterns.cpp" \
        --debug \
        > debug_logs/ubuntu_test_output.log 2>&1 || {
        
        TEST_EXIT_CODE=$?
        log_msg "Test failed with exit code: $TEST_EXIT_CODE"
        
        if [ $TEST_EXIT_CODE -eq 124 ]; then
            log_msg "Test timed out - possible infinite loop or hang"
        else
            log_msg "Test crashed - analyzing output..."
        fi
        
        # Display relevant parts of the log
        echo ""
        echo "=== Last 50 lines of test output ==="
        tail -50 debug_logs/ubuntu_test_output.log
        
        echo ""
        echo "=== Segfault/crash analysis ==="
        grep -A 10 -B 5 "Segmentation fault\|segfault\|SIGSEGV\|Aborted\|ERROR" \
            debug_logs/ubuntu_test_output.log || echo "No obvious crash indicators found"
        
        echo ""
        echo "=== Debug messages ==="
        grep "DEBUG\|WARNING" debug_logs/ubuntu_test_output.log | tail -20
        
        return $TEST_EXIT_CODE
    }
    
    log_msg "Ubuntu-specific test completed successfully!"
    
    # Show debug output summary
    echo ""
    echo "=== Debug Output Summary ==="
    grep -c "DEBUG" debug_logs/ubuntu_test_output.log || echo "0"
    echo " debug messages logged"
    
    grep "WARNING" debug_logs/ubuntu_test_output.log | wc -l || echo "0"
    echo " warnings logged"
}

# Function to run with GDB for crash analysis
run_with_gdb() {
    log_msg "Running problematic test under GDB..."
    
    cd "$BUILD_DIR"
    
    # Create GDB script
    cat > gdb_script.txt << 'EOF'
set confirm off
set print frame-arguments all
set print pretty on
set pagination off
run -v test/Regressions/ubuntu-assert-patterns.cpp --debug
bt full
info registers
list
quit
EOF
    
    # Run under GDB
    timeout 300 gdb -batch -x gdb_script.txt \
        $(which python3) \
        --args python3 -m lit -v \
        "$SCRIPT_DIR/test/Regressions/ubuntu-assert-patterns.cpp" \
        --debug > debug_logs/gdb_analysis.log 2>&1 || {
        
        log_msg "GDB analysis completed (may have crashed)"
        
        echo ""
        echo "=== GDB Analysis Results ==="
        cat debug_logs/gdb_analysis.log
    }
}

# Function to run valgrind analysis
run_valgrind() {
    log_msg "Running with Valgrind for memory error detection..."
    
    cd "$BUILD_DIR"
    
    # Run under valgrind
    timeout 600 valgrind \
        --tool=memcheck \
        --leak-check=full \
        --show-leak-kinds=all \
        --track-origins=yes \
        --verbose \
        python3 -m lit -v \
        "$SCRIPT_DIR/test/Regressions/ubuntu-assert-patterns.cpp" \
        > debug_logs/valgrind_analysis.log 2>&1 || {
        
        log_msg "Valgrind analysis completed"
        
        echo ""
        echo "=== Valgrind Results ==="
        grep -A 5 -B 2 "ERROR SUMMARY\|Invalid\|Segmentation fault" \
            debug_logs/valgrind_analysis.log || echo "No major issues found"
    }
}

# Function to cleanup
cleanup() {
    log_msg "Cleaning up..."
    restore_original
    log_msg "Cleanup completed"
}

# Main execution
main() {
    case "${1:-all}" in
        patch)
            apply_debug_patch
            ;;
        build)
            setup_debug_build
            build_debug
            ;;
        test)
            run_ubuntu_tests
            ;;
        gdb)
            run_with_gdb
            ;;
        valgrind)
            run_valgrind
            ;;
        cleanup)
            cleanup
            ;;
        all)
            apply_debug_patch
            setup_debug_build
            build_debug
            run_ubuntu_tests
            run_with_gdb
            run_valgrind
            ;;
        *)
            echo "Usage: $0 [patch|build|test|gdb|valgrind|cleanup|all]"
            echo "  patch    - Apply debug logging patch"
            echo "  build    - Configure and build debug version"
            echo "  test     - Run Ubuntu-specific tests"
            echo "  gdb      - Run tests under GDB"
            echo "  valgrind - Run tests under Valgrind"
            echo "  cleanup  - Restore original files"
            echo "  all      - Run complete debugging pipeline (default)"
            exit 1
            ;;
    esac
}

# Trap cleanup on exit
trap cleanup EXIT

# Execute main function
main "$@"