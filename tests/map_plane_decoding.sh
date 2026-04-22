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

OUTPUT="$($BIN --inspect-first-map-plane0 --data "$VALID_DATA_DIR")"

for expected in \
  "plane0 compressed bytes: 1434" \
  "plane0 carmack expanded bytes: 3190" \
  "plane0 rlew expanded bytes: 8192" \
  "plane0 decoded words: 4096" \
  "plane0 cells: [0,0]=1 [31,31]=108 [32,32]=108 [63,63]=1"; do
  if [[ "$OUTPUT" != *"$expected"* ]]; then
    echo "missing expected output: $expected"
    echo "got: $OUTPUT"
    exit 1
  fi
done

echo "map plane decoding passed"
