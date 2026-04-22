#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$ROOT/build"
BIN="$BUILD_DIR/wolf3d_port"
VALID_DATA_DIR="$(cd "$ROOT/../game-data/base" && pwd)"
INVALID_DATA_DIR="$ROOT/does-not-exist"

mkdir -p "$BUILD_DIR"

gcc -std=c17 -Wall -Wextra -Werror \
  -I"$ROOT/include" \
  "$ROOT/src/main.c" \
  "$ROOT/src/platform/platform_stub.c" \
  "$ROOT/src/platform/filesystem.c" \
  -o "$BIN"

SUCCESS_OUTPUT="$($BIN --check-data --data "$VALID_DATA_DIR")"
EXPECTED_PREFIX="data path ok: $VALID_DATA_DIR"
if [[ "$SUCCESS_OUTPUT" != "$EXPECTED_PREFIX" ]]; then
  echo "expected success output: $EXPECTED_PREFIX"
  echo "got: $SUCCESS_OUTPUT"
  exit 1
fi

set +e
FAIL_OUTPUT="$($BIN --check-data --data "$INVALID_DATA_DIR" 2>&1)"
FAIL_CODE=$?
set -e

if [[ $FAIL_CODE -eq 0 ]]; then
  echo "expected invalid path check to fail"
  exit 1
fi

if [[ "$FAIL_OUTPUT" != *"missing required Wolf3D data files"* ]]; then
  echo "expected missing-data error message"
  echo "got: $FAIL_OUTPUT"
  exit 1
fi

echo "data path validation passed"
