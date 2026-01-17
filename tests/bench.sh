#!/bin/bash

PROG="./ite"
INPUT="resources/bench.jpg"
OUTPUT="output.jpg"
TRIALS=100

# Add flags here (ensure -t is included)
FLAGS="-t"

# Check if program exists
if [ ! -f "$PROG" ]; then
    echo "Error: Binary '$PROG' not found."
    exit 1
fi

echo "Starting benchmark: $TRIALS trials"
echo "Command: $PROG $FLAGS -i $INPUT -o $OUTPUT"
echo "------------------------------------------"

TOTAL_TIME=0

for ((i=1; i<=TRIALS; i++)); do
    # Run the program and capture output
    # Note: We pipe to awk to find the line starting with "Enhancement"
    # and print the 3rd word (the actual time value).
    EXEC_TIME=$($PROG $FLAGS -i "$INPUT" -o "$OUTPUT" 2>&1 | awk '/Enhancement time:/ {print $3}')

    # Safety check: ensure we actually got a number
    if [[ -z "$EXEC_TIME" ]]; then
        echo "Trial $i: FAILED (Could not parse time)"
        continue
    fi

    echo "Trial $i: $EXEC_TIME seconds"

    # Accumulate total using bc (bash doesn't do float math natively)
    TOTAL_TIME=$(echo "$TOTAL_TIME + $EXEC_TIME" | bc -l)
done

# Calculate and print results
if (( $(echo "$TOTAL_TIME > 0" | bc -l) )); then
    AVG_TIME=$(echo "$TOTAL_TIME / $TRIALS" | bc -l)
    printf -- "------------------------------------------\n"
    printf "Average Enhancement Time: %.4f seconds\n" "$AVG_TIME"
else
    echo "Benchmark failed to collect timing data."
fi