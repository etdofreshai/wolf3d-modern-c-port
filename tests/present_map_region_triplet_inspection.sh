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

REGION_TRIPLETS_OUTPUT="$($BIN --inspect-present-map-region-triplets 2 31 31 2 2 --data "$VALID_DATA_DIR")"

for expected in \
  "present map2 region31,31 triplet size: 2x2" \
  "present map2 region31,31 triplets: [0,0]=4,0,0 [1,0]=1,0,0 [0,1]=126,0,0 [1,1]=126,182,0"; do
  if [[ "$REGION_TRIPLETS_OUTPUT" != *"$expected"* ]]; then
    echo "missing expected present-map region triplet output: $expected"
    echo "got: $REGION_TRIPLETS_OUTPUT"
    exit 1
  fi
done

echo "present map region triplet inspection passed"
