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

OUTPUT="$($BIN --validate-present-map-catalog 3 --data "$VALID_DATA_DIR")"

for expected in \
  "present map catalog validation total slots: 100" \
  "present map catalog validation total present: 60" \
  "present map catalog validation count: 3" \
  "present map catalog validation all valid: yes" \
  "present map0 slot: 0" \
  "present map0 validation: yes" \
  "present map1 slot: 1" \
  "present map1 validation: yes" \
  "present map2 slot: 2" \
  "present map2 validation: yes"; do
  if [[ "$OUTPUT" != *"$expected"* ]]; then
    echo "missing expected present-map catalog validation output: $expected"
    echo "got: $OUTPUT"
    exit 1
  fi
done

echo "present map catalog validation passed"
