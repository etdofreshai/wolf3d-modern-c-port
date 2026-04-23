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

bool wolf_read_map_presence_summary(const char *data_dir, wolf_map_presence_summary *summary, char *error_buffer, size_t error_buffer_size)
{
    char maphead_path[4096];
    FILE *file;
    wolf_maphead_summary maphead;
    size_t map_index;

    if (error_buffer != NULL && error_buffer_size > 0)
    {
        error_buffer[0] = '\0';
    }

    if (data_dir == NULL || summary == NULL)
    {
        set_error(error_buffer, error_buffer_size, "could not inspect map presence summary");
        return false;
    }

    if (!wolf_read_maphead_summary(data_dir, &maphead, error_buffer, error_buffer_size))
    {
        return false;
    }

    memset(summary, 0, sizeof(*summary));
    summary->total_slots = maphead.map_count;

    snprintf(maphead_path, sizeof(maphead_path), "%s/MAPHEAD.WL6", data_dir);
    file = fopen(maphead_path, "rb");
    if (file == NULL)
    {
        snprintf(error_buffer, error_buffer_size, "could not open %s", maphead_path);
        return false;
    }

    if (fseek(file, 2, SEEK_SET) != 0)
    {
        fclose(file);
        set_error(error_buffer, error_buffer_size, "could not seek to MAPHEAD.WL6 offsets");
        return false;
    }

    for (map_index = 0; map_index < maphead.map_count; ++map_index)
    {
        unsigned char offset_bytes[4];
        uint32_t map_offset;
        bool is_present;

        if (fread(offset_bytes, 1, sizeof(offset_bytes), file) != sizeof(offset_bytes))
        {
            fclose(file);
            set_error(error_buffer, error_buffer_size, "could not read MAPHEAD.WL6 offset table");
            return false;
        }

        map_offset = read_u32_le(offset_bytes);
        is_present = map_offset != 0 && map_offset != 0xffffffffu;
        if (is_present)
        {
            if (!summary->has_present_slot)
            {
                summary->has_present_slot = true;
                summary->first_present_slot = map_index;
            }
            summary->last_present_slot = map_index;
            summary->present_slots += 1;
        }
        else if (!summary->has_empty_slot)
        {
            summary->has_empty_slot = true;
            summary->first_empty_slot = map_index;
        }
    }

    fclose(file);
    return true;
}

bool wolf_read_map_slot(const char *data_dir, size_t map_index, uint32_t *offset, bool *is_present, char *error_buffer, size_t error_buffer_size)
{
    char maphead_path[4096];
    unsigned char offset_bytes[4];
    FILE *file;
    wolf_maphead_summary maphead;
    uint32_t map_offset;

    if (error_buffer != NULL && error_buffer_size > 0)
    {
        error_buffer[0] = '\0';
    }

    if (data_dir == NULL || offset == NULL || is_present == NULL)
    {
        set_error(error_buffer, error_buffer_size, "could not inspect map slot");
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
    *offset = map_offset;
    *is_present = map_offset != 0 && map_offset != 0xffffffffu;
    return true;
}

bool wolf_read_map_summary(const char *data_dir, size_t map_index, wolf_map_summary *summary, char *error_buffer, size_t error_buffer_size)
{
    char gamemaps_path[4096];
    unsigned char header[38];
    FILE *file;
    uint32_t map_offset;
    bool is_present;
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

    if (!wolf_read_map_slot(data_dir, map_index, &map_offset, &is_present, error_buffer, error_buffer_size))
    {
        return false;
    }

    if (!is_present)
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

bool wolf_read_present_map_catalog(const char *data_dir, size_t count, wolf_present_map_summary *entries, size_t entries_count, size_t *loaded_count, wolf_map_presence_summary *presence_summary, char *error_buffer, size_t error_buffer_size)
{
    wolf_map_presence_summary local_presence_summary;
    size_t slot_index;
    size_t entry_index;

    if (error_buffer != NULL && error_buffer_size > 0)
    {
        error_buffer[0] = '\0';
    }

    if (data_dir == NULL || entries == NULL || loaded_count == NULL || count > entries_count)
    {
        set_error(error_buffer, error_buffer_size, "could not inspect present map catalog");
        return false;
    }

    if (!wolf_read_map_presence_summary(data_dir, &local_presence_summary, error_buffer, error_buffer_size))
    {
        return false;
    }

    count = count < local_presence_summary.present_slots ? count : local_presence_summary.present_slots;
    entry_index = 0;
    for (slot_index = 0; slot_index < local_presence_summary.total_slots && entry_index < count; ++slot_index)
    {
        uint32_t map_offset = 0;
        bool is_present = false;

        if (!wolf_read_map_slot(data_dir, slot_index, &map_offset, &is_present, error_buffer, error_buffer_size))
        {
            return false;
        }

        (void)map_offset;
        if (!is_present)
        {
            continue;
        }

        entries[entry_index].slot_index = slot_index;
        if (!wolf_read_map_summary(data_dir, slot_index, &entries[entry_index].summary, error_buffer, error_buffer_size))
        {
            return false;
        }

        entry_index += 1;
    }

    *loaded_count = entry_index;
    if (presence_summary != NULL)
    {
        *presence_summary = local_presence_summary;
    }

    return true;
}

static bool wolf_find_present_map_slot(const char *data_dir, size_t present_index, size_t *slot_index, wolf_map_presence_summary *presence_summary, char *error_buffer, size_t error_buffer_size)
{
    wolf_map_presence_summary local_presence_summary;
    size_t current_slot_index;
    size_t current_present_index;

    if (error_buffer != NULL && error_buffer_size > 0)
    {
        error_buffer[0] = '\0';
    }

    if (data_dir == NULL || slot_index == NULL)
    {
        set_error(error_buffer, error_buffer_size, "could not inspect present map");
        return false;
    }

    if (!wolf_read_map_presence_summary(data_dir, &local_presence_summary, error_buffer, error_buffer_size))
    {
        return false;
    }

    if (present_index >= local_presence_summary.present_slots)
    {
        set_error(error_buffer, error_buffer_size, "present map index is out of range");
        return false;
    }

    current_present_index = 0;
    for (current_slot_index = 0; current_slot_index < local_presence_summary.total_slots; ++current_slot_index)
    {
        uint32_t map_offset = 0;
        bool is_present = false;

        if (!wolf_read_map_slot(data_dir, current_slot_index, &map_offset, &is_present, error_buffer, error_buffer_size))
        {
            return false;
        }

        (void)map_offset;
        if (!is_present)
        {
            continue;
        }

        if (current_present_index == present_index)
        {
            *slot_index = current_slot_index;
            if (presence_summary != NULL)
            {
                *presence_summary = local_presence_summary;
            }
            return true;
        }

        current_present_index += 1;
    }

    set_error(error_buffer, error_buffer_size, "present map index is out of range");
    return false;
}

bool wolf_read_present_map_summary(const char *data_dir, size_t present_index, wolf_present_map_summary *entry, wolf_map_presence_summary *presence_summary, char *error_buffer, size_t error_buffer_size)
{
    size_t slot_index;
    wolf_map_presence_summary local_presence_summary;

    if (error_buffer != NULL && error_buffer_size > 0)
    {
        error_buffer[0] = '\0';
    }

    if (data_dir == NULL || entry == NULL)
    {
        set_error(error_buffer, error_buffer_size, "could not inspect present map");
        return false;
    }

    if (!wolf_find_present_map_slot(data_dir, present_index, &slot_index, &local_presence_summary, error_buffer, error_buffer_size))
    {
        return false;
    }

    entry->slot_index = slot_index;
    if (!wolf_read_map_summary(data_dir, slot_index, &entry->summary, error_buffer, error_buffer_size))
    {
        return false;
    }

    if (presence_summary != NULL)
    {
        *presence_summary = local_presence_summary;
    }

    return true;
}

bool wolf_read_present_index_for_slot(const char *data_dir, size_t slot_index, size_t *present_index, bool *is_present, wolf_map_presence_summary *presence_summary, char *error_buffer, size_t error_buffer_size)
{
    wolf_map_presence_summary local_presence_summary;
    size_t current_slot_index;
    size_t current_present_index;

    if (error_buffer != NULL && error_buffer_size > 0)
    {
        error_buffer[0] = '\0';
    }

    if (data_dir == NULL || present_index == NULL || is_present == NULL)
    {
        set_error(error_buffer, error_buffer_size, "could not inspect present map slot");
        return false;
    }

    if (!wolf_read_map_presence_summary(data_dir, &local_presence_summary, error_buffer, error_buffer_size))
    {
        return false;
    }

    if (slot_index >= local_presence_summary.total_slots)
    {
        set_error(error_buffer, error_buffer_size, "map index is out of range");
        return false;
    }

    current_present_index = 0;
    for (current_slot_index = 0; current_slot_index <= slot_index; ++current_slot_index)
    {
        uint32_t map_offset = 0;
        bool current_is_present = false;

        if (!wolf_read_map_slot(data_dir, current_slot_index, &map_offset, &current_is_present, error_buffer, error_buffer_size))
        {
            return false;
        }

        (void)map_offset;
        if (current_slot_index == slot_index)
        {
            *is_present = current_is_present;
            *present_index = current_is_present ? current_present_index : (size_t)-1;
            if (presence_summary != NULL)
            {
                *presence_summary = local_presence_summary;
            }
            return true;
        }

        if (current_is_present)
        {
            current_present_index += 1;
        }
    }

    set_error(error_buffer, error_buffer_size, "map index is out of range");
    return false;
}

bool wolf_load_map_catalog(const char *data_dir, size_t count, wolf_loaded_map *maps, size_t maps_count, size_t *loaded_count, wolf_maphead_summary *maphead_summary, char *error_buffer, size_t error_buffer_size)
{
    wolf_maphead_summary local_maphead;
    size_t loaded_index;

    if (error_buffer != NULL && error_buffer_size > 0)
    {
        error_buffer[0] = '\0';
    }

    if (data_dir == NULL || maps == NULL || loaded_count == NULL || count > maps_count)
    {
        set_error(error_buffer, error_buffer_size, "could not load map catalog");
        return false;
    }

    if (!wolf_read_maphead_summary(data_dir, &local_maphead, error_buffer, error_buffer_size))
    {
        return false;
    }

    count = count < local_maphead.map_count ? count : local_maphead.map_count;
    for (loaded_index = 0; loaded_index < count; ++loaded_index)
    {
        if (!wolf_load_map(data_dir, loaded_index, &maps[loaded_index], error_buffer, error_buffer_size))
        {
            return false;
        }
    }

    *loaded_count = count;
    if (maphead_summary != NULL)
    {
        *maphead_summary = local_maphead;
    }

    return true;
}

bool wolf_load_present_map(const char *data_dir, size_t present_index, wolf_loaded_present_map *entry, wolf_map_presence_summary *presence_summary, char *error_buffer, size_t error_buffer_size)
{
    size_t slot_index;
    wolf_map_presence_summary local_presence_summary;

    if (error_buffer != NULL && error_buffer_size > 0)
    {
        error_buffer[0] = '\0';
    }

    if (data_dir == NULL || entry == NULL)
    {
        set_error(error_buffer, error_buffer_size, "could not load present map");
        return false;
    }

    if (!wolf_find_present_map_slot(data_dir, present_index, &slot_index, &local_presence_summary, error_buffer, error_buffer_size))
    {
        return false;
    }

    entry->slot_index = slot_index;
    if (!wolf_load_map(data_dir, slot_index, &entry->map, error_buffer, error_buffer_size))
    {
        return false;
    }

    if (presence_summary != NULL)
    {
        *presence_summary = local_presence_summary;
    }

    return true;
}

bool wolf_load_present_map_catalog(const char *data_dir, size_t count, wolf_loaded_present_map *entries, size_t entries_count, size_t *loaded_count, wolf_map_presence_summary *presence_summary, char *error_buffer, size_t error_buffer_size)
{
    wolf_map_presence_summary local_presence_summary;
    size_t slot_index;
    size_t entry_index;

    if (error_buffer != NULL && error_buffer_size > 0)
    {
        error_buffer[0] = '\0';
    }

    if (data_dir == NULL || entries == NULL || loaded_count == NULL || count > entries_count)
    {
        set_error(error_buffer, error_buffer_size, "could not load present map catalog");
        return false;
    }

    if (!wolf_read_map_presence_summary(data_dir, &local_presence_summary, error_buffer, error_buffer_size))
    {
        return false;
    }

    count = count < local_presence_summary.present_slots ? count : local_presence_summary.present_slots;
    entry_index = 0;
    for (slot_index = 0; slot_index < local_presence_summary.total_slots && entry_index < count; ++slot_index)
    {
        uint32_t map_offset = 0;
        bool is_present = false;

        if (!wolf_read_map_slot(data_dir, slot_index, &map_offset, &is_present, error_buffer, error_buffer_size))
        {
            return false;
        }

        (void)map_offset;
        if (!is_present)
        {
            continue;
        }

        entries[entry_index].slot_index = slot_index;
        if (!wolf_load_map(data_dir, slot_index, &entries[entry_index].map, error_buffer, error_buffer_size))
        {
            return false;
        }

        entry_index += 1;
    }

    *loaded_count = entry_index;
    if (presence_summary != NULL)
    {
        *presence_summary = local_presence_summary;
    }

    return true;
}

bool wolf_validate_map(const char *data_dir, size_t map_index, wolf_map_summary *summary, wolf_map_plane_header headers[3], char *error_buffer, size_t error_buffer_size)
{
    wolf_map_summary local_summary;
    wolf_map_plane_header local_headers[3];

    if (error_buffer != NULL && error_buffer_size > 0)
    {
        error_buffer[0] = '\0';
    }

    if (data_dir == NULL)
    {
        set_error(error_buffer, error_buffer_size, "could not validate map");
        return false;
    }

    if (!wolf_read_map_summary(data_dir, map_index, &local_summary, error_buffer, error_buffer_size))
    {
        return false;
    }

    if (!wolf_map_header_is_valid(&local_summary) || !wolf_map_planes_are_in_bounds(&local_summary))
    {
        set_error(error_buffer, error_buffer_size, "map header is invalid");
        return false;
    }

    if (!wolf_read_map_plane_headers(data_dir, map_index, local_headers, error_buffer, error_buffer_size))
    {
        return false;
    }

    if (!wolf_map_plane_headers_are_valid(&local_summary, local_headers))
    {
        set_error(error_buffer, error_buffer_size, "map plane table is invalid");
        return false;
    }

    if (summary != NULL)
    {
        *summary = local_summary;
    }

    if (headers != NULL)
    {
        memcpy(headers, local_headers, sizeof(local_headers));
    }

    return true;
}

bool wolf_validate_map_load(const char *data_dir, size_t map_index, wolf_loaded_map *map, wolf_map_plane_header headers[3], char *error_buffer, size_t error_buffer_size)
{
    wolf_loaded_map local_map;
    wolf_map_plane_header local_headers[3];
    size_t plane_index;

    if (error_buffer != NULL && error_buffer_size > 0)
    {
        error_buffer[0] = '\0';
    }

    if (data_dir == NULL)
    {
        set_error(error_buffer, error_buffer_size, "could not validate map load");
        return false;
    }

    if (!wolf_load_map(data_dir, map_index, &local_map, error_buffer, error_buffer_size))
    {
        return false;
    }

    if (!wolf_read_map_plane_headers(data_dir, map_index, local_headers, error_buffer, error_buffer_size)
        || !wolf_map_plane_headers_are_valid(&local_map.summary, local_headers))
    {
        if (error_buffer != NULL && error_buffer_size > 0 && error_buffer[0] == '\0')
        {
            set_error(error_buffer, error_buffer_size, "loaded map plane table is invalid");
        }
        return false;
    }

    for (plane_index = 0; plane_index < 3; ++plane_index)
    {
        if (!wolf_map_plane_load_result_matches_header(&local_headers[plane_index], &local_map.plane_results[plane_index]))
        {
            set_error(error_buffer, error_buffer_size, "loaded map does not match plane headers");
            return false;
        }
    }

    if (map != NULL)
    {
        *map = local_map;
    }

    if (headers != NULL)
    {
        memcpy(headers, local_headers, sizeof(local_headers));
    }

    return true;
}

bool wolf_validate_present_map_load(const char *data_dir, size_t present_index, wolf_loaded_present_map *entry, wolf_map_plane_header headers[3], wolf_map_presence_summary *presence_summary, char *error_buffer, size_t error_buffer_size)
{
    wolf_loaded_present_map local_entry;
    wolf_map_plane_header local_headers[3];
    wolf_map_presence_summary local_presence_summary;
    size_t plane_index;

    if (error_buffer != NULL && error_buffer_size > 0)
    {
        error_buffer[0] = '\0';
    }

    if (data_dir == NULL)
    {
        set_error(error_buffer, error_buffer_size, "could not validate present map load");
        return false;
    }

    if (!wolf_load_present_map(data_dir, present_index, &local_entry, &local_presence_summary, error_buffer, error_buffer_size))
    {
        return false;
    }

    if (!wolf_read_map_plane_headers(data_dir, local_entry.slot_index, local_headers, error_buffer, error_buffer_size)
        || !wolf_map_plane_headers_are_valid(&local_entry.map.summary, local_headers))
    {
        if (error_buffer != NULL && error_buffer_size > 0 && error_buffer[0] == '\0')
        {
            set_error(error_buffer, error_buffer_size, "loaded present map plane table is invalid");
        }
        return false;
    }

    for (plane_index = 0; plane_index < 3; ++plane_index)
    {
        if (!wolf_map_plane_load_result_matches_header(&local_headers[plane_index], &local_entry.map.plane_results[plane_index]))
        {
            set_error(error_buffer, error_buffer_size, "loaded present map does not match plane headers");
            return false;
        }
    }

    if (entry != NULL)
    {
        *entry = local_entry;
    }

    if (headers != NULL)
    {
        memcpy(headers, local_headers, sizeof(local_headers));
    }

    if (presence_summary != NULL)
    {
        *presence_summary = local_presence_summary;
    }

    return true;
}

bool wolf_validate_map_catalog(const char *data_dir, size_t count, bool *valid_flags, size_t valid_flags_count, size_t *validated_count, char *error_buffer, size_t error_buffer_size)
{
    wolf_maphead_summary maphead_summary;
    size_t index;
    size_t limit;

    if (error_buffer != NULL && error_buffer_size > 0)
    {
        error_buffer[0] = '\0';
    }

    if (data_dir == NULL || valid_flags == NULL || validated_count == NULL || count > valid_flags_count)
    {
        set_error(error_buffer, error_buffer_size, "could not validate map catalog");
        return false;
    }

    if (!wolf_read_maphead_summary(data_dir, &maphead_summary, error_buffer, error_buffer_size))
    {
        return false;
    }

    limit = count;
    if (limit > maphead_summary.map_count)
    {
        limit = maphead_summary.map_count;
    }

    for (index = 0; index < limit; ++index)
    {
        uint32_t map_offset = 0;
        bool is_present = false;

        valid_flags[index] = false;
        if (!wolf_read_map_slot(data_dir, index, &map_offset, &is_present, error_buffer, error_buffer_size))
        {
            return false;
        }

        (void)map_offset;
        if (!is_present)
        {
            continue;
        }

        if (!wolf_validate_map(data_dir, index, NULL, NULL, error_buffer, error_buffer_size))
        {
            return false;
        }
        valid_flags[index] = true;
    }

    *validated_count = limit;
    return true;
}

bool wolf_validate_map_plane_table_catalog(const char *data_dir, size_t count, bool *valid_flags, size_t valid_flags_count, size_t *validated_count, wolf_maphead_summary *maphead_summary, char *error_buffer, size_t error_buffer_size)
{
    wolf_maphead_summary local_maphead_summary;
    size_t index;
    size_t limit;

    if (error_buffer != NULL && error_buffer_size > 0)
    {
        error_buffer[0] = '\0';
    }

    if (data_dir == NULL || valid_flags == NULL || validated_count == NULL || count > valid_flags_count)
    {
        set_error(error_buffer, error_buffer_size, "could not validate map plane table catalog");
        return false;
    }

    if (!wolf_read_maphead_summary(data_dir, &local_maphead_summary, error_buffer, error_buffer_size))
    {
        return false;
    }

    limit = count;
    if (limit > local_maphead_summary.map_count)
    {
        limit = local_maphead_summary.map_count;
    }

    for (index = 0; index < limit; ++index)
    {
        uint32_t map_offset = 0;
        bool is_present = false;
        wolf_map_plane_table table;

        valid_flags[index] = false;
        if (!wolf_read_map_slot(data_dir, index, &map_offset, &is_present, error_buffer, error_buffer_size))
        {
            return false;
        }

        (void)map_offset;
        if (!is_present)
        {
            continue;
        }

        if (!wolf_read_map_plane_table(data_dir, index, &table, error_buffer, error_buffer_size))
        {
            return false;
        }

        valid_flags[index] = wolf_map_plane_headers_are_valid(&table.summary, table.headers);
    }

    *validated_count = limit;
    if (maphead_summary != NULL)
    {
        *maphead_summary = local_maphead_summary;
    }

    return true;
}

bool wolf_validate_present_map_catalog(const char *data_dir, size_t count, size_t *slot_indices, bool *valid_flags, size_t entries_count, size_t *validated_count, wolf_map_presence_summary *presence_summary, char *error_buffer, size_t error_buffer_size)
{
    wolf_map_presence_summary local_presence_summary;
    size_t slot_index;
    size_t entry_index;

    if (error_buffer != NULL && error_buffer_size > 0)
    {
        error_buffer[0] = '\0';
    }

    if (data_dir == NULL || slot_indices == NULL || valid_flags == NULL || validated_count == NULL || count > entries_count)
    {
        set_error(error_buffer, error_buffer_size, "could not validate present map catalog");
        return false;
    }

    if (!wolf_read_map_presence_summary(data_dir, &local_presence_summary, error_buffer, error_buffer_size))
    {
        return false;
    }

    count = count < local_presence_summary.present_slots ? count : local_presence_summary.present_slots;
    entry_index = 0;
    for (slot_index = 0; slot_index < local_presence_summary.total_slots && entry_index < count; ++slot_index)
    {
        uint32_t map_offset = 0;
        bool is_present = false;

        if (!wolf_read_map_slot(data_dir, slot_index, &map_offset, &is_present, error_buffer, error_buffer_size))
        {
            return false;
        }

        (void)map_offset;
        if (!is_present)
        {
            continue;
        }

        slot_indices[entry_index] = slot_index;
        valid_flags[entry_index] = false;
        if (!wolf_validate_map(data_dir, slot_index, NULL, NULL, error_buffer, error_buffer_size))
        {
            return false;
        }
        valid_flags[entry_index] = true;
        entry_index += 1;
    }

    *validated_count = entry_index;
    if (presence_summary != NULL)
    {
        *presence_summary = local_presence_summary;
    }

    return true;
}

bool wolf_validate_present_map_load_catalog(const char *data_dir, size_t count, size_t *slot_indices, bool *valid_flags, size_t entries_count, size_t *validated_count, wolf_map_presence_summary *presence_summary, char *error_buffer, size_t error_buffer_size)
{
    wolf_map_presence_summary local_presence_summary;
    size_t slot_index;
    size_t entry_index;

    if (error_buffer != NULL && error_buffer_size > 0)
    {
        error_buffer[0] = '\0';
    }

    if (data_dir == NULL || slot_indices == NULL || valid_flags == NULL || validated_count == NULL || count > entries_count)
    {
        set_error(error_buffer, error_buffer_size, "could not validate present map load catalog");
        return false;
    }

    if (!wolf_read_map_presence_summary(data_dir, &local_presence_summary, error_buffer, error_buffer_size))
    {
        return false;
    }

    count = count < local_presence_summary.present_slots ? count : local_presence_summary.present_slots;
    entry_index = 0;
    for (slot_index = 0; slot_index < local_presence_summary.total_slots && entry_index < count; ++slot_index)
    {
        uint32_t map_offset = 0;
        bool is_present = false;

        if (!wolf_read_map_slot(data_dir, slot_index, &map_offset, &is_present, error_buffer, error_buffer_size))
        {
            return false;
        }

        (void)map_offset;
        if (!is_present)
        {
            continue;
        }

        slot_indices[entry_index] = slot_index;
        valid_flags[entry_index] = false;
        if (!wolf_validate_map_load(data_dir, slot_index, NULL, NULL, error_buffer, error_buffer_size))
        {
            return false;
        }

        valid_flags[entry_index] = true;
        entry_index += 1;
    }

    *validated_count = entry_index;
    if (presence_summary != NULL)
    {
        *presence_summary = local_presence_summary;
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

bool wolf_map_dimensions_are_supported(const wolf_map_summary *summary)
{
    if (!wolf_map_header_is_valid(summary))
    {
        return false;
    }

    if (summary->width > 64 || summary->height > 64)
    {
        return false;
    }

    return true;
}

bool wolf_map_plane_header_is_valid_for_map(const wolf_map_summary *summary, const wolf_map_plane_header *header)
{
    size_t expected_words;

    if (!wolf_map_dimensions_are_supported(summary) || header == NULL)
    {
        return false;
    }

    if (header->length < 4
        || header->carmack_expanded_bytes < 2
        || (header->carmack_expanded_bytes % 2) != 0
        || (header->rlew_expanded_bytes % 2) != 0)
    {
        return false;
    }

    expected_words = (size_t)summary->width * (size_t)summary->height;
    if (expected_words == 0)
    {
        return false;
    }

    return header->rlew_expanded_bytes == (expected_words * sizeof(uint16_t))
        && header->decoded_words == expected_words;
}

bool wolf_map_plane_header_matches_summary(const wolf_map_summary *summary, size_t plane_index, const wolf_map_plane_header *header)
{
    if (!wolf_map_header_is_valid(summary) || header == NULL || plane_index >= 3)
    {
        return false;
    }

    return header->offset == summary->plane_offsets[plane_index]
        && header->length == summary->plane_lengths[plane_index];
}

bool wolf_map_plane_header_is_in_bounds(const wolf_map_summary *summary, size_t plane_index, const wolf_map_plane_header *header)
{
    uint64_t plane_end;

    if (!wolf_map_plane_header_matches_summary(summary, plane_index, header)
        || summary->gamemaps_file_size == 0)
    {
        return false;
    }

    plane_end = (uint64_t)header->offset + (uint64_t)header->length;
    if (plane_end > summary->gamemaps_file_size)
    {
        return false;
    }

    if (plane_index < 2 && plane_end > summary->plane_offsets[plane_index + 1])
    {
        return false;
    }

    return true;
}

bool wolf_map_plane_load_result_matches_header(const wolf_map_plane_header *header, const wolf_map_plane_load_result *result)
{
    if (header == NULL || result == NULL)
    {
        return false;
    }

    return result->compressed_bytes == header->length
        && result->carmack_expanded_bytes == header->carmack_expanded_bytes
        && result->rlew_expanded_bytes == header->rlew_expanded_bytes
        && result->decoded_words == header->decoded_words;
}

bool wolf_map_plane_is_valid(const wolf_map_summary *summary, size_t plane_index, const wolf_map_plane_header *header)
{
    return wolf_map_plane_header_is_valid_for_map(summary, header)
        && wolf_map_plane_header_matches_summary(summary, plane_index, header)
        && wolf_map_plane_header_is_in_bounds(summary, plane_index, header);
}

bool wolf_map_plane_headers_are_valid(const wolf_map_summary *summary, const wolf_map_plane_header headers[3])
{
    size_t plane_index;

    if (summary == NULL || headers == NULL)
    {
        return false;
    }

    for (plane_index = 0; plane_index < 3; ++plane_index)
    {
        if (!wolf_map_plane_is_valid(summary, plane_index, &headers[plane_index]))
        {
            return false;
        }
    }

    return true;
}

bool wolf_map_planes_are_in_bounds(const wolf_map_summary *summary)
{
    size_t i;
    uint64_t end;

    if (!wolf_map_header_is_valid(summary) || summary->gamemaps_file_size == 0)
    {
        return false;
    }

    for (i = 0; i < 3; ++i)
    {
        end = (uint64_t)summary->plane_offsets[i] + (uint64_t)summary->plane_lengths[i];
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

bool wolf_map_plane_header_from_bytes(const wolf_map_summary *summary, size_t plane_index, uint32_t plane_offset, const uint8_t *compressed_bytes, size_t compressed_size, wolf_map_plane_header *header, char *error_buffer, size_t error_buffer_size)
{
    uint16_t *carmack_words;
    size_t carmack_word_count;
    bool ok;

    if (error_buffer != NULL && error_buffer_size > 0)
    {
        error_buffer[0] = '\0';
    }

    if (summary == NULL || compressed_bytes == NULL || header == NULL || plane_index >= 3)
    {
        set_error(error_buffer, error_buffer_size, "could not inspect map plane header");
        return false;
    }

    if (!wolf_map_header_is_valid(summary) || !wolf_map_planes_are_in_bounds(summary))
    {
        set_error(error_buffer, error_buffer_size, "map header is invalid");
        return false;
    }

    if (compressed_size != summary->plane_lengths[plane_index])
    {
        set_error(error_buffer, error_buffer_size, "map plane size does not match header summary");
        return false;
    }

    if (compressed_size < 4)
    {
        set_error(error_buffer, error_buffer_size, "map plane is too small");
        return false;
    }

    header->offset = plane_offset;
    header->length = (uint16_t)compressed_size;
    header->carmack_expanded_bytes = read_u16_le(compressed_bytes);
    if ((header->carmack_expanded_bytes % 2) != 0)
    {
        set_error(error_buffer, error_buffer_size, "carmack-expanded size must be even");
        return false;
    }

    carmack_word_count = (size_t)(header->carmack_expanded_bytes / 2);
    if (carmack_word_count == 0)
    {
        set_error(error_buffer, error_buffer_size, "carmack-expanded plane is empty");
        return false;
    }

    carmack_words = (uint16_t *)malloc(carmack_word_count * sizeof(uint16_t));
    if (carmack_words == NULL)
    {
        set_error(error_buffer, error_buffer_size, "out of memory expanding map plane");
        return false;
    }

    ok = wolf_carmack_expand_bytes(compressed_bytes + 2, compressed_size - 2, carmack_words, carmack_word_count);
    if (!ok)
    {
        free(carmack_words);
        set_error(error_buffer, error_buffer_size, "could not Carmack-expand map plane");
        return false;
    }

    header->rlew_expanded_bytes = carmack_words[0];
    free(carmack_words);
    if ((header->rlew_expanded_bytes % 2) != 0)
    {
        set_error(error_buffer, error_buffer_size, "RLEW-expanded size must be even");
        return false;
    }

    header->decoded_words = (size_t)(header->rlew_expanded_bytes / 2);
    if (!wolf_map_plane_is_valid(summary, plane_index, header))
    {
        set_error(error_buffer, error_buffer_size, "map plane header does not match map dimensions");
        return false;
    }

    return true;
}

bool wolf_read_map_plane_header(const char *data_dir, size_t map_index, size_t plane_index, wolf_map_plane_header *header, char *error_buffer, size_t error_buffer_size)
{
    char path[4096];
    FILE *file;
    wolf_map_summary summary;
    unsigned char *compressed_bytes;
    size_t compressed_size;
    bool ok;

    if (error_buffer != NULL && error_buffer_size > 0)
    {
        error_buffer[0] = '\0';
    }

    if (data_dir == NULL || header == NULL || plane_index >= 3)
    {
        set_error(error_buffer, error_buffer_size, "could not inspect map plane header");
        return false;
    }

    if (!wolf_read_map_summary(data_dir, map_index, &summary, error_buffer, error_buffer_size))
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

    ok = wolf_map_plane_header_from_bytes(&summary,
        plane_index,
        summary.plane_offsets[plane_index],
        compressed_bytes,
        compressed_size,
        header,
        error_buffer,
        error_buffer_size);
    free(compressed_bytes);
    return ok;
}

bool wolf_read_map_plane_headers(const char *data_dir, size_t map_index, wolf_map_plane_header headers[3], char *error_buffer, size_t error_buffer_size)
{
    size_t plane_index;

    if (error_buffer != NULL && error_buffer_size > 0)
    {
        error_buffer[0] = '\0';
    }

    if (data_dir == NULL || headers == NULL)
    {
        set_error(error_buffer, error_buffer_size, "could not inspect map plane table");
        return false;
    }

    for (plane_index = 0; plane_index < 3; ++plane_index)
    {
        if (!wolf_read_map_plane_header(data_dir, map_index, plane_index, &headers[plane_index], error_buffer, error_buffer_size))
        {
            return false;
        }
    }

    return true;
}

bool wolf_read_present_map_plane_headers(const char *data_dir, size_t present_index, wolf_present_map_summary *entry, wolf_map_plane_header headers[3], wolf_map_presence_summary *presence_summary, char *error_buffer, size_t error_buffer_size)
{
    wolf_present_map_summary local_entry;
    wolf_map_presence_summary local_presence_summary;

    if (error_buffer != NULL && error_buffer_size > 0)
    {
        error_buffer[0] = '\0';
    }

    if (data_dir == NULL || headers == NULL)
    {
        set_error(error_buffer, error_buffer_size, "could not inspect present map plane table");
        return false;
    }

    if (!wolf_read_present_map_summary(data_dir, present_index, &local_entry, &local_presence_summary, error_buffer, error_buffer_size))
    {
        return false;
    }

    if (!wolf_read_map_plane_headers(data_dir, local_entry.slot_index, headers, error_buffer, error_buffer_size))
    {
        return false;
    }

    if (entry != NULL)
    {
        *entry = local_entry;
    }

    if (presence_summary != NULL)
    {
        *presence_summary = local_presence_summary;
    }

    return true;
}

bool wolf_read_map_plane_table(const char *data_dir, size_t map_index, wolf_map_plane_table *table, char *error_buffer, size_t error_buffer_size)
{
    if (error_buffer != NULL && error_buffer_size > 0)
    {
        error_buffer[0] = '\0';
    }

    if (data_dir == NULL || table == NULL)
    {
        set_error(error_buffer, error_buffer_size, "could not inspect map plane table");
        return false;
    }

    if (!wolf_read_map_summary(data_dir, map_index, &table->summary, error_buffer, error_buffer_size))
    {
        return false;
    }

    if (!wolf_read_map_plane_headers(data_dir, map_index, table->headers, error_buffer, error_buffer_size))
    {
        return false;
    }

    return true;
}

bool wolf_read_map_plane_table_catalog(const char *data_dir, size_t count, wolf_map_plane_table *tables, size_t tables_count, size_t *loaded_count, wolf_maphead_summary *maphead_summary, char *error_buffer, size_t error_buffer_size)
{
    wolf_maphead_summary local_maphead;
    size_t index;
    size_t limit;

    if (error_buffer != NULL && error_buffer_size > 0)
    {
        error_buffer[0] = '\0';
    }

    if (data_dir == NULL || tables == NULL || loaded_count == NULL || count > tables_count)
    {
        set_error(error_buffer, error_buffer_size, "could not inspect map plane table catalog");
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
        if (!wolf_read_map_plane_table(data_dir, index, &tables[index], error_buffer, error_buffer_size))
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

bool wolf_read_present_map_plane_table(const char *data_dir, size_t present_index, wolf_present_map_plane_table *table, wolf_map_presence_summary *presence_summary, char *error_buffer, size_t error_buffer_size)
{
    wolf_present_map_summary present_map;
    wolf_map_presence_summary local_presence_summary;

    if (error_buffer != NULL && error_buffer_size > 0)
    {
        error_buffer[0] = '\0';
    }

    if (data_dir == NULL || table == NULL)
    {
        set_error(error_buffer, error_buffer_size, "could not inspect present map plane table");
        return false;
    }

    if (!wolf_read_present_map_summary(data_dir, present_index, &present_map, &local_presence_summary, error_buffer, error_buffer_size))
    {
        return false;
    }

    table->slot_index = present_map.slot_index;
    if (!wolf_read_map_plane_table(data_dir, present_map.slot_index, &table->table, error_buffer, error_buffer_size))
    {
        return false;
    }

    if (presence_summary != NULL)
    {
        *presence_summary = local_presence_summary;
    }

    return true;
}

bool wolf_read_present_map_plane_table_catalog(const char *data_dir, size_t count, wolf_present_map_plane_table *tables, size_t tables_count, size_t *loaded_count, wolf_map_presence_summary *presence_summary, char *error_buffer, size_t error_buffer_size)
{
    wolf_map_presence_summary local_presence_summary;
    size_t index;
    size_t limit;

    if (error_buffer != NULL && error_buffer_size > 0)
    {
        error_buffer[0] = '\0';
    }

    if (data_dir == NULL || tables == NULL || loaded_count == NULL || count > tables_count)
    {
        set_error(error_buffer, error_buffer_size, "could not inspect present map plane table catalog");
        return false;
    }

    if (!wolf_read_map_presence_summary(data_dir, &local_presence_summary, error_buffer, error_buffer_size))
    {
        return false;
    }

    limit = count;
    if (limit > local_presence_summary.present_slots)
    {
        limit = local_presence_summary.present_slots;
    }

    for (index = 0; index < limit; ++index)
    {
        if (!wolf_read_present_map_plane_table(data_dir, index, &tables[index], NULL, error_buffer, error_buffer_size))
        {
            return false;
        }
    }

    *loaded_count = limit;
    if (presence_summary != NULL)
    {
        *presence_summary = local_presence_summary;
    }

    return true;
}

bool wolf_decode_map_plane(const uint8_t *compressed_bytes, size_t compressed_size, uint16_t rlew_tag, uint16_t *dest, size_t dest_words, wolf_map_plane_load_result *result, char *error_buffer, size_t error_buffer_size)
{
    uint16_t *carmack_words;
    size_t carmack_word_count;
    bool ok;

    if (error_buffer != NULL && error_buffer_size > 0)
    {
        error_buffer[0] = '\0';
    }

    if (compressed_bytes == NULL || dest == NULL || result == NULL)
    {
        set_error(error_buffer, error_buffer_size, "could not decode map plane");
        return false;
    }

    if (compressed_size < 4)
    {
        set_error(error_buffer, error_buffer_size, "map plane is too small");
        return false;
    }

    result->compressed_bytes = (uint16_t)compressed_size;
    result->carmack_expanded_bytes = read_u16_le(compressed_bytes);
    if ((result->carmack_expanded_bytes % 2) != 0)
    {
        set_error(error_buffer, error_buffer_size, "carmack-expanded size must be even");
        return false;
    }

    carmack_word_count = (size_t)(result->carmack_expanded_bytes / 2);
    if (carmack_word_count == 0)
    {
        set_error(error_buffer, error_buffer_size, "carmack-expanded plane is empty");
        return false;
    }

    carmack_words = (uint16_t *)malloc(carmack_word_count * sizeof(uint16_t));
    if (carmack_words == NULL)
    {
        set_error(error_buffer, error_buffer_size, "out of memory expanding map plane");
        return false;
    }

    ok = wolf_carmack_expand_bytes(compressed_bytes + 2, compressed_size - 2, carmack_words, carmack_word_count);
    if (!ok)
    {
        free(carmack_words);
        set_error(error_buffer, error_buffer_size, "could not Carmack-expand map plane");
        return false;
    }

    result->rlew_expanded_bytes = carmack_words[0];
    if ((result->rlew_expanded_bytes % 2) != 0)
    {
        free(carmack_words);
        set_error(error_buffer, error_buffer_size, "RLEW-expanded size must be even");
        return false;
    }

    result->decoded_words = (size_t)(result->rlew_expanded_bytes / 2);
    if (result->decoded_words > dest_words)
    {
        free(carmack_words);
        set_error(error_buffer, error_buffer_size, "destination buffer is too small for map plane");
        return false;
    }

    ok = wolf_rlew_expand_words(carmack_words + 1, carmack_word_count - 1, dest, result->decoded_words, rlew_tag);
    free(carmack_words);
    if (!ok)
    {
        set_error(error_buffer, error_buffer_size, "could not RLEW-expand map plane");
        return false;
    }

    return true;
}

bool wolf_load_map_plane_words(const char *data_dir, size_t map_index, size_t plane_index, uint16_t *dest, size_t dest_words, wolf_map_plane_load_result *result, char *error_buffer, size_t error_buffer_size)
{
    char path[4096];
    FILE *file;
    wolf_map_summary summary;
    wolf_maphead_summary maphead;
    wolf_map_plane_header plane_header;
    unsigned char *compressed_bytes;
    size_t compressed_size;
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

    if (!wolf_read_map_plane_header(data_dir, map_index, plane_index, &plane_header, error_buffer, error_buffer_size))
    {
        return false;
    }

    compressed_size = plane_header.length;

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

    ok = wolf_decode_map_plane(compressed_bytes,
        compressed_size,
        maphead.rlew_tag,
        dest,
        dest_words,
        result,
        error_buffer,
        error_buffer_size);
    free(compressed_bytes);
    if (!ok)
    {
        return false;
    }

    if (!wolf_map_plane_load_result_matches_header(&plane_header, result))
    {
        set_error(error_buffer, error_buffer_size, "decoded map plane does not match plane header");
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

    if (!wolf_map_dimensions_are_supported(&map->summary) || !wolf_map_planes_are_in_bounds(&map->summary))
    {
        set_error(error_buffer, error_buffer_size, "map header is invalid");
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

bool wolf_map_cell_index(const wolf_map_summary *summary, size_t x, size_t y, size_t *index)
{
    if (summary == NULL || index == NULL)
    {
        return false;
    }

    if (summary->width == 0 || summary->height == 0 || summary->width > 64 || summary->height > 64)
    {
        return false;
    }

    if (x >= summary->width || y >= summary->height)
    {
        return false;
    }

    *index = (y * (size_t)summary->width) + x;
    return true;
}

bool wolf_map_get_plane_result(const wolf_loaded_map *map, size_t plane_index, const wolf_map_plane_load_result **result)
{
    if (map == NULL || result == NULL || plane_index >= 3)
    {
        return false;
    }

    if (map->summary.width == 0 || map->summary.height == 0 || map->summary.width > 64 || map->summary.height > 64)
    {
        return false;
    }

    *result = &map->plane_results[plane_index];
    return true;
}

bool wolf_map_get_plane_words(const wolf_loaded_map *map, size_t plane_index, const uint16_t **words, size_t *word_count)
{
    if (map == NULL || words == NULL || word_count == NULL || plane_index >= 3)
    {
        return false;
    }

    if (map->summary.width == 0 || map->summary.height == 0 || map->summary.width > 64 || map->summary.height > 64)
    {
        return false;
    }

    *words = map->plane_words[plane_index];
    *word_count = (size_t)map->summary.width * (size_t)map->summary.height;
    return true;
}

bool wolf_map_get_row(const wolf_loaded_map *map, size_t plane_index, size_t y, const uint16_t **row_words, size_t *row_length)
{
    const uint16_t *words;
    size_t word_count;
    size_t row_start;

    if (map == NULL || row_words == NULL || row_length == NULL)
    {
        return false;
    }

    if (!wolf_map_get_plane_words(map, plane_index, &words, &word_count))
    {
        return false;
    }

    if (y >= map->summary.height)
    {
        return false;
    }

    row_start = y * (size_t)map->summary.width;
    if (row_start >= word_count)
    {
        return false;
    }

    *row_words = words + row_start;
    *row_length = (size_t)map->summary.width;
    return true;
}

bool wolf_map_get_column(const wolf_loaded_map *map, size_t plane_index, size_t x, uint16_t *column_words, size_t column_capacity, size_t *column_length)
{
    const uint16_t *words;
    size_t word_count;
    size_t y;

    if (map == NULL || column_words == NULL || column_length == NULL)
    {
        return false;
    }

    if (!wolf_map_get_plane_words(map, plane_index, &words, &word_count))
    {
        return false;
    }

    if (x >= map->summary.width || column_capacity < map->summary.height)
    {
        return false;
    }

    for (y = 0; y < map->summary.height; ++y)
    {
        size_t index = (y * (size_t)map->summary.width) + x;
        if (index >= word_count)
        {
            return false;
        }
        column_words[y] = words[index];
    }

    *column_length = (size_t)map->summary.height;
    return true;
}

bool wolf_map_get_region(const wolf_loaded_map *map, size_t plane_index, size_t x, size_t y, size_t region_width, size_t region_height, uint16_t *region_words, size_t region_capacity, size_t *region_word_count)
{
    const uint16_t *words;
    size_t word_count;
    size_t row;
    size_t column;
    size_t write_index = 0;

    if (map == NULL || region_words == NULL || region_word_count == NULL)
    {
        return false;
    }

    if (!wolf_map_get_plane_words(map, plane_index, &words, &word_count))
    {
        return false;
    }

    if (region_width == 0
        || region_height == 0
        || x >= map->summary.width
        || y >= map->summary.height
        || region_width > ((size_t)map->summary.width - x)
        || region_height > ((size_t)map->summary.height - y)
        || region_capacity < (region_width * region_height))
    {
        return false;
    }

    for (row = 0; row < region_height; ++row)
    {
        for (column = 0; column < region_width; ++column)
        {
            size_t index = ((y + row) * (size_t)map->summary.width) + (x + column);
            if (index >= word_count)
            {
                return false;
            }
            region_words[write_index++] = words[index];
        }
    }

    *region_word_count = write_index;
    return true;
}

bool wolf_map_get_cell(const wolf_loaded_map *map, size_t plane_index, size_t x, size_t y, uint16_t *value)
{
    size_t index;
    const uint16_t *words;
    size_t word_count;

    if (map == NULL || value == NULL)
    {
        return false;
    }

    if (!wolf_map_get_plane_words(map, plane_index, &words, &word_count))
    {
        return false;
    }

    if (!wolf_map_cell_index(&map->summary, x, y, &index) || index >= word_count)
    {
        return false;
    }

    *value = words[index];
    return true;
}

bool wolf_present_map_get_slot_index(const wolf_loaded_present_map *entry, size_t *slot_index)
{
    if (entry == NULL || slot_index == NULL)
    {
        return false;
    }

    *slot_index = entry->slot_index;
    return true;
}

bool wolf_present_map_get_plane_result(const wolf_loaded_present_map *entry, size_t plane_index, const wolf_map_plane_load_result **result)
{
    if (entry == NULL)
    {
        return false;
    }

    return wolf_map_get_plane_result(&entry->map, plane_index, result);
}

bool wolf_present_map_get_plane_words(const wolf_loaded_present_map *entry, size_t plane_index, const uint16_t **words, size_t *word_count)
{
    if (entry == NULL)
    {
        return false;
    }

    return wolf_map_get_plane_words(&entry->map, plane_index, words, word_count);
}

bool wolf_present_map_get_row(const wolf_loaded_present_map *entry, size_t plane_index, size_t y, const uint16_t **row_words, size_t *row_length)
{
    if (entry == NULL)
    {
        return false;
    }

    return wolf_map_get_row(&entry->map, plane_index, y, row_words, row_length);
}

bool wolf_present_map_get_column(const wolf_loaded_present_map *entry, size_t plane_index, size_t x, uint16_t *column_words, size_t column_capacity, size_t *column_length)
{
    if (entry == NULL)
    {
        return false;
    }

    return wolf_map_get_column(&entry->map, plane_index, x, column_words, column_capacity, column_length);
}

bool wolf_present_map_get_region(const wolf_loaded_present_map *entry, size_t plane_index, size_t x, size_t y, size_t region_width, size_t region_height, uint16_t *region_words, size_t region_capacity, size_t *region_word_count)
{
    if (entry == NULL)
    {
        return false;
    }

    return wolf_map_get_region(&entry->map, plane_index, x, y, region_width, region_height, region_words, region_capacity, region_word_count);
}

bool wolf_present_map_get_cell(const wolf_loaded_present_map *entry, size_t plane_index, size_t x, size_t y, uint16_t *value)
{
    if (entry == NULL)
    {
        return false;
    }

    return wolf_map_get_cell(&entry->map, plane_index, x, y, value);
}
