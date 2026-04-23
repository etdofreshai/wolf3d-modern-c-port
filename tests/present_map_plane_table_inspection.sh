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

OUTPUT="$($BIN --inspect-present-map-plane-table 2 --data "$VALID_DATA_DIR")"

for expected in \
  "present map plane table index: 2" \
  "present map plane table slot: 2" \
  "present map plane table name: Wolf1 Map3" \
  "present map plane table header valid: yes" \
  "present map plane0 table offset: 5833 length: 1916 carmack: 4916 rlew: 8192 words: 4096" \
  "present map plane1 table offset: 7749 length: 1404 carmack: 2180 rlew: 8192 words: 4096" \
  "present map plane2 table offset: 9153 length: 10 carmack: 8 rlew: 8192 words: 4096"; do
  if [[ "$OUTPUT" != *"$expected"* ]]; then
    echo "missing expected present-map plane table output: $expected"
    echo "got: $OUTPUT"
    exit 1
  fi
done

echo "present map plane table inspection passed"
