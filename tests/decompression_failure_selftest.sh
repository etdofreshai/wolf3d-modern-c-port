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

OUTPUT="$($BIN --self-test-decompression-failures)"

for expected in \
  "rlew truncated tag ok" \
  "rlew trailing data ok" \
  "carmack truncated literal ok" \
  "carmack trailing data ok" \
  "carmack invalid far ok"; do
  if [[ "$OUTPUT" != *"$expected"* ]]; then
    echo "missing expected output: $expected"
    echo "got: $OUTPUT"
    exit 1
  fi
done

echo "decompression failure self-test passed"
