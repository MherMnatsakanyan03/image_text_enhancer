#!/bin/bash

# 1. Environment Setup
# Set the number of threads
export OMP_NUM_THREADS=1

# Configuration
PROG="./ite"
INPUT="resources/bench.jpg"
OUTPUT="output.jpg"
TRIALS=1000
WARMUP=10

# Processing Flags
# -t : Enable benchmark table
FLAGS="-t --do-deskew --do-adaptive-gaussian --do-median --do-adaptive-median --binarization bataineh --do-despeckle --do-dilation --do-erosion --do-color-pass"

# 2. Validation
if [ ! -f "$PROG" ]; then
    echo "Error: Binary '$PROG' not found."
    exit 1
fi

if [ ! -f "$INPUT" ]; then
    echo "Error: Input image '$INPUT' not found."
    exit 1
fi

# 3. Execution
echo "=========================================================="
echo " Starting Internal Benchmark"
echo " Binary:      $PROG"
echo " Threads:     $OMP_NUM_THREADS"
echo " Trials:      $TRIALS"
echo " Warmup:      $WARMUP"
echo " Affinity:    Cores 0-7 (via taskset)"
echo "=========================================================="

# We use taskset to reduce OS scheduler jitter, but we let C++ handle the looping.
taskset -c 0-7 "$PROG" $FLAGS \
    --input "$INPUT" \
    --output "$OUTPUT" \
    --trials "$TRIALS" \
    --warmup "$WARMUP"