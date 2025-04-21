#!/bin/bash

# Path to QEMU binary
QEMU_BIN="./build/qemu-system-aarch64"

# Path to test binary
TEST_BIN="./build/bin_test/simple_add_test.bin"

# Path to debug log
DEBUG_LOG="$HOME/Desktop/debug.log"

# Run QEMU with TDA4VH machine and debug options
$QEMU_BIN -M tda4vh -nographic -kernel $TEST_BIN -d cpu -D $DEBUG_LOG -monitor none -serial stdio

echo "Test completed. Debug log saved to $DEBUG_LOG"
