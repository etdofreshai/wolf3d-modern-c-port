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

OUTPUT="$($BIN --validate-map-plane-table-catalog 3 --data "$VALID_DATA_DIR")"

for expected in \
  "map plane table catalog total: 100" \
  "map plane table catalog validation count: 3" \
  "map plane table catalog validation all valid: yes" \
  "map plane table0 validation: yes" \
  "map plane table1 validation: yes" \
  "map plane table2 validation: yes"; do
  if [[ "$OUTPUT" != *"$expected"* ]]; then
    echo "missing expected map-plane-table catalog validation output: $expected"
    echo "got: $OUTPUT"
    exit 1
  fi
done

echo "map plane table catalog validation passed"
