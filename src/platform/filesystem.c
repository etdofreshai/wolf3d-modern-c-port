#include "wolf3d/filesystem.h"

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static const char *required_files[] = {
    "VGAGRAPH.WL6",
    "VGAHEAD.WL6",
    "VGADICT.WL6",
    "MAPHEAD.WL6",
    "GAMEMAPS.WL6",
    "AUDIOHED.WL6",
    "AUDIOT.WL6",
    "VSWAP.WL6"
};

const char *wolf_default_data_dir(void)
{
    return "../game-data/base";
}

static bool path_is_dir(const char *path)
{
    struct stat st;

    if (stat(path, &st) != 0)
    {
        return false;
    }

    return S_ISDIR(st.st_mode);
}

static bool path_is_file(const char *path)
{
    struct stat st;

    if (stat(path, &st) != 0)
    {
        return false;
    }

    return S_ISREG(st.st_mode);
}

bool wolf_is_valid_data_dir(const char *path, char *error_buffer, size_t error_buffer_size)
{
    size_t i;
    char candidate[PATH_MAX];

    if (error_buffer != NULL && error_buffer_size > 0)
    {
        error_buffer[0] = '\0';
    }

    if (path == NULL || path[0] == '\0')
    {
        if (error_buffer != NULL && error_buffer_size > 0)
        {
            snprintf(error_buffer, error_buffer_size, "missing required Wolf3D data files: no data path provided");
        }
        return false;
    }

    if (!path_is_dir(path))
    {
        if (error_buffer != NULL && error_buffer_size > 0)
        {
            snprintf(error_buffer, error_buffer_size, "missing required Wolf3D data files: data directory not found: %s", path);
        }
        return false;
    }

    for (i = 0; i < (sizeof(required_files) / sizeof(required_files[0])); ++i)
    {
        snprintf(candidate, sizeof(candidate), "%s/%s", path, required_files[i]);
        if (!path_is_file(candidate))
        {
            if (error_buffer != NULL && error_buffer_size > 0)
            {
                snprintf(error_buffer, error_buffer_size, "missing required Wolf3D data files: %s", required_files[i]);
            }
            return false;
        }
    }

    return true;
}
