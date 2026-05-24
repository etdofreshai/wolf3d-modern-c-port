#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$ROOT/build"
BIN="$BUILD_DIR/wolf3d_port"
VALID_DATA_DIR="$(cd "$ROOT/../game-data/base" && pwd)"
OVERFLOW_VALUE="999999999999999999999999999999"

mkdir -p "$BUILD_DIR"

gcc -std=c17 -Wall -Wextra -Werror \
  -I"$ROOT/include" \
  "$ROOT/src/main.c" \
  "$ROOT/src/platform/platform_stub.c" \
  "$ROOT/src/platform/filesystem.c" \
  "$ROOT/src/core/assets.c" \
  -o "$BIN"

set +e
LOAD_OUTPUT="$($BIN --inspect-present-map-load "$OVERFLOW_VALUE" --data "$VALID_DATA_DIR" 2>&1)"
LOAD_STATUS=$?
COLUMN_OUTPUT="$($BIN --inspect-present-map-column 2 0 "$OVERFLOW_VALUE" --data "$VALID_DATA_DIR" 2>&1)"
COLUMN_STATUS=$?
set -e

if [[ $LOAD_STATUS -eq 0 ]]; then
  echo "expected present-map load overflow parse to fail"
  exit 1
fi
if [[ $COLUMN_STATUS -eq 0 ]]; then
  echo "expected present-map column overflow parse to fail"
  exit 1
fi

if [[ "$LOAD_OUTPUT" != *"--inspect-present-map-load index must be a non-negative integer"* ]]; then
  echo "missing expected present-map load overflow message"
  echo "got: $LOAD_OUTPUT"
  exit 1
fi

if [[ "$COLUMN_OUTPUT" != *"--inspect-present-map-column x must be a non-negative integer"* ]]; then
  echo "missing expected present-map column overflow message"
  echo "got: $COLUMN_OUTPUT"
  exit 1
fi

echo "present map CLI overflow validation passed"
