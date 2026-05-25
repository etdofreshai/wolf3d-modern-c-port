#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$ROOT/build"
BIN="$BUILD_DIR/wolf3d_port"
TMP_DIR="$BUILD_DIR/maphead_alignment_data"

mkdir -p "$BUILD_DIR"
rm -rf "$TMP_DIR"
mkdir -p "$TMP_DIR"

gcc -std=c17 -Wall -Wextra -Werror \
  -I"$ROOT/include" \
  "$ROOT/src/main.c" \
  "$ROOT/src/platform/platform_stub.c" \
  "$ROOT/src/platform/filesystem.c" \
  "$ROOT/src/core/assets.c" \
  -o "$BIN"

for required_file in VGAGRAPH.WL6 VGAHEAD.WL6 VGADICT.WL6 GAMEMAPS.WL6 AUDIOHED.WL6 AUDIOT.WL6 VSWAP.WL6; do
  : > "$TMP_DIR/$required_file"
done

python3 - <<'PY' "$TMP_DIR/MAPHEAD.WL6"
import sys
from pathlib import Path
# RLEW tag + one full offset + one dangling byte. The MAPHEAD offset table
# must be 4-byte aligned after the tag; this malformed fixture must never be
# silently truncated into a one-slot catalog.
Path(sys.argv[1]).write_bytes(bytes([0xcd, 0xab, 0x2a, 0x00, 0x00, 0x00, 0xff]))
PY

set +e
"$BIN" --inspect-maphead --data "$TMP_DIR" > "$BUILD_DIR/maphead_alignment.out" 2> "$BUILD_DIR/maphead_alignment.err"
STATUS=$?
set -e

if [[ "$STATUS" -eq 0 ]]; then
  echo "expected malformed MAPHEAD.WL6 offset table to fail"
  echo "stdout: $(<"$BUILD_DIR/maphead_alignment.out")"
  exit 1
fi

ERROR_OUTPUT="$(<"$BUILD_DIR/maphead_alignment.err")"
if [[ "$ERROR_OUTPUT" != *"MAPHEAD.WL6 offset table is truncated"* ]]; then
  echo "expected truncated MAPHEAD offset-table error"
  echo "got: $ERROR_OUTPUT"
  exit 1
fi

echo "maphead alignment validation passed"
