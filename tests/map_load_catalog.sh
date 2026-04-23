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

OUTPUT="$($BIN --inspect-map-load-catalog 2 --data "$VALID_DATA_DIR")"

for expected in \
  "map load catalog total: 100" \
  "map load0 name: Wolf1 Map1" \
  "map load0 size: 64x64" \
  "map load0 plane0 result: compressed=1434 carmack=3190 rlew=8192 words=4096" \
  "map load0 plane1 result: compressed=795 carmack=1128 rlew=8192 words=4096" \
  "map load0 plane2 result: compressed=10 carmack=8 rlew=8192 words=4096" \
  "map load0 plane0 sample cells: [0,0]=1 [31,31]=108 [32,32]=108 [63,63]=1" \
  "map load0 plane1 sample cells: [0,0]=0 [31,31]=0 [32,32]=0 [63,63]=0" \
  "map load0 plane2 sample cells: [0,0]=0 [31,31]=0 [32,32]=0 [63,63]=0" \
  "map load1 name: Wolf1 Map2" \
  "map load1 size: 64x64" \
  "map load1 plane0 result: compressed=1732 carmack=4236 rlew=8192 words=4096" \
  "map load1 plane1 result: compressed=1757 carmack=2878 rlew=8192 words=4096" \
  "map load1 plane2 result: compressed=10 carmack=8 rlew=8192 words=4096" \
  "map load1 plane0 sample cells: [0,0]=1 [31,31]=120 [32,32]=1 [63,63]=1" \
  "map load1 plane1 sample cells: [0,0]=0 [31,31]=0 [32,32]=0 [63,63]=0" \
  "map load1 plane2 sample cells: [0,0]=0 [31,31]=0 [32,32]=0 [63,63]=0"; do
  if [[ "$OUTPUT" != *"$expected"* ]]; then
    echo "missing expected map-load catalog output: $expected"
    echo "got: $OUTPUT"
    exit 1
  fi
done

echo "map load catalog passed"
