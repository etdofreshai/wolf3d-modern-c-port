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

OUTPUT="$($BIN --inspect-maphead --data "$VALID_DATA_DIR")"

if [[ "$OUTPUT" != *"maphead rlew_tag: 0xabcd"* ]]; then
  echo "expected RLEW tag output"
  echo "got: $OUTPUT"
  exit 1
fi

if [[ "$OUTPUT" != *"map count: 100"* ]]; then
  echo "expected map count output"
  echo "got: $OUTPUT"
  exit 1
fi

if [[ "$OUTPUT" != *"first map offset: 2250"* ]]; then
  echo "expected first map offset output"
  echo "got: $OUTPUT"
  exit 1
fi

echo "maphead inspection passed"
