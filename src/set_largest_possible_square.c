#include "set_largest_possible_square.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

// Note: Parts of this code (the basic algorithm used in this file, basically - though much of the optimization is mine) are based on code by Yohann Boniface,
// at https://gist.github.com/Sigmanificient/05387fa40bf9f4e38cc1da7727ac382b
// (that code doesn't actually work even on the 10000x10000 boards it's supposed to work on, but it's a good starting point :p)

struct square {
    uint32_t size;
    uint32_t x;
    uint32_t y;
};

struct solver {
    uint32_t x;
    uint32_t y;
    struct square best;
};

static int min3(uint32_t x, uint32_t y, uint32_t z)
{
    if (x > y)
        return (y < z) ? y : z;
    return (x < z) ? x : z;
}

static void check_square(const char processed_square, uint32_t *restrict square_size_values, uint32_t *restrict prev_row_square_size_values, struct solver *solver)
{
    register uint32_t square_size;

    if (processed_square == 'o') {
        square_size_values[solver->x] = 0;
        return;
    }

    square_size = 1;
    if (solver->x != 0 && solver->y != 0)
        square_size += min3(
            prev_row_square_size_values[solver->x - 1],
            prev_row_square_size_values[solver->x],
            square_size_values[solver->x - 1]
        );

    square_size_values[solver->x] = square_size;

    if (solver->best.size < square_size)
        solver->best = (struct square) { square_size, solver->x, solver->y };
}

static void find_largest_possible_square(const struct board_information *board_info, struct solver *solver)
{
    uint32_t square_size_values[2][board_info->num_cols - 1]; // vla moment (maybe revise this later lol)
    memset(square_size_values, 0, sizeof(square_size_values));

    for (solver->y = 0; solver->y < board_info->num_rows; ++solver->y)
        for (solver->x = 0; solver->x < board_info->num_cols - 1; ++solver->x)
            check_square(board_info->board[solver->y * board_info->num_cols + solver->x], square_size_values[solver->y & 1], square_size_values[(solver->y + 1) & 1], solver);
}

static void fill_square(const struct board_information *board_info, struct solver solver)
{
    for (uint32_t i = 0; i < solver.best.size; ++i)
        memset(board_info->board + (solver.best.y - i) * board_info->num_cols + solver.best.x - solver.best.size + 1, 'x', solver.best.size);
}

void set_largest_possible_square(const struct board_information *board_info)
{
    struct solver solver = {};
    find_largest_possible_square(board_info, &solver);
    fill_square(board_info, solver);
}
