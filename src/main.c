#include <stdio.h>
#include <string.h>

#include "wolf3d/assets.h"
#include "wolf3d/filesystem.h"
#include "wolf3d/platform.h"
#include "wolf3d/port.h"

const char *wolf_port_version(void)
{
    return WOLF3D_PORT_VERSION;
}

int main(int argc, char **argv)
{
    int i;
    const char *data_path = wolf_default_data_dir();
    int check_data = 0;
    int inspect_maphead = 0;
    char error_buffer[256];
    wolf_maphead_summary maphead_summary;

    for (i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "--version") == 0)
        {
            puts(wolf_port_version());
            return 0;
        }

        if (strcmp(argv[i], "--check-data") == 0)
        {
            check_data = 1;
            continue;
        }

        if (strcmp(argv[i], "--inspect-maphead") == 0)
        {
            inspect_maphead = 1;
            continue;
        }

        if (strcmp(argv[i], "--data") == 0)
        {
            if ((i + 1) >= argc)
            {
                fputs("--data requires a path\n", stderr);
                return 1;
            }

            data_path = argv[++i];
            continue;
        }
    }

    if (check_data)
    {
        if (!wolf_is_valid_data_dir(data_path, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        printf("data path ok: %s\n", data_path);
        return 0;
    }

    if (inspect_maphead)
    {
        if (!wolf_is_valid_data_dir(data_path, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        if (!wolf_read_maphead_summary(data_path, &maphead_summary, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        printf("maphead rlew_tag: 0x%04x\n", maphead_summary.rlew_tag);
        printf("map count: %zu\n", maphead_summary.map_count);
        printf("first map offset: %u\n", maphead_summary.first_map_offset);
        return 0;
    }

    if (!wolf_platform_init())
    {
        fputs("platform init failed\n", stderr);
        return 1;
    }

    puts("wolf3d_port stub");
    wolf_platform_shutdown();
    return 0;
}
