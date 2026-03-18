#!/bin/bash

# 1. Build
echo "=========================================================="
echo " Building ite (Release)"
echo "=========================================================="

cmake -S . -B cmake-build-release -DCMAKE_BUILD_TYPE=Release
cmake --build cmake-build-release --target ITE_cli -- -j"$(nproc)"

if [ $? -ne 0 ]; then
    echo "Error: Build failed."
    exit 1
fi

# 2. Environment Setup
# Capture current git branch
GIT_BRANCH=$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo "unknown")
GIT_BRANCH="${GIT_BRANCH##*/}"

# Set the number of threads
export OMP_NUM_THREADS=1

# Configuration
PROG="./cmake-build-release/src/ite"
TEST_DIR="resources/test_images"
LOG_DIR="bench_logs"
OUTPUT_DIR="output"
TRIALS=1000
WARMUP=10
TIME_LIMIT=30 # Minutes

# Processing Flags
# -t : Enable benchmark table
FLAGS="-t --time-limit $TIME_LIMIT --do-deskew --do-adaptive-gaussian --do-median --do-adaptive-median --binarization bataineh --do-despeckle --do-dilation --do-erosion --do-color-pass"

# 3. Validation
if [ ! -f "$PROG" ]; then
    echo "Error: Binary '$PROG' not found."
    exit 1
fi

if [ ! -d "$TEST_DIR" ]; then
    echo "Error: Test directory '$TEST_DIR' not found."
    exit 1
fi

# Create output directories if they don't exist
mkdir -p "$LOG_DIR"
mkdir -p "$OUTPUT_DIR"

# 3. Execution Loop
echo "=========================================================="
echo " Starting Bulk Benchmark"
echo " Branch:      $GIT_BRANCH"
echo " Binary:      $PROG"
echo " Threads:     $OMP_NUM_THREADS"
echo " Trials:      $TRIALS"
echo " Warmup:      $WARMUP"
echo " Time Limt:   $TIME_LIMIT m"
echo " Affinity:    Cores 0-7 (via taskset)"
echo " Input Dir:   $TEST_DIR"
echo " Log Dir:     $LOG_DIR"
echo "=========================================================="

# Loop over all images in the test directory
for INPUT_IMG in "$TEST_DIR"/*; do
    
    # Check if file exists (handles empty directory case)
    [ -e "$INPUT_IMG" ] || continue

    # Extract filename (e.g., "photo.jpg")
    BASENAME=$(basename "$INPUT_IMG")
    
    # Define specific output paths
    # Log format: bench_logs.bak5.bak4/bench_photo.jpg_8cores_main.txt
    LOG_FILE="$LOG_DIR/bench_${BASENAME}_${OMP_NUM_THREADS}cores_${GIT_BRANCH}.txt"
    OUTPUT_IMG="$OUTPUT_DIR/out_${BASENAME}"

    echo "Processing: $BASENAME -> $LOG_FILE"

    # Run the benchmark
    # We pipe both stdout (1) and stderr (2) to the log file
    taskset -c 0-7 "$PROG" $FLAGS \
        --input "$INPUT_IMG" \
        --output "$OUTPUT_IMG" \
        --trials "$TRIALS" \
        --warmup "$WARMUP" > "$LOG_FILE"

done

echo "=========================================================="
echo " Benchmark Complete."
echo " Results saved in: $LOG_DIR"
echo "=========================================================="
