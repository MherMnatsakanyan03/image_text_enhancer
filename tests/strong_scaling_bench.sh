#!/bin/bash
# ==============================================================================
#  Strong Scaling Benchmark — Image Text Enhancer
#  Target CPU: AMD Ryzen 9950X3D (16 physical cores, 32 logical threads / SMT)
#
#  "Strong scaling": problem size is FIXED; we vary the number of OMP threads
#  and measure wall-clock throughput, speedup, and parallel efficiency.
#
#  Two images are benchmarked:
#    SMALL  →  resources/test_images/color_historic_document_2_2K.JPG  (~2K)
#    LARGE  →  resources/test_images/color_text_8K.jpg_no-copy                 (~8K)
#
#  Thread sweep (physical cores, then SMT pairs):
#    1  2  4  8  16  32
#
#  CPU affinity strategy:
#    ≤ 16 threads → pin to physical cores 0-N  (taskset -c 0-<N-1>)
#     = 32 threads → pin to all logical cores  (taskset -c 0-31)
#
#  Outputs:
#    bench_logs.bak5.bak4/strong_scaling/<image>/<image>_<N>threads.txt  — raw per-run log
#    bench_logs.bak5.bak4/strong_scaling/<image>/summary.csv             — speedup table
#
#  Usage:
#    ./tests/strong_scaling_bench.sh [--scope small|large|both]
#
#  Scope selection:
#    small  → benchmark only the 2K image
#    large  → benchmark only the sp image
#    both   → benchmark both images (default)
# ==============================================================================

set -euo pipefail

# ---------------------------------------------------------------------------
# 1. Configuration — tune these for the target machine / budget
# ---------------------------------------------------------------------------
PROG="./cmake-build-release/src/ite"
TEST_DIR="resources/test_images"
LOG_BASE="bench_logs/strong_scaling"

# Images to benchmark (relative to TEST_DIR)
SMALL_IMG="color_historic_document_2_2K.JPG"
LARGE_IMG="color_text_8K.jpg"

# Thread counts to sweep.
# Ryzen 9950X3D: 16 physical cores, 32 logical (SMT enabled).
# Powers-of-two plus the physical-core ceiling and full SMT.
THREAD_COUNTS=(1 2 4 6 8 10 12 14 16 32)

# Trials / warmup — adapted per image size so the benchmark finishes in
# a reasonable wall time on this CPU class.
#
#   SMALL (2K): processing is fast → more trials for stable statistics
#   LARGE (8K): processing is slower → fewer trials, longer per-trial
SMALL_TRIALS=50
SMALL_WARMUP=5
SMALL_TIME_LIMIT=30   # minutes hard cap per (image × thread-count) run

LARGE_TRIALS=10
LARGE_TRIALS_T1=5     # number of trials for the T=1 (serial) run of the large image
LARGE_WARMUP=2
LARGE_TIME_LIMIT=600   # minutes hard cap per run

# Full pipeline flags
FLAGS="-t --do-deskew --do-adaptive-gaussian --do-adaptive-median --binarization bataineh --do-despeckle
--do-dilation --do-erosion --do-color-pass"

OUTPUT_DIR="output/strong_scaling"
BENCH_SCOPE="both"

usage() {
    cat <<'EOF'
Usage: tests/strong_scaling_bench.sh [--scope small|large|both]

Options:
  --scope VALUE   Select benchmark image set (default: both)
  -h, --help      Show this help text and exit
EOF
}

parse_args() {
    while [ "$#" -gt 0 ]; do
        case "$1" in
            --scope)
                if [ "$#" -lt 2 ]; then
                    echo "Error: --scope requires a value (small|large|both)." >&2
                    usage
                    exit 1
                fi
                BENCH_SCOPE="$2"
                shift 2
                ;;
            -h|--help)
                usage
                exit 0
                ;;
            *)
                echo "Error: Unknown argument '$1'." >&2
                usage
                exit 1
                ;;
        esac
    done
}

parse_args "$@"

case "$BENCH_SCOPE" in
    small)
        SELECTED_IMAGES=("$SMALL_IMG")
        ;;
    large)
        SELECTED_IMAGES=("$LARGE_IMG")
        ;;
    both)
        SELECTED_IMAGES=("$SMALL_IMG" "$LARGE_IMG")
        ;;
    *)
        echo "Error: Invalid --scope '$BENCH_SCOPE'. Expected: small, large, or both." >&2
        usage
        exit 1
        ;;
esac

# ---------------------------------------------------------------------------
# 2. Validation
# ---------------------------------------------------------------------------
if [ ! -f "$PROG" ]; then
    echo "Error: Binary '$PROG' not found. Build the project first." >&2
    exit 1
fi

for IMG in "${SELECTED_IMAGES[@]}"; do
    if [ ! -f "$TEST_DIR/$IMG" ]; then
        echo "Error: Image '$TEST_DIR/$IMG' not found." >&2
        exit 1
    fi
done

if ! command -v taskset &>/dev/null; then
    echo "Error: 'taskset' is required (install util-linux)." >&2
    exit 1
fi

mkdir -p "$OUTPUT_DIR"

# ---------------------------------------------------------------------------
# 3. Helper: return the taskset CPU-list for a given thread count
#    Physical cores 0-15 map to logical CPUs 0-15 on a typical Zen 5 layout.
#    SMT siblings are 16-31.  For ≤16 threads we stay on physical cores only.
# ---------------------------------------------------------------------------
cpu_list_for_threads() {
    local n="$1"
    if   [ "$n" -eq 1  ]; then echo "0"
    elif [ "$n" -eq 2  ]; then echo "0-1"
    elif [ "$n" -eq 4  ]; then echo "0-3"
    elif [ "$n" -eq 8  ]; then echo "0-7"
    elif [ "$n" -eq 16 ]; then echo "0-15"
    elif [ "$n" -eq 32 ]; then echo "0-31"
    else
        # Generic fallback: 0 to N-1
        echo "0-$((n-1))"
    fi
}

# ---------------------------------------------------------------------------
# 4. Helper: run one (image, thread-count) combination and capture total time
# ---------------------------------------------------------------------------
# Returns the average TOTAL wall time in ms via the global LAST_TOTAL_MS.
LAST_TOTAL_MS=""

run_benchmark() {
    local img_path="$1"
    local basename="$2"
    local threads="$3"
    local trials="$4"
    local warmup="$5"
    local time_limit="$6"
    local log_dir="$7"

    local cpu_list
    cpu_list=$(cpu_list_for_threads "$threads")

    local log_file="$log_dir/${basename}_${threads}threads.txt"
    local out_img="$OUTPUT_DIR/out_${basename}_${threads}threads.jpg"

    export OMP_NUM_THREADS="$threads"

    printf "    [T=%2d | CPUs %-5s] %-45s → %s\n" \
        "$threads" "$cpu_list" "$basename" "$log_file"

    # Run; redirect stderr (progress bar) to /dev/null to keep output clean
    taskset -c "$cpu_list" "$PROG" $FLAGS \
        --input  "$img_path" \
        --output "$out_img"  \
        --trials "$trials"   \
        --warmup "$warmup"   \
        --time-limit "$time_limit" \
        > "$log_file" 2>/dev/null

    # Extract the TOTAL average (ms) from the benchmark table
    # Line looks like:  "TOTAL                            68280.985  ..."
    LAST_TOTAL_MS=$(grep -E '^TOTAL' "$log_file" | awk '{print $2}')
    if [ -z "$LAST_TOTAL_MS" ]; then
        LAST_TOTAL_MS="N/A"
    fi
}

# ---------------------------------------------------------------------------
# 5. Helper: write CSV summary for one image
# ---------------------------------------------------------------------------
write_summary() {
    local log_dir="$1"
    local csv="$log_dir/summary.csv"

    # summary_data is a global associative array:  threads → total_ms
    echo "threads,avg_total_ms,speedup,parallel_efficiency_pct" > "$csv"

    local serial_ms="${summary_data[1]:-}"
    if [ -z "$serial_ms" ] || [ "$serial_ms" = "N/A" ]; then
        echo "  [warn] No serial (1-thread) result; speedup columns will be N/A."
        for t in "${THREAD_COUNTS[@]}"; do
            local ms="${summary_data[$t]:-N/A}"
            echo "$t,$ms,N/A,N/A" >> "$csv"
        done
    else
        for t in "${THREAD_COUNTS[@]}"; do
            local ms="${summary_data[$t]:-N/A}"
            if [ "$ms" = "N/A" ]; then
                echo "$t,N/A,N/A,N/A" >> "$csv"
            else
                # Use awk for floating-point arithmetic
                local speedup eff
                speedup=$(awk "BEGIN{printf \"%.4f\", $serial_ms/$ms}")
                eff=$(awk     "BEGIN{printf \"%.2f\",  ($serial_ms/$ms)/$t*100}")
                echo "$t,$ms,$speedup,$eff" >> "$csv"
            fi
        done
    fi

    echo ""
    echo "  === Summary: $(basename "$log_dir") ==="
    column -t -s',' "$csv"
    echo ""
}

# ---------------------------------------------------------------------------
# 6. Main loop
# ---------------------------------------------------------------------------
echo ""
echo "=============================================================="
echo "  Strong Scaling Benchmark — Image Text Enhancer"
echo "  Scope: $BENCH_SCOPE"
echo "  Thread sweep: ${THREAD_COUNTS[*]}"
echo "  Pipeline: deskew + adaptive-gaussian + median + adaptive-median"
echo "            + bataineh binarization + despeckle + dilation + erosion"
echo "            + color-pass"
echo "=============================================================="
echo ""

declare -A IMAGE_SPECS=(
    ["$SMALL_IMG"]="$SMALL_TRIALS:0:$SMALL_WARMUP:$SMALL_TIME_LIMIT"
    ["$LARGE_IMG"]="$LARGE_TRIALS:$LARGE_TRIALS_T1:$LARGE_WARMUP:$LARGE_TIME_LIMIT"
)

for IMG_FILE in "${SELECTED_IMAGES[@]}"; do

    IMG_PATH="$TEST_DIR/$IMG_FILE"
    SPEC="${IMAGE_SPECS[$IMG_FILE]}"
    TRIALS="${SPEC%%:*}"
    REST="${SPEC#*:}"
    T1_TRIALS="${REST%%:*}"
    REST="${REST#*:}"
    WARMUP="${REST%%:*}"
    TIME_LIMIT="${REST##*:}"

    LOG_DIR="$LOG_BASE/$IMG_FILE"
    mkdir -p "$LOG_DIR"

    echo "--------------------------------------------------------------"
    echo "  Image : $IMG_FILE"
    echo "  Trials: $TRIALS  |  Warmup: $WARMUP  |  Time cap: ${TIME_LIMIT}m/run"
    echo "  Note: For T=1, using $T1_TRIALS trials and 1 warm-up (overriding default $TRIALS) to keep runtime
    reasonable."
    echo "--------------------------------------------------------------"

    declare -A summary_data=()

    for T in "${THREAD_COUNTS[@]}"; do
        # Special case: if T1_TRIALS is set (>0) and this is the serial run, use it
        local_trials="$TRIALS"
        local_warmup="$WARMUP"
        if [ "$T" -eq 1 ] && [ "${T1_TRIALS:-0}" -gt 0 ]; then
            local_trials="$T1_TRIALS"
            local_warmup="0"
        fi
        run_benchmark \
            "$IMG_PATH" \
            "$IMG_FILE" \
            "$T" \
            "$local_trials" \
            "$local_warmup" \
            "$TIME_LIMIT" \
            "$LOG_DIR"
        summary_data[$T]="$LAST_TOTAL_MS"
    done

    write_summary "$LOG_DIR"
    unset summary_data

done

echo "=============================================================="
echo "  Benchmark complete."
echo "  Raw logs : $LOG_BASE/<image>/<image>_<N>threads.txt"
echo "  Summaries: $LOG_BASE/<image>/summary.csv"
echo "=============================================================="
