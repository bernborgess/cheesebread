#!/usr/bin/env bash
# Build the original C benchmark, run it, run the Cangjie translation, diff stdout.
set -u

CJ_OUT=/home/bernborgess/repos/cheesebread/cangjie_compiler/output
source "$CJ_OUT/envsetup.sh" >/dev/null 2>&1

WCET_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORK="${TMPDIR:-/tmp}/opencode/wcet-cmp"
mkdir -p "$WORK"

printf "%-16s %-8s %-8s %s\n" "BENCH" "C" "CJ" "STATUS"
printf "%-16s %-8s %-8s %s\n" "-----" "-" "--" "------"

for d in "$WCET_DIR"/*/; do
    name=$(basename "$d")
    csrc="$d$name.c"
    cjsrc="$d$name.cj"

    [ -f "$csrc" ] || continue

    # ---- C reference ----
    cbin="$WORK/${name}_c"
    gcc -O2 -w "$csrc" -o "$cbin" -lm 2>/dev/null
    if [ $? -ne 0 ]; then
        printf "%-16s %-8s %-8s %s\n" "$name" "BUILDFAIL" "-" "C compile failed"
        continue
    fi

    if grep -qE 'scanf|getchar|fread|fgets' "$csrc"; then
        cin=$(cd "$WORK" && timeout 60 "./${name}_c" </dev/null 2>&1)
    else
        cin=$(cd "$WORK" && timeout 60 "./${name}_c" 2>&1)
    fi
    crc=$?
    printf '%s' "$cin" > "$WORK/${name}_c.out"

    # ---- Cangjie ----
    [ -s "$cjsrc" ] || { printf "%-16s %-8s %-8s %s\n" "$name" "ok" "EMPTY" "no .cj source"; continue; }

    cjbin="$WORK/${name}_cj"
    if ! "$CANGJIE_HOME/bin/cjc" "$cjsrc" -o "$cjbin" >"$WORK/$name.cjerr" 2>&1; then
        printf "%-16s %-8s %-8s %s\n" "$name" "ok" "COMPILEFAIL" "$(grep -oE 'error:.*' "$WORK/$name.cjerr" | head -1 | cut -c1-50)"
        continue
    fi

    cjout=$(cd "$WORK" && timeout 60 "./${name}_cj" </dev/null 2>&1)
    cjrc=$?
    printf '%s' "$cjout" > "$WORK/${name}_cj.out"

    if diff -q "$WORK/${name}_c.out" "$WORK/${name}_cj.out" >/dev/null 2>&1 && [ "$crc" = "$cjrc" ]; then
        printf "%-16s %-8s %-8s %s\n" "$name" "ok" "ok" "MATCH"
    else
        printf "%-16s %-8s %-8s %s\n" "$name" "rc=$crc" "rc=$cjrc" "DIFF"
    fi
done
