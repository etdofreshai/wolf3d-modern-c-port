#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wolf3d/assets.h"
#include "wolf3d/filesystem.h"
#include "wolf3d/platform.h"
#include "wolf3d/port.h"

const char *wolf_port_version(void)
{
    return WOLF3D_PORT_VERSION;
}

static int run_carmack_self_test(void)
{
    static const uint8_t literal_encoded[] = {
        0x34, 0x12,
        0x00, 0xa7, 0x55,
        0x00, 0xa8, 0x66};
    static const uint8_t near_copy_encoded[] = {
        0x11, 0x11,
        0x22, 0x22,
        0x02, 0xa7, 0x02};
    static const uint8_t far_copy_encoded[] = {
        0xaa, 0xaa,
        0xbb, 0xbb,
        0xcc, 0xcc,
        0x02, 0xa8, 0x00, 0x00};
    static const uint8_t near_escape_encoded[] = {
        0x00, 0xa7, 0x12};
    static const uint8_t far_escape_encoded[] = {
        0x00, 0xa8, 0x34};
    static const uint8_t invalid_near_encoded[] = {
        0x11, 0x11,
        0x01, 0xa7, 0x02};
    uint16_t decoded[5];

    if (!wolf_carmack_expand_bytes(literal_encoded, sizeof(literal_encoded), decoded, 3)
        || decoded[0] != 0x1234
        || decoded[1] != 0xa755
        || decoded[2] != 0xa866)
    {
        fputs("carmack literal self-test failed\n", stderr);
        return 1;
    }
    printf("carmack literal ok: %04x %04x %04x\n", decoded[0], decoded[1], decoded[2]);

    if (!wolf_carmack_expand_bytes(near_copy_encoded, sizeof(near_copy_encoded), decoded, 4)
        || decoded[0] != 0x1111
        || decoded[1] != 0x2222
        || decoded[2] != 0x1111
        || decoded[3] != 0x2222)
    {
        fputs("carmack near-copy self-test failed\n", stderr);
        return 1;
    }
    printf("carmack near copy ok: %04x %04x %04x %04x\n", decoded[0], decoded[1], decoded[2], decoded[3]);

    if (!wolf_carmack_expand_bytes(far_copy_encoded, sizeof(far_copy_encoded), decoded, 5)
        || decoded[0] != 0xaaaa
        || decoded[1] != 0xbbbb
        || decoded[2] != 0xcccc
        || decoded[3] != 0xaaaa
        || decoded[4] != 0xbbbb)
    {
        fputs("carmack far-copy self-test failed\n", stderr);
        return 1;
    }
    printf("carmack far copy ok: %04x %04x %04x %04x %04x\n", decoded[0], decoded[1], decoded[2], decoded[3], decoded[4]);

    if (!wolf_carmack_expand_bytes(near_escape_encoded, sizeof(near_escape_encoded), decoded, 1)
        || decoded[0] != 0xa712)
    {
        fputs("carmack near-escape self-test failed\n", stderr);
        return 1;
    }
    printf("carmack near escape ok: %04x\n", decoded[0]);

    if (!wolf_carmack_expand_bytes(far_escape_encoded, sizeof(far_escape_encoded), decoded, 1)
        || decoded[0] != 0xa834)
    {
        fputs("carmack far-escape self-test failed\n", stderr);
        return 1;
    }
    printf("carmack far escape ok: %04x\n", decoded[0]);

    if (wolf_carmack_expand_bytes(invalid_near_encoded, sizeof(invalid_near_encoded), decoded, 2))
    {
        fputs("carmack invalid near self-test failed\n", stderr);
        return 1;
    }
    puts("carmack invalid near ok");

    return 0;
}

static int run_decompression_failure_self_test(void)
{
    static const uint16_t rlew_truncated_tag[] = {0xabcd};
    static const uint16_t rlew_trailing_data[] = {0x1111, 0x2222};
    static const uint8_t carmack_truncated_literal[] = {0x00, 0xa7};
    static const uint8_t carmack_trailing_data[] = {0x11, 0x11, 0x22, 0x22};
    static const uint8_t carmack_invalid_far[] = {0x11, 0x11, 0x01, 0xa8, 0x02, 0x00};
    uint16_t decoded[4];

    if (wolf_rlew_expand_words(rlew_truncated_tag, 1, decoded, 1, 0xabcd))
    {
        fputs("rlew truncated-tag self-test failed\n", stderr);
        return 1;
    }
    puts("rlew truncated tag ok");

    if (wolf_rlew_expand_words(rlew_trailing_data, 2, decoded, 1, 0xabcd))
    {
        fputs("rlew trailing-data self-test failed\n", stderr);
        return 1;
    }
    puts("rlew trailing data ok");

    if (wolf_carmack_expand_bytes(carmack_truncated_literal, sizeof(carmack_truncated_literal), decoded, 1))
    {
        fputs("carmack truncated-literal self-test failed\n", stderr);
        return 1;
    }
    puts("carmack truncated literal ok");

    if (wolf_carmack_expand_bytes(carmack_trailing_data, sizeof(carmack_trailing_data), decoded, 1))
    {
        fputs("carmack trailing-data self-test failed\n", stderr);
        return 1;
    }
    puts("carmack trailing data ok");

    if (wolf_carmack_expand_bytes(carmack_invalid_far, sizeof(carmack_invalid_far), decoded, 2))
    {
        fputs("carmack invalid-far self-test failed\n", stderr);
        return 1;
    }
    puts("carmack invalid far ok");

    return 0;
}

static int run_map_plane_decode_self_test(void)
{
    static const uint8_t compressed_plane[] = {
        0x0c, 0x00,
        0x08, 0x00,
        0x01, 0x00,
        0x02, 0x00,
        0xcd, 0xab,
        0x02, 0x00,
        0x03, 0x00};
    static const uint8_t odd_carmack_size_plane[] = {
        0x05, 0x00,
        0x08, 0x00,
        0x01, 0x00,
        0x02, 0x00};
    uint16_t decoded[8];
    wolf_map_plane_load_result result;
    char error_buffer[256];

    if (!wolf_decode_map_plane(compressed_plane,
            sizeof(compressed_plane),
            0xabcd,
            decoded,
            (sizeof(decoded) / sizeof(decoded[0])),
            &result,
            error_buffer,
            sizeof(error_buffer))
        || result.compressed_bytes != sizeof(compressed_plane)
        || result.carmack_expanded_bytes != 12
        || result.rlew_expanded_bytes != 8
        || result.decoded_words != 4
        || decoded[0] != 1
        || decoded[1] != 2
        || decoded[2] != 3
        || decoded[3] != 3)
    {
        fputs("map plane decode self-test failed\n", stderr);
        return 1;
    }
    printf("map plane decode ok: compressed=%u carmack=%u rlew=%u words=%zu values=%u,%u,%u,%u\n",
        result.compressed_bytes,
        result.carmack_expanded_bytes,
        result.rlew_expanded_bytes,
        result.decoded_words,
        decoded[0],
        decoded[1],
        decoded[2],
        decoded[3]);

    if (wolf_decode_map_plane(compressed_plane,
            sizeof(compressed_plane),
            0xabcd,
            decoded,
            3,
            &result,
            error_buffer,
            sizeof(error_buffer))
        || strcmp(error_buffer, "destination buffer is too small for map plane") != 0)
    {
        fputs("map plane short-dest self-test failed\n", stderr);
        return 1;
    }
    puts("map plane decode short dest ok");

    if (wolf_decode_map_plane(odd_carmack_size_plane,
            sizeof(odd_carmack_size_plane),
            0xabcd,
            decoded,
            (sizeof(decoded) / sizeof(decoded[0])),
            &result,
            error_buffer,
            sizeof(error_buffer))
        || strcmp(error_buffer, "carmack-expanded size must be even") != 0)
    {
        fputs("map plane odd-carmack-size self-test failed\n", stderr);
        return 1;
    }
    puts("map plane decode odd carmack size ok");

    return 0;
}

static int run_map_plane_header_bytes_self_test(void)
{
    static const uint8_t valid_plane[] = {
        0x0c, 0x00,
        0x08, 0x00,
        0x01, 0x00,
        0x02, 0x00,
        0xcd, 0xab,
        0x02, 0x00,
        0x03, 0x00};
    static const uint8_t odd_carmack_plane[] = {
        0x05, 0x00,
        0x08, 0x00,
        0x01, 0x00,
        0x02, 0x00};
    static const uint8_t empty_carmack_plane[] = {
        0x00, 0x00,
        0x01, 0x00};
    static const uint8_t odd_rlew_plane[] = {
        0x06, 0x00,
        0x09, 0x00,
        0x01, 0x00,
        0x02, 0x00};
    wolf_map_summary summary;
    wolf_map_plane_header header;
    char error_buffer[256];

    memset(&summary, 0, sizeof(summary));
    summary.plane_offsets[0] = 11;
    summary.plane_offsets[1] = 25;
    summary.plane_offsets[2] = 29;
    summary.plane_lengths[0] = (uint16_t)sizeof(valid_plane);
    summary.plane_lengths[1] = 4;
    summary.plane_lengths[2] = 4;
    summary.width = 2;
    summary.height = 2;
    memcpy(summary.name, "Hdr", sizeof("Hdr"));
    summary.gamemaps_file_size = 64;

    if (!wolf_map_plane_header_from_bytes(&summary,
            0,
            summary.plane_offsets[0],
            valid_plane,
            sizeof(valid_plane),
            &header,
            error_buffer,
            sizeof(error_buffer))
        || header.offset != summary.plane_offsets[0]
        || header.length != sizeof(valid_plane)
        || header.carmack_expanded_bytes != 12
        || header.rlew_expanded_bytes != 8
        || header.decoded_words != 4)
    {
        fputs("map plane header bytes self-test failed\n", stderr);
        return 1;
    }
    printf("map plane header bytes ok: length=%u carmack=%u rlew=%u words=%zu\n",
        header.length,
        header.carmack_expanded_bytes,
        header.rlew_expanded_bytes,
        header.decoded_words);

    summary.plane_lengths[0] = (uint16_t)sizeof(odd_carmack_plane);
    if (wolf_map_plane_header_from_bytes(&summary,
            0,
            summary.plane_offsets[0],
            odd_carmack_plane,
            sizeof(odd_carmack_plane),
            &header,
            error_buffer,
            sizeof(error_buffer))
        || strcmp(error_buffer, "carmack-expanded size must be even") != 0)
    {
        fputs("map plane header odd-carmack self-test failed\n", stderr);
        return 1;
    }
    puts("map plane header odd carmack ok");

    summary.plane_lengths[0] = (uint16_t)sizeof(empty_carmack_plane);
    if (wolf_map_plane_header_from_bytes(&summary,
            0,
            summary.plane_offsets[0],
            empty_carmack_plane,
            sizeof(empty_carmack_plane),
            &header,
            error_buffer,
            sizeof(error_buffer))
        || strcmp(error_buffer, "carmack-expanded plane is empty") != 0)
    {
        fputs("map plane header empty-carmack self-test failed\n", stderr);
        return 1;
    }
    puts("map plane header empty carmack ok");

    summary.plane_lengths[0] = (uint16_t)sizeof(odd_rlew_plane);
    if (wolf_map_plane_header_from_bytes(&summary,
            0,
            summary.plane_offsets[0],
            odd_rlew_plane,
            sizeof(odd_rlew_plane),
            &header,
            error_buffer,
            sizeof(error_buffer))
        || strcmp(error_buffer, "RLEW-expanded size must be even") != 0)
    {
        fputs("map plane header odd-rlew self-test failed\n", stderr);
        return 1;
    }
    puts("map plane header odd rlew ok");

    summary.plane_lengths[0] = (uint16_t)sizeof(valid_plane);
    if (wolf_map_plane_header_from_bytes(&summary,
            0,
            summary.plane_offsets[0],
            valid_plane,
            sizeof(valid_plane) - 2,
            &header,
            error_buffer,
            sizeof(error_buffer))
        || strcmp(error_buffer, "map plane size does not match header summary") != 0)
    {
        fputs("map plane header size-mismatch self-test failed\n", stderr);
        return 1;
    }
    puts("map plane header size mismatch ok");

    return 0;
}

static int run_map_helper_self_test(void)
{
    wolf_loaded_map map;
    const wolf_map_plane_load_result *plane_result = NULL;
    const uint16_t *plane_words = NULL;
    const uint16_t *row_words = NULL;
    size_t word_count = 0;
    size_t row_length = 0;
    size_t index = 0;
    uint16_t column_words[64];
    size_t column_length = 0;
    uint16_t region_words[12];
    size_t region_word_count = 0;
    uint16_t cell_value = 0;
    size_t i;

    memset(&map, 0, sizeof(map));
    map.summary.width = 4;
    map.summary.height = 3;
    memcpy(map.summary.name, "SelfTest", sizeof("SelfTest"));

    for (i = 0; i < 12; ++i)
    {
        map.plane_words[1][i] = (uint16_t)(100 + i);
    }

    if (!wolf_map_cell_index(&map.summary, 3, 2, &index) || index != 11)
    {
        fputs("map helper index self-test failed\n", stderr);
        return 1;
    }
    printf("map helper index ok: %zu\n", index);

    if (!wolf_map_get_plane_result(&map, 1, &plane_result)
        || plane_result->compressed_bytes != 0
        || plane_result->decoded_words != 0)
    {
        fputs("map helper plane-result self-test failed\n", stderr);
        return 1;
    }
    printf("map helper plane result ok: compressed=%u words=%zu\n", plane_result->compressed_bytes, plane_result->decoded_words);

    if (!wolf_map_get_plane_words(&map, 1, &plane_words, &word_count)
        || word_count != 12
        || plane_words[0] != 100
        || plane_words[11] != 111)
    {
        fputs("map helper plane self-test failed\n", stderr);
        return 1;
    }
    printf("map helper plane ok: count=%zu first=%u last=%u\n", word_count, plane_words[0], plane_words[11]);

    if (!wolf_map_get_row(&map, 1, 1, &row_words, &row_length)
        || row_length != 4
        || row_words[0] != 104
        || row_words[3] != 107)
    {
        fputs("map helper row self-test failed\n", stderr);
        return 1;
    }
    printf("map helper row ok: length=%zu left=%u right=%u\n", row_length, row_words[0], row_words[3]);

    if (!wolf_map_get_column(&map, 1, 2, column_words, (sizeof(column_words) / sizeof(column_words[0])), &column_length)
        || column_length != 3
        || column_words[0] != 102
        || column_words[2] != 110)
    {
        fputs("map helper column self-test failed\n", stderr);
        return 1;
    }
    printf("map helper column ok: count=%zu top=%u bottom=%u\n", column_length, column_words[0], column_words[2]);

    if (!wolf_map_get_region(&map, 1, 1, 0, 2, 2, region_words, (sizeof(region_words) / sizeof(region_words[0])), &region_word_count)
        || region_word_count != 4
        || region_words[0] != 101
        || region_words[3] != 106)
    {
        fputs("map helper region self-test failed\n", stderr);
        return 1;
    }
    printf("map helper region ok: count=%zu top-left=%u bottom-right=%u\n", region_word_count, region_words[0], region_words[3]);

    if (!wolf_map_get_cell(&map, 1, 3, 1, &cell_value) || cell_value != 107)
    {
        fputs("map helper cell self-test failed\n", stderr);
        return 1;
    }
    printf("map helper cell ok: %u\n", cell_value);

    if (wolf_map_cell_index(&map.summary, 4, 0, &index))
    {
        fputs("map helper out-of-bounds index self-test failed\n", stderr);
        return 1;
    }
    puts("map helper oob index ok");

    if (wolf_map_get_row(&map, 1, 3, &row_words, &row_length))
    {
        fputs("map helper out-of-bounds row self-test failed\n", stderr);
        return 1;
    }
    puts("map helper oob row ok");

    if (wolf_map_get_cell(&map, 1, 0, 3, &cell_value))
    {
        fputs("map helper out-of-bounds cell self-test failed\n", stderr);
        return 1;
    }
    puts("map helper oob cell ok");

    if (wolf_map_get_plane_words(&map, 3, &plane_words, &word_count))
    {
        fputs("map helper invalid-plane self-test failed\n", stderr);
        return 1;
    }
    puts("map helper invalid plane ok");

    if (wolf_map_get_plane_result(&map, 3, &plane_result))
    {
        fputs("map helper invalid-plane-result self-test failed\n", stderr);
        return 1;
    }
    puts("map helper invalid plane result ok");

    if (wolf_map_get_region(&map, 1, 3, 2, 2, 2, region_words, (sizeof(region_words) / sizeof(region_words[0])), &region_word_count))
    {
        fputs("map helper invalid-region self-test failed\n", stderr);
        return 1;
    }
    puts("map helper invalid region ok");

    return 0;
}

static int run_present_map_helper_self_test(void)
{
    wolf_loaded_present_map present_map;
    const wolf_map_plane_load_result *plane_result = NULL;
    const uint16_t *plane_words = NULL;
    const uint16_t *row_words = NULL;
    size_t slot_index = 0;
    size_t word_count = 0;
    size_t row_length = 0;
    uint16_t column_words[64];
    size_t column_length = 0;
    uint16_t region_words[12];
    size_t region_word_count = 0;
    uint16_t cell_value = 0;
    size_t i;

    memset(&present_map, 0, sizeof(present_map));
    present_map.slot_index = 7;
    present_map.map.summary.width = 3;
    present_map.map.summary.height = 2;
    memcpy(present_map.map.summary.name, "Present", sizeof("Present"));
    present_map.map.plane_results[0].compressed_bytes = 12;
    present_map.map.plane_results[0].decoded_words = 6;

    for (i = 0; i < 6; ++i)
    {
        present_map.map.plane_words[0][i] = (uint16_t)(200 + i);
    }

    if (!wolf_present_map_get_slot_index(&present_map, &slot_index) || slot_index != 7)
    {
        fputs("present map helper slot self-test failed\n", stderr);
        return 1;
    }
    printf("present map helper slot ok: %zu\n", slot_index);

    if (!wolf_present_map_get_plane_result(&present_map, 0, &plane_result)
        || plane_result->compressed_bytes != 12
        || plane_result->decoded_words != 6)
    {
        fputs("present map helper plane-result self-test failed\n", stderr);
        return 1;
    }
    printf("present map helper plane result ok: compressed=%u words=%zu\n", plane_result->compressed_bytes, plane_result->decoded_words);

    if (!wolf_present_map_get_plane_words(&present_map, 0, &plane_words, &word_count)
        || word_count != 6
        || plane_words[0] != 200
        || plane_words[5] != 205)
    {
        fputs("present map helper plane self-test failed\n", stderr);
        return 1;
    }
    printf("present map helper plane ok: count=%zu first=%u last=%u\n", word_count, plane_words[0], plane_words[5]);

    if (!wolf_present_map_get_row(&present_map, 0, 1, &row_words, &row_length)
        || row_length != 3
        || row_words[0] != 203
        || row_words[2] != 205)
    {
        fputs("present map helper row self-test failed\n", stderr);
        return 1;
    }
    printf("present map helper row ok: length=%zu left=%u right=%u\n", row_length, row_words[0], row_words[2]);

    if (!wolf_present_map_get_column(&present_map, 0, 1, column_words, (sizeof(column_words) / sizeof(column_words[0])), &column_length)
        || column_length != 2
        || column_words[0] != 201
        || column_words[1] != 204)
    {
        fputs("present map helper column self-test failed\n", stderr);
        return 1;
    }
    printf("present map helper column ok: count=%zu top=%u bottom=%u\n", column_length, column_words[0], column_words[1]);

    if (!wolf_present_map_get_region(&present_map, 0, 0, 0, 2, 2, region_words, (sizeof(region_words) / sizeof(region_words[0])), &region_word_count)
        || region_word_count != 4
        || region_words[0] != 200
        || region_words[3] != 204)
    {
        fputs("present map helper region self-test failed\n", stderr);
        return 1;
    }
    printf("present map helper region ok: count=%zu top-left=%u bottom-right=%u\n", region_word_count, region_words[0], region_words[3]);

    if (!wolf_present_map_get_cell(&present_map, 0, 2, 0, &cell_value) || cell_value != 202)
    {
        fputs("present map helper cell self-test failed\n", stderr);
        return 1;
    }
    printf("present map helper cell ok: %u\n", cell_value);

    if (wolf_present_map_get_plane_words(&present_map, 3, &plane_words, &word_count))
    {
        fputs("present map helper invalid-plane self-test failed\n", stderr);
        return 1;
    }
    puts("present map helper invalid plane ok");

    if (wolf_present_map_get_cell(&present_map, 0, 3, 0, &cell_value))
    {
        fputs("present map helper invalid-cell self-test failed\n", stderr);
        return 1;
    }
    puts("present map helper invalid cell ok");

    return 0;
}

static void print_loaded_map_summary(const char *label_prefix, const wolf_loaded_map *map)
{
    const wolf_map_plane_load_result *plane_result0 = NULL;
    const wolf_map_plane_load_result *plane_result1 = NULL;
    const wolf_map_plane_load_result *plane_result2 = NULL;

    if (label_prefix == NULL || map == NULL)
    {
        return;
    }

    if (!wolf_map_get_plane_result(map, 0, &plane_result0)
        || !wolf_map_get_plane_result(map, 1, &plane_result1)
        || !wolf_map_get_plane_result(map, 2, &plane_result2))
    {
        return;
    }

    printf("%s load name: %s\n", label_prefix, map->summary.name);
    printf("%s load size: %ux%u\n", label_prefix, map->summary.width, map->summary.height);
    printf("%s plane0 result: compressed=%u carmack=%u rlew=%u words=%zu\n",
        label_prefix,
        plane_result0->compressed_bytes,
        plane_result0->carmack_expanded_bytes,
        plane_result0->rlew_expanded_bytes,
        plane_result0->decoded_words);
    printf("%s plane1 result: compressed=%u carmack=%u rlew=%u words=%zu\n",
        label_prefix,
        plane_result1->compressed_bytes,
        plane_result1->carmack_expanded_bytes,
        plane_result1->rlew_expanded_bytes,
        plane_result1->decoded_words);
    printf("%s plane2 result: compressed=%u carmack=%u rlew=%u words=%zu\n",
        label_prefix,
        plane_result2->compressed_bytes,
        plane_result2->carmack_expanded_bytes,
        plane_result2->rlew_expanded_bytes,
        plane_result2->decoded_words);
    printf("%s plane0 sample cells: [0,0]=%u [31,31]=%u [32,32]=%u [63,63]=%u\n",
        label_prefix,
        map->plane_words[0][0],
        map->plane_words[0][(31 * 64) + 31],
        map->plane_words[0][(32 * 64) + 32],
        map->plane_words[0][(63 * 64) + 63]);
    printf("%s plane1 sample cells: [0,0]=%u [31,31]=%u [32,32]=%u [63,63]=%u\n",
        label_prefix,
        map->plane_words[1][0],
        map->plane_words[1][(31 * 64) + 31],
        map->plane_words[1][(32 * 64) + 32],
        map->plane_words[1][(63 * 64) + 63]);
    printf("%s plane2 sample cells: [0,0]=%u [31,31]=%u [32,32]=%u [63,63]=%u\n",
        label_prefix,
        map->plane_words[2][0],
        map->plane_words[2][(31 * 64) + 31],
        map->plane_words[2][(32 * 64) + 32],
        map->plane_words[2][(63 * 64) + 63]);
}

static int run_map_validation_self_test(void)
{
    wolf_map_summary valid_summary;
    wolf_map_plane_header valid_header;
    wolf_map_plane_header plane_headers[3];
    wolf_map_plane_load_result load_result;

    memset(&valid_summary, 0, sizeof(valid_summary));
    valid_summary.plane_offsets[0] = 11;
    valid_summary.plane_offsets[1] = 1445;
    valid_summary.plane_offsets[2] = 2240;
    valid_summary.plane_lengths[0] = 1434;
    valid_summary.plane_lengths[1] = 795;
    valid_summary.plane_lengths[2] = 10;
    valid_summary.width = 64;
    valid_summary.height = 64;
    memcpy(valid_summary.name, "Wolf1 Map1", sizeof("Wolf1 Map1"));

    memset(&valid_header, 0, sizeof(valid_header));
    valid_header.offset = valid_summary.plane_offsets[0];
    valid_header.length = valid_summary.plane_lengths[0];
    valid_header.carmack_expanded_bytes = 3190;
    valid_header.rlew_expanded_bytes = 8192;
    valid_header.decoded_words = 4096;

    valid_summary.gamemaps_file_size = 4096;

    if (!wolf_map_header_is_valid(&valid_summary))
    {
        fputs("map header valid self-test failed\n", stderr);
        return 1;
    }
    puts("map header valid ok");

    valid_summary.plane_lengths[2] = 0;
    if (wolf_map_header_is_valid(&valid_summary))
    {
        fputs("map header missing-plane self-test failed\n", stderr);
        return 1;
    }
    puts("map header missing plane ok");
    valid_summary.plane_lengths[2] = 10;

    if (!wolf_map_dimensions_are_supported(&valid_summary))
    {
        fputs("map dimensions supported self-test failed\n", stderr);
        return 1;
    }
    puts("map dimensions supported ok");

    valid_summary.width = 65;
    if (wolf_map_dimensions_are_supported(&valid_summary))
    {
        fputs("map dimensions oversized self-test failed\n", stderr);
        return 1;
    }
    puts("map dimensions oversized ok");
    valid_summary.width = 64;

    if (!wolf_map_plane_header_is_valid_for_map(&valid_summary, &valid_header))
    {
        fputs("map plane header valid self-test failed\n", stderr);
        return 1;
    }
    puts("map plane header valid ok");

    if (!wolf_map_plane_header_matches_summary(&valid_summary, 0, &valid_header))
    {
        fputs("map plane header summary-match self-test failed\n", stderr);
        return 1;
    }
    puts("map plane header summary match ok");

    if (!wolf_map_plane_header_is_in_bounds(&valid_summary, 0, &valid_header))
    {
        fputs("map plane header bounds self-test failed\n", stderr);
        return 1;
    }
    puts("map plane header bounds ok");

    load_result.compressed_bytes = valid_header.length;
    load_result.carmack_expanded_bytes = valid_header.carmack_expanded_bytes;
    load_result.rlew_expanded_bytes = valid_header.rlew_expanded_bytes;
    load_result.decoded_words = valid_header.decoded_words;
    if (!wolf_map_plane_load_result_matches_header(&valid_header, &load_result))
    {
        fputs("map plane load-result-match self-test failed\n", stderr);
        return 1;
    }
    puts("map plane load result match ok");

    load_result.decoded_words -= 1;
    if (wolf_map_plane_load_result_matches_header(&valid_header, &load_result))
    {
        fputs("map plane load-result-mismatch self-test failed\n", stderr);
        return 1;
    }
    puts("map plane load result mismatch ok");
    load_result.decoded_words = valid_header.decoded_words;

    plane_headers[0] = valid_header;
    plane_headers[1].offset = valid_summary.plane_offsets[1];
    plane_headers[1].length = valid_summary.plane_lengths[1];
    plane_headers[1].carmack_expanded_bytes = 930;
    plane_headers[1].rlew_expanded_bytes = 8192;
    plane_headers[1].decoded_words = 4096;
    plane_headers[2].offset = valid_summary.plane_offsets[2];
    plane_headers[2].length = valid_summary.plane_lengths[2];
    plane_headers[2].carmack_expanded_bytes = 12;
    plane_headers[2].rlew_expanded_bytes = 8192;
    plane_headers[2].decoded_words = 4096;

    if (!wolf_map_plane_headers_are_valid(&valid_summary, plane_headers))
    {
        fputs("map plane table valid self-test failed\n", stderr);
        return 1;
    }
    puts("map plane table valid ok");

    plane_headers[2].length = (uint16_t)(valid_summary.plane_lengths[2] + 1);
    if (wolf_map_plane_headers_are_valid(&valid_summary, plane_headers))
    {
        fputs("map plane table invalid self-test failed\n", stderr);
        return 1;
    }
    puts("map plane table invalid ok");
    plane_headers[2].length = valid_summary.plane_lengths[2];

    valid_header.offset = valid_summary.plane_offsets[1];
    if (wolf_map_plane_header_matches_summary(&valid_summary, 0, &valid_header))
    {
        fputs("map plane header offset-mismatch self-test failed\n", stderr);
        return 1;
    }
    puts("map plane header offset mismatch ok");
    valid_header.offset = valid_summary.plane_offsets[0];

    valid_header.length = (uint16_t)(valid_summary.plane_offsets[1] - valid_summary.plane_offsets[0] + 1);
    if (wolf_map_plane_header_is_in_bounds(&valid_summary, 0, &valid_header))
    {
        fputs("map plane header out-of-bounds self-test failed\n", stderr);
        return 1;
    }
    puts("map plane header out of bounds ok");
    valid_header.length = valid_summary.plane_lengths[0];

    valid_header.rlew_expanded_bytes = 8191;
    if (wolf_map_plane_header_is_valid_for_map(&valid_summary, &valid_header))
    {
        fputs("map plane header odd-size self-test failed\n", stderr);
        return 1;
    }
    puts("map plane header odd size ok");
    valid_header.rlew_expanded_bytes = 8192;

    valid_header.carmack_expanded_bytes = 0;
    if (wolf_map_plane_header_is_valid_for_map(&valid_summary, &valid_header))
    {
        fputs("map plane header missing-carmack-size self-test failed\n", stderr);
        return 1;
    }
    puts("map plane header missing Carmack size ok");
    valid_header.carmack_expanded_bytes = 3190;

    valid_header.decoded_words = 4095;
    if (wolf_map_plane_header_is_valid_for_map(&valid_summary, &valid_header))
    {
        fputs("map plane header wrong-decoded-size self-test failed\n", stderr);
        return 1;
    }
    puts("map plane header wrong decoded size ok");
    valid_header.decoded_words = 4096;

    valid_header.rlew_expanded_bytes = 8190;
    if (wolf_map_plane_header_is_valid_for_map(&valid_summary, &valid_header))
    {
        fputs("map plane header wrong-expanded-size self-test failed\n", stderr);
        return 1;
    }
    puts("map plane header wrong expanded size ok");
    valid_header.rlew_expanded_bytes = 8192;

    valid_summary.gamemaps_file_size = 4096;
    valid_summary.plane_offsets[0] = 11;
    valid_summary.plane_offsets[1] = 1445;
    valid_summary.plane_offsets[2] = 2240;
    valid_summary.plane_lengths[0] = 1434;
    valid_summary.plane_lengths[1] = 795;
    valid_summary.plane_lengths[2] = 10;
    if (!wolf_map_planes_are_in_bounds(&valid_summary))
    {
        fputs("map plane bounds valid self-test failed\n", stderr);
        return 1;
    }

    valid_summary.plane_lengths[2] = 0;
    if (wolf_map_planes_are_in_bounds(&valid_summary))
    {
        fputs("map plane bounds invalid-summary self-test failed\n", stderr);
        return 1;
    }
    puts("map plane bounds reject invalid summary ok");
    valid_summary.plane_lengths[2] = 10;

    valid_summary.gamemaps_file_size = 0xffffffffu;
    valid_summary.plane_offsets[0] = 0xfffffff0u;
    valid_summary.plane_lengths[0] = 0x0020u;
    valid_summary.plane_offsets[1] = 0xfffffffeu;
    if (wolf_map_planes_are_in_bounds(&valid_summary))
    {
        fputs("map plane bounds overflow self-test failed\n", stderr);
        return 1;
    }
    puts("map plane bounds overflow ok");

    return 0;
}

int main(int argc, char **argv)
{
    int i;
    const char *data_path = wolf_default_data_dir();
    int check_data = 0;
    int inspect_maphead = 0;
    int inspect_first_map = 0;
    int inspect_map = 0;
    size_t inspect_map_index = 0;
    int inspect_map_slot = 0;
    size_t inspect_map_slot_index = 0;
    int inspect_present_index_for_slot = 0;
    size_t inspect_present_index_for_slot_slot_index = 0;
    int inspect_map_presence_summary = 0;
    int inspect_map_catalog = 0;
    size_t inspect_map_catalog_count = 0;
    int inspect_present_map_catalog = 0;
    size_t inspect_present_map_catalog_count = 0;
    int inspect_present_map_load = 0;
    size_t inspect_present_map_load_index = 0;
    int inspect_present_map_cell = 0;
    size_t inspect_present_map_cell_index = 0;
    size_t inspect_present_map_cell_x = 0;
    size_t inspect_present_map_cell_y = 0;
    int inspect_present_map_row = 0;
    size_t inspect_present_map_row_index = 0;
    size_t inspect_present_map_row_plane_index = 0;
    size_t inspect_present_map_row_y = 0;
    int inspect_present_map_column = 0;
    size_t inspect_present_map_column_index = 0;
    size_t inspect_present_map_column_plane_index = 0;
    size_t inspect_present_map_column_x = 0;
    int inspect_present_map_region = 0;
    size_t inspect_present_map_region_index = 0;
    size_t inspect_present_map_region_plane_index = 0;
    size_t inspect_present_map_region_x = 0;
    size_t inspect_present_map_region_y = 0;
    size_t inspect_present_map_region_width = 0;
    size_t inspect_present_map_region_height = 0;
    int inspect_present_map_load_catalog = 0;
    size_t inspect_present_map_load_catalog_count = 0;
    int inspect_map_load_catalog = 0;
    size_t inspect_map_load_catalog_count = 0;
    int validate_map_catalog = 0;
    size_t validate_map_catalog_count = 0;
    int validate_map_plane_table_catalog = 0;
    size_t validate_map_plane_table_catalog_count = 0;
    int validate_present_map_catalog = 0;
    size_t validate_present_map_catalog_count = 0;
    int validate_present_map_load = 0;
    size_t validate_present_map_load_index = 0;
    int validate_present_map_load_catalog = 0;
    size_t validate_present_map_load_catalog_count = 0;
    int validate_map_header = 0;
    size_t validate_map_header_index = 0;
    int validate_map_load = 0;
    size_t validate_map_load_index = 0;
    int validate_map_plane_header = 0;
    size_t validate_map_plane_header_map_index = 0;
    size_t validate_map_plane_header_index = 0;
    int validate_map_plane_table = 0;
    size_t validate_map_plane_table_index = 0;
    int validate_first_map_planes = 0;
    int self_test_rlew = 0;
    int self_test_carmack = 0;
    int self_test_decompression_failures = 0;
    int self_test_map_plane_decode = 0;
    int self_test_map_plane_header_bytes = 0;
    int self_test_map_helpers = 0;
    int self_test_present_map_helpers = 0;
    int self_test_map_validation = 0;
    int inspect_first_map_plane = 0;
    size_t inspect_first_map_plane_index = 0;
    int inspect_map_plane = 0;
    size_t inspect_map_plane_map_index = 0;
    size_t inspect_map_plane_index = 0;
    int inspect_map_plane_header = 0;
    size_t inspect_map_plane_header_map_index = 0;
    size_t inspect_map_plane_header_index = 0;
    int inspect_map_plane_table = 0;
    size_t inspect_map_plane_table_index = 0;
    int inspect_map_plane_table_catalog = 0;
    size_t inspect_map_plane_table_catalog_count = 0;
    int inspect_present_map_plane_table = 0;
    size_t inspect_present_map_plane_table_index = 0;
    int inspect_present_map_plane_table_catalog = 0;
    size_t inspect_present_map_plane_table_catalog_count = 0;
    int inspect_map_overview = 0;
    size_t inspect_map_overview_index = 0;
    int inspect_first_map_load = 0;
    int inspect_map_load = 0;
    size_t inspect_map_load_index = 0;
    int inspect_map_cell = 0;
    size_t inspect_map_cell_map_index = 0;
    size_t inspect_map_cell_x = 0;
    size_t inspect_map_cell_y = 0;
    int inspect_map_row = 0;
    size_t inspect_map_row_map_index = 0;
    size_t inspect_map_row_plane_index = 0;
    size_t inspect_map_row_y = 0;
    int inspect_map_column = 0;
    size_t inspect_map_column_map_index = 0;
    size_t inspect_map_column_plane_index = 0;
    size_t inspect_map_column_x = 0;
    int inspect_map_region = 0;
    size_t inspect_map_region_map_index = 0;
    size_t inspect_map_region_plane_index = 0;
    size_t inspect_map_region_x = 0;
    size_t inspect_map_region_y = 0;
    size_t inspect_map_region_width = 0;
    size_t inspect_map_region_height = 0;
    char error_buffer[256];
    wolf_maphead_summary maphead_summary;
    wolf_map_presence_summary map_presence_summary;
    wolf_map_summary map_summary;
    wolf_map_plane_header plane_header;
    wolf_map_plane_header plane_headers[3];
    wolf_map_plane_table plane_table_catalog[100];
    wolf_present_map_plane_table present_plane_table_catalog[100];
    wolf_map_plane_load_result plane_load_result;
    wolf_loaded_map loaded_map;
    uint16_t plane_words[64 * 64];
    uint16_t region_words[64 * 64];

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

        if (strcmp(argv[i], "--inspect-map") == 0)
        {
            char *end = NULL;
            long parsed_index;
            if ((i + 1) >= argc)
            {
                fputs("--inspect-map requires an index\n", stderr);
                return 1;
            }

            parsed_index = strtol(argv[++i], &end, 10);
            if (end == argv[i] || *end != '\0' || parsed_index < 0)
            {
                fputs("--inspect-map index must be a non-negative integer\n", stderr);
                return 1;
            }

            inspect_map = 1;
            inspect_map_index = (size_t)parsed_index;
            continue;
        }

        if (strcmp(argv[i], "--inspect-map-slot") == 0)
        {
            char *end = NULL;
            long parsed_index;
            if ((i + 1) >= argc)
            {
                fputs("--inspect-map-slot requires an index\n", stderr);
                return 1;
            }

            parsed_index = strtol(argv[++i], &end, 10);
            if (end == argv[i] || *end != '\0' || parsed_index < 0)
            {
                fputs("--inspect-map-slot index must be a non-negative integer\n", stderr);
                return 1;
            }

            inspect_map_slot = 1;
            inspect_map_slot_index = (size_t)parsed_index;
            continue;
        }

        if (strcmp(argv[i], "--inspect-present-index-for-slot") == 0)
        {
            char *end = NULL;
            long parsed_index;
            if ((i + 1) >= argc)
            {
                fputs("--inspect-present-index-for-slot requires a slot index\n", stderr);
                return 1;
            }

            errno = 0;
            parsed_index = strtol(argv[++i], &end, 10);
            if (errno == ERANGE || end == argv[i] || *end != '\0' || parsed_index < 0)
            {
                fputs("--inspect-present-index-for-slot slot index must be a non-negative integer\n", stderr);
                return 1;
            }

            inspect_present_index_for_slot = 1;
            inspect_present_index_for_slot_slot_index = (size_t)parsed_index;
            continue;
        }

        if (strcmp(argv[i], "--inspect-map-presence-summary") == 0)
        {
            inspect_map_presence_summary = 1;
            continue;
        }

        if (strcmp(argv[i], "--inspect-map-catalog") == 0)
        {
            char *end = NULL;
            long parsed_count;
            if ((i + 1) >= argc)
            {
                fputs("--inspect-map-catalog requires a count\n", stderr);
                return 1;
            }

            parsed_count = strtol(argv[++i], &end, 10);
            if (end == argv[i] || *end != '\0' || parsed_count < 0)
            {
                fputs("--inspect-map-catalog count must be a non-negative integer\n", stderr);
                return 1;
            }

            inspect_map_catalog = 1;
            inspect_map_catalog_count = (size_t)parsed_count;
            continue;
        }

        if (strcmp(argv[i], "--inspect-present-map-catalog") == 0)
        {
            char *end = NULL;
            long parsed_count;
            if ((i + 1) >= argc)
            {
                fputs("--inspect-present-map-catalog requires a count\n", stderr);
                return 1;
            }

            parsed_count = strtol(argv[++i], &end, 10);
            if (end == argv[i] || *end != '\0' || parsed_count < 0)
            {
                fputs("--inspect-present-map-catalog count must be a non-negative integer\n", stderr);
                return 1;
            }

            inspect_present_map_catalog = 1;
            inspect_present_map_catalog_count = (size_t)parsed_count;
            continue;
        }

        if (strcmp(argv[i], "--inspect-present-map-load") == 0)
        {
            char *end = NULL;
            long parsed_index;
            if ((i + 1) >= argc)
            {
                fputs("--inspect-present-map-load requires an index\n", stderr);
                return 1;
            }

            parsed_index = strtol(argv[++i], &end, 10);
            if (end == argv[i] || *end != '\0' || parsed_index < 0)
            {
                fputs("--inspect-present-map-load index must be a non-negative integer\n", stderr);
                return 1;
            }

            inspect_present_map_load = 1;
            inspect_present_map_load_index = (size_t)parsed_index;
            continue;
        }

        if (strcmp(argv[i], "--inspect-present-map-cell") == 0)
        {
            char *map_end = NULL;
            char *x_end = NULL;
            char *y_end = NULL;
            long parsed_map_index;
            long parsed_x;
            long parsed_y;
            if ((i + 3) >= argc)
            {
                fputs("--inspect-present-map-cell requires a present-map index, x, and y\n", stderr);
                return 1;
            }

            parsed_map_index = strtol(argv[++i], &map_end, 10);
            parsed_x = strtol(argv[++i], &x_end, 10);
            parsed_y = strtol(argv[++i], &y_end, 10);
            if (map_end == argv[i - 2] || *map_end != '\0' || parsed_map_index < 0)
            {
                fputs("--inspect-present-map-cell index must be a non-negative integer\n", stderr);
                return 1;
            }
            if (x_end == argv[i - 1] || *x_end != '\0' || parsed_x < 0)
            {
                fputs("--inspect-present-map-cell x must be a non-negative integer\n", stderr);
                return 1;
            }
            if (y_end == argv[i] || *y_end != '\0' || parsed_y < 0)
            {
                fputs("--inspect-present-map-cell y must be a non-negative integer\n", stderr);
                return 1;
            }

            inspect_present_map_cell = 1;
            inspect_present_map_cell_index = (size_t)parsed_map_index;
            inspect_present_map_cell_x = (size_t)parsed_x;
            inspect_present_map_cell_y = (size_t)parsed_y;
            continue;
        }

        if (strcmp(argv[i], "--inspect-present-map-row") == 0)
        {
            char *map_end = NULL;
            char *plane_end = NULL;
            char *y_end = NULL;
            long parsed_map_index;
            long parsed_plane_index;
            long parsed_y;
            if ((i + 3) >= argc)
            {
                fputs("--inspect-present-map-row requires a present-map index, plane index, and y\n", stderr);
                return 1;
            }

            parsed_map_index = strtol(argv[++i], &map_end, 10);
            parsed_plane_index = strtol(argv[++i], &plane_end, 10);
            parsed_y = strtol(argv[++i], &y_end, 10);
            if (map_end == argv[i - 2] || *map_end != '\0' || parsed_map_index < 0)
            {
                fputs("--inspect-present-map-row index must be a non-negative integer\n", stderr);
                return 1;
            }
            if (plane_end == argv[i - 1] || *plane_end != '\0' || parsed_plane_index < 0 || parsed_plane_index > 2)
            {
                fputs("--inspect-present-map-row plane index must be 0, 1, or 2\n", stderr);
                return 1;
            }
            if (y_end == argv[i] || *y_end != '\0' || parsed_y < 0)
            {
                fputs("--inspect-present-map-row y must be a non-negative integer\n", stderr);
                return 1;
            }

            inspect_present_map_row = 1;
            inspect_present_map_row_index = (size_t)parsed_map_index;
            inspect_present_map_row_plane_index = (size_t)parsed_plane_index;
            inspect_present_map_row_y = (size_t)parsed_y;
            continue;
        }

        if (strcmp(argv[i], "--inspect-present-map-column") == 0)
        {
            char *map_end = NULL;
            char *plane_end = NULL;
            char *x_end = NULL;
            long parsed_map_index;
            long parsed_plane_index;
            long parsed_x;
            if ((i + 3) >= argc)
            {
                fputs("--inspect-present-map-column requires a present-map index, plane index, and x\n", stderr);
                return 1;
            }

            parsed_map_index = strtol(argv[++i], &map_end, 10);
            parsed_plane_index = strtol(argv[++i], &plane_end, 10);
            parsed_x = strtol(argv[++i], &x_end, 10);
            if (map_end == argv[i - 2] || *map_end != '\0' || parsed_map_index < 0)
            {
                fputs("--inspect-present-map-column index must be a non-negative integer\n", stderr);
                return 1;
            }
            if (plane_end == argv[i - 1] || *plane_end != '\0' || parsed_plane_index < 0 || parsed_plane_index > 2)
            {
                fputs("--inspect-present-map-column plane index must be 0, 1, or 2\n", stderr);
                return 1;
            }
            if (x_end == argv[i] || *x_end != '\0' || parsed_x < 0)
            {
                fputs("--inspect-present-map-column x must be a non-negative integer\n", stderr);
                return 1;
            }

            inspect_present_map_column = 1;
            inspect_present_map_column_index = (size_t)parsed_map_index;
            inspect_present_map_column_plane_index = (size_t)parsed_plane_index;
            inspect_present_map_column_x = (size_t)parsed_x;
            continue;
        }

        if (strcmp(argv[i], "--inspect-present-map-region") == 0)
        {
            char *map_end = NULL;
            char *plane_end = NULL;
            char *x_end = NULL;
            char *y_end = NULL;
            char *width_end = NULL;
            char *height_end = NULL;
            long parsed_map_index;
            long parsed_plane_index;
            long parsed_x;
            long parsed_y;
            long parsed_width;
            long parsed_height;
            if ((i + 6) >= argc)
            {
                fputs("--inspect-present-map-region requires a present-map index, plane index, x, y, width, and height\n", stderr);
                return 1;
            }

            parsed_map_index = strtol(argv[++i], &map_end, 10);
            parsed_plane_index = strtol(argv[++i], &plane_end, 10);
            parsed_x = strtol(argv[++i], &x_end, 10);
            parsed_y = strtol(argv[++i], &y_end, 10);
            parsed_width = strtol(argv[++i], &width_end, 10);
            parsed_height = strtol(argv[++i], &height_end, 10);
            if (map_end == argv[i - 5] || *map_end != '\0' || parsed_map_index < 0)
            {
                fputs("--inspect-present-map-region index must be a non-negative integer\n", stderr);
                return 1;
            }
            if (plane_end == argv[i - 4] || *plane_end != '\0' || parsed_plane_index < 0 || parsed_plane_index > 2)
            {
                fputs("--inspect-present-map-region plane index must be 0, 1, or 2\n", stderr);
                return 1;
            }
            if (x_end == argv[i - 3] || *x_end != '\0' || parsed_x < 0)
            {
                fputs("--inspect-present-map-region x must be a non-negative integer\n", stderr);
                return 1;
            }
            if (y_end == argv[i - 2] || *y_end != '\0' || parsed_y < 0)
            {
                fputs("--inspect-present-map-region y must be a non-negative integer\n", stderr);
                return 1;
            }
            if (width_end == argv[i - 1] || *width_end != '\0' || parsed_width <= 0)
            {
                fputs("--inspect-present-map-region width must be a positive integer\n", stderr);
                return 1;
            }
            if (height_end == argv[i] || *height_end != '\0' || parsed_height <= 0)
            {
                fputs("--inspect-present-map-region height must be a positive integer\n", stderr);
                return 1;
            }

            inspect_present_map_region = 1;
            inspect_present_map_region_index = (size_t)parsed_map_index;
            inspect_present_map_region_plane_index = (size_t)parsed_plane_index;
            inspect_present_map_region_x = (size_t)parsed_x;
            inspect_present_map_region_y = (size_t)parsed_y;
            inspect_present_map_region_width = (size_t)parsed_width;
            inspect_present_map_region_height = (size_t)parsed_height;
            continue;
        }

        if (strcmp(argv[i], "--inspect-present-map-load-catalog") == 0)
        {
            char *end = NULL;
            long parsed_count;
            if ((i + 1) >= argc)
            {
                fputs("--inspect-present-map-load-catalog requires a count\n", stderr);
                return 1;
            }

            parsed_count = strtol(argv[++i], &end, 10);
            if (end == argv[i] || *end != '\0' || parsed_count < 0)
            {
                fputs("--inspect-present-map-load-catalog count must be a non-negative integer\n", stderr);
                return 1;
            }

            inspect_present_map_load_catalog = 1;
            inspect_present_map_load_catalog_count = (size_t)parsed_count;
            continue;
        }

        if (strcmp(argv[i], "--inspect-map-load-catalog") == 0)
        {
            char *end = NULL;
            long parsed_count;
            if ((i + 1) >= argc)
            {
                fputs("--inspect-map-load-catalog requires a count\n", stderr);
                return 1;
            }

            parsed_count = strtol(argv[++i], &end, 10);
            if (end == argv[i] || *end != '\0' || parsed_count < 0)
            {
                fputs("--inspect-map-load-catalog count must be a non-negative integer\n", stderr);
                return 1;
            }

            inspect_map_load_catalog = 1;
            inspect_map_load_catalog_count = (size_t)parsed_count;
            continue;
        }

        if (strcmp(argv[i], "--validate-map-catalog") == 0)
        {
            char *end = NULL;
            long parsed_count;
            if ((i + 1) >= argc)
            {
                fputs("--validate-map-catalog requires a count\n", stderr);
                return 1;
            }

            parsed_count = strtol(argv[++i], &end, 10);
            if (end == argv[i] || *end != '\0' || parsed_count < 0)
            {
                fputs("--validate-map-catalog count must be a non-negative integer\n", stderr);
                return 1;
            }

            validate_map_catalog = 1;
            validate_map_catalog_count = (size_t)parsed_count;
            continue;
        }

        if (strcmp(argv[i], "--validate-map-plane-table-catalog") == 0)
        {
            char *end = NULL;
            long parsed_count;
            if ((i + 1) >= argc)
            {
                fputs("--validate-map-plane-table-catalog requires a count\n", stderr);
                return 1;
            }

            parsed_count = strtol(argv[++i], &end, 10);
            if (end == argv[i] || *end != '\0' || parsed_count < 0)
            {
                fputs("--validate-map-plane-table-catalog count must be a non-negative integer\n", stderr);
                return 1;
            }

            validate_map_plane_table_catalog = 1;
            validate_map_plane_table_catalog_count = (size_t)parsed_count;
            continue;
        }

        if (strcmp(argv[i], "--validate-present-map-catalog") == 0)
        {
            char *end = NULL;
            long parsed_count;
            if ((i + 1) >= argc)
            {
                fputs("--validate-present-map-catalog requires a count\n", stderr);
                return 1;
            }

            parsed_count = strtol(argv[++i], &end, 10);
            if (end == argv[i] || *end != '\0' || parsed_count < 0)
            {
                fputs("--validate-present-map-catalog count must be a non-negative integer\n", stderr);
                return 1;
            }

            validate_present_map_catalog = 1;
            validate_present_map_catalog_count = (size_t)parsed_count;
            continue;
        }

        if (strcmp(argv[i], "--validate-present-map-load-catalog") == 0)
        {
            char *end = NULL;
            long parsed_count;
            if ((i + 1) >= argc)
            {
                fputs("--validate-present-map-load-catalog requires a count\n", stderr);
                return 1;
            }

            parsed_count = strtol(argv[++i], &end, 10);
            if (end == argv[i] || *end != '\0' || parsed_count < 0)
            {
                fputs("--validate-present-map-load-catalog count must be a non-negative integer\n", stderr);
                return 1;
            }

            validate_present_map_load_catalog = 1;
            validate_present_map_load_catalog_count = (size_t)parsed_count;
            continue;
        }

        if (strcmp(argv[i], "--validate-present-map-load") == 0)
        {
            char *end = NULL;
            long parsed_index;
            if ((i + 1) >= argc)
            {
                fputs("--validate-present-map-load requires an index\n", stderr);
                return 1;
            }

            parsed_index = strtol(argv[++i], &end, 10);
            if (end == argv[i] || *end != '\0' || parsed_index < 0)
            {
                fputs("--validate-present-map-load index must be a non-negative integer\n", stderr);
                return 1;
            }

            validate_present_map_load = 1;
            validate_present_map_load_index = (size_t)parsed_index;
            continue;
        }

        if (strcmp(argv[i], "--validate-map-header") == 0)
        {
            char *end = NULL;
            long parsed_index;
            if ((i + 1) >= argc)
            {
                fputs("--validate-map-header requires an index\n", stderr);
                return 1;
            }

            parsed_index = strtol(argv[++i], &end, 10);
            if (end == argv[i] || *end != '\0' || parsed_index < 0)
            {
                fputs("--validate-map-header index must be a non-negative integer\n", stderr);
                return 1;
            }

            validate_map_header = 1;
            validate_map_header_index = (size_t)parsed_index;
            continue;
        }

        if (strcmp(argv[i], "--validate-map-load") == 0)
        {
            char *end = NULL;
            long parsed_index;
            if ((i + 1) >= argc)
            {
                fputs("--validate-map-load requires an index\n", stderr);
                return 1;
            }

            parsed_index = strtol(argv[++i], &end, 10);
            if (end == argv[i] || *end != '\0' || parsed_index < 0)
            {
                fputs("--validate-map-load index must be a non-negative integer\n", stderr);
                return 1;
            }

            validate_map_load = 1;
            validate_map_load_index = (size_t)parsed_index;
            continue;
        }

        if (strcmp(argv[i], "--validate-map-plane-header") == 0)
        {
            char *map_end = NULL;
            char *plane_end = NULL;
            long parsed_map_index;
            long parsed_plane_index;
            if ((i + 2) >= argc)
            {
                fputs("--validate-map-plane-header requires a map index and plane index\n", stderr);
                return 1;
            }

            parsed_map_index = strtol(argv[++i], &map_end, 10);
            parsed_plane_index = strtol(argv[++i], &plane_end, 10);
            if (map_end == argv[i - 1] || *map_end != '\0' || parsed_map_index < 0)
            {
                fputs("--validate-map-plane-header map index must be a non-negative integer\n", stderr);
                return 1;
            }
            if (plane_end == argv[i] || *plane_end != '\0' || parsed_plane_index < 0 || parsed_plane_index > 2)
            {
                fputs("--validate-map-plane-header plane index must be 0, 1, or 2\n", stderr);
                return 1;
            }

            validate_map_plane_header = 1;
            validate_map_plane_header_map_index = (size_t)parsed_map_index;
            validate_map_plane_header_index = (size_t)parsed_plane_index;
            continue;
        }

        if (strcmp(argv[i], "--validate-map-plane-table") == 0)
        {
            char *end = NULL;
            long parsed_index;
            if ((i + 1) >= argc)
            {
                fputs("--validate-map-plane-table requires a map index\n", stderr);
                return 1;
            }

            parsed_index = strtol(argv[++i], &end, 10);
            if (end == argv[i] || *end != '\0' || parsed_index < 0)
            {
                fputs("--validate-map-plane-table index must be a non-negative integer\n", stderr);
                return 1;
            }

            validate_map_plane_table = 1;
            validate_map_plane_table_index = (size_t)parsed_index;
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

        if (strcmp(argv[i], "--self-test-carmack") == 0)
        {
            self_test_carmack = 1;
            continue;
        }

        if (strcmp(argv[i], "--self-test-decompression-failures") == 0)
        {
            self_test_decompression_failures = 1;
            continue;
        }

        if (strcmp(argv[i], "--self-test-map-plane-decode") == 0)
        {
            self_test_map_plane_decode = 1;
            continue;
        }

        if (strcmp(argv[i], "--self-test-map-plane-header-bytes") == 0)
        {
            self_test_map_plane_header_bytes = 1;
            continue;
        }

        if (strcmp(argv[i], "--self-test-map-helpers") == 0)
        {
            self_test_map_helpers = 1;
            continue;
        }

        if (strcmp(argv[i], "--self-test-present-map-helpers") == 0)
        {
            self_test_present_map_helpers = 1;
            continue;
        }

        if (strcmp(argv[i], "--self-test-map-validation") == 0)
        {
            self_test_map_validation = 1;
            continue;
        }

        if (strcmp(argv[i], "--inspect-map-plane") == 0)
        {
            char *map_end = NULL;
            char *plane_end = NULL;
            long parsed_map_index;
            long parsed_plane_index;
            if ((i + 2) >= argc)
            {
                fputs("--inspect-map-plane requires a map index and plane index\n", stderr);
                return 1;
            }

            parsed_map_index = strtol(argv[++i], &map_end, 10);
            parsed_plane_index = strtol(argv[++i], &plane_end, 10);
            if (map_end == argv[i - 1] || *map_end != '\0' || parsed_map_index < 0)
            {
                fputs("--inspect-map-plane map index must be a non-negative integer\n", stderr);
                return 1;
            }
            if (plane_end == argv[i] || *plane_end != '\0' || parsed_plane_index < 0 || parsed_plane_index > 2)
            {
                fputs("--inspect-map-plane plane index must be 0, 1, or 2\n", stderr);
                return 1;
            }

            inspect_map_plane = 1;
            inspect_map_plane_map_index = (size_t)parsed_map_index;
            inspect_map_plane_index = (size_t)parsed_plane_index;
            continue;
        }

        if (strcmp(argv[i], "--inspect-map-plane-header") == 0)
        {
            char *map_end = NULL;
            char *plane_end = NULL;
            long parsed_map_index;
            long parsed_plane_index;
            if ((i + 2) >= argc)
            {
                fputs("--inspect-map-plane-header requires a map index and plane index\n", stderr);
                return 1;
            }

            parsed_map_index = strtol(argv[++i], &map_end, 10);
            parsed_plane_index = strtol(argv[++i], &plane_end, 10);
            if (map_end == argv[i - 1] || *map_end != '\0' || parsed_map_index < 0)
            {
                fputs("--inspect-map-plane-header map index must be a non-negative integer\n", stderr);
                return 1;
            }
            if (plane_end == argv[i] || *plane_end != '\0' || parsed_plane_index < 0 || parsed_plane_index > 2)
            {
                fputs("--inspect-map-plane-header plane index must be 0, 1, or 2\n", stderr);
                return 1;
            }

            inspect_map_plane_header = 1;
            inspect_map_plane_header_map_index = (size_t)parsed_map_index;
            inspect_map_plane_header_index = (size_t)parsed_plane_index;
            continue;
        }

        if (strcmp(argv[i], "--inspect-map-plane-table") == 0)
        {
            char *end = NULL;
            long parsed_index;
            if ((i + 1) >= argc)
            {
                fputs("--inspect-map-plane-table requires a map index\n", stderr);
                return 1;
            }

            parsed_index = strtol(argv[++i], &end, 10);
            if (end == argv[i] || *end != '\0' || parsed_index < 0)
            {
                fputs("--inspect-map-plane-table index must be a non-negative integer\n", stderr);
                return 1;
            }

            inspect_map_plane_table = 1;
            inspect_map_plane_table_index = (size_t)parsed_index;
            continue;
        }

        if (strcmp(argv[i], "--inspect-map-plane-table-catalog") == 0)
        {
            char *end = NULL;
            long parsed_count;
            if ((i + 1) >= argc)
            {
                fputs("--inspect-map-plane-table-catalog requires a count\n", stderr);
                return 1;
            }

            parsed_count = strtol(argv[++i], &end, 10);
            if (end == argv[i] || *end != '\0' || parsed_count < 0)
            {
                fputs("--inspect-map-plane-table-catalog count must be a non-negative integer\n", stderr);
                return 1;
            }

            inspect_map_plane_table_catalog = 1;
            inspect_map_plane_table_catalog_count = (size_t)parsed_count;
            continue;
        }

        if (strcmp(argv[i], "--inspect-present-map-plane-table") == 0)
        {
            char *end = NULL;
            long parsed_index;
            if ((i + 1) >= argc)
            {
                fputs("--inspect-present-map-plane-table requires a present-map index\n", stderr);
                return 1;
            }

            parsed_index = strtol(argv[++i], &end, 10);
            if (end == argv[i] || *end != '\0' || parsed_index < 0)
            {
                fputs("--inspect-present-map-plane-table index must be a non-negative integer\n", stderr);
                return 1;
            }

            inspect_present_map_plane_table = 1;
            inspect_present_map_plane_table_index = (size_t)parsed_index;
            continue;
        }

        if (strcmp(argv[i], "--inspect-present-map-plane-table-catalog") == 0)
        {
            char *end = NULL;
            long parsed_count;
            if ((i + 1) >= argc)
            {
                fputs("--inspect-present-map-plane-table-catalog requires a count\n", stderr);
                return 1;
            }

            parsed_count = strtol(argv[++i], &end, 10);
            if (end == argv[i] || *end != '\0' || parsed_count < 0)
            {
                fputs("--inspect-present-map-plane-table-catalog count must be a non-negative integer\n", stderr);
                return 1;
            }

            inspect_present_map_plane_table_catalog = 1;
            inspect_present_map_plane_table_catalog_count = (size_t)parsed_count;
            continue;
        }

        if (strcmp(argv[i], "--inspect-map-overview") == 0)
        {
            char *end = NULL;
            long parsed_index;
            if ((i + 1) >= argc)
            {
                fputs("--inspect-map-overview requires an index\n", stderr);
                return 1;
            }

            parsed_index = strtol(argv[++i], &end, 10);
            if (end == argv[i] || *end != '\0' || parsed_index < 0)
            {
                fputs("--inspect-map-overview index must be a non-negative integer\n", stderr);
                return 1;
            }

            inspect_map_overview = 1;
            inspect_map_overview_index = (size_t)parsed_index;
            continue;
        }

        if (strcmp(argv[i], "--inspect-map-load") == 0)
        {
            char *end = NULL;
            long parsed_index;
            if ((i + 1) >= argc)
            {
                fputs("--inspect-map-load requires an index\n", stderr);
                return 1;
            }

            parsed_index = strtol(argv[++i], &end, 10);
            if (end == argv[i] || *end != '\0' || parsed_index < 0)
            {
                fputs("--inspect-map-load index must be a non-negative integer\n", stderr);
                return 1;
            }

            inspect_map_load = 1;
            inspect_map_load_index = (size_t)parsed_index;
            continue;
        }

        if (strcmp(argv[i], "--inspect-map-cell") == 0)
        {
            char *map_end = NULL;
            char *x_end = NULL;
            char *y_end = NULL;
            long parsed_map_index;
            long parsed_x;
            long parsed_y;
            if ((i + 3) >= argc)
            {
                fputs("--inspect-map-cell requires a map index, x, and y\n", stderr);
                return 1;
            }

            parsed_map_index = strtol(argv[++i], &map_end, 10);
            parsed_x = strtol(argv[++i], &x_end, 10);
            parsed_y = strtol(argv[++i], &y_end, 10);
            if (map_end == argv[i - 2] || *map_end != '\0' || parsed_map_index < 0)
            {
                fputs("--inspect-map-cell map index must be a non-negative integer\n", stderr);
                return 1;
            }
            if (x_end == argv[i - 1] || *x_end != '\0' || parsed_x < 0)
            {
                fputs("--inspect-map-cell x must be a non-negative integer\n", stderr);
                return 1;
            }
            if (y_end == argv[i] || *y_end != '\0' || parsed_y < 0)
            {
                fputs("--inspect-map-cell y must be a non-negative integer\n", stderr);
                return 1;
            }

            inspect_map_cell = 1;
            inspect_map_cell_map_index = (size_t)parsed_map_index;
            inspect_map_cell_x = (size_t)parsed_x;
            inspect_map_cell_y = (size_t)parsed_y;
            continue;
        }

        if (strcmp(argv[i], "--inspect-map-row") == 0)
        {
            char *map_end = NULL;
            char *plane_end = NULL;
            char *y_end = NULL;
            long parsed_map_index;
            long parsed_plane_index;
            long parsed_y;
            if ((i + 3) >= argc)
            {
                fputs("--inspect-map-row requires a map index, plane index, and y\n", stderr);
                return 1;
            }

            parsed_map_index = strtol(argv[++i], &map_end, 10);
            parsed_plane_index = strtol(argv[++i], &plane_end, 10);
            parsed_y = strtol(argv[++i], &y_end, 10);
            if (map_end == argv[i - 2] || *map_end != '\0' || parsed_map_index < 0)
            {
                fputs("--inspect-map-row map index must be a non-negative integer\n", stderr);
                return 1;
            }
            if (plane_end == argv[i - 1] || *plane_end != '\0' || parsed_plane_index < 0 || parsed_plane_index > 2)
            {
                fputs("--inspect-map-row plane index must be 0, 1, or 2\n", stderr);
                return 1;
            }
            if (y_end == argv[i] || *y_end != '\0' || parsed_y < 0)
            {
                fputs("--inspect-map-row y must be a non-negative integer\n", stderr);
                return 1;
            }

            inspect_map_row = 1;
            inspect_map_row_map_index = (size_t)parsed_map_index;
            inspect_map_row_plane_index = (size_t)parsed_plane_index;
            inspect_map_row_y = (size_t)parsed_y;
            continue;
        }

        if (strcmp(argv[i], "--inspect-map-column") == 0)
        {
            char *map_end = NULL;
            char *plane_end = NULL;
            char *x_end = NULL;
            long parsed_map_index;
            long parsed_plane_index;
            long parsed_x;
            if ((i + 3) >= argc)
            {
                fputs("--inspect-map-column requires a map index, plane index, and x\n", stderr);
                return 1;
            }

            parsed_map_index = strtol(argv[++i], &map_end, 10);
            parsed_plane_index = strtol(argv[++i], &plane_end, 10);
            parsed_x = strtol(argv[++i], &x_end, 10);
            if (map_end == argv[i - 2] || *map_end != '\0' || parsed_map_index < 0)
            {
                fputs("--inspect-map-column map index must be a non-negative integer\n", stderr);
                return 1;
            }
            if (plane_end == argv[i - 1] || *plane_end != '\0' || parsed_plane_index < 0 || parsed_plane_index > 2)
            {
                fputs("--inspect-map-column plane index must be 0, 1, or 2\n", stderr);
                return 1;
            }
            if (x_end == argv[i] || *x_end != '\0' || parsed_x < 0)
            {
                fputs("--inspect-map-column x must be a non-negative integer\n", stderr);
                return 1;
            }

            inspect_map_column = 1;
            inspect_map_column_map_index = (size_t)parsed_map_index;
            inspect_map_column_plane_index = (size_t)parsed_plane_index;
            inspect_map_column_x = (size_t)parsed_x;
            continue;
        }

        if (strcmp(argv[i], "--inspect-map-region") == 0)
        {
            char *map_end = NULL;
            char *plane_end = NULL;
            char *x_end = NULL;
            char *y_end = NULL;
            char *width_end = NULL;
            char *height_end = NULL;
            long parsed_map_index;
            long parsed_plane_index;
            long parsed_x;
            long parsed_y;
            long parsed_width;
            long parsed_height;
            if ((i + 6) >= argc)
            {
                fputs("--inspect-map-region requires a map index, plane index, x, y, width, and height\n", stderr);
                return 1;
            }

            parsed_map_index = strtol(argv[++i], &map_end, 10);
            parsed_plane_index = strtol(argv[++i], &plane_end, 10);
            parsed_x = strtol(argv[++i], &x_end, 10);
            parsed_y = strtol(argv[++i], &y_end, 10);
            parsed_width = strtol(argv[++i], &width_end, 10);
            parsed_height = strtol(argv[++i], &height_end, 10);
            if (map_end == argv[i - 5] || *map_end != '\0' || parsed_map_index < 0)
            {
                fputs("--inspect-map-region map index must be a non-negative integer\n", stderr);
                return 1;
            }
            if (plane_end == argv[i - 4] || *plane_end != '\0' || parsed_plane_index < 0 || parsed_plane_index > 2)
            {
                fputs("--inspect-map-region plane index must be 0, 1, or 2\n", stderr);
                return 1;
            }
            if (x_end == argv[i - 3] || *x_end != '\0' || parsed_x < 0)
            {
                fputs("--inspect-map-region x must be a non-negative integer\n", stderr);
                return 1;
            }
            if (y_end == argv[i - 2] || *y_end != '\0' || parsed_y < 0)
            {
                fputs("--inspect-map-region y must be a non-negative integer\n", stderr);
                return 1;
            }
            if (width_end == argv[i - 1] || *width_end != '\0' || parsed_width <= 0)
            {
                fputs("--inspect-map-region width must be a positive integer\n", stderr);
                return 1;
            }
            if (height_end == argv[i] || *height_end != '\0' || parsed_height <= 0)
            {
                fputs("--inspect-map-region height must be a positive integer\n", stderr);
                return 1;
            }

            inspect_map_region = 1;
            inspect_map_region_map_index = (size_t)parsed_map_index;
            inspect_map_region_plane_index = (size_t)parsed_plane_index;
            inspect_map_region_x = (size_t)parsed_x;
            inspect_map_region_y = (size_t)parsed_y;
            inspect_map_region_width = (size_t)parsed_width;
            inspect_map_region_height = (size_t)parsed_height;
            continue;
        }

        if (strcmp(argv[i], "--inspect-first-map-load") == 0)
        {
            inspect_first_map_load = 1;
            continue;
        }

        if (strcmp(argv[i], "--inspect-first-map-plane0") == 0)
        {
            inspect_first_map_plane = 1;
            inspect_first_map_plane_index = 0;
            continue;
        }

        if (strcmp(argv[i], "--inspect-first-map-plane") == 0)
        {
            char *end = NULL;
            long parsed_index;
            if ((i + 1) >= argc)
            {
                fputs("--inspect-first-map-plane requires an index\n", stderr);
                return 1;
            }

            parsed_index = strtol(argv[++i], &end, 10);
            if (end == argv[i] || *end != '\0' || parsed_index < 0 || parsed_index > 2)
            {
                fputs("--inspect-first-map-plane index must be 0, 1, or 2\n", stderr);
                return 1;
            }

            inspect_first_map_plane = 1;
            inspect_first_map_plane_index = (size_t)parsed_index;
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

    if (inspect_map)
    {
        if (!wolf_is_valid_data_dir(data_path, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        if (!wolf_read_map_summary(data_path, inspect_map_index, &map_summary, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        printf("map%zu name: %s\n", inspect_map_index, map_summary.name);
        printf("map%zu size: %ux%u\n", inspect_map_index, map_summary.width, map_summary.height);
        printf("map%zu plane0 offset: %u\n", inspect_map_index, map_summary.plane_offsets[0]);
        printf("map%zu plane1 offset: %u\n", inspect_map_index, map_summary.plane_offsets[1]);
        printf("map%zu plane2 offset: %u\n", inspect_map_index, map_summary.plane_offsets[2]);
        printf("map%zu plane0 length: %u\n", inspect_map_index, map_summary.plane_lengths[0]);
        return 0;
    }

    if (inspect_map_slot)
    {
        uint32_t map_offset = 0;
        bool is_present = false;

        if (!wolf_is_valid_data_dir(data_path, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        if (!wolf_read_map_slot(data_path, inspect_map_slot_index, &map_offset, &is_present, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        printf("map%zu present: %s\n", inspect_map_slot_index, is_present ? "yes" : "no");
        printf("map%zu offset: %u\n", inspect_map_slot_index, map_offset);
        return 0;
    }

    if (inspect_present_index_for_slot)
    {
        size_t present_index = 0;
        bool is_present = false;

        if (!wolf_is_valid_data_dir(data_path, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        if (!wolf_read_present_index_for_slot(data_path,
                inspect_present_index_for_slot_slot_index,
                &present_index,
                &is_present,
                &map_presence_summary,
                error_buffer,
                sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        printf("map slot %zu present: %s\n", inspect_present_index_for_slot_slot_index, is_present ? "yes" : "no");
        if (is_present)
        {
            if (!wolf_read_map_summary(data_path, inspect_present_index_for_slot_slot_index, &map_summary, error_buffer, sizeof(error_buffer)))
            {
                fputs(error_buffer, stderr);
                fputc('\n', stderr);
                return 1;
            }

            printf("map slot %zu present index: %zu\n", inspect_present_index_for_slot_slot_index, present_index);
            printf("map slot %zu name: %s\n", inspect_present_index_for_slot_slot_index, map_summary.name);
        }
        return 0;
    }

    if (inspect_map_presence_summary)
    {
        if (!wolf_is_valid_data_dir(data_path, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        if (!wolf_read_map_presence_summary(data_path, &map_presence_summary, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        printf("map slots total: %zu\n", map_presence_summary.total_slots);
        printf("map slots present: %zu\n", map_presence_summary.present_slots);
        printf("first present slot: %zu\n", map_presence_summary.has_present_slot ? map_presence_summary.first_present_slot : map_presence_summary.total_slots);
        printf("last present slot: %zu\n", map_presence_summary.has_present_slot ? map_presence_summary.last_present_slot : map_presence_summary.total_slots);
        printf("first empty slot: %zu\n", map_presence_summary.has_empty_slot ? map_presence_summary.first_empty_slot : map_presence_summary.total_slots);
        return 0;
    }

    if (inspect_map_catalog)
    {
        wolf_map_summary *catalog = NULL;
        size_t loaded_count = 0;
        size_t map_index;

        if (!wolf_is_valid_data_dir(data_path, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        catalog = (wolf_map_summary *)calloc(inspect_map_catalog_count == 0 ? 1 : inspect_map_catalog_count, sizeof(wolf_map_summary));
        if (catalog == NULL)
        {
            fputs("out of memory allocating map catalog\n", stderr);
            return 1;
        }

        if (!wolf_read_map_catalog(data_path,
                inspect_map_catalog_count,
                catalog,
                inspect_map_catalog_count,
                &loaded_count,
                &maphead_summary,
                error_buffer,
                sizeof(error_buffer)))
        {
            free(catalog);
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        printf("map catalog total: %zu\n", maphead_summary.map_count);
        for (map_index = 0; map_index < loaded_count; ++map_index)
        {
            printf("map%zu catalog name: %s\n", map_index, catalog[map_index].name);
            printf("map%zu catalog size: %ux%u\n", map_index, catalog[map_index].width, catalog[map_index].height);
        }

        free(catalog);
        return 0;
    }

    if (inspect_present_map_catalog)
    {
        wolf_present_map_summary *catalog = NULL;
        size_t loaded_count = 0;
        size_t map_index;

        if (!wolf_is_valid_data_dir(data_path, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        catalog = (wolf_present_map_summary *)calloc(inspect_present_map_catalog_count == 0 ? 1 : inspect_present_map_catalog_count, sizeof(wolf_present_map_summary));
        if (catalog == NULL)
        {
            fputs("out of memory allocating present map catalog\n", stderr);
            return 1;
        }

        if (!wolf_read_present_map_catalog(data_path,
                inspect_present_map_catalog_count,
                catalog,
                inspect_present_map_catalog_count,
                &loaded_count,
                &map_presence_summary,
                error_buffer,
                sizeof(error_buffer)))
        {
            free(catalog);
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        printf("present map catalog total slots: %zu\n", map_presence_summary.total_slots);
        printf("present map catalog total present: %zu\n", map_presence_summary.present_slots);
        for (map_index = 0; map_index < loaded_count; ++map_index)
        {
            printf("present map%zu slot: %zu\n", map_index, catalog[map_index].slot_index);
            printf("present map%zu name: %s\n", map_index, catalog[map_index].summary.name);
            printf("present map%zu size: %ux%u\n", map_index, catalog[map_index].summary.width, catalog[map_index].summary.height);
        }

        free(catalog);
        return 0;
    }

    if (inspect_present_map_cell)
    {
        wolf_loaded_present_map present_map;
        size_t plane_index;

        if (!wolf_is_valid_data_dir(data_path, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        if (!wolf_load_present_map(data_path,
                inspect_present_map_cell_index,
                &present_map,
                &map_presence_summary,
                error_buffer,
                sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        for (plane_index = 0; plane_index < 3; ++plane_index)
        {
            uint16_t value = 0;
            if (!wolf_map_get_cell(&present_map.map, plane_index, inspect_present_map_cell_x, inspect_present_map_cell_y, &value))
            {
                fputs("could not inspect present map cell\n", stderr);
                return 1;
            }

            printf("present map%zu cell[%zu,%zu] plane%zu: %u\n",
                inspect_present_map_cell_index,
                inspect_present_map_cell_x,
                inspect_present_map_cell_y,
                plane_index,
                value);
        }

        return 0;
    }

    if (inspect_present_map_row)
    {
        wolf_loaded_present_map present_map;
        const uint16_t *row_words = NULL;
        size_t row_length = 0;

        if (!wolf_is_valid_data_dir(data_path, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        if (!wolf_load_present_map(data_path,
                inspect_present_map_row_index,
                &present_map,
                &map_presence_summary,
                error_buffer,
                sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        if (!wolf_map_get_row(&present_map.map,
                inspect_present_map_row_plane_index,
                inspect_present_map_row_y,
                &row_words,
                &row_length))
        {
            fputs("could not inspect present map row\n", stderr);
            return 1;
        }

        printf("present map%zu plane%zu row%zu length: %zu\n",
            inspect_present_map_row_index,
            inspect_present_map_row_plane_index,
            inspect_present_map_row_y,
            row_length);
        if (row_length == 0)
        {
            puts("present map row is empty");
            return 0;
        }
        printf("present map%zu plane%zu row%zu cells: [0]=%u [1]=%u [31]=%u [32]=%u [33]=%u [34]=%u [63]=%u\n",
            inspect_present_map_row_index,
            inspect_present_map_row_plane_index,
            inspect_present_map_row_y,
            row_words[0],
            row_words[1],
            row_words[31],
            row_words[32],
            row_words[33],
            row_words[34],
            row_words[63]);
        return 0;
    }

    if (inspect_present_map_column)
    {
        wolf_loaded_present_map present_map;
        uint16_t column_words[64];
        size_t column_length = 0;

        if (!wolf_is_valid_data_dir(data_path, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        if (!wolf_load_present_map(data_path,
                inspect_present_map_column_index,
                &present_map,
                &map_presence_summary,
                error_buffer,
                sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        if (!wolf_map_get_column(&present_map.map,
                inspect_present_map_column_plane_index,
                inspect_present_map_column_x,
                column_words,
                (sizeof(column_words) / sizeof(column_words[0])),
                &column_length))
        {
            fprintf(stderr,
                "present map column is out of bounds: map=%zu plane=%zu x=%zu\n",
                inspect_present_map_column_index,
                inspect_present_map_column_plane_index,
                inspect_present_map_column_x);
            return 1;
        }

        printf("present map%zu plane%zu column%zu length: %zu\n",
            inspect_present_map_column_index,
            inspect_present_map_column_plane_index,
            inspect_present_map_column_x,
            column_length);
        printf("present map%zu plane%zu column%zu cells: [0]=%u [1]=%u [31]=%u [32]=%u [33]=%u [34]=%u [63]=%u\n",
            inspect_present_map_column_index,
            inspect_present_map_column_plane_index,
            inspect_present_map_column_x,
            column_words[0],
            column_words[1],
            column_words[31],
            column_words[32],
            column_words[33],
            column_words[34],
            column_words[63]);
        return 0;
    }

    if (inspect_present_map_region)
    {
        wolf_loaded_present_map present_map;
        size_t region_word_count = 0;

        if (!wolf_is_valid_data_dir(data_path, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        if (!wolf_load_present_map(data_path,
                inspect_present_map_region_index,
                &present_map,
                &map_presence_summary,
                error_buffer,
                sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        if (!wolf_map_get_region(&present_map.map,
                inspect_present_map_region_plane_index,
                inspect_present_map_region_x,
                inspect_present_map_region_y,
                inspect_present_map_region_width,
                inspect_present_map_region_height,
                region_words,
                (sizeof(region_words) / sizeof(region_words[0])),
                &region_word_count))
        {
            fputs("could not inspect present map region\n", stderr);
            return 1;
        }

        printf("present map%zu plane%zu region%zu,%zu size: %zux%zu\n",
            inspect_present_map_region_index,
            inspect_present_map_region_plane_index,
            inspect_present_map_region_x,
            inspect_present_map_region_y,
            inspect_present_map_region_width,
            inspect_present_map_region_height);
        printf("present map%zu plane%zu region%zu,%zu cells: [0,0]=%u [1,0]=%u [0,1]=%u [1,1]=%u\n",
            inspect_present_map_region_index,
            inspect_present_map_region_plane_index,
            inspect_present_map_region_x,
            inspect_present_map_region_y,
            region_words[0],
            region_words[1],
            region_words[2],
            region_words[3]);
        (void)region_word_count;
        return 0;
    }

    if (inspect_present_map_load)
    {
        wolf_loaded_present_map present_map;
        const wolf_map_plane_load_result *plane_result = NULL;
        size_t plane_index;

        if (!wolf_is_valid_data_dir(data_path, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        if (!wolf_load_present_map(data_path,
                inspect_present_map_load_index,
                &present_map,
                &map_presence_summary,
                error_buffer,
                sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        printf("present map load index: %zu\n", inspect_present_map_load_index);
        printf("present map load slot: %zu\n", present_map.slot_index);
        printf("present map load name: %s\n", present_map.map.summary.name);
        printf("present map load size: %ux%u\n", present_map.map.summary.width, present_map.map.summary.height);
        for (plane_index = 0; plane_index < 3; ++plane_index)
        {
            uint16_t top_left = 0;
            uint16_t mid_left = 0;
            uint16_t center = 0;
            uint16_t bottom_right = 0;

            if (!wolf_map_get_plane_result(&present_map.map, plane_index, &plane_result)
                || !wolf_map_get_cell(&present_map.map, plane_index, 0, 0, &top_left)
                || !wolf_map_get_cell(&present_map.map, plane_index, 31, 31, &mid_left)
                || !wolf_map_get_cell(&present_map.map, plane_index, 32, 32, &center)
                || !wolf_map_get_cell(&present_map.map, plane_index, 63, 63, &bottom_right))
            {
                fputs("could not inspect loaded present map\n", stderr);
                return 1;
            }

            printf("present map plane%zu result: compressed=%u carmack=%u rlew=%u words=%zu\n",
                plane_index,
                plane_result->compressed_bytes,
                plane_result->carmack_expanded_bytes,
                plane_result->rlew_expanded_bytes,
                plane_result->decoded_words);
            printf("present map plane%zu sample cells: [0,0]=%u [31,31]=%u [32,32]=%u [63,63]=%u\n",
                plane_index,
                top_left,
                mid_left,
                center,
                bottom_right);
        }

        return 0;
    }

    if (inspect_present_map_load_catalog)
    {
        wolf_loaded_present_map *catalog = NULL;
        size_t loaded_count = 0;
        size_t map_index;

        if (!wolf_is_valid_data_dir(data_path, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        catalog = (wolf_loaded_present_map *)calloc(inspect_present_map_load_catalog_count == 0 ? 1 : inspect_present_map_load_catalog_count,
            sizeof(wolf_loaded_present_map));
        if (catalog == NULL)
        {
            fputs("out of memory allocating present map load catalog\n", stderr);
            return 1;
        }

        if (!wolf_load_present_map_catalog(data_path,
                inspect_present_map_load_catalog_count,
                catalog,
                inspect_present_map_load_catalog_count,
                &loaded_count,
                &map_presence_summary,
                error_buffer,
                sizeof(error_buffer)))
        {
            free(catalog);
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        printf("present map load catalog total slots: %zu\n", map_presence_summary.total_slots);
        printf("present map load catalog total present: %zu\n", map_presence_summary.present_slots);
        for (map_index = 0; map_index < loaded_count; ++map_index)
        {
            size_t plane_index;

            printf("present map load%zu slot: %zu\n", map_index, catalog[map_index].slot_index);
            printf("present map load%zu name: %s\n", map_index, catalog[map_index].map.summary.name);
            printf("present map load%zu size: %ux%u\n", map_index, catalog[map_index].map.summary.width, catalog[map_index].map.summary.height);
            for (plane_index = 0; plane_index < 3; ++plane_index)
            {
                uint16_t top_left = 0;
                uint16_t mid_left = 0;
                uint16_t center = 0;
                uint16_t bottom_right = 0;

                if (!wolf_map_get_cell(&catalog[map_index].map, plane_index, 0, 0, &top_left)
                    || !wolf_map_get_cell(&catalog[map_index].map, plane_index, 31, 31, &mid_left)
                    || !wolf_map_get_cell(&catalog[map_index].map, plane_index, 32, 32, &center)
                    || !wolf_map_get_cell(&catalog[map_index].map, plane_index, 63, 63, &bottom_right))
                {
                    free(catalog);
                    fputs("could not sample loaded present map\n", stderr);
                    return 1;
                }

                printf("present map load%zu plane%zu result: compressed=%u carmack=%u rlew=%u words=%zu\n",
                    map_index,
                    plane_index,
                    catalog[map_index].map.plane_results[plane_index].compressed_bytes,
                    catalog[map_index].map.plane_results[plane_index].carmack_expanded_bytes,
                    catalog[map_index].map.plane_results[plane_index].rlew_expanded_bytes,
                    catalog[map_index].map.plane_results[plane_index].decoded_words);
                printf("present map load%zu plane%zu sample cells: [0,0]=%u [31,31]=%u [32,32]=%u [63,63]=%u\n",
                    map_index,
                    plane_index,
                    top_left,
                    mid_left,
                    center,
                    bottom_right);
            }
        }

        free(catalog);
        return 0;
    }

    if (inspect_map_load_catalog)
    {
        wolf_loaded_map *catalog = NULL;
        size_t loaded_count = 0;
        size_t map_index;

        if (!wolf_is_valid_data_dir(data_path, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        catalog = (wolf_loaded_map *)calloc(inspect_map_load_catalog_count == 0 ? 1 : inspect_map_load_catalog_count,
            sizeof(wolf_loaded_map));
        if (catalog == NULL)
        {
            fputs("out of memory allocating map load catalog\n", stderr);
            return 1;
        }

        if (!wolf_load_map_catalog(data_path,
                inspect_map_load_catalog_count,
                catalog,
                inspect_map_load_catalog_count,
                &loaded_count,
                &maphead_summary,
                error_buffer,
                sizeof(error_buffer)))
        {
            free(catalog);
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        printf("map load catalog total: %zu\n", maphead_summary.map_count);
        for (map_index = 0; map_index < loaded_count; ++map_index)
        {
            size_t plane_index;

            printf("map load%zu name: %s\n", map_index, catalog[map_index].summary.name);
            printf("map load%zu size: %ux%u\n", map_index, catalog[map_index].summary.width, catalog[map_index].summary.height);
            for (plane_index = 0; plane_index < 3; ++plane_index)
            {
                uint16_t top_left = 0;
                uint16_t mid_left = 0;
                uint16_t center = 0;
                uint16_t bottom_right = 0;

                if (!wolf_map_get_cell(&catalog[map_index], plane_index, 0, 0, &top_left)
                    || !wolf_map_get_cell(&catalog[map_index], plane_index, 31, 31, &mid_left)
                    || !wolf_map_get_cell(&catalog[map_index], plane_index, 32, 32, &center)
                    || !wolf_map_get_cell(&catalog[map_index], plane_index, 63, 63, &bottom_right))
                {
                    free(catalog);
                    fputs("could not sample loaded map catalog entry\n", stderr);
                    return 1;
                }

                printf("map load%zu plane%zu result: compressed=%u carmack=%u rlew=%u words=%zu\n",
                    map_index,
                    plane_index,
                    catalog[map_index].plane_results[plane_index].compressed_bytes,
                    catalog[map_index].plane_results[plane_index].carmack_expanded_bytes,
                    catalog[map_index].plane_results[plane_index].rlew_expanded_bytes,
                    catalog[map_index].plane_results[plane_index].decoded_words);
                printf("map load%zu plane%zu sample cells: [0,0]=%u [31,31]=%u [32,32]=%u [63,63]=%u\n",
                    map_index,
                    plane_index,
                    top_left,
                    mid_left,
                    center,
                    bottom_right);
            }
        }

        free(catalog);
        return 0;
    }

    if (validate_map_catalog)
    {
        bool *valid_flags = NULL;
        size_t validated_count = 0;
        size_t map_index;
        int all_valid = 1;

        if (!wolf_is_valid_data_dir(data_path, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        valid_flags = (bool *)calloc(validate_map_catalog_count == 0 ? 1 : validate_map_catalog_count, sizeof(bool));
        if (valid_flags == NULL)
        {
            fputs("out of memory allocating map catalog validation flags\n", stderr);
            return 1;
        }

        if (!wolf_validate_map_catalog(data_path,
                validate_map_catalog_count,
                valid_flags,
                validate_map_catalog_count,
                &validated_count,
                error_buffer,
                sizeof(error_buffer)))
        {
            free(valid_flags);
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        for (map_index = 0; map_index < validated_count; ++map_index)
        {
            if (!valid_flags[map_index])
            {
                all_valid = 0;
                break;
            }
        }

        printf("map catalog validation count: %zu\n", validated_count);
        printf("map catalog validation all valid: %s\n", all_valid ? "yes" : "no");
        for (map_index = 0; map_index < validated_count; ++map_index)
        {
            printf("map%zu validation: %s\n", map_index, valid_flags[map_index] ? "yes" : "no");
        }

        free(valid_flags);
        return all_valid ? 0 : 1;
    }

    if (validate_map_plane_table_catalog)
    {
        bool *valid_flags = NULL;
        size_t validated_count = 0;
        size_t map_index;
        int all_valid = 1;

        if (!wolf_is_valid_data_dir(data_path, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        valid_flags = (bool *)calloc(validate_map_plane_table_catalog_count == 0 ? 1 : validate_map_plane_table_catalog_count, sizeof(bool));
        if (valid_flags == NULL)
        {
            fputs("out of memory allocating map plane table catalog validation flags\n", stderr);
            return 1;
        }

        if (!wolf_validate_map_plane_table_catalog(data_path,
                validate_map_plane_table_catalog_count,
                valid_flags,
                validate_map_plane_table_catalog_count,
                &validated_count,
                &maphead_summary,
                error_buffer,
                sizeof(error_buffer)))
        {
            free(valid_flags);
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        for (map_index = 0; map_index < validated_count; ++map_index)
        {
            if (!valid_flags[map_index])
            {
                all_valid = 0;
                break;
            }
        }

        printf("map plane table catalog total: %zu\n", maphead_summary.map_count);
        printf("map plane table catalog validation count: %zu\n", validated_count);
        printf("map plane table catalog validation all valid: %s\n", all_valid ? "yes" : "no");
        for (map_index = 0; map_index < validated_count; ++map_index)
        {
            printf("map plane table%zu validation: %s\n", map_index, valid_flags[map_index] ? "yes" : "no");
        }

        free(valid_flags);
        return all_valid ? 0 : 1;
    }

    if (validate_present_map_catalog)
    {
        size_t *slot_indices = NULL;
        bool *valid_flags = NULL;
        size_t validated_count = 0;
        size_t map_index;
        int all_valid = 1;

        if (!wolf_is_valid_data_dir(data_path, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        slot_indices = (size_t *)calloc(validate_present_map_catalog_count == 0 ? 1 : validate_present_map_catalog_count, sizeof(size_t));
        valid_flags = (bool *)calloc(validate_present_map_catalog_count == 0 ? 1 : validate_present_map_catalog_count, sizeof(bool));
        if (slot_indices == NULL || valid_flags == NULL)
        {
            free(slot_indices);
            free(valid_flags);
            fputs("out of memory allocating present map catalog validation buffers\n", stderr);
            return 1;
        }

        if (!wolf_validate_present_map_catalog(data_path,
                validate_present_map_catalog_count,
                slot_indices,
                valid_flags,
                validate_present_map_catalog_count,
                &validated_count,
                &map_presence_summary,
                error_buffer,
                sizeof(error_buffer)))
        {
            free(slot_indices);
            free(valid_flags);
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        for (map_index = 0; map_index < validated_count; ++map_index)
        {
            if (!valid_flags[map_index])
            {
                all_valid = 0;
                break;
            }
        }

        printf("present map catalog validation total slots: %zu\n", map_presence_summary.total_slots);
        printf("present map catalog validation total present: %zu\n", map_presence_summary.present_slots);
        printf("present map catalog validation count: %zu\n", validated_count);
        printf("present map catalog validation all valid: %s\n", all_valid ? "yes" : "no");
        for (map_index = 0; map_index < validated_count; ++map_index)
        {
            printf("present map%zu slot: %zu\n", map_index, slot_indices[map_index]);
            printf("present map%zu validation: %s\n", map_index, valid_flags[map_index] ? "yes" : "no");
        }

        free(slot_indices);
        free(valid_flags);
        return all_valid ? 0 : 1;
    }

    if (validate_present_map_load_catalog)
    {
        size_t *slot_indices = NULL;
        bool *valid_flags = NULL;
        size_t validated_count = 0;
        size_t map_index;
        int all_valid = 1;

        if (!wolf_is_valid_data_dir(data_path, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        slot_indices = (size_t *)calloc(validate_present_map_load_catalog_count == 0 ? 1 : validate_present_map_load_catalog_count, sizeof(size_t));
        valid_flags = (bool *)calloc(validate_present_map_load_catalog_count == 0 ? 1 : validate_present_map_load_catalog_count, sizeof(bool));
        if (slot_indices == NULL || valid_flags == NULL)
        {
            free(slot_indices);
            free(valid_flags);
            fputs("out of memory allocating present map load catalog validation buffers\n", stderr);
            return 1;
        }

        if (!wolf_validate_present_map_load_catalog(data_path,
                validate_present_map_load_catalog_count,
                slot_indices,
                valid_flags,
                validate_present_map_load_catalog_count,
                &validated_count,
                &map_presence_summary,
                error_buffer,
                sizeof(error_buffer)))
        {
            free(slot_indices);
            free(valid_flags);
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        for (map_index = 0; map_index < validated_count; ++map_index)
        {
            if (!valid_flags[map_index])
            {
                all_valid = 0;
                break;
            }
        }

        printf("present map load catalog validation total slots: %zu\n", map_presence_summary.total_slots);
        printf("present map load catalog validation total present: %zu\n", map_presence_summary.present_slots);
        printf("present map load catalog validation count: %zu\n", validated_count);
        printf("present map load catalog validation all valid: %s\n", all_valid ? "yes" : "no");
        for (map_index = 0; map_index < validated_count; ++map_index)
        {
            printf("present map load%zu slot: %zu\n", map_index, slot_indices[map_index]);
            printf("present map load%zu validation: %s\n", map_index, valid_flags[map_index] ? "yes" : "no");
        }

        free(slot_indices);
        free(valid_flags);
        return all_valid ? 0 : 1;
    }

    if (validate_present_map_load)
    {
        wolf_loaded_present_map present_map;
        size_t plane_index;
        int all_valid = 1;

        if (!wolf_is_valid_data_dir(data_path, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        if (!wolf_validate_present_map_load(data_path,
                validate_present_map_load_index,
                &present_map,
                plane_headers,
                &map_presence_summary,
                error_buffer,
                sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        printf("present map load validation index: %zu\n", validate_present_map_load_index);
        printf("present map load validation slot: %zu\n", present_map.slot_index);
        printf("present map load validation name: %s\n", present_map.map.summary.name);
        printf("present map load validation size: %ux%u\n", present_map.map.summary.width, present_map.map.summary.height);
        printf("present map load validation plane table valid: %s\n",
            wolf_map_plane_headers_are_valid(&present_map.map.summary, plane_headers) ? "yes" : "no");
        for (plane_index = 0; plane_index < 3; ++plane_index)
        {
            int plane_valid = wolf_map_plane_load_result_matches_header(&plane_headers[plane_index], &present_map.map.plane_results[plane_index]);
            printf("present map plane%zu load result match: %s\n", plane_index, plane_valid ? "yes" : "no");
            if (!plane_valid)
            {
                all_valid = 0;
            }
        }

        return all_valid ? 0 : 1;
    }

    if (validate_map_header)
    {
        if (!wolf_is_valid_data_dir(data_path, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        if (!wolf_read_map_summary(data_path, validate_map_header_index, &map_summary, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        printf("map%zu header valid: %s\n", validate_map_header_index, wolf_map_header_is_valid(&map_summary) ? "yes" : "no");
        printf("map%zu plane bounds valid: %s\n", validate_map_header_index, wolf_map_planes_are_in_bounds(&map_summary) ? "yes" : "no");
        printf("map%zu name: %s\n", validate_map_header_index, map_summary.name);
        printf("map%zu size: %ux%u\n", validate_map_header_index, map_summary.width, map_summary.height);
        return (wolf_map_header_is_valid(&map_summary) && wolf_map_planes_are_in_bounds(&map_summary)) ? 0 : 1;
    }

    if (validate_map_load)
    {
        size_t plane_index;
        int all_valid = 1;

        if (!wolf_is_valid_data_dir(data_path, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        if (!wolf_validate_map_load(data_path, validate_map_load_index, &loaded_map, plane_headers, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        printf("map%zu load valid: yes\n", validate_map_load_index);
        printf("map%zu load name: %s\n", validate_map_load_index, loaded_map.summary.name);
        printf("map%zu load size: %ux%u\n", validate_map_load_index, loaded_map.summary.width, loaded_map.summary.height);
        printf("map%zu load plane table valid: %s\n",
            validate_map_load_index,
            wolf_map_plane_headers_are_valid(&loaded_map.summary, plane_headers) ? "yes" : "no");
        for (plane_index = 0; plane_index < 3; ++plane_index)
        {
            int plane_valid = wolf_map_plane_load_result_matches_header(&plane_headers[plane_index], &loaded_map.plane_results[plane_index]);
            printf("map%zu plane%zu load result match: %s\n", validate_map_load_index, plane_index, plane_valid ? "yes" : "no");
            if (!plane_valid)
            {
                all_valid = 0;
            }
        }

        return all_valid ? 0 : 1;
    }

    if (validate_map_plane_header)
    {
        size_t expected_words;
        int header_valid;
        int size_matches;

        if (!wolf_is_valid_data_dir(data_path, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        if (!wolf_read_map_summary(data_path, validate_map_plane_header_map_index, &map_summary, error_buffer, sizeof(error_buffer))
            || !wolf_read_map_plane_header(data_path, validate_map_plane_header_map_index, validate_map_plane_header_index, &plane_header, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        expected_words = (size_t)map_summary.width * (size_t)map_summary.height;
        header_valid = wolf_map_plane_header_is_valid_for_map(&map_summary, &plane_header);
        size_matches = (plane_header.decoded_words == expected_words);
        printf("map%zu plane%zu header valid: %s\n", validate_map_plane_header_map_index, validate_map_plane_header_index, header_valid ? "yes" : "no");
        printf("map%zu plane%zu summary match: %s\n", validate_map_plane_header_map_index, validate_map_plane_header_index,
            wolf_map_plane_header_matches_summary(&map_summary, validate_map_plane_header_index, &plane_header) ? "yes" : "no");
        printf("map%zu plane%zu bounds match: %s\n", validate_map_plane_header_map_index, validate_map_plane_header_index,
            wolf_map_plane_header_is_in_bounds(&map_summary, validate_map_plane_header_index, &plane_header) ? "yes" : "no");
        printf("map%zu plane%zu decoded size matches map: %s\n", validate_map_plane_header_map_index, validate_map_plane_header_index, size_matches ? "yes" : "no");
        printf("map%zu plane%zu expected words: %zu\n", validate_map_plane_header_map_index, validate_map_plane_header_index, expected_words);
        printf("map%zu plane%zu decoded words: %zu\n", validate_map_plane_header_map_index, validate_map_plane_header_index, plane_header.decoded_words);
        return (header_valid
            && wolf_map_plane_header_matches_summary(&map_summary, validate_map_plane_header_index, &plane_header)
            && wolf_map_plane_header_is_in_bounds(&map_summary, validate_map_plane_header_index, &plane_header)
            && size_matches) ? 0 : 1;
    }

    if (validate_map_plane_table)
    {
        size_t plane_index;

        if (!wolf_is_valid_data_dir(data_path, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        if (!wolf_read_map_summary(data_path, validate_map_plane_table_index, &map_summary, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        if (!wolf_read_map_plane_headers(data_path, validate_map_plane_table_index, plane_headers, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        printf("map%zu plane table valid: %s\n",
            validate_map_plane_table_index,
            wolf_map_plane_headers_are_valid(&map_summary, plane_headers) ? "yes" : "no");
        for (plane_index = 0; plane_index < 3; ++plane_index)
        {
            printf("map%zu plane%zu valid: %s\n",
                validate_map_plane_table_index,
                plane_index,
                wolf_map_plane_is_valid(&map_summary, plane_index, &plane_headers[plane_index]) ? "yes" : "no");
        }

        return wolf_map_plane_headers_are_valid(&map_summary, plane_headers) ? 0 : 1;
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

    if (self_test_carmack)
    {
        return run_carmack_self_test();
    }

    if (self_test_decompression_failures)
    {
        return run_decompression_failure_self_test();
    }

    if (self_test_map_plane_decode)
    {
        return run_map_plane_decode_self_test();
    }

    if (self_test_map_plane_header_bytes)
    {
        return run_map_plane_header_bytes_self_test();
    }

    if (self_test_map_helpers)
    {
        return run_map_helper_self_test();
    }

    if (self_test_present_map_helpers)
    {
        return run_present_map_helper_self_test();
    }

    if (self_test_map_validation)
    {
        return run_map_validation_self_test();
    }

    if (inspect_map_overview)
    {
        if (!wolf_is_valid_data_dir(data_path, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        if (!wolf_load_map(data_path, inspect_map_overview_index, &loaded_map, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        printf("map%zu overview name: %s\n", inspect_map_overview_index, loaded_map.summary.name);
        printf("map%zu overview size: %ux%u\n", inspect_map_overview_index, loaded_map.summary.width, loaded_map.summary.height);
        printf("map%zu plane0 cells: [0,0]=%u [31,31]=%u [32,32]=%u [63,63]=%u\n",
            inspect_map_overview_index,
            loaded_map.plane_words[0][0],
            loaded_map.plane_words[0][(31 * 64) + 31],
            loaded_map.plane_words[0][(32 * 64) + 32],
            loaded_map.plane_words[0][(63 * 64) + 63]);
        printf("map%zu plane1 cells: [0,0]=%u [31,31]=%u [32,32]=%u [63,63]=%u\n",
            inspect_map_overview_index,
            loaded_map.plane_words[1][0],
            loaded_map.plane_words[1][(31 * 64) + 31],
            loaded_map.plane_words[1][(32 * 64) + 32],
            loaded_map.plane_words[1][(63 * 64) + 63]);
        printf("map%zu plane2 cells: [0,0]=%u [31,31]=%u [32,32]=%u [63,63]=%u\n",
            inspect_map_overview_index,
            loaded_map.plane_words[2][0],
            loaded_map.plane_words[2][(31 * 64) + 31],
            loaded_map.plane_words[2][(32 * 64) + 32],
            loaded_map.plane_words[2][(63 * 64) + 63]);
        return 0;
    }

    if (inspect_first_map_load)
    {
        if (!wolf_is_valid_data_dir(data_path, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        if (!wolf_load_first_map(data_path, &loaded_map, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        print_loaded_map_summary("first map", &loaded_map);
        return 0;
    }

    if (inspect_map_load)
    {
        const wolf_map_plane_load_result *plane_result0 = NULL;
        const wolf_map_plane_load_result *plane_result1 = NULL;
        const wolf_map_plane_load_result *plane_result2 = NULL;

        if (!wolf_is_valid_data_dir(data_path, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        if (!wolf_load_map(data_path, inspect_map_load_index, &loaded_map, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        if (!wolf_map_get_plane_result(&loaded_map, 0, &plane_result0)
            || !wolf_map_get_plane_result(&loaded_map, 1, &plane_result1)
            || !wolf_map_get_plane_result(&loaded_map, 2, &plane_result2))
        {
            fputs("could not inspect loaded map plane results\n", stderr);
            return 1;
        }

        printf("map%zu load name: %s\n", inspect_map_load_index, loaded_map.summary.name);
        printf("map%zu load size: %ux%u\n", inspect_map_load_index, loaded_map.summary.width, loaded_map.summary.height);
        printf("map%zu plane0 result: compressed=%u carmack=%u rlew=%u words=%zu\n",
            inspect_map_load_index,
            plane_result0->compressed_bytes,
            plane_result0->carmack_expanded_bytes,
            plane_result0->rlew_expanded_bytes,
            plane_result0->decoded_words);
        printf("map%zu plane1 result: compressed=%u carmack=%u rlew=%u words=%zu\n",
            inspect_map_load_index,
            plane_result1->compressed_bytes,
            plane_result1->carmack_expanded_bytes,
            plane_result1->rlew_expanded_bytes,
            plane_result1->decoded_words);
        printf("map%zu plane2 result: compressed=%u carmack=%u rlew=%u words=%zu\n",
            inspect_map_load_index,
            plane_result2->compressed_bytes,
            plane_result2->carmack_expanded_bytes,
            plane_result2->rlew_expanded_bytes,
            plane_result2->decoded_words);
        printf("map%zu plane0 sample cells: [0,0]=%u [31,31]=%u [32,32]=%u [63,63]=%u\n",
            inspect_map_load_index,
            loaded_map.plane_words[0][0],
            loaded_map.plane_words[0][(31 * 64) + 31],
            loaded_map.plane_words[0][(32 * 64) + 32],
            loaded_map.plane_words[0][(63 * 64) + 63]);
        printf("map%zu plane1 sample cells: [0,0]=%u [31,31]=%u [32,32]=%u [63,63]=%u\n",
            inspect_map_load_index,
            loaded_map.plane_words[1][0],
            loaded_map.plane_words[1][(31 * 64) + 31],
            loaded_map.plane_words[1][(32 * 64) + 32],
            loaded_map.plane_words[1][(63 * 64) + 63]);
        printf("map%zu plane2 sample cells: [0,0]=%u [31,31]=%u [32,32]=%u [63,63]=%u\n",
            inspect_map_load_index,
            loaded_map.plane_words[2][0],
            loaded_map.plane_words[2][(31 * 64) + 31],
            loaded_map.plane_words[2][(32 * 64) + 32],
            loaded_map.plane_words[2][(63 * 64) + 63]);
        return 0;
    }

    if (inspect_map_cell)
    {
        uint16_t plane0_value;
        uint16_t plane1_value;
        uint16_t plane2_value;

        if (!wolf_is_valid_data_dir(data_path, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        if (!wolf_load_map(data_path, inspect_map_cell_map_index, &loaded_map, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        if (!wolf_map_get_cell(&loaded_map, 0, inspect_map_cell_x, inspect_map_cell_y, &plane0_value)
            || !wolf_map_get_cell(&loaded_map, 1, inspect_map_cell_x, inspect_map_cell_y, &plane1_value)
            || !wolf_map_get_cell(&loaded_map, 2, inspect_map_cell_x, inspect_map_cell_y, &plane2_value))
        {
            fprintf(stderr, "map cell is out of bounds: %zux%zu\n", inspect_map_cell_x, inspect_map_cell_y);
            return 1;
        }

        printf("map%zu cell[%zu,%zu] plane0: %u\n", inspect_map_cell_map_index, inspect_map_cell_x, inspect_map_cell_y, plane0_value);
        printf("map%zu cell[%zu,%zu] plane1: %u\n", inspect_map_cell_map_index, inspect_map_cell_x, inspect_map_cell_y, plane1_value);
        printf("map%zu cell[%zu,%zu] plane2: %u\n", inspect_map_cell_map_index, inspect_map_cell_x, inspect_map_cell_y, plane2_value);
        return 0;
    }

    if (inspect_map_row)
    {
        const uint16_t *row_words = NULL;
        size_t row_length = 0;

        if (!wolf_is_valid_data_dir(data_path, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        if (!wolf_load_map(data_path, inspect_map_row_map_index, &loaded_map, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        if (!wolf_map_get_row(&loaded_map,
                inspect_map_row_plane_index,
                inspect_map_row_y,
                &row_words,
                &row_length))
        {
            fprintf(stderr,
                "map row is out of bounds: map=%zu plane=%zu y=%zu\n",
                inspect_map_row_map_index,
                inspect_map_row_plane_index,
                inspect_map_row_y);
            return 1;
        }

        printf("map%zu plane%zu row%zu length: %zu\n",
            inspect_map_row_map_index,
            inspect_map_row_plane_index,
            inspect_map_row_y,
            row_length);
        printf("map%zu plane%zu row%zu cells: [0]=%u [1]=%u [31]=%u [32]=%u [33]=%u [34]=%u [63]=%u\n",
            inspect_map_row_map_index,
            inspect_map_row_plane_index,
            inspect_map_row_y,
            row_words[0],
            row_words[1],
            row_words[31],
            row_words[32],
            row_words[33],
            row_words[34],
            row_words[63]);
        return 0;
    }

    if (inspect_map_column)
    {
        uint16_t column_words[64];
        size_t column_length = 0;

        if (!wolf_is_valid_data_dir(data_path, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        if (!wolf_load_map(data_path, inspect_map_column_map_index, &loaded_map, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        if (!wolf_map_get_column(&loaded_map,
                inspect_map_column_plane_index,
                inspect_map_column_x,
                column_words,
                (sizeof(column_words) / sizeof(column_words[0])),
                &column_length))
        {
            fprintf(stderr,
                "map column is out of bounds: map=%zu plane=%zu x=%zu\n",
                inspect_map_column_map_index,
                inspect_map_column_plane_index,
                inspect_map_column_x);
            return 1;
        }

        printf("map%zu plane%zu column%zu length: %zu\n",
            inspect_map_column_map_index,
            inspect_map_column_plane_index,
            inspect_map_column_x,
            column_length);
        printf("map%zu plane%zu column%zu cells: [0]=%u [1]=%u [31]=%u [32]=%u [33]=%u [34]=%u [63]=%u\n",
            inspect_map_column_map_index,
            inspect_map_column_plane_index,
            inspect_map_column_x,
            column_words[0],
            column_words[1],
            column_words[31],
            column_words[32],
            column_words[33],
            column_words[34],
            column_words[63]);
        return 0;
    }

    if (inspect_map_region)
    {
        size_t region_word_count = 0;

        if (!wolf_is_valid_data_dir(data_path, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        if (!wolf_load_map(data_path, inspect_map_region_map_index, &loaded_map, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        if (!wolf_map_get_region(&loaded_map,
                inspect_map_region_plane_index,
                inspect_map_region_x,
                inspect_map_region_y,
                inspect_map_region_width,
                inspect_map_region_height,
                region_words,
                (sizeof(region_words) / sizeof(region_words[0])),
                &region_word_count))
        {
            fprintf(stderr,
                "map region is out of bounds: map=%zu plane=%zu x=%zu y=%zu size=%zux%zu\n",
                inspect_map_region_map_index,
                inspect_map_region_plane_index,
                inspect_map_region_x,
                inspect_map_region_y,
                inspect_map_region_width,
                inspect_map_region_height);
            return 1;
        }

        printf("map%zu plane%zu region%zu,%zu size: %zux%zu\n",
            inspect_map_region_map_index,
            inspect_map_region_plane_index,
            inspect_map_region_x,
            inspect_map_region_y,
            inspect_map_region_width,
            inspect_map_region_height);
        printf("map%zu plane%zu region%zu,%zu cells: [0,0]=%u [%zu,0]=%u [0,%zu]=%u [%zu,%zu]=%u\n",
            inspect_map_region_map_index,
            inspect_map_region_plane_index,
            inspect_map_region_x,
            inspect_map_region_y,
            region_words[0],
            inspect_map_region_width - 1,
            region_words[inspect_map_region_width - 1],
            inspect_map_region_height - 1,
            region_words[(inspect_map_region_height - 1) * inspect_map_region_width],
            inspect_map_region_width - 1,
            inspect_map_region_height - 1,
            region_words[region_word_count - 1]);
        return 0;
    }

    if (inspect_map_plane_header)
    {
        if (!wolf_is_valid_data_dir(data_path, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        if (!wolf_read_map_summary(data_path, inspect_map_plane_header_map_index, &map_summary, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        if (!wolf_read_map_plane_header(data_path,
                inspect_map_plane_header_map_index,
                inspect_map_plane_header_index,
                &plane_header,
                error_buffer,
                sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        printf("map%zu plane%zu header valid: %s\n",
            inspect_map_plane_header_map_index,
            inspect_map_plane_header_index,
            (wolf_map_header_is_valid(&map_summary) && wolf_map_planes_are_in_bounds(&map_summary)) ? "yes" : "no");
        printf("map%zu plane%zu offset: %u\n", inspect_map_plane_header_map_index, inspect_map_plane_header_index, plane_header.offset);
        printf("map%zu plane%zu length: %u\n", inspect_map_plane_header_map_index, inspect_map_plane_header_index, plane_header.length);
        printf("map%zu plane%zu carmack expanded bytes: %u\n", inspect_map_plane_header_map_index, inspect_map_plane_header_index, plane_header.carmack_expanded_bytes);
        printf("map%zu plane%zu rlew expanded bytes: %u\n", inspect_map_plane_header_map_index, inspect_map_plane_header_index, plane_header.rlew_expanded_bytes);
        printf("map%zu plane%zu decoded words: %zu\n", inspect_map_plane_header_map_index, inspect_map_plane_header_index, plane_header.decoded_words);
        return 0;
    }

    if (inspect_map_plane_table_catalog)
    {
        size_t loaded_count = 0;
        size_t map_index;

        if (!wolf_is_valid_data_dir(data_path, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        if (!wolf_read_map_plane_table_catalog(data_path,
                inspect_map_plane_table_catalog_count,
                plane_table_catalog,
                (sizeof(plane_table_catalog) / sizeof(plane_table_catalog[0])),
                &loaded_count,
                &maphead_summary,
                error_buffer,
                sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        printf("map plane table catalog total: %zu\n", maphead_summary.map_count);
        for (map_index = 0; map_index < loaded_count; ++map_index)
        {
            size_t plane_index;

            printf("map%zu plane table name: %s\n", map_index, plane_table_catalog[map_index].summary.name);
            printf("map%zu plane table header valid: %s\n",
                map_index,
                wolf_map_plane_headers_are_valid(&plane_table_catalog[map_index].summary, plane_table_catalog[map_index].headers) ? "yes" : "no");
            for (plane_index = 0; plane_index < 3; ++plane_index)
            {
                printf("map%zu plane%zu table offset: %u length: %u carmack: %u rlew: %u words: %zu\n",
                    map_index,
                    plane_index,
                    plane_table_catalog[map_index].headers[plane_index].offset,
                    plane_table_catalog[map_index].headers[plane_index].length,
                    plane_table_catalog[map_index].headers[plane_index].carmack_expanded_bytes,
                    plane_table_catalog[map_index].headers[plane_index].rlew_expanded_bytes,
                    plane_table_catalog[map_index].headers[plane_index].decoded_words);
            }
        }
        return 0;
    }

    if (inspect_map_plane_table)
    {
        size_t plane_index;
        wolf_map_plane_table plane_table;

        if (!wolf_is_valid_data_dir(data_path, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        if (!wolf_read_map_plane_table(data_path, inspect_map_plane_table_index, &plane_table, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        printf("map%zu plane table header valid: %s\n",
            inspect_map_plane_table_index,
            (wolf_map_header_is_valid(&plane_table.summary) && wolf_map_planes_are_in_bounds(&plane_table.summary)) ? "yes" : "no");
        for (plane_index = 0; plane_index < 3; ++plane_index)
        {
            printf("map%zu plane%zu table offset: %u length: %u carmack: %u rlew: %u words: %zu\n",
                inspect_map_plane_table_index,
                plane_index,
                plane_table.headers[plane_index].offset,
                plane_table.headers[plane_index].length,
                plane_table.headers[plane_index].carmack_expanded_bytes,
                plane_table.headers[plane_index].rlew_expanded_bytes,
                plane_table.headers[plane_index].decoded_words);
        }
        return 0;
    }

    if (inspect_present_map_plane_table_catalog)
    {
        size_t loaded_count = 0;
        size_t map_index;

        if (!wolf_is_valid_data_dir(data_path, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        if (!wolf_read_present_map_plane_table_catalog(data_path,
                inspect_present_map_plane_table_catalog_count,
                present_plane_table_catalog,
                (sizeof(present_plane_table_catalog) / sizeof(present_plane_table_catalog[0])),
                &loaded_count,
                &map_presence_summary,
                error_buffer,
                sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        printf("present map plane table catalog total slots: %zu\n", map_presence_summary.total_slots);
        printf("present map plane table catalog total present: %zu\n", map_presence_summary.present_slots);
        for (map_index = 0; map_index < loaded_count; ++map_index)
        {
            size_t plane_index;

            printf("present map plane table%zu slot: %zu\n", map_index, present_plane_table_catalog[map_index].slot_index);
            printf("present map plane table%zu name: %s\n", map_index, present_plane_table_catalog[map_index].table.summary.name);
            printf("present map plane table%zu header valid: %s\n",
                map_index,
                wolf_map_plane_headers_are_valid(&present_plane_table_catalog[map_index].table.summary, present_plane_table_catalog[map_index].table.headers) ? "yes" : "no");
            for (plane_index = 0; plane_index < 3; ++plane_index)
            {
                printf("present map plane%zu table%zu offset: %u length: %u carmack: %u rlew: %u words: %zu\n",
                    plane_index,
                    map_index,
                    present_plane_table_catalog[map_index].table.headers[plane_index].offset,
                    present_plane_table_catalog[map_index].table.headers[plane_index].length,
                    present_plane_table_catalog[map_index].table.headers[plane_index].carmack_expanded_bytes,
                    present_plane_table_catalog[map_index].table.headers[plane_index].rlew_expanded_bytes,
                    present_plane_table_catalog[map_index].table.headers[plane_index].decoded_words);
            }
        }
        return 0;
    }

    if (inspect_present_map_plane_table)
    {
        wolf_present_map_summary present_map;
        size_t plane_index;

        if (!wolf_is_valid_data_dir(data_path, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        if (!wolf_read_present_map_plane_headers(data_path,
                inspect_present_map_plane_table_index,
                &present_map,
                plane_headers,
                &map_presence_summary,
                error_buffer,
                sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        printf("present map plane table index: %zu\n", inspect_present_map_plane_table_index);
        printf("present map plane table slot: %zu\n", present_map.slot_index);
        printf("present map plane table name: %s\n", present_map.summary.name);
        printf("present map plane table header valid: %s\n",
            wolf_map_plane_headers_are_valid(&present_map.summary, plane_headers) ? "yes" : "no");
        for (plane_index = 0; plane_index < 3; ++plane_index)
        {
            printf("present map plane%zu table offset: %u length: %u carmack: %u rlew: %u words: %zu\n",
                plane_index,
                plane_headers[plane_index].offset,
                plane_headers[plane_index].length,
                plane_headers[plane_index].carmack_expanded_bytes,
                plane_headers[plane_index].rlew_expanded_bytes,
                plane_headers[plane_index].decoded_words);
        }
        return 0;
    }

    if (inspect_first_map_plane)
    {
        if (!wolf_is_valid_data_dir(data_path, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        if (!wolf_load_first_map_plane_words(data_path, inspect_first_map_plane_index, plane_words, (sizeof(plane_words) / sizeof(plane_words[0])), &plane_load_result, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        printf("plane%zu compressed bytes: %u\n", inspect_first_map_plane_index, plane_load_result.compressed_bytes);
        printf("plane%zu carmack expanded bytes: %u\n", inspect_first_map_plane_index, plane_load_result.carmack_expanded_bytes);
        printf("plane%zu rlew expanded bytes: %u\n", inspect_first_map_plane_index, plane_load_result.rlew_expanded_bytes);
        printf("plane%zu decoded words: %zu\n", inspect_first_map_plane_index, plane_load_result.decoded_words);
        printf("plane%zu cells: [0,0]=%u [31,31]=%u [32,32]=%u [63,63]=%u\n",
            inspect_first_map_plane_index,
            plane_words[0],
            plane_words[(31 * 64) + 31],
            plane_words[(32 * 64) + 32],
            plane_words[(63 * 64) + 63]);
        return 0;
    }

    if (inspect_map_plane)
    {
        if (!wolf_is_valid_data_dir(data_path, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        if (!wolf_load_map_plane_words(data_path, inspect_map_plane_map_index, inspect_map_plane_index, plane_words, (sizeof(plane_words) / sizeof(plane_words[0])), &plane_load_result, error_buffer, sizeof(error_buffer)))
        {
            fputs(error_buffer, stderr);
            fputc('\n', stderr);
            return 1;
        }

        printf("map%zu plane%zu compressed bytes: %u\n", inspect_map_plane_map_index, inspect_map_plane_index, plane_load_result.compressed_bytes);
        printf("map%zu plane%zu carmack expanded bytes: %u\n", inspect_map_plane_map_index, inspect_map_plane_index, plane_load_result.carmack_expanded_bytes);
        printf("map%zu plane%zu rlew expanded bytes: %u\n", inspect_map_plane_map_index, inspect_map_plane_index, plane_load_result.rlew_expanded_bytes);
        printf("map%zu plane%zu decoded words: %zu\n", inspect_map_plane_map_index, inspect_map_plane_index, plane_load_result.decoded_words);
        printf("map%zu plane%zu cells: [0,0]=%u [31,31]=%u [32,32]=%u [63,63]=%u\n",
            inspect_map_plane_map_index,
            inspect_map_plane_index,
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
