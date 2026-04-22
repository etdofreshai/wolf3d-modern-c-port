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

CELL_31_OUTPUT="$($BIN --inspect-map-cell 1 31 31 --data "$VALID_DATA_DIR")"
CELL_32_OUTPUT="$($BIN --inspect-map-cell 1 32 32 --data "$VALID_DATA_DIR")"

for expected in \
  "map1 cell[31,31] plane0: 120" \
  "map1 cell[31,31] plane1: 0" \
  "map1 cell[31,31] plane2: 0"; do
  if [[ "$CELL_31_OUTPUT" != *"$expected"* ]]; then
    echo "missing expected cell output: $expected"
    echo "got: $CELL_31_OUTPUT"
    exit 1
  fi
done

for expected in \
  "map1 cell[32,32] plane0: 1" \
  "map1 cell[32,32] plane1: 0" \
  "map1 cell[32,32] plane2: 0"; do
  if [[ "$CELL_32_OUTPUT" != *"$expected"* ]]; then
    echo "missing expected cell output: $expected"
    echo "got: $CELL_32_OUTPUT"
    exit 1
  fi
done

echo "map cell inspection passed"
