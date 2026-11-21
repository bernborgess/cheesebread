#!/usr/bin/env bash

set -euo pipefail

ARG="${1:-build-and-run}" # or just 'build'

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
COMPILER_DIR="$ROOT_DIR/cangjie_compiler"
BUILD_DIR="$COMPILER_DIR/build/build"
CACHE_FILE="$BUILD_DIR/CMakeCache.txt"

# Keep this script compatible with setup.sh: use the same clang toolchain.
CLANG_VERSION="${CLANG_VERSION:-18}"
CLANG_DIR="/usr/lib/llvm-${CLANG_VERSION}/bin"
if [[ ! -x "$CLANG_DIR/clang" || ! -x "$CLANG_DIR/clang++" ]]; then
    echo "clang-${CLANG_VERSION} does not exist. Please fix your clang installation." >&2
    exit 1
fi
export PATH="$CLANG_DIR:$PATH"

# Mirror the environment setup.sh exports for the compiler build.
export ARCH="${ARCH:-$(uname -m)}"
export WORKSPACE="${WORKSPACE:-$ROOT_DIR}"
export BUILD_ROOT="${BUILD_ROOT:-$ROOT_DIR/buildtools}"
export CMAKE_PREFIX_PATH="$BUILD_ROOT/libedit-3.1:$BUILD_ROOT/ncurses-6.5/usr${CMAKE_PREFIX_PATH:+:$CMAKE_PREFIX_PATH}"
export OPENSSL_PATH="${OPENSSL_PATH:-/usr/lib/${ARCH}-linux-gnu/libssl.so}"
if [[ -f "$OPENSSL_PATH" ]]; then
    export LD_LIBRARY_PATH="$OPENSSL_PATH${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
fi

cache_value() {
    grep -E "^$1:" "$CACHE_FILE" | head -n 1 | cut -d= -f2- || true
}

needs_configure=false
if [[ ! -f "$CACHE_FILE" ]]; then
    needs_configure=true
elif ! grep -Fqx 'CANGJIE_BUILD_TESTS:BOOL=ON' "$CACHE_FILE" || \
     ! grep -Fqx 'CANGJIE_CODEGEN_CJNATIVE_BACKEND:BOOL=ON' "$CACHE_FILE"; then
    needs_configure=true
else
    cached_cc="$(cache_value CMAKE_C_COMPILER)"
    cached_cxx="$(cache_value CMAKE_CXX_COMPILER)"
    if [[ "$cached_cc" != "$CLANG_DIR/clang" || "$cached_cxx" != "$CLANG_DIR/clang++" ]]; then
        cat >&2 <<EOF
error: $BUILD_DIR was configured with:
  CMAKE_C_COMPILER=$cached_cc
  CMAKE_CXX_COMPILER=$cached_cxx
but run_unit.sh requires clang-${CLANG_VERSION}:
  $CLANG_DIR/clang
  $CLANG_DIR/clang++

CMake cannot switch compilers in an existing cache. Rebuild the compiler
directory with clang-${CLANG_VERSION} first, for example:
  FRESH=true ./setup.sh
or remove the build directory and rerun this script.
EOF
        exit 1
    fi
fi

if $needs_configure; then
    cmake -S "$COMPILER_DIR" -B "$BUILD_DIR" \
        -DCANGJIE_BUILD_TESTS=ON \
        -DCANGJIE_CODEGEN_CJNATIVE_BACKEND=ON \
        -DCMAKE_C_COMPILER="$CLANG_DIR/clang" \
        -DCMAKE_CXX_COMPILER="$CLANG_DIR/clang++"
fi

cmake --build "$BUILD_DIR" --target CompetitionSSATests

if [ "$ARG" != "build" ]
then
    ctest --test-dir "$BUILD_DIR" -R '^CompetitionSSATests$' --output-on-failure
fi
