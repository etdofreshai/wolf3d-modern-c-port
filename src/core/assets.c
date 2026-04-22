#include "wolf3d/assets.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

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
        if (error_buffer != NULL && error_buffer_size > 0)
        {
            snprintf(error_buffer, error_buffer_size, "could not inspect MAPHEAD.WL6");
        }
        return false;
    }

    snprintf(path, sizeof(path), "%s/MAPHEAD.WL6", data_dir);
    file = fopen(path, "rb");
    if (file == NULL)
    {
        if (error_buffer != NULL && error_buffer_size > 0)
        {
            snprintf(error_buffer, error_buffer_size, "could not open %s", path);
        }
        return false;
    }

    if (fread(header, 1, sizeof(header), file) != sizeof(header))
    {
        fclose(file);
        if (error_buffer != NULL && error_buffer_size > 0)
        {
            snprintf(error_buffer, error_buffer_size, "could not read MAPHEAD.WL6 header");
        }
        return false;
    }

    if (fseek(file, 0, SEEK_END) != 0)
    {
        fclose(file);
        if (error_buffer != NULL && error_buffer_size > 0)
        {
            snprintf(error_buffer, error_buffer_size, "could not size MAPHEAD.WL6");
        }
        return false;
    }

    file_size = ftell(file);
    fclose(file);
    if (file_size < 6)
    {
        if (error_buffer != NULL && error_buffer_size > 0)
        {
            snprintf(error_buffer, error_buffer_size, "MAPHEAD.WL6 is too small");
        }
        return false;
    }

    summary->rlew_tag = read_u16_le(header);
    summary->map_count = (size_t)((file_size - 2) / 4);
    summary->first_map_offset = read_u32_le(header + 2);
    return true;
}
