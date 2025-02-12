#pragma once

#include "loaded_file.h"
#include <stdbool.h>

// Returns the contents of the file at filename in the returned buffer
bool load_file_in_mem(struct loaded_file *result, const char *filename);
