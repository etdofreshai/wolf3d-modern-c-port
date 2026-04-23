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

OUTPUT="$($BIN --inspect-first-map-load --data "$VALID_DATA_DIR")"

for expected in \
  "first map load name: Wolf1 Map1" \
  "first map load size: 64x64" \
  "first map plane0 result: compressed=1434 carmack=3190 rlew=8192 words=4096" \
  "first map plane1 result: compressed=795 carmack=1128 rlew=8192 words=4096" \
  "first map plane2 result: compressed=10 carmack=8 rlew=8192 words=4096" \
  "first map plane0 sample cells: [0,0]=1 [31,31]=108 [32,32]=108 [63,63]=1" \
  "first map plane1 sample cells: [0,0]=0 [31,31]=0 [32,32]=0 [63,63]=0" \
  "first map plane2 sample cells: [0,0]=0 [31,31]=0 [32,32]=0 [63,63]=0"; do
  if [[ "$OUTPUT" != *"$expected"* ]]; then
    echo "missing expected first-map load output: $expected"
    echo "got: $OUTPUT"
    exit 1
  fi
done

echo "first map load inspection passed"
