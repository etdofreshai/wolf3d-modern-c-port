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
} wolf_map_summary;

bool wolf_read_maphead_summary(const char *data_dir, wolf_maphead_summary *summary, char *error_buffer, size_t error_buffer_size);
bool wolf_read_first_map_summary(const char *data_dir, wolf_map_summary *summary, char *error_buffer, size_t error_buffer_size);

#endif
