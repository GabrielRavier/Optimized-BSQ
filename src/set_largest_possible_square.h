#pragma once

#include "board_information.h"

// This function modifies the passed board_info by replacing every '.' in the
// largest possible square (that does not contain 'o' obstacles) with 'x'.
void set_largest_possible_square(const struct board_information *board_info);
