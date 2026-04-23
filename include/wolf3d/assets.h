#ifndef WOLF3D_ASSETS_H
#define WOLF3D_ASSETS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct wolf_maphead_summary
{
    uint16_t rlew_tag;
    size_t map_count;
    uint32_t first_map_offset;
} wolf_maphead_summary;

typedef struct wolf_map_presence_summary
{
    size_t total_slots;
    size_t present_slots;
    bool has_present_slot;
    size_t first_present_slot;
    size_t last_present_slot;
    bool has_empty_slot;
    size_t first_empty_slot;
} wolf_map_presence_summary;

typedef struct wolf_map_summary
{
    uint32_t plane_offsets[3];
    uint16_t plane_lengths[3];
    uint16_t width;
    uint16_t height;
    char name[17];
    uint32_t gamemaps_file_size;
} wolf_map_summary;

typedef struct wolf_present_map_summary
{
    size_t slot_index;
    wolf_map_summary summary;
} wolf_present_map_summary;

typedef struct wolf_map_plane_header
{
    uint32_t offset;
    uint16_t length;
    uint16_t carmack_expanded_bytes;
    uint16_t rlew_expanded_bytes;
    size_t decoded_words;
} wolf_map_plane_header;

typedef struct wolf_map_plane_table
{
    wolf_map_summary summary;
    wolf_map_plane_header headers[3];
} wolf_map_plane_table;

typedef struct wolf_present_map_plane_table
{
    size_t slot_index;
    wolf_map_plane_table table;
} wolf_present_map_plane_table;

typedef struct wolf_map_plane_load_result
{
    uint16_t compressed_bytes;
    uint16_t carmack_expanded_bytes;
    uint16_t rlew_expanded_bytes;
    size_t decoded_words;
} wolf_map_plane_load_result;

typedef struct wolf_loaded_map
{
    wolf_map_summary summary;
    wolf_map_plane_load_result plane_results[3];
    uint16_t plane_words[3][64 * 64];
} wolf_loaded_map;

typedef struct wolf_loaded_present_map
{
    size_t slot_index;
    wolf_loaded_map map;
} wolf_loaded_present_map;

bool wolf_read_maphead_summary(const char *data_dir, wolf_maphead_summary *summary, char *error_buffer, size_t error_buffer_size);
bool wolf_read_map_presence_summary(const char *data_dir, wolf_map_presence_summary *summary, char *error_buffer, size_t error_buffer_size);
bool wolf_read_map_summary(const char *data_dir, size_t map_index, wolf_map_summary *summary, char *error_buffer, size_t error_buffer_size);
bool wolf_read_map_slot(const char *data_dir, size_t map_index, uint32_t *offset, bool *is_present, char *error_buffer, size_t error_buffer_size);
bool wolf_read_map_catalog(const char *data_dir, size_t count, wolf_map_summary *summaries, size_t summaries_count, size_t *loaded_count, wolf_maphead_summary *maphead_summary, char *error_buffer, size_t error_buffer_size);
bool wolf_read_present_map_catalog(const char *data_dir, size_t count, wolf_present_map_summary *entries, size_t entries_count, size_t *loaded_count, wolf_map_presence_summary *presence_summary, char *error_buffer, size_t error_buffer_size);
bool wolf_read_present_map_summary(const char *data_dir, size_t present_index, wolf_present_map_summary *entry, wolf_map_presence_summary *presence_summary, char *error_buffer, size_t error_buffer_size);
bool wolf_read_present_index_for_slot(const char *data_dir, size_t slot_index, size_t *present_index, bool *is_present, wolf_map_presence_summary *presence_summary, char *error_buffer, size_t error_buffer_size);
bool wolf_load_map_catalog(const char *data_dir, size_t count, wolf_loaded_map *maps, size_t maps_count, size_t *loaded_count, wolf_maphead_summary *maphead_summary, char *error_buffer, size_t error_buffer_size);
bool wolf_load_present_map(const char *data_dir, size_t present_index, wolf_loaded_present_map *entry, wolf_map_presence_summary *presence_summary, char *error_buffer, size_t error_buffer_size);
bool wolf_load_present_map_catalog(const char *data_dir, size_t count, wolf_loaded_present_map *entries, size_t entries_count, size_t *loaded_count, wolf_map_presence_summary *presence_summary, char *error_buffer, size_t error_buffer_size);
bool wolf_validate_map(const char *data_dir, size_t map_index, wolf_map_summary *summary, wolf_map_plane_header headers[3], char *error_buffer, size_t error_buffer_size);
bool wolf_validate_map_load(const char *data_dir, size_t map_index, wolf_loaded_map *map, wolf_map_plane_header headers[3], char *error_buffer, size_t error_buffer_size);
bool wolf_validate_present_map_load(const char *data_dir, size_t present_index, wolf_loaded_present_map *entry, wolf_map_plane_header headers[3], wolf_map_presence_summary *presence_summary, char *error_buffer, size_t error_buffer_size);
bool wolf_validate_map_catalog(const char *data_dir, size_t count, bool *valid_flags, size_t valid_flags_count, size_t *validated_count, char *error_buffer, size_t error_buffer_size);
bool wolf_validate_present_map_catalog(const char *data_dir, size_t count, size_t *slot_indices, bool *valid_flags, size_t entries_count, size_t *validated_count, wolf_map_presence_summary *presence_summary, char *error_buffer, size_t error_buffer_size);
bool wolf_validate_present_map_load_catalog(const char *data_dir, size_t count, size_t *slot_indices, bool *valid_flags, size_t entries_count, size_t *validated_count, wolf_map_presence_summary *presence_summary, char *error_buffer, size_t error_buffer_size);
bool wolf_read_first_map_summary(const char *data_dir, wolf_map_summary *summary, char *error_buffer, size_t error_buffer_size);
bool wolf_map_header_is_valid(const wolf_map_summary *summary);
bool wolf_map_dimensions_are_supported(const wolf_map_summary *summary);
bool wolf_map_plane_header_is_valid_for_map(const wolf_map_summary *summary, const wolf_map_plane_header *header);
bool wolf_map_plane_header_matches_summary(const wolf_map_summary *summary, size_t plane_index, const wolf_map_plane_header *header);
bool wolf_map_plane_header_is_in_bounds(const wolf_map_summary *summary, size_t plane_index, const wolf_map_plane_header *header);
bool wolf_map_plane_load_result_matches_header(const wolf_map_plane_header *header, const wolf_map_plane_load_result *result);
bool wolf_map_plane_headers_are_valid(const wolf_map_summary *summary, const wolf_map_plane_header headers[3]);
bool wolf_map_plane_is_valid(const wolf_map_summary *summary, size_t plane_index, const wolf_map_plane_header *header);
bool wolf_map_planes_are_in_bounds(const wolf_map_summary *summary);
bool wolf_first_map_planes_are_in_bounds(const wolf_map_summary *summary);
bool wolf_map_plane_header_from_bytes(const wolf_map_summary *summary, size_t plane_index, uint32_t plane_offset, const uint8_t *compressed_bytes, size_t compressed_size, wolf_map_plane_header *header, char *error_buffer, size_t error_buffer_size);
bool wolf_read_map_plane_header(const char *data_dir, size_t map_index, size_t plane_index, wolf_map_plane_header *header, char *error_buffer, size_t error_buffer_size);
bool wolf_read_map_plane_headers(const char *data_dir, size_t map_index, wolf_map_plane_header headers[3], char *error_buffer, size_t error_buffer_size);
bool wolf_read_present_map_plane_headers(const char *data_dir, size_t present_index, wolf_present_map_summary *entry, wolf_map_plane_header headers[3], wolf_map_presence_summary *presence_summary, char *error_buffer, size_t error_buffer_size);
bool wolf_read_map_plane_table(const char *data_dir, size_t map_index, wolf_map_plane_table *table, char *error_buffer, size_t error_buffer_size);
bool wolf_read_map_plane_table_catalog(const char *data_dir, size_t count, wolf_map_plane_table *tables, size_t tables_count, size_t *loaded_count, wolf_maphead_summary *maphead_summary, char *error_buffer, size_t error_buffer_size);
bool wolf_read_present_map_plane_table(const char *data_dir, size_t present_index, wolf_present_map_plane_table *table, wolf_map_presence_summary *presence_summary, char *error_buffer, size_t error_buffer_size);
bool wolf_read_present_map_plane_table_catalog(const char *data_dir, size_t count, wolf_present_map_plane_table *tables, size_t tables_count, size_t *loaded_count, wolf_map_presence_summary *presence_summary, char *error_buffer, size_t error_buffer_size);
bool wolf_rlew_expand_words(const uint16_t *source, size_t source_words, uint16_t *dest, size_t dest_words, uint16_t rlew_tag);
bool wolf_carmack_expand_bytes(const uint8_t *source, size_t source_size, uint16_t *dest, size_t dest_words);
bool wolf_decode_map_plane(const uint8_t *compressed_bytes, size_t compressed_size, uint16_t rlew_tag, uint16_t *dest, size_t dest_words, wolf_map_plane_load_result *result, char *error_buffer, size_t error_buffer_size);
bool wolf_load_map_plane_words(const char *data_dir, size_t map_index, size_t plane_index, uint16_t *dest, size_t dest_words, wolf_map_plane_load_result *result, char *error_buffer, size_t error_buffer_size);
bool wolf_load_first_map_plane_words(const char *data_dir, size_t plane_index, uint16_t *dest, size_t dest_words, wolf_map_plane_load_result *result, char *error_buffer, size_t error_buffer_size);
bool wolf_load_map(const char *data_dir, size_t map_index, wolf_loaded_map *map, char *error_buffer, size_t error_buffer_size);
bool wolf_load_first_map(const char *data_dir, wolf_loaded_map *map, char *error_buffer, size_t error_buffer_size);
bool wolf_map_cell_index(const wolf_map_summary *summary, size_t x, size_t y, size_t *index);
bool wolf_map_get_plane_result(const wolf_loaded_map *map, size_t plane_index, const wolf_map_plane_load_result **result);
bool wolf_map_get_plane_words(const wolf_loaded_map *map, size_t plane_index, const uint16_t **words, size_t *word_count);
bool wolf_map_get_row(const wolf_loaded_map *map, size_t plane_index, size_t y, const uint16_t **row_words, size_t *row_length);
bool wolf_map_get_column(const wolf_loaded_map *map, size_t plane_index, size_t x, uint16_t *column_words, size_t column_capacity, size_t *column_length);
bool wolf_map_get_region(const wolf_loaded_map *map, size_t plane_index, size_t x, size_t y, size_t region_width, size_t region_height, uint16_t *region_words, size_t region_capacity, size_t *region_word_count);
bool wolf_map_get_cell(const wolf_loaded_map *map, size_t plane_index, size_t x, size_t y, uint16_t *value);
bool wolf_present_map_get_slot_index(const wolf_loaded_present_map *entry, size_t *slot_index);
bool wolf_present_map_get_plane_result(const wolf_loaded_present_map *entry, size_t plane_index, const wolf_map_plane_load_result **result);
bool wolf_present_map_get_plane_words(const wolf_loaded_present_map *entry, size_t plane_index, const uint16_t **words, size_t *word_count);
bool wolf_present_map_get_row(const wolf_loaded_present_map *entry, size_t plane_index, size_t y, const uint16_t **row_words, size_t *row_length);
bool wolf_present_map_get_column(const wolf_loaded_present_map *entry, size_t plane_index, size_t x, uint16_t *column_words, size_t column_capacity, size_t *column_length);
bool wolf_present_map_get_region(const wolf_loaded_present_map *entry, size_t plane_index, size_t x, size_t y, size_t region_width, size_t region_height, uint16_t *region_words, size_t region_capacity, size_t *region_word_count);
bool wolf_present_map_get_cell(const wolf_loaded_present_map *entry, size_t plane_index, size_t x, size_t y, uint16_t *value);

#endif
