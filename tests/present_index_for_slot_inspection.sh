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

PRESENT_OUTPUT="$($BIN --inspect-present-index-for-slot 2 --data "$VALID_DATA_DIR")"
EMPTY_OUTPUT="$($BIN --inspect-present-index-for-slot 60 --data "$VALID_DATA_DIR")"
set +e
OVERFLOW_OUTPUT="$($BIN --inspect-present-index-for-slot 999999999999999999999999999999 --data "$VALID_DATA_DIR" 2>&1)"
OVERFLOW_STATUS=$?
set -e

for expected in \
  "map slot 2 present: yes" \
  "map slot 2 present index: 2" \
  "map slot 2 name: Wolf1 Map3"; do
  if [[ "$PRESENT_OUTPUT" != *"$expected"* ]]; then
    echo "missing expected present-slot output: $expected"
    echo "got: $PRESENT_OUTPUT"
    exit 1
  fi
done

if [[ "$EMPTY_OUTPUT" != *"map slot 60 present: no"* ]]; then
  echo "missing expected empty-slot output"
  echo "got: $EMPTY_OUTPUT"
  exit 1
fi

if [[ $OVERFLOW_STATUS -eq 0 || "$OVERFLOW_OUTPUT" != *"--inspect-present-index-for-slot slot index must be a non-negative integer"* ]]; then
  echo "missing expected overflow validation output"
  echo "got status: $OVERFLOW_STATUS"
  echo "got: $OVERFLOW_OUTPUT"
  exit 1
fi

echo "present index-for-slot inspection passed"
