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

OUTPUT="$($BIN --inspect-map-catalog 5 --data "$VALID_DATA_DIR")"

for expected in \
  "map catalog total: 100" \
  "map0 catalog name: Wolf1 Map1" \
  "map0 catalog size: 64x64" \
  "map1 catalog name: Wolf1 Map2" \
  "map1 catalog size: 64x64" \
  "map2 catalog name: Wolf1 Map3" \
  "map2 catalog size: 64x64" \
  "map3 catalog name: Wolf1 Map4" \
  "map3 catalog size: 64x64" \
  "map4 catalog name: Wolf1 Map5" \
  "map4 catalog size: 64x64"; do
  if [[ "$OUTPUT" != *"$expected"* ]]; then
    echo "missing expected catalog output: $expected"
    echo "got: $OUTPUT"
    exit 1
  fi
done

echo "map catalog inspection passed"
