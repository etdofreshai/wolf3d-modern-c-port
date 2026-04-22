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

PLANE1_OUTPUT="$($BIN --inspect-first-map-plane 1 --data "$VALID_DATA_DIR")"
PLANE2_OUTPUT="$($BIN --inspect-first-map-plane 2 --data "$VALID_DATA_DIR")"

for expected in \
  "plane1 compressed bytes: 795" \
  "plane1 carmack expanded bytes: 1128" \
  "plane1 rlew expanded bytes: 8192" \
  "plane1 decoded words: 4096" \
  "plane1 cells: [0,0]=0 [31,31]=0 [32,32]=0 [63,63]=0"; do
  if [[ "$PLANE1_OUTPUT" != *"$expected"* ]]; then
    echo "missing expected plane1 output: $expected"
    echo "got: $PLANE1_OUTPUT"
    exit 1
  fi
done

for expected in \
  "plane2 compressed bytes: 10" \
  "plane2 carmack expanded bytes: 8" \
  "plane2 rlew expanded bytes: 8192" \
  "plane2 decoded words: 4096" \
  "plane2 cells: [0,0]=0 [31,31]=0 [32,32]=0 [63,63]=0"; do
  if [[ "$PLANE2_OUTPUT" != *"$expected"* ]]; then
    echo "missing expected plane2 output: $expected"
    echo "got: $PLANE2_OUTPUT"
    exit 1
  fi
done

echo "map plane layers passed"
