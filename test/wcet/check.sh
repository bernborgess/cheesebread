#!/usr/bin/env bash
# Compile and run every translated WCET benchmark, report status.
# Usage: ./check.sh [--run]

set -u

CJ_OUT=/home/bernborgess/repos/cheesebread/cangjie_compiler/output
source "$CJ_OUT/envsetup.sh" >/dev/null 2>&1

WCET_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORK="${TMPDIR:-/tmp}/opencode/wcet-check"
mkdir -p "$WORK"

DO_RUN=0
[ "${1:-}" = "--run" ] && DO_RUN=1

printf "%-16s %8s  %-10s %s\n" "BENCH" "BYTES" "COMPILE" "RUN"
printf "%-16s %8s  %-10s %s\n" "-----" "-----" "-------" "---"

for d in "$WCET_DIR"/*/; do
    name=$(basename "$d")
    src="$d$name.cj"

    [ -f "$src" ] || continue

    bytes=$(stat -c%s "$src")

    if [ "$bytes" -eq 0 ]; then
        printf "%-16s %8s  %-10s %s\n" "$name" "$bytes" "EMPTY" "-"
        continue
    fi

    out="$WORK/$name"
    err=$("$CANGJIE_HOME/bin/cjc" "$src" -o "$out" 2>&1)
    rc=$?

    if [ $rc -ne 0 ]; then
        reason=$(printf '%s' "$err" | grep -oE 'error:.*' | head -1 | cut -c1-60)
        printf "%-16s %8s  %-10s %s\n" "$name" "$bytes" "FAIL" "${reason:-unknown}"
        continue
    fi

    if [ $DO_RUN -eq 0 ]; then
        printf "%-16s %8s  %-10s %s\n" "$name" "$bytes" "OK" "-"
        continue
    fi

    runout=$(cd "$WORK" && timeout 30 "./$name" </dev/null 2>&1)
    runrc=$?
    printf "%-16s %8s  %-10s %s\n" "$name" "$bytes" "OK" "exit=$runrc"
done
