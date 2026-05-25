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

STATS_OUTPUT="$($BIN --inspect-present-map-plane-stats 2 1 --data "$VALID_DATA_DIR")"

for expected in \
  "present map2 plane1 stats words: 4096" \
  "present map2 plane1 stats nonzero: 370" \
  "present map2 plane1 stats min: 0" \
  "present map2 plane1 stats max: 210"; do
  if [[ "$STATS_OUTPUT" != *"$expected"* ]]; then
    echo "missing expected present-map plane stats output: $expected"
    echo "got: $STATS_OUTPUT"
    exit 1
  fi
done

echo "present map plane stats inspection passed"
