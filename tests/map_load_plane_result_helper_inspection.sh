#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$ROOT/build"
BIN="$BUILD_DIR/wolf3d_port"
VALID_DATA_DIR="$ROOT/../game-data/base"

mkdir -p "$BUILD_DIR"

gcc -std=c17 -Wall -Wextra -Werror \
  -I"$ROOT/include" \
  "$ROOT/src/main.c" \
  "$ROOT/src/platform/platform_stub.c" \
  "$ROOT/src/platform/filesystem.c" \
  "$ROOT/src/core/assets.c" \
  -o "$BIN"

MAP_OUTPUT="$($BIN --inspect-map-load-result 1 0 --data "$VALID_DATA_DIR")"
PRESENT_OUTPUT="$($BIN --inspect-present-map-load-result 2 1 --data "$VALID_DATA_DIR")"

for expected in \
  "map1 load result plane0 compressed: 1732" \
  "map1 load result plane0 carmack: 4236" \
  "map1 load result plane0 rlew: 8192" \
  "map1 load result plane0 words: 4096"; do
  if [[ "$MAP_OUTPUT" != *"$expected"* ]]; then
    echo "missing expected map-load result output: $expected"
    echo "got: $MAP_OUTPUT"
    exit 1
  fi
done

for expected in \
  "present map load result index: 2" \
  "present map load result slot: 2" \
  "present map load result plane1 compressed: 1404" \
  "present map load result plane1 carmack: 2180" \
  "present map load result plane1 rlew: 8192" \
  "present map load result plane1 words: 4096"; do
  if [[ "$PRESENT_OUTPUT" != *"$expected"* ]]; then
    echo "missing expected present-map load result output: $expected"
    echo "got: $PRESENT_OUTPUT"
    exit 1
  fi
done

echo "map load plane result helper inspection passed"
