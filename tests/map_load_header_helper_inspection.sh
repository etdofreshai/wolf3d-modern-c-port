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

MAP_OUTPUT="$($BIN --inspect-map-load 1 --data "$VALID_DATA_DIR")"
PRESENT_OUTPUT="$($BIN --inspect-present-map-load 2 --data "$VALID_DATA_DIR")"

for expected in \
  "map1 plane0 header: offset=2292 length=1732 carmack=4236 rlew=8192 words=4096" \
  "map1 plane0 header/result match: yes" \
  "map1 plane1 header: offset=4024 length=1757 carmack=2878 rlew=8192 words=4096" \
  "map1 plane1 header/result match: yes"; do
  if [[ "$MAP_OUTPUT" != *"$expected"* ]]; then
    echo "missing expected map-load helper output: $expected"
    echo "got: $MAP_OUTPUT"
    exit 1
  fi
done

for expected in \
  "present map plane0 header: offset=5833 length=1916 carmack=4916 rlew=8192 words=4096" \
  "present map plane0 header/result match: yes" \
  "present map plane1 header: offset=7749 length=1404 carmack=2180 rlew=8192 words=4096" \
  "present map plane1 header/result match: yes"; do
  if [[ "$PRESENT_OUTPUT" != *"$expected"* ]]; then
    echo "missing expected present-map helper output: $expected"
    echo "got: $PRESENT_OUTPUT"
    exit 1
  fi
done

echo "map load header helper inspection passed"
