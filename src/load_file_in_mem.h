#pragma once

#include <stddef.h>
#include <stdbool.h>

struct loaded_file {
    char *data;
    size_t size;
};

// Returns the contents of the file at filename in the returned buffer
bool load_file_in_mem(struct loaded_file *result, const char *filename);
