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

MAP_OUTPUT="$($BIN --inspect-map 1 --data "$VALID_DATA_DIR")"
PLANE_OUTPUT="$($BIN --inspect-map-plane 1 0 --data "$VALID_DATA_DIR")"

for expected in \
  "map1 name: Wolf1 Map2" \
  "map1 size: 64x64" \
  "map1 plane0 offset: 2292" \
  "map1 plane1 offset: 4024" \
  "map1 plane2 offset: 5781" \
  "map1 plane0 length: 1732"; do
  if [[ "$MAP_OUTPUT" != *"$expected"* ]]; then
    echo "missing expected map output: $expected"
    echo "got: $MAP_OUTPUT"
    exit 1
  fi
done

for expected in \
  "map1 plane0 compressed bytes: 1732" \
  "map1 plane0 carmack expanded bytes: 4236" \
  "map1 plane0 rlew expanded bytes: 8192" \
  "map1 plane0 decoded words: 4096" \
  "map1 plane0 cells: [0,0]=1 [31,31]=120 [32,32]=1 [63,63]=1"; do
  if [[ "$PLANE_OUTPUT" != *"$expected"* ]]; then
    echo "missing expected plane output: $expected"
    echo "got: $PLANE_OUTPUT"
    exit 1
  fi
done

echo "map index loading passed"
