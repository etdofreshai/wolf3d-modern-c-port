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

PLANE0_OUTPUT="$($BIN --inspect-map-column 1 0 31 --data "$VALID_DATA_DIR")"
PLANE1_OUTPUT="$($BIN --inspect-map-column 1 1 31 --data "$VALID_DATA_DIR")"

for expected in \
  "map1 plane0 column31 length: 64" \
  "map1 plane0 column31 cells: [0]=12 [1]=12 [31]=120 [32]=91 [33]=117 [34]=117 [63]=9"; do
  if [[ "$PLANE0_OUTPUT" != *"$expected"* ]]; then
    echo "missing expected column output: $expected"
    echo "got: $PLANE0_OUTPUT"
    exit 1
  fi
done

for expected in \
  "map1 plane1 column31 length: 64" \
  "map1 plane1 column31 cells: [0]=0 [1]=0 [31]=0 [32]=0 [33]=0 [34]=0 [63]=0"; do
  if [[ "$PLANE1_OUTPUT" != *"$expected"* ]]; then
    echo "missing expected column output: $expected"
    echo "got: $PLANE1_OUTPUT"
    exit 1
  fi
done

echo "map column inspection passed"
