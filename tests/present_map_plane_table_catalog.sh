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

OUTPUT="$($BIN --inspect-present-map-plane-table-catalog 2 --data "$VALID_DATA_DIR")"

for expected in \
  "present map plane table catalog total slots: 100" \
  "present map plane table catalog total present: 60" \
  "present map plane table0 slot: 0" \
  "present map plane table0 name: Wolf1 Map1" \
  "present map plane table0 header valid: yes" \
  "present map plane0 table0 offset: 11 length: 1434 carmack: 3190 rlew: 8192 words: 4096" \
  "present map plane1 table0 offset: 1445 length: 795 carmack: 1128 rlew: 8192 words: 4096" \
  "present map plane2 table0 offset: 2240 length: 10 carmack: 8 rlew: 8192 words: 4096" \
  "present map plane table1 slot: 1" \
  "present map plane table1 name: Wolf1 Map2" \
  "present map plane table1 header valid: yes" \
  "present map plane0 table1 offset: 2292 length: 1732 carmack: 4236 rlew: 8192 words: 4096" \
  "present map plane1 table1 offset: 4024 length: 1757 carmack: 2878 rlew: 8192 words: 4096" \
  "present map plane2 table1 offset: 5781 length: 10 carmack: 8 rlew: 8192 words: 4096"; do
  if [[ "$OUTPUT" != *"$expected"* ]]; then
    echo "missing expected present-map plane-table catalog output: $expected"
    echo "got: $OUTPUT"
    exit 1
  fi
done

echo "present map plane table catalog passed"
