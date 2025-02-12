#pragma once

#include "loaded_file.h"
#include "board_information.h"
#include <stdbool.h>

// Checks that the file is in the correct format and fills in the board_info struct with the board information
bool verify_file(struct loaded_file *file_as_buffer,
		 struct board_information *board_info);
