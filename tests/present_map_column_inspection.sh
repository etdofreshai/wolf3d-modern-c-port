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

OUTPUT="$($BIN --inspect-present-map-column 2 0 31 --data "$VALID_DATA_DIR")"

for expected in \
  "present map2 plane0 column31 length: 64" \
  "present map2 plane0 column31 cells: [0]=1 [1]=112 [31]=4 [32]=126 [33]=126 [34]=126 [63]=1"; do
  if [[ "$OUTPUT" != *"$expected"* ]]; then
    echo "missing expected present-map column output: $expected"
    echo "got: $OUTPUT"
    exit 1
  fi
done

echo "present map column inspection passed"
