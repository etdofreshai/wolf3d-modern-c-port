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

OUTPUT="$($BIN --inspect-map-presence-summary --data "$VALID_DATA_DIR")"

for expected in \
  "map slots total: 100" \
  "map slots present: 60" \
  "first present slot: 0" \
  "last present slot: 59" \
  "first empty slot: 60"; do
  if [[ "$OUTPUT" != *"$expected"* ]]; then
    echo "missing expected presence output: $expected"
    echo "got: $OUTPUT"
    exit 1
  fi
done

echo "map presence summary passed"
