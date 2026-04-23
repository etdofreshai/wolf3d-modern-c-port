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
if [[ -x "$ROOT/tests/carmack_selftest.sh" ]]; then
  "$ROOT/tests/carmack_selftest.sh"
fi
if [[ -x "$ROOT/tests/decompression_failure_selftest.sh" ]]; then
  "$ROOT/tests/decompression_failure_selftest.sh"
fi
if [[ -x "$ROOT/tests/map_helper_selftest.sh" ]]; then
  "$ROOT/tests/map_helper_selftest.sh"
fi
if [[ -x "$ROOT/tests/map_validation_selftest.sh" ]]; then
  "$ROOT/tests/map_validation_selftest.sh"
fi
if [[ -x "$ROOT/tests/map_plane_decoding.sh" ]]; then
  "$ROOT/tests/map_plane_decoding.sh"
fi
if [[ -x "$ROOT/tests/map_plane_layers.sh" ]]; then
  "$ROOT/tests/map_plane_layers.sh"
fi
if [[ -x "$ROOT/tests/map_index_loading.sh" ]]; then
  "$ROOT/tests/map_index_loading.sh"
fi
if [[ -x "$ROOT/tests/map_header_validation.sh" ]]; then
  "$ROOT/tests/map_header_validation.sh"
fi
if [[ -x "$ROOT/tests/map_overview_loading.sh" ]]; then
  "$ROOT/tests/map_overview_loading.sh"
fi
if [[ -x "$ROOT/tests/map_plane_header_inspection.sh" ]]; then
  "$ROOT/tests/map_plane_header_inspection.sh"
fi
if [[ -x "$ROOT/tests/map_plane_header_validation.sh" ]]; then
  "$ROOT/tests/map_plane_header_validation.sh"
fi
if [[ -x "$ROOT/tests/map_plane_table_validation.sh" ]]; then
  "$ROOT/tests/map_plane_table_validation.sh"
fi
if [[ -x "$ROOT/tests/map_slot_validation.sh" ]]; then
  "$ROOT/tests/map_slot_validation.sh"
fi
if [[ -x "$ROOT/tests/map_presence_summary.sh" ]]; then
  "$ROOT/tests/map_presence_summary.sh"
fi
if [[ -x "$ROOT/tests/map_catalog_inspection.sh" ]]; then
  "$ROOT/tests/map_catalog_inspection.sh"
fi
if [[ -x "$ROOT/tests/map_catalog_validation.sh" ]]; then
  "$ROOT/tests/map_catalog_validation.sh"
fi
if [[ -x "$ROOT/tests/map_cell_inspection.sh" ]]; then
  "$ROOT/tests/map_cell_inspection.sh"
fi
if [[ -x "$ROOT/tests/map_row_inspection.sh" ]]; then
  "$ROOT/tests/map_row_inspection.sh"
fi
if [[ -x "$ROOT/tests/map_column_inspection.sh" ]]; then
  "$ROOT/tests/map_column_inspection.sh"
fi
if [[ -x "$ROOT/tests/map_region_inspection.sh" ]]; then
  "$ROOT/tests/map_region_inspection.sh"
fi
if [[ -x "$ROOT/tests/map_plane_table_inspection.sh" ]]; then
  "$ROOT/tests/map_plane_table_inspection.sh"
fi

echo "all automated checks passed"
