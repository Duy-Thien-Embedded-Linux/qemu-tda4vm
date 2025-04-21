#!/bin/bash

# Path to QEMU binary
QEMU_BIN="./build/qemu-system-aarch64"

# Path to test binary
TEST_BIN="./build/binary_test/execute_test/simple_add.bin"

# Run QEMU with TDA4VH machine
$QEMU_BIN -M tda4vh -nographic -kernel $TEST_BIN -monitor none -serial stdio
