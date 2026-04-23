#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$ROOT/build"
BIN="$BUILD_DIR/wolf3d_port"

mkdir -p "$BUILD_DIR"

gcc -std=c17 -Wall -Wextra -Werror \
  -I"$ROOT/include" \
  "$ROOT/src/main.c" \
  "$ROOT/src/platform/platform_stub.c" \
  "$ROOT/src/platform/filesystem.c" \
  "$ROOT/src/core/assets.c" \
  -o "$BIN"

OUTPUT="$($BIN --self-test-map-validation)"

for expected in \
  "map header valid ok" \
  "map header missing plane ok" \
  "map dimensions supported ok" \
  "map dimensions oversized ok" \
  "map plane header valid ok" \
  "map plane load result match ok" \
  "map plane load result mismatch ok" \
  "map plane table valid ok" \
  "map plane table invalid ok" \
  "map plane header odd size ok" \
  "map plane header missing Carmack size ok" \
  "map plane header wrong decoded size ok" \
  "map plane bounds overflow ok"; do
  if [[ "$OUTPUT" != *"$expected"* ]]; then
    echo "missing expected output: $expected"
    echo "got: $OUTPUT"
    exit 1
  fi
done

echo "map validation self-test passed"
