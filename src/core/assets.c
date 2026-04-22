#include "wolf3d/assets.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WOLF_CARMACK_NEARTAG 0xa7
#define WOLF_CARMACK_FARTAG 0xa8

static void set_error(char *error_buffer, size_t error_buffer_size, const char *message)
{
    if (error_buffer != NULL && error_buffer_size > 0)
    {
        snprintf(error_buffer, error_buffer_size, "%s", message);
    }
}

bool wolf_rlew_expand_words(const uint16_t *source, size_t source_words, uint16_t *dest, size_t dest_words, uint16_t rlew_tag)
{
    size_t source_index = 0;
    size_t dest_index = 0;

    if (source == NULL || dest == NULL)
    {
        return false;
    }

    while (source_index < source_words && dest_index < dest_words)
    {
        uint16_t value = source[source_index++];
        if (value != rlew_tag)
        {
            dest[dest_index++] = value;
            continue;
        }

        if ((source_index + 1) >= source_words)
        {
            return false;
        }

        {
            uint16_t count = source[source_index++];
            uint16_t repeated_value = source[source_index++];
            size_t repeat_index;
            if ((dest_index + count) > dest_words)
            {
                return false;
            }
            for (repeat_index = 0; repeat_index < count; ++repeat_index)
            {
                dest[dest_index++] = repeated_value;
            }
        }
    }

    return (dest_index == dest_words) && (source_index == source_words);
}

bool wolf_carmack_expand_bytes(const uint8_t *source, size_t source_size, uint16_t *dest, size_t dest_words)
{
    size_t source_index = 0;
    size_t dest_index = 0;

    if (source == NULL || dest == NULL)
    {
        return false;
    }

    while (dest_index < dest_words)
    {
        uint16_t word;
        uint8_t high_byte;
        uint8_t count;
        size_t copy_index;
        size_t repeat_index;

        if ((source_index + 1) >= source_size)
        {
            return false;
        }

        word = (uint16_t)(source[source_index] | ((uint16_t)source[source_index + 1] << 8));
        source_index += 2;
        high_byte = (uint8_t)(word >> 8);
        count = (uint8_t)(word & 0xff);

        if (high_byte == WOLF_CARMACK_NEARTAG)
        {
            if (count == 0)
            {
                if (source_index >= source_size)
                {
                    return false;
                }
                dest[dest_index++] = (uint16_t)(word | source[source_index++]);
                continue;
            }

            if (source_index >= source_size)
            {
                return false;
            }

            copy_index = dest_index - source[source_index++];
            if (copy_index >= dest_index || (dest_index + count) > dest_words)
            {
                return false;
            }

            for (repeat_index = 0; repeat_index < count; ++repeat_index)
            {
                dest[dest_index++] = dest[copy_index++];
            }
            continue;
        }

        if (high_byte == WOLF_CARMACK_FARTAG)
        {
            if (count == 0)
            {
                if (source_index >= source_size)
                {
                    return false;
                }
                dest[dest_index++] = (uint16_t)(word | source[source_index++]);
                continue;
            }

            if ((source_index + 1) >= source_size)
            {
                return false;
            }

            copy_index = (size_t)(source[source_index] | ((size_t)source[source_index + 1] << 8));
            source_index += 2;
            if (copy_index >= dest_index || (dest_index + count) > dest_words)
            {
                return false;
            }

            for (repeat_index = 0; repeat_index < count; ++repeat_index)
            {
                dest[dest_index++] = dest[copy_index++];
            }
            continue;
        }

        dest[dest_index++] = word;
    }

    return source_index == source_size;
}

static uint16_t read_u16_le(const unsigned char *bytes)
{
    return (uint16_t)(bytes[0] | ((uint16_t)bytes[1] << 8));
}

static uint32_t read_u32_le(const unsigned char *bytes)
{
    return (uint32_t)bytes[0]
        | ((uint32_t)bytes[1] << 8)
        | ((uint32_t)bytes[2] << 16)
        | ((uint32_t)bytes[3] << 24);
}

bool wolf_read_maphead_summary(const char *data_dir, wolf_maphead_summary *summary, char *error_buffer, size_t error_buffer_size)
{
    char path[4096];
    unsigned char header[6];
    FILE *file;
    long file_size;

    if (error_buffer != NULL && error_buffer_size > 0)
    {
        error_buffer[0] = '\0';
    }

    if (data_dir == NULL || summary == NULL)
    {
        set_error(error_buffer, error_buffer_size, "could not inspect MAPHEAD.WL6");
        return false;
    }

    snprintf(path, sizeof(path), "%s/MAPHEAD.WL6", data_dir);
    file = fopen(path, "rb");
    if (file == NULL)
    {
        snprintf(error_buffer, error_buffer_size, "could not open %s", path);
        return false;
    }

    if (fread(header, 1, sizeof(header), file) != sizeof(header))
    {
        fclose(file);
        set_error(error_buffer, error_buffer_size, "could not read MAPHEAD.WL6 header");
        return false;
    }

    if (fseek(file, 0, SEEK_END) != 0)
    {
        fclose(file);
        set_error(error_buffer, error_buffer_size, "could not size MAPHEAD.WL6");
        return false;
    }

    file_size = ftell(file);
    fclose(file);
    if (file_size < 6)
    {
        set_error(error_buffer, error_buffer_size, "MAPHEAD.WL6 is too small");
        return false;
    }

    summary->rlew_tag = read_u16_le(header);
    summary->map_count = (size_t)((file_size - 2) / 4);
    summary->first_map_offset = read_u32_le(header + 2);
    return true;
}

bool wolf_read_map_summary(const char *data_dir, size_t map_index, wolf_map_summary *summary, char *error_buffer, size_t error_buffer_size)
{
    char maphead_path[4096];
    char gamemaps_path[4096];
    unsigned char header[38];
    unsigned char offset_bytes[4];
    FILE *file;
    wolf_maphead_summary maphead;
    uint32_t map_offset;
    size_t i;
    long file_size;

    if (error_buffer != NULL && error_buffer_size > 0)
    {
        error_buffer[0] = '\0';
    }

    if (data_dir == NULL || summary == NULL)
    {
        set_error(error_buffer, error_buffer_size, "could not inspect map");
        return false;
    }

    if (!wolf_read_maphead_summary(data_dir, &maphead, error_buffer, error_buffer_size))
    {
        return false;
    }

    if (map_index >= maphead.map_count)
    {
        set_error(error_buffer, error_buffer_size, "map index is out of range");
        return false;
    }

    snprintf(maphead_path, sizeof(maphead_path), "%s/MAPHEAD.WL6", data_dir);
    file = fopen(maphead_path, "rb");
    if (file == NULL)
    {
        snprintf(error_buffer, error_buffer_size, "could not open %s", maphead_path);
        return false;
    }

    if (fseek(file, (long)(2 + (map_index * 4)), SEEK_SET) != 0)
    {
        fclose(file);
        set_error(error_buffer, error_buffer_size, "could not seek to map offset");
        return false;
    }

    if (fread(offset_bytes, 1, sizeof(offset_bytes), file) != sizeof(offset_bytes))
    {
        fclose(file);
        set_error(error_buffer, error_buffer_size, "could not read map offset");
        return false;
    }
    fclose(file);

    map_offset = read_u32_le(offset_bytes);
    if (map_offset == 0 || map_offset == 0xffffffffu)
    {
        set_error(error_buffer, error_buffer_size, "map index does not point to a map header");
        return false;
    }

    snprintf(gamemaps_path, sizeof(gamemaps_path), "%s/GAMEMAPS.WL6", data_dir);
    file = fopen(gamemaps_path, "rb");
    if (file == NULL)
    {
        snprintf(error_buffer, error_buffer_size, "could not open %s", gamemaps_path);
        return false;
    }

    if (fseek(file, (long)map_offset, SEEK_SET) != 0)
    {
        fclose(file);
        set_error(error_buffer, error_buffer_size, "could not seek to map header");
        return false;
    }

    if (fread(header, 1, sizeof(header), file) != sizeof(header))
    {
        fclose(file);
        set_error(error_buffer, error_buffer_size, "could not read map header");
        return false;
    }

    if (fseek(file, 0, SEEK_END) != 0)
    {
        fclose(file);
        set_error(error_buffer, error_buffer_size, "could not size GAMEMAPS.WL6");
        return false;
    }

    file_size = ftell(file);
    fclose(file);

    for (i = 0; i < 3; ++i)
    {
        summary->plane_offsets[i] = read_u32_le(header + (i * 4));
    }

    for (i = 0; i < 3; ++i)
    {
        summary->plane_lengths[i] = read_u16_le(header + 12 + (i * 2));
    }

    summary->width = read_u16_le(header + 18);
    summary->height = read_u16_le(header + 20);
    memcpy(summary->name, header + 22, 16);
    summary->name[16] = '\0';
    summary->gamemaps_file_size = (uint32_t)file_size;
    return true;
}

bool wolf_read_map_catalog(const char *data_dir, size_t count, wolf_map_summary *summaries, size_t summaries_count, size_t *loaded_count, wolf_maphead_summary *maphead_summary, char *error_buffer, size_t error_buffer_size)
{
    wolf_maphead_summary local_maphead;
    size_t index;
    size_t limit;

    if (error_buffer != NULL && error_buffer_size > 0)
    {
        error_buffer[0] = '\0';
    }

    if (data_dir == NULL || summaries == NULL || loaded_count == NULL || count > summaries_count)
    {
        set_error(error_buffer, error_buffer_size, "could not inspect map catalog");
        return false;
    }

    if (!wolf_read_maphead_summary(data_dir, &local_maphead, error_buffer, error_buffer_size))
    {
        return false;
    }

    limit = count;
    if (limit > local_maphead.map_count)
    {
        limit = local_maphead.map_count;
    }

    for (index = 0; index < limit; ++index)
    {
        if (!wolf_read_map_summary(data_dir, index, &summaries[index], error_buffer, error_buffer_size))
        {
            return false;
        }
    }

    *loaded_count = limit;
    if (maphead_summary != NULL)
    {
        *maphead_summary = local_maphead;
    }
    return true;
}

bool wolf_read_first_map_summary(const char *data_dir, wolf_map_summary *summary, char *error_buffer, size_t error_buffer_size)
{
    return wolf_read_map_summary(data_dir, 0, summary, error_buffer, error_buffer_size);
}

bool wolf_map_header_is_valid(const wolf_map_summary *summary)
{
    size_t i;

    if (summary == NULL)
    {
        return false;
    }

    if (summary->width == 0 || summary->height == 0 || summary->name[0] == '\0')
    {
        return false;
    }

    for (i = 0; i < 3; ++i)
    {
        if (summary->plane_offsets[i] == 0 || summary->plane_lengths[i] == 0)
        {
            return false;
        }
    }

    return true;
}

bool wolf_map_planes_are_in_bounds(const wolf_map_summary *summary)
{
    size_t i;
    uint32_t end;

    if (summary == NULL)
    {
        return false;
    }

    for (i = 0; i < 3; ++i)
    {
        end = summary->plane_offsets[i] + summary->plane_lengths[i];
        if (end > summary->gamemaps_file_size)
        {
            return false;
        }
        if (i < 2 && end > summary->plane_offsets[i + 1])
        {
            return false;
        }
    }

    return true;
}

bool wolf_first_map_planes_are_in_bounds(const wolf_map_summary *summary)
{
    return wolf_map_planes_are_in_bounds(summary);
}

bool wolf_load_map_plane_words(const char *data_dir, size_t map_index, size_t plane_index, uint16_t *dest, size_t dest_words, wolf_map_plane_load_result *result, char *error_buffer, size_t error_buffer_size)
{
    char path[4096];
    FILE *file;
    wolf_map_summary summary;
    wolf_maphead_summary maphead;
    unsigned char *compressed_bytes;
    uint16_t *carmack_words;
    uint16_t carmack_expanded_bytes;
    uint16_t rlew_expanded_bytes;
    size_t compressed_size;
    size_t carmack_word_count;
    bool ok;

    if (error_buffer != NULL && error_buffer_size > 0)
    {
        error_buffer[0] = '\0';
    }

    if (data_dir == NULL || dest == NULL || result == NULL || plane_index >= 3)
    {
        set_error(error_buffer, error_buffer_size, "could not load map plane");
        return false;
    }

    if (!wolf_read_maphead_summary(data_dir, &maphead, error_buffer, error_buffer_size)
        || !wolf_read_map_summary(data_dir, map_index, &summary, error_buffer, error_buffer_size))
    {
        return false;
    }

    if (!wolf_map_header_is_valid(&summary) || !wolf_map_planes_are_in_bounds(&summary))
    {
        set_error(error_buffer, error_buffer_size, "map header is invalid");
        return false;
    }

    compressed_size = summary.plane_lengths[plane_index];
    if (compressed_size < 4)
    {
        set_error(error_buffer, error_buffer_size, "map plane is too small");
        return false;
    }

    snprintf(path, sizeof(path), "%s/GAMEMAPS.WL6", data_dir);
    file = fopen(path, "rb");
    if (file == NULL)
    {
        snprintf(error_buffer, error_buffer_size, "could not open %s", path);
        return false;
    }

    if (fseek(file, (long)summary.plane_offsets[plane_index], SEEK_SET) != 0)
    {
        fclose(file);
        set_error(error_buffer, error_buffer_size, "could not seek to map plane");
        return false;
    }

    compressed_bytes = (unsigned char *)malloc(compressed_size);
    if (compressed_bytes == NULL)
    {
        fclose(file);
        set_error(error_buffer, error_buffer_size, "out of memory reading map plane");
        return false;
    }

    if (fread(compressed_bytes, 1, compressed_size, file) != compressed_size)
    {
        free(compressed_bytes);
        fclose(file);
        set_error(error_buffer, error_buffer_size, "could not read map plane");
        return false;
    }
    fclose(file);

    carmack_expanded_bytes = read_u16_le(compressed_bytes);
    if ((carmack_expanded_bytes % 2) != 0)
    {
        free(compressed_bytes);
        set_error(error_buffer, error_buffer_size, "carmack-expanded size must be even");
        return false;
    }

    carmack_word_count = (size_t)(carmack_expanded_bytes / 2);
    if (carmack_word_count == 0)
    {
        free(compressed_bytes);
        set_error(error_buffer, error_buffer_size, "carmack-expanded plane is empty");
        return false;
    }

    carmack_words = (uint16_t *)malloc(carmack_word_count * sizeof(uint16_t));
    if (carmack_words == NULL)
    {
        free(compressed_bytes);
        set_error(error_buffer, error_buffer_size, "out of memory expanding map plane");
        return false;
    }

    ok = wolf_carmack_expand_bytes(compressed_bytes + 2, compressed_size - 2, carmack_words, carmack_word_count);
    free(compressed_bytes);
    if (!ok)
    {
        free(carmack_words);
        set_error(error_buffer, error_buffer_size, "could not Carmack-expand map plane");
        return false;
    }

    rlew_expanded_bytes = carmack_words[0];
    if ((rlew_expanded_bytes % 2) != 0)
    {
        free(carmack_words);
        set_error(error_buffer, error_buffer_size, "RLEW-expanded size must be even");
        return false;
    }

    result->compressed_bytes = summary.plane_lengths[plane_index];
    result->carmack_expanded_bytes = carmack_expanded_bytes;
    result->rlew_expanded_bytes = rlew_expanded_bytes;
    result->decoded_words = (size_t)(rlew_expanded_bytes / 2);

    if (result->decoded_words > dest_words)
    {
        free(carmack_words);
        set_error(error_buffer, error_buffer_size, "destination buffer is too small for map plane");
        return false;
    }

    ok = wolf_rlew_expand_words(carmack_words + 1, carmack_word_count - 1, dest, result->decoded_words, maphead.rlew_tag);
    free(carmack_words);
    if (!ok)
    {
        set_error(error_buffer, error_buffer_size, "could not RLEW-expand map plane");
        return false;
    }

    return true;
}

bool wolf_load_first_map_plane_words(const char *data_dir, size_t plane_index, uint16_t *dest, size_t dest_words, wolf_map_plane_load_result *result, char *error_buffer, size_t error_buffer_size)
{
    return wolf_load_map_plane_words(data_dir, 0, plane_index, dest, dest_words, result, error_buffer, error_buffer_size);
}

bool wolf_load_map(const char *data_dir, size_t map_index, wolf_loaded_map *map, char *error_buffer, size_t error_buffer_size)
{
    size_t plane_index;

    if (error_buffer != NULL && error_buffer_size > 0)
    {
        error_buffer[0] = '\0';
    }

    if (data_dir == NULL || map == NULL)
    {
        set_error(error_buffer, error_buffer_size, "could not load map");
        return false;
    }

    if (!wolf_read_map_summary(data_dir, map_index, &map->summary, error_buffer, error_buffer_size))
    {
        return false;
    }

    for (plane_index = 0; plane_index < 3; ++plane_index)
    {
        if (!wolf_load_map_plane_words(data_dir,
                map_index,
                plane_index,
                map->plane_words[plane_index],
                (sizeof(map->plane_words[plane_index]) / sizeof(map->plane_words[plane_index][0])),
                &map->plane_results[plane_index],
                error_buffer,
                error_buffer_size))
        {
            return false;
        }
    }

    return true;
}

bool wolf_load_first_map(const char *data_dir, wolf_loaded_map *map, char *error_buffer, size_t error_buffer_size)
{
    return wolf_load_map(data_dir, 0, map, error_buffer, error_buffer_size);
}
