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

OUTPUT="$($BIN --validate-map-plane-header 1 0 --data "$VALID_DATA_DIR")"

for expected in \
  "map1 plane0 header valid: yes" \
  "map1 plane0 decoded size matches map: yes" \
  "map1 plane0 expected words: 4096" \
  "map1 plane0 decoded words: 4096"; do
  if [[ "$OUTPUT" != *"$expected"* ]]; then
    echo "missing expected plane-header validation output: $expected"
    echo "got: $OUTPUT"
    exit 1
  fi
done

echo "map plane header validation passed"
