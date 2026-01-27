#!/bin/bash

# Set OpenMP threads to 8
export OMP_NUM_THREADS=8

PROG="./ite"
INPUT="resources/bench.jpg"
OUTPUT="output.jpg"
TRIALS=1000
WARMUP_RUNS=10

# Add flags here (ensure -t is included)
FLAGS="-t --do-adaptive-gaussian --do-adaptive-median --do-dilation --do-erosion --median-size 15 --do-deskew --sauvola-k 0.4"

# Check if program exists
if [ ! -f "$PROG" ]; then
    echo "Error: Binary '$PROG' not found."
    exit 1
fi

# Warmup phase
echo "Warming up caches with $WARMUP_RUNS runs..."
echo "------------------------------------------"
for ((i=1; i<=WARMUP_RUNS; i++)); do
    taskset -c 0-7 $PROG $FLAGS -i "$INPUT" -o "$OUTPUT" > /dev/null 2>&1
done

echo "Warmup complete."

echo ""
echo "Starting benchmark: $TRIALS trials"
echo "Command: taskset -c 0-7 $PROG $FLAGS -i $INPUT -o $OUTPUT"
echo "OMP_NUM_THREADS: $OMP_NUM_THREADS"
echo "------------------------------------------"

TOTAL_TIME=0
TOTAL_TIME_SQ=0
TIMES=()
SUCCESSFUL_TRIALS=0

for ((i=1; i<=TRIALS; i++)); do
    # Run the program and capture output
    # Note: We pipe to awk to find the line starting with "Enhancement"
    # and print the 3rd word (the actual time value).
    EXEC_TIME=$(taskset -c 0-7 $PROG $FLAGS -i "$INPUT" -o "$OUTPUT" 2>&1 | awk '/Enhancement time:/ {print $3}')

    # Safety check: ensure we actually got a number
    if [[ -z "$EXEC_TIME" ]]; then
        echo "Trial $i: FAILED (Could not parse time)"
        continue
    fi

    # Store time in array for later calculation
    TIMES+=("$EXEC_TIME")
    SUCCESSFUL_TRIALS=$((SUCCESSFUL_TRIALS + 1))

    # Accumulate total and sum of squares using bc (bash doesn't do float math natively)
    TOTAL_TIME=$(echo "$TOTAL_TIME + $EXEC_TIME" | bc -l)
    TOTAL_TIME_SQ=$(echo "$TOTAL_TIME_SQ + ($EXEC_TIME * $EXEC_TIME)" | bc -l)
done

# Calculate and print results
if (( $(echo "$TOTAL_TIME > 0" | bc -l) )); then
    AVG_TIME=$(echo "$TOTAL_TIME / $SUCCESSFUL_TRIALS" | bc -l)

    # Calculate standard deviation: sqrt((sum(x^2)/n) - (sum(x)/n)^2)
    MEAN_OF_SQUARES=$(echo "$TOTAL_TIME_SQ / $SUCCESSFUL_TRIALS" | bc -l)
    SQUARE_OF_MEAN=$(echo "$AVG_TIME * $AVG_TIME" | bc -l)
    VARIANCE=$(echo "$MEAN_OF_SQUARES - $SQUARE_OF_MEAN" | bc -l)
    STD_DEV=$(echo "sqrt($VARIANCE)" | bc -l)

    # Calculate median by sorting the array
    IFS=$'\n' SORTED_TIMES=($(sort -n <<<"${TIMES[*]}"))
    unset IFS

    MID=$((SUCCESSFUL_TRIALS / 2))
    if (( SUCCESSFUL_TRIALS % 2 == 1 )); then
        # Odd number of elements: take the middle one
        MEDIAN=${SORTED_TIMES[$MID]}
    else
        # Even number of elements: average the two middle ones
        MEDIAN=$(echo "(${SORTED_TIMES[$MID-1]} + ${SORTED_TIMES[$MID]}) / 2" | bc -l)
    fi

    # Calculate coefficient of variation (CV = std_dev / mean * 100%)
    CV=$(echo "($STD_DEV / $AVG_TIME) * 100" | bc -l)

    printf -- "------------------------------------------\n"
    printf "Successful Trials: %d / %d\n" "$SUCCESSFUL_TRIALS" "$TRIALS"
    printf "n = %d\n" "$SUCCESSFUL_TRIALS"
    printf "µ = %.4f s ± %.4f s\n" "$AVG_TIME" "$STD_DEV"
    printf "Median: %.4f s\n" "$MEDIAN"
    printf "CV = %.2f%%\n" "$CV"
else
    echo "Benchmark failed to collect timing data."
fi