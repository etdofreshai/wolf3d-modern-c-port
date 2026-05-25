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

OUTPUT="$($BIN --self-test-map-plane-table-helpers)"

for expected in \
  "map plane table helper ok: name=TableMap plane1 offset=1445 words=2048" \
  "map plane table helper null output ok" \
  "present map plane table helper ok: slot=7 name=PresentTable plane0 length=10 words=2" \
  "present map plane table helper null output ok"; do
  if [[ "$OUTPUT" != *"$expected"* ]]; then
    echo "missing expected output: $expected"
    echo "got: $OUTPUT"
    exit 1
  fi
done

echo "map plane table helper self-test passed"
