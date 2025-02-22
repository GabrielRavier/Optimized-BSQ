#pragma once

#include <stddef.h>

// board is a pointer to the board
// num_rows is the amount of rows in the board
// num_cols is the amount of columns in every row, including the newline
// character
struct board_information {
    char *board;
    size_t num_rows;
    size_t num_cols;
};
