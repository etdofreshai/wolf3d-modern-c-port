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

OUTPUT="$($BIN --validate-map-plane-table 1 --data "$VALID_DATA_DIR")"

for expected in \
  "map1 plane table valid: yes" \
  "map1 plane0 valid: yes" \
  "map1 plane1 valid: yes" \
  "map1 plane2 valid: yes"; do
  if [[ "$OUTPUT" != *"$expected"* ]]; then
    echo "missing expected plane-table validation output: $expected"
    echo "got: $OUTPUT"
    exit 1
  fi
done

echo "map plane table validation passed"
