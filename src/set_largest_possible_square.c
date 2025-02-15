#include "set_largest_possible_square.h"
#include <string.h>
#include <assert.h>
#include <x86intrin.h>
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

__attribute__((hot))
static int min3(uint32_t x, uint32_t y, uint32_t z)
{
    if (x > y)
        return (y < z) ? y : z;
    return (x < z) ? x : z;
}

__attribute__((hot))
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
}

// If there is a value in the array that is larger than the current maximum, the maximum value is updated to that value
// Note that the position shall always be that of the earliest maximum value in the array - that is, if there are multiple samples of the maximum value, the index of the first one must be returned
__attribute__((hot))
static void find_u32_arr_larger_with_pos(const uint32_t *arr, size_t size, uint32_t *max, uint32_t *pos)
{
    if (size == 0)
        return;
#if !defined(__SSE2__)
    for (size_t i = 0; i < size; ++i) {
        if (arr[i] > *max) {
            *max = arr[i];
            *pos = i;
        }
    }
#else
    __m128i *arr128 = (__m128i *)arr;
    __m128i *arr128_end = (__m128i *)(arr + size - 4);

    // We cache the value of _mm_set1_epi32(*max) to avoid having to reload it on every loop iteration (we only need to reload it when *max is updated)
    __m128i max_splatted = _mm_set1_epi32(*max);

    for (; arr128 < arr128_end; ++arr128) {
        // First load the values
        __m128i values = _mm_loadu_si128(arr128);

        // Check if any of the values are larger than the current maximum
        __m128i cmp = _mm_cmpgt_epi32(values, max_splatted);

        if (_mm_movemask_epi8(cmp)) {
            // Find the largest value in those and update the maximum and position
            for (size_t i = 0; i < 4; ++i)
                if (((uint32_t *)&values)[i] > *max) {
                    *max = ((uint32_t *)&values)[i];
                    *pos = (uint32_t *)arr128 - arr + i;
                }

            // *max has been updated, so max_splatted is now. Update it too.
            max_splatted = _mm_set1_epi32(*max);
        }
    }

    for (size_t i = (uint32_t *)arr128 - arr; i < size; ++i) {
        if (arr[i] > *max) {
            *max = arr[i];
            *pos = i;
        }
    }
#endif
}

__attribute__((hot))
static void check_line(const struct board_information *board_info, uint32_t square_size_values[2][board_info->num_cols - 1], struct solver *solver)
{
    for (solver->x = 0; solver->x < board_info->num_cols - 1; ++solver->x)
        check_square(board_info->board[solver->y * board_info->num_cols + solver->x], square_size_values[solver->y & 1], square_size_values[(solver->y + 1) & 1], solver);

    uint32_t largest_square_size = solver->best.size;
    uint32_t largest_square_x;
    find_u32_arr_larger_with_pos(square_size_values[solver->y & 1], board_info->num_cols - 1, &largest_square_size, &largest_square_x);

    if (largest_square_size > solver->best.size) {
        solver->best.size = largest_square_size;
        solver->best.x = largest_square_x;
        solver->best.y = solver->y;
    }
}

__attribute__((hot))
static void find_largest_possible_square(const struct board_information *board_info, struct solver *solver)
{
    uint32_t square_size_values[2][board_info->num_cols - 1]; // vla moment (maybe revise this later lol)
    memset(square_size_values, 0, sizeof(square_size_values));

    for (solver->y = 0; solver->y < board_info->num_rows; ++solver->y)
        check_line(board_info, square_size_values, solver);
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
