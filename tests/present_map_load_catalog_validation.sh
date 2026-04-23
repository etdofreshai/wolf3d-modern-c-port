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

OUTPUT="$($BIN --validate-present-map-load-catalog 2 --data "$VALID_DATA_DIR")"

for expected in \
  "present map load catalog validation total slots: 100" \
  "present map load catalog validation total present: 60" \
  "present map load catalog validation count: 2" \
  "present map load catalog validation all valid: yes" \
  "present map load0 slot: 0" \
  "present map load0 validation: yes" \
  "present map load1 slot: 1" \
  "present map load1 validation: yes"; do
  if [[ "$OUTPUT" != *"$expected"* ]]; then
    echo "missing expected present-map load catalog validation output: $expected"
    echo "got: $OUTPUT"
    exit 1
  fi
done

echo "present map load catalog validation passed"
