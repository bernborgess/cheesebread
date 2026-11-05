#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
COMPILER_DIR="$ROOT_DIR/cangjie_compiler"
BUILD_DIR="$COMPILER_DIR/build/build"
CACHE_FILE="$BUILD_DIR/CMakeCache.txt"

if [[ ! -f "$CACHE_FILE" ]] || \
   ! grep -Fqx 'CANGJIE_BUILD_TESTS:BOOL=ON' "$CACHE_FILE" || \
   ! grep -Fqx 'CANGJIE_CODEGEN_CJNATIVE_BACKEND:BOOL=ON' "$CACHE_FILE"; then
    cmake -S "$COMPILER_DIR" -B "$BUILD_DIR" \
        -DCANGJIE_BUILD_TESTS=ON \
        -DCANGJIE_CODEGEN_CJNATIVE_BACKEND=ON
fi

cmake --build "$BUILD_DIR" --target CompetitionSSATests
ctest --test-dir "$BUILD_DIR" -R '^CompetitionSSATests$' --output-on-failure
