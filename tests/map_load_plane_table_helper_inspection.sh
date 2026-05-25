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
  "map1 load table name: Wolf1 Map2" \
  "map1 load table plane0 offset: 2292 length: 1732 words: 4096" \
  "map1 load table plane1 offset: 4024 length: 1757 words: 4096"; do
  if [[ "$MAP_OUTPUT" != *"$expected"* ]]; then
    echo "missing expected map-load plane-table output: $expected"
    echo "got: $MAP_OUTPUT"
    exit 1
  fi
done

for expected in \
  "present map load table slot: 2" \
  "present map load table name: Wolf1 Map3" \
  "present map load table plane1 offset: 7749 length: 1404 words: 4096"; do
  if [[ "$PRESENT_OUTPUT" != *"$expected"* ]]; then
    echo "missing expected present-map plane-table output: $expected"
    echo "got: $PRESENT_OUTPUT"
    exit 1
  fi
done

echo "map load plane table helper inspection passed"
