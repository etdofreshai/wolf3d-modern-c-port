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

set +e
OUTPUT="$($BIN --validate-map-catalog 61 --data "$VALID_DATA_DIR" 2>&1)"
STATUS=$?
set -e

if [[ $STATUS -ne 1 ]]; then
  echo "expected sparse map catalog validation to exit 1, got: $STATUS"
  echo "got: $OUTPUT"
  exit 1
fi

for expected in \
  "map catalog validation count: 61" \
  "map catalog validation all valid: no" \
  "map59 validation: yes" \
  "map60 validation: no"; do
  if [[ "$OUTPUT" != *"$expected"* ]]; then
    echo "missing expected sparse map-catalog validation output: $expected"
    echo "got: $OUTPUT"
    exit 1
  fi
done

echo "sparse map catalog validation passed"
