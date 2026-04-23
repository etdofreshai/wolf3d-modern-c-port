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

OUTPUT="$($BIN --self-test-map-helpers)"

for expected in \
  "map helper index ok: 11" \
  "map helper plane ok: count=12 first=100 last=111" \
  "map helper column ok: count=3 top=102 bottom=110" \
  "map helper cell ok: 107" \
  "map helper oob index ok" \
  "map helper oob cell ok" \
  "map helper invalid plane ok"; do
  if [[ "$OUTPUT" != *"$expected"* ]]; then
    echo "missing expected output: $expected"
    echo "got: $OUTPUT"
    exit 1
  fi
done

echo "map helper self-test passed"
