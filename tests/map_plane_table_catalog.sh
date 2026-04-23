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

OUTPUT="$($BIN --inspect-map-plane-table-catalog 2 --data "$VALID_DATA_DIR")"

for expected in \
  "map plane table catalog total: 100" \
  "map0 plane table name: Wolf1 Map1" \
  "map0 plane table header valid: yes" \
  "map0 plane0 table offset: 11 length: 1434 carmack: 3190 rlew: 8192 words: 4096" \
  "map0 plane1 table offset: 1445 length: 795 carmack: 1128 rlew: 8192 words: 4096" \
  "map0 plane2 table offset: 2240 length: 10 carmack: 8 rlew: 8192 words: 4096" \
  "map1 plane table name: Wolf1 Map2" \
  "map1 plane table header valid: yes" \
  "map1 plane0 table offset: 2292 length: 1732 carmack: 4236 rlew: 8192 words: 4096" \
  "map1 plane1 table offset: 4024 length: 1757 carmack: 2878 rlew: 8192 words: 4096" \
  "map1 plane2 table offset: 5781 length: 10 carmack: 8 rlew: 8192 words: 4096"; do
  if [[ "$OUTPUT" != *"$expected"* ]]; then
    echo "missing expected plane-table catalog output: $expected"
    echo "got: $OUTPUT"
    exit 1
  fi
done

echo "map plane table catalog passed"
