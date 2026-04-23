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

OUTPUT="$($BIN --self-test-present-map-helpers)"

for expected in \
  "present map helper slot ok: 7" \
  "present map helper plane result ok: compressed=12 words=6" \
  "present map helper plane ok: count=6 first=200 last=205" \
  "present map helper row ok: length=3 left=203 right=205" \
  "present map helper column ok: count=2 top=201 bottom=204" \
  "present map helper region ok: count=4 top-left=200 bottom-right=204" \
  "present map helper cell ok: 202" \
  "present map helper invalid plane ok" \
  "present map helper invalid cell ok"; do
  if [[ "$OUTPUT" != *"$expected"* ]]; then
    echo "missing expected output: $expected"
    echo "got: $OUTPUT"
    exit 1
  fi
done

echo "present map helper self-test passed"
