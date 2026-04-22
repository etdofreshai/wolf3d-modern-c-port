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

typedef struct wolf_map_summary
{
    uint32_t plane_offsets[3];
    uint16_t plane_lengths[3];
    uint16_t width;
    uint16_t height;
    char name[17];
    uint32_t gamemaps_file_size;
} wolf_map_summary;

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

bool wolf_read_maphead_summary(const char *data_dir, wolf_maphead_summary *summary, char *error_buffer, size_t error_buffer_size);
bool wolf_read_map_summary(const char *data_dir, size_t map_index, wolf_map_summary *summary, char *error_buffer, size_t error_buffer_size);
bool wolf_read_map_catalog(const char *data_dir, size_t count, wolf_map_summary *summaries, size_t summaries_count, size_t *loaded_count, wolf_maphead_summary *maphead_summary, char *error_buffer, size_t error_buffer_size);
bool wolf_read_first_map_summary(const char *data_dir, wolf_map_summary *summary, char *error_buffer, size_t error_buffer_size);
bool wolf_map_header_is_valid(const wolf_map_summary *summary);
bool wolf_map_planes_are_in_bounds(const wolf_map_summary *summary);
bool wolf_first_map_planes_are_in_bounds(const wolf_map_summary *summary);
bool wolf_rlew_expand_words(const uint16_t *source, size_t source_words, uint16_t *dest, size_t dest_words, uint16_t rlew_tag);
bool wolf_carmack_expand_bytes(const uint8_t *source, size_t source_size, uint16_t *dest, size_t dest_words);
bool wolf_load_map_plane_words(const char *data_dir, size_t map_index, size_t plane_index, uint16_t *dest, size_t dest_words, wolf_map_plane_load_result *result, char *error_buffer, size_t error_buffer_size);
bool wolf_load_first_map_plane_words(const char *data_dir, size_t plane_index, uint16_t *dest, size_t dest_words, wolf_map_plane_load_result *result, char *error_buffer, size_t error_buffer_size);
bool wolf_load_map(const char *data_dir, size_t map_index, wolf_loaded_map *map, char *error_buffer, size_t error_buffer_size);
bool wolf_load_first_map(const char *data_dir, wolf_loaded_map *map, char *error_buffer, size_t error_buffer_size);

#endif
