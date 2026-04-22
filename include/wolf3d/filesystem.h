#ifndef WOLF3D_FILESYSTEM_H
#define WOLF3D_FILESYSTEM_H

#include <stdbool.h>
#include <stddef.h>

bool wolf_is_valid_data_dir(const char *path, char *error_buffer, size_t error_buffer_size);
const char *wolf_default_data_dir(void);

#endif
