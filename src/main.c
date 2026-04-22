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
    int inspect_first_map = 0;
    int validate_first_map_planes = 0;
    int self_test_rlew = 0;
    int inspect_first_map_plane0 = 0;
    char error_buffer[256];
    wolf_maphead_summary maphead_summary;
    wolf_map_summary map_summary;
    wolf_map_plane_load_result plane_load_result;
    uint16_t plane_words[64 * 64];

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

        if (strcmp(argv[i], "--inspect-first-map") == 0)
        {
            inspect_first_map = 1;
            continue;
        }

        if (strcmp(argv[i], "--validate-first-map-planes") == 0)
        {
            validate_first_map_planes = 1;
            continue;
        }

        if (strcmp(argv[i], "--self-test-rlew") == 0)
        {
            self_test_rlew = 1;
            continue;
        }

        if (strcmp(argv[i], "--inspect-first-map-plane0") == 0)
        {
            inspect_first_map_plane0 = 1;
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

    if (inspect_first_map)
    {
        if (!wolf_is_valid_data_dir(data_path, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        if (!wolf_read_first_map_summary(data_path, &map_summary, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        printf("first map name: %s\n", map_summary.name);
        printf("first map size: %ux%u\n", map_summary.width, map_summary.height);
        printf("first map plane0 offset: %u\n", map_summary.plane_offsets[0]);
        printf("first map plane1 offset: %u\n", map_summary.plane_offsets[1]);
        printf("first map plane2 offset: %u\n", map_summary.plane_offsets[2]);
        printf("first map plane0 length: %u\n", map_summary.plane_lengths[0]);
        return 0;
    }

    if (validate_first_map_planes)
    {
        printf("plane bounds valid: ");
        if (!wolf_is_valid_data_dir(data_path, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        if (!wolf_read_first_map_summary(data_path, &map_summary, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        printf("%s\n", wolf_first_map_planes_are_in_bounds(&map_summary) ? "yes" : "no");
        printf("plane0 end: %u\n", map_summary.plane_offsets[0] + map_summary.plane_lengths[0]);
        printf("plane1 end: %u\n", map_summary.plane_offsets[1] + map_summary.plane_lengths[1]);
        printf("plane2 end: %u\n", map_summary.plane_offsets[2] + map_summary.plane_lengths[2]);
        return wolf_first_map_planes_are_in_bounds(&map_summary) ? 0 : 1;
    }

    if (self_test_rlew)
    {
        static const uint16_t encoded[] = {0x1111, 0xabcd, 3, 0x2222, 0x3333};
        uint16_t decoded[5];
        if (!wolf_rlew_expand_words(encoded, 5, decoded, 5, 0xabcd))
        {
            fputs("rlew self-test failed\n", stderr);
            return 1;
        }
        printf("rlew self-test ok: %04x %04x %04x %04x %04x\n",
            decoded[0], decoded[1], decoded[2], decoded[3], decoded[4]);
        return 0;
    }

    if (inspect_first_map_plane0)
    {
        if (!wolf_is_valid_data_dir(data_path, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        if (!wolf_load_first_map_plane_words(data_path, 0, plane_words, (sizeof(plane_words) / sizeof(plane_words[0])), &plane_load_result, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        printf("plane0 compressed bytes: %u\n", plane_load_result.compressed_bytes);
        printf("plane0 carmack expanded bytes: %u\n", plane_load_result.carmack_expanded_bytes);
        printf("plane0 rlew expanded bytes: %u\n", plane_load_result.rlew_expanded_bytes);
        printf("plane0 decoded words: %zu\n", plane_load_result.decoded_words);
        printf("plane0 cells: [0,0]=%u [31,31]=%u [32,32]=%u [63,63]=%u\n",
            plane_words[0],
            plane_words[(31 * 64) + 31],
            plane_words[(32 * 64) + 32],
            plane_words[(63 * 64) + 63]);
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
