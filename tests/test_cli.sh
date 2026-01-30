#!/bin/bash

# Configuration: Set the path to your compiled binary
PROG="./ite"

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color

PASSED=0
FAILED=0

# Helper function to run a test case
# Usage: run_test "description" expected_exit_code [arguments...]
run_test() {
    local desc=$1
    local expected=$2
    shift 2

    echo -n "Testing: $desc... "

    # Run the program and capture output/stderr to /dev/null for a clean console
    $PROG "$@" > /dev/null 2>&1
    local actual=$?

    if [ $actual -eq $expected ]; then
        echo -e "${GREEN}PASS${NC} (Exit: $actual)"
        ((PASSED++))
    else
        echo -e "${RED}FAIL${NC} (Expected $expected, got $actual)"
        echo "  Cmd: $PROG $@"
        ((FAILED++))
    fi
}

echo "Starting CLI Validation Tests..."
echo "--------------------------------"

# --- 1. Basic Usage & Help ---
run_test "Show help (-h)" 0 -h
run_test "No arguments (should print help and return 0)" 0
run_test "Missing output path" 0 -i input.jpg

# --- 2. Input Validation: Unknown/Missing Options ---
run_test "Unknown short option (-z)" 2 -z
run_test "Unknown long option (--fake)" 2 --fake
run_test "Missing value for option (-i)" 2 -i

# --- 3. Type Validation (Floats & Ints) ---
run_test "Invalid float for --sigma (string 'abc')" 2 -i in.jpg -o out.jpg --sigma abc
run_test "Invalid uint for --median-size (negative '-5')" 2 -i in.jpg -o out.jpg --median-size -5
run_test "Invalid uint for --median-size (float '2.5')" 2 -i in.jpg -o out.jpg --median-size 2.5

# --- 4. Logical Constraints (Positive/Non-negative/Odd) ---
run_test "Sigma must be > 0 (given 0.0)" 2 -i in.jpg -o out.jpg --sigma 0.0
run_test "Kernel size must be > 0 (given 0)" 2 -i in.jpg -o out.jpg --kernel-size 0
run_test "Adaptive median max must be odd (given 4)" 2 -i in.jpg -o out.jpg --adaptive-median-max 4
run_test "Adaptive median max must be >= 3 (given 1)" 2 -i in.jpg -o out.jpg --adaptive-median-max 1
run_test "Despeckle thresh can be 0 (non-negative)" 2 -i in.jpg -o out.jpg --despeckle-thresh -1

# --- 5. Valid Combinations (Simulated) ---
# Note: These might still return 1 if the files 'in.jpg' don't exist,
# but they test that the CLI parser itself accepts the flags.
run_test "Valid toggles and params" 1 -i in.jpg -o out.jpg --do-gaussian --sigma 1.5 --do-deskew

echo "--------------------------------"
echo -e "Tests Completed: ${GREEN}$PASSED passed${NC}, ${RED}$FAILED failed${NC}"

if [ $FAILED -gt 0 ]; then
    exit 1
fi