#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"

"$ROOT/tests/smoke_version.sh"
"$ROOT/tests/data_path_validation.sh"
"$ROOT/tests/maphead_inspection.sh"
if [[ -x "$ROOT/tests/gamemaps_inspection.sh" ]]; then
  "$ROOT/tests/gamemaps_inspection.sh"
fi
if [[ -x "$ROOT/tests/plane_validation.sh" ]]; then
  "$ROOT/tests/plane_validation.sh"
fi
if [[ -x "$ROOT/tests/rlew_selftest.sh" ]]; then
  "$ROOT/tests/rlew_selftest.sh"
fi

echo "all automated checks passed"
