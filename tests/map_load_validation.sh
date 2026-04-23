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

OUTPUT="$($BIN --validate-map-load 1 --data "$VALID_DATA_DIR")"

for expected in \
  "map1 load valid: yes" \
  "map1 load name: Wolf1 Map2" \
  "map1 load size: 64x64" \
  "map1 load plane table valid: yes" \
  "map1 plane0 load result match: yes" \
  "map1 plane1 load result match: yes" \
  "map1 plane2 load result match: yes"; do
  if [[ "$OUTPUT" != *"$expected"* ]]; then
    echo "missing expected map-load validation output: $expected"
    echo "got: $OUTPUT"
    exit 1
  fi
done

echo "map load validation passed"
