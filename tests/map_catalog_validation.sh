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

OUTPUT="$($BIN --validate-map-catalog 3 --data "$VALID_DATA_DIR")"

for expected in \
  "map catalog validation count: 3" \
  "map catalog validation all valid: yes" \
  "map0 validation: yes" \
  "map1 validation: yes" \
  "map2 validation: yes"; do
  if [[ "$OUTPUT" != *"$expected"* ]]; then
    echo "missing expected map-catalog validation output: $expected"
    echo "got: $OUTPUT"
    exit 1
  fi
done

echo "map catalog validation passed"
