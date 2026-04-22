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
  -o "$BIN"

OUTPUT="$($BIN --version)"
EXPECTED="wolf3d_port 0.1.0-dev"

if [[ "$OUTPUT" != "$EXPECTED" ]]; then
  echo "expected: $EXPECTED"
  echo "got: $OUTPUT"
  exit 1
fi

echo "smoke test passed"
