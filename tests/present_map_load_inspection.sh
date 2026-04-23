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

OUTPUT="$($BIN --inspect-present-map-load 2 --data "$VALID_DATA_DIR")"

for expected in \
  "present map load index: 2" \
  "present map load slot: 2" \
  "present map load name: Wolf1 Map3" \
  "present map load size: 64x64" \
  "present map plane0 result: compressed=1916 carmack=4916 rlew=8192 words=4096" \
  "present map plane1 result: compressed=1404 carmack=2180 rlew=8192 words=4096" \
  "present map plane2 result: compressed=10 carmack=8 rlew=8192 words=4096" \
  "present map plane0 sample cells: [0,0]=2 [31,31]=4 [32,32]=126 [63,63]=2" \
  "present map plane1 sample cells: [0,0]=0 [31,31]=0 [32,32]=182 [63,63]=0" \
  "present map plane2 sample cells: [0,0]=0 [31,31]=0 [32,32]=0 [63,63]=0"; do
  if [[ "$OUTPUT" != *"$expected"* ]]; then
    echo "missing expected present-map load inspection output: $expected"
    echo "got: $OUTPUT"
    exit 1
  fi
done

echo "present map load inspection passed"
