#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$ROOT/build"
BIN="$BUILD_DIR/wolf3d_port"
VALID_DATA_DIR="$(cd "$ROOT/../game-data/base" && pwd)"

mkdir -p "$BUILD_DIR"

gcc -std=c17 -Wall -Wextra -Werror \
  -I"$ROOT/include" \
  "$ROOT/src/main.c" \
  "$ROOT/src/platform/platform_stub.c" \
  "$ROOT/src/platform/filesystem.c" \
  "$ROOT/src/core/assets.c" \
  -o "$BIN"

CELL_OUTPUT="$($BIN --inspect-present-map-cell 2 32 32 --data "$VALID_DATA_DIR")"
ROW_OUTPUT="$($BIN --inspect-present-map-row 2 0 31 --data "$VALID_DATA_DIR")"
REGION_OUTPUT="$($BIN --inspect-present-map-region 2 1 31 31 2 2 --data "$VALID_DATA_DIR")"

for expected in \
  "present map2 cell[32,32] plane0: 126" \
  "present map2 cell[32,32] plane1: 182" \
  "present map2 cell[32,32] plane2: 0"; do
  if [[ "$CELL_OUTPUT" != *"$expected"* ]]; then
    echo "missing expected present-map cell output: $expected"
    echo "got: $CELL_OUTPUT"
    exit 1
  fi
done

for expected in \
  "present map2 plane0 row31 length: 64" \
  "present map2 plane0 row31 cells: [0]=15 [1]=121 [31]=4 [32]=1 [33]=2 [34]=2 [63]=2"; do
  if [[ "$ROW_OUTPUT" != *"$expected"* ]]; then
    echo "missing expected present-map row output: $expected"
    echo "got: $ROW_OUTPUT"
    exit 1
  fi
done

for expected in \
  "present map2 plane1 region31,31 size: 2x2" \
  "present map2 plane1 region31,31 cells: [0,0]=0 [1,0]=0 [0,1]=0 [1,1]=182"; do
  if [[ "$REGION_OUTPUT" != *"$expected"* ]]; then
    echo "missing expected present-map region output: $expected"
    echo "got: $REGION_OUTPUT"
    exit 1
  fi
done

echo "present map helper inspection passed"
