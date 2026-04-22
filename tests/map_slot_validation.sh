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

OUTPUT59="$($BIN --inspect-map-slot 59 --data "$VALID_DATA_DIR")"
OUTPUT60="$($BIN --inspect-map-slot 60 --data "$VALID_DATA_DIR")"

for expected in \
  "map59 present: yes" \
  "map59 offset: 150610"; do
  if [[ "$OUTPUT59" != *"$expected"* ]]; then
    echo "missing expected slot output: $expected"
    echo "got: $OUTPUT59"
    exit 1
  fi
done

for expected in \
  "map60 present: no" \
  "map60 offset: 0"; do
  if [[ "$OUTPUT60" != *"$expected"* ]]; then
    echo "missing expected slot output: $expected"
    echo "got: $OUTPUT60"
    exit 1
  fi
done

if "$BIN" --inspect-map-slot 100 --data "$VALID_DATA_DIR" >"$BUILD_DIR/map_slot_oob.out" 2>"$BUILD_DIR/map_slot_oob.err"; then
  echo "expected out-of-range map slot inspection to fail"
  exit 1
fi

if [[ "$(<"$BUILD_DIR/map_slot_oob.err")" != *"map index is out of range"* ]]; then
  echo "missing out-of-range map slot error"
  echo "stderr: $(<"$BUILD_DIR/map_slot_oob.err")"
  exit 1
fi

echo "map slot validation passed"
