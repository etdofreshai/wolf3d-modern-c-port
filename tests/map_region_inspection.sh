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

OUTPUT="$($BIN --inspect-map-region 1 0 31 31 2 2 --data "$VALID_DATA_DIR")"

for expected in \
  "map1 plane0 region31,31 size: 2x2" \
  "map1 plane0 region31,31 cells: [0,0]=120 [1,0]=120 [0,1]=91 [1,1]=1"; do
  if [[ "$OUTPUT" != *"$expected"* ]]; then
    echo "missing expected region output: $expected"
    echo "got: $OUTPUT"
    exit 1
  fi
done

echo "map region inspection passed"
