#!/usr/bin/env bash

set -uo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CJ_OUTPUT="$ROOT_DIR/cangjie_compiler/output"
CJC="$CJ_OUTPUT/bin/cjc"
SUITE="${1:-all}"

case "$SUITE" in
    all|handmade|wcet|flavors|cangjiebench) ;;
    -h|--help)
        printf 'Usage: %s [all|handmade|wcet|flavors|cangjiebench]\n' "$0"
        exit 0
        ;;
    *)
        printf 'Unknown suite: %s\nUsage: %s [all|handmade|wcet|flavors|cangjiebench]\n' "$SUITE" "$0" >&2
        exit 2
        ;;
esac

if [[ ! -x "$CJC" || ! -f "$CJ_OUTPUT/envsetup.sh" ]]; then
    printf 'Cangjie compiler output not found. Run setup.sh first.\n' >&2
    exit 2
fi

source "$CJ_OUTPUT/envsetup.sh"
export CANGJIE_HOME="$CJ_OUTPUT"

TMP_ROOT="$(mktemp -d "${TMPDIR:-/tmp}/cheesebread-test-noXXXX")"
mkdir -p "$TMP_ROOT/logs"

TOTAL=0
PASSED=0
FAILED=0
INPUT_PATHS=()
INPUT_BACKUPS=()
GENERATED_INPUTS=()
shopt -s nullglob

cleanup() {
    local index
    for ((index = 0; index < ${#INPUT_PATHS[@]}; index++)); do
        rm -f -- "${INPUT_PATHS[index]}"
        cp -a -- "${INPUT_BACKUPS[index]}" "${INPUT_PATHS[index]}"
    done
    for input in "${GENERATED_INPUTS[@]}"; do
        rm -f -- "$input"
    done
    rm -rf -- "$TMP_ROOT"
}
trap cleanup EXIT

run_case() {
    local label="$1"
    local work_dir="$2"
    local expected="$3"
    local log
    local status

    shift 3
    TOTAL=$((TOTAL + 1))
    log="$TMP_ROOT/logs/$TOTAL.log"

    if (cd "$work_dir" && "$CJC" "$@") >"$log" 2>&1; then
        :
    else
        status=$?
        FAILED=$((FAILED + 1))
        printf 'FAIL %s (compiler exit %s)\n' "$label" "$status"
        tail -n 8 "$log"
        return
    fi

    if grep -Fq 'sassert fail' "$log"; then
        FAILED=$((FAILED + 1))
        printf 'FAIL %s (static assertion failed)\n' "$label"
        grep -F 'sassert fail' "$log"
        return
    fi

    if [[ -n "$expected" ]]; then
        if [[ ! -f "$work_dir/output.txt" ]] || ! diff -u "$expected" "$work_dir/output.txt" >>"$log"; then
            FAILED=$((FAILED + 1))
            printf 'FAIL %s (output differs from expected.txt)\n' "$label"
            tail -n 8 "$log"
            return
        fi
    fi

    PASSED=$((PASSED + 1))
    printf 'PASS %s\n' "$label"
}

run_handmade() {
    local source_dir source name expected
    for source in "$ROOT_DIR"/test/handmade/*/code.cj; do
        [[ -f "$source" ]] || continue
        source_dir="$(dirname "$source")"
        name="$(basename "$source_dir")"
        expected=""
        if [[ -f "$source_dir/expected.txt" ]]; then
            expected="$source_dir/expected.txt"
        fi
        run_case "handmade/$name" "$source_dir" "$expected" code.cj -O2 -j1 -Woff unused
    done
}

run_wcet() {
    local source_dir name source input backup
    for source_dir in "$ROOT_DIR"/test/wcet/*/; do
        name="$(basename "$source_dir")"
        source="$source_dir/$name.cj"
        [[ -f "$source" ]] || continue
        input="$source_dir/input.txt"
        if [[ -e "$input" || -L "$input" ]]; then
            backup="$TMP_ROOT/wcet-input-$name"
            cp -a -- "$input" "$backup"
            INPUT_PATHS+=("$input")
            INPUT_BACKUPS+=("$backup")
        else
            GENERATED_INPUTS+=("$input")
        fi
        printf '[%s.cj, 0, x]\n' "$name" >"$input"
        run_case "wcet/$name" "$source_dir" "" "$name.cj" -O2 -j1
    done
}

run_flavors() {
    local source name
    for source in "$ROOT_DIR"/test/flavors/*.cj; do
        [[ -f "$source" ]] || continue
        name="$(basename "$source" .cj)"
        run_case "flavors/$name" "$ROOT_DIR/test/flavors" "" "$name.cj" -O2 -j1
    done
}

run_cangjiebench() {
    local id source
    mkdir -p "$TMP_ROOT/cangjiebench-outs"
    for id in {0..163}; do
        source="$ROOT_DIR/test/CangjieBench/$id.cj"
        [[ -f "$source" ]] || continue
        run_case "CangjieBench/$id" "$ROOT_DIR/test/CangjieBench" "" "$id.cj" -o "$TMP_ROOT/cangjiebench-outs" -Woff unused -O2 -j1 --output-type=staticlib
    done
}

printf 'Running suite: %s\nCompiler: %s\n' "$SUITE" "$CJC"
if [[ "$SUITE" == all || "$SUITE" == handmade ]]; then run_handmade; fi
if [[ "$SUITE" == all || "$SUITE" == wcet ]]; then run_wcet; fi
if [[ "$SUITE" == all || "$SUITE" == flavors ]]; then run_flavors; fi
if [[ "$SUITE" == all || "$SUITE" == cangjiebench ]]; then run_cangjiebench; fi

printf '\nResults: %s passed, %s failed, %s total\n' "$PASSED" "$FAILED" "$TOTAL"
if [[ "$TOTAL" -eq 0 ]]; then
    printf 'No tests found for suite: %s\n' "$SUITE" >&2
    exit 2
fi
[[ "$FAILED" -eq 0 ]]