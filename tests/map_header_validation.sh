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

OUTPUT="$($BIN --validate-map-header 1 --data "$VALID_DATA_DIR")"

grep -F "map1 header valid: yes" <<<"$OUTPUT"
grep -F "map1 plane bounds valid: yes" <<<"$OUTPUT"
grep -F "map1 name: Wolf1 Map2" <<<"$OUTPUT"
grep -F "map1 size: 64x64" <<<"$OUTPUT"

echo "map header validation passed"
