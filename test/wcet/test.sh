#!/usr/bin/env bash

TESTS=('adpcm' 'bs' 'bsort100' 'cnt' 'compress' 'cover' 'crc' 'duff' 'edn' \
    'expint' 'fac' 'fdct' 'fft1' 'fibcall' 'fir' 'insertsort' 'janne_complex' \
    'jfdctint' 'lcdnum' 'lms' 'ludcmp' 'matmult' 'minver' 'ndes' 'ns' \
    'nsichneu' 'prime' 'qsort-exam' 'qurt' 'recursion' 'select' 'sqrt' 'st' \
    'statemate' 'ud')

CJC_OUTPUT_DIR=../../cangjie_compiler/output
CJC_PATH=$CJC_OUTPUT_DIR/bin/cjc
source $CJC_OUTPUT_DIR/envsetup.sh

GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color

TOTAL_PASSED=0
TOTAL_ASSERTIONS=0

# Print table header
printf "\n%-15s | %-8s | %-8s | %-10s | %-8s\n" "Benchmark" "Passed" "Total" "Ratio (%)" "Status"
printf "%s\n" "----------------------------------------------------------------"

for test in "${TESTS[@]}"; do
    cd "$test" || continue

    cat << EOF > input.txt
[$test.cj, 0, x]
EOF

    # Capture cjc output with benchmark name pre-pended to the line
    RAW_LOG=$( { echo -n "[BENCHMARK:$test] "; cjc "$test.cj" -O2 -j1 -Woff unused; } 2>&1 )

    cd ..

    # Extract 'Passed X out of Y' from output
    PASSED=$(echo "$RAW_LOG" | grep -oP 'Passed \K[0-9]+' || echo "0")
    TOTAL=$(echo "$RAW_LOG" | grep -oP 'out of \K[0-9]+' || echo "0")

    if [ "$TOTAL" -gt 0 ]; then
        RATIO=$(awk "BEGIN {printf \"%.1f\", ($PASSED/$TOTAL)*100}")
        TOTAL_PASSED=$((TOTAL_PASSED + PASSED))
        TOTAL_ASSERTIONS=$((TOTAL_ASSERTIONS + TOTAL))
    else
        RATIO="0.0"
    fi

    # Determine status tag
    if [ "$PASSED" -eq "$TOTAL" ] && [ "$TOTAL" -gt 0 ]; then
        STATUS="${GREEN}PASS${NC}"
    else
        STATUS="${RED}FAIL${NC}"
    fi

    # Render formatted row
    printf "%-15s | %-8d | %-8d | %-9.1f%% | %b\n" "$test" "$PASSED" "$TOTAL" "$RATIO" "$STATUS"
done

# Print final summary line
printf "%s\n" "----------------------------------------------------------------"
if [ "$TOTAL_ASSERTIONS" -gt 0 ]; then
    OVERALL_RATIO=$(awk "BEGIN {printf \"%.2f\", ($TOTAL_PASSED/$TOTAL_ASSERTIONS)*100}")
else
    OVERALL_RATIO="0.00"
fi

printf "${GREEN}%-15s | %-8d | %-8d | %-9.2f%%${NC}\n\n" "OVERALL TOTAL" "$TOTAL_PASSED" "$TOTAL_ASSERTIONS" "$OVERALL_RATIO"