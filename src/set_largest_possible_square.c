#include "set_largest_possible_square.h"
#include <string.h>
#include <stdlib.h>
#include <x86intrin.h>
#include <stdbool.h>

struct square {
    size_t i;
    size_t j;
    size_t size;
};

// make_obstacle_amounts returns a pointer to an array of integers, where each
// integer represents the amount of obstacles in the rectangle formed by the
// top left corner of the board and the current square.
__attribute__((hot))
static int *make_obstacle_amounts(const struct board_information *board_info)
{
    int *result = malloc(sizeof(int) *
        ((board_info->num_cols - 1) * board_info->num_rows));
    for (size_t i = 0; i < board_info->num_rows; ++i)
        for (size_t j = 0; j < (board_info->num_cols - 1); ++j) {
            int previous_row =
                (i == 0) ?
                    0 :
                result[(i - 1) * (board_info->num_cols - 1) + j];

            int previous_col =
                (j == 0) ?
                    0 :
                result[i * (board_info->num_cols - 1) + (j - 1)];

            int previous_diagonal =
                (i == 0 || j == 0) ?
                    0 :
                result[(i - 1) * (board_info->num_cols - 1) + (j - 1)];

            result[i * (board_info->num_cols - 1) + j] =
                (previous_row + previous_col - previous_diagonal) +
                (board_info->board[i * board_info->num_cols + j] == 'o');
        }
    return result;
}

static void set_square(const struct board_information *board_info,
    const struct square *square)
{
    for (size_t i = 0; i < square->size; ++i)
        memset(&board_info->board[(square->i + i) *
            board_info->num_cols + square->j], 'x', square->size);
}

__attribute__((hot))
static bool check_valid_square(const struct square *square,
    const int *obstacle_amounts, const struct board_information *board_info)
{
    int obstacles_bottom_right = obstacle_amounts[(square->i + square->size - 1) *
        (board_info->num_cols - 1) + (square->j + square->size - 1)];
    int obstacles_bottom_left = (square->j == 0 ? 0 : obstacle_amounts[
        (square->i + square->size - 1) * (board_info->num_cols - 1) +
        (square->j - 1)]);
    int obstacles_top_right = (square->i == 0 ? 0 : obstacle_amounts[
        (square->i - 1) * (board_info->num_cols - 1) +
        (square->j + square->size - 1)]);
    int obstacles_top_left = ((square->i == 0 || square->j == 0) ? 0 :
        obstacle_amounts[(square->i - 1) * (board_info->num_cols - 1) +
        (square->j - 1)]);

    return (obstacles_bottom_right - obstacles_bottom_left -
        obstacles_top_right + obstacles_top_left) == 0;
}

// find_largest_square finds the largest square that does not contain any
// obstacles. The square is represented by the top left corner and its size.
// The function modifies the passed largest_square to contain the largest
// square found.
__attribute__((hot))
void find_largest_square(struct square *largest_square,
    const struct board_information *board_info, const int *obstacle_amounts)
{
    for (size_t i = 0; i < board_info->num_rows; ++i) {
        if (largest_square->size >= (board_info->num_rows - i))
            break;

        for (size_t j = 0; j < (board_info->num_cols - 1); ++j) {
            if (largest_square->size >= (board_info->num_cols - 1 - j))
                break;

            size_t max_possible_size = (board_info->num_rows - i < board_info->num_cols - 1 - j) ?
                                        board_info->num_rows - i : board_info->num_cols - 1 - j;
            struct square current_square = { i, j, largest_square->size + 1 };
            while (current_square.size <= max_possible_size &&
                check_valid_square(&current_square, obstacle_amounts, board_info)) {
                largest_square->i = i;
                largest_square->j = j;
                largest_square->size = current_square.size;
                ++current_square.size;
            }
        }
    }
}

// The board is represented as a string of characters, with each row separated
// by a newline character. The board is a rectangle, with each row having the
// same number of columns. The board is made of two types of characters:
// - '.' represents an empty space
// - 'o' represents an obstacle
// This function modifies the passed board_info by replacing every '.' in the
// largest possible square (that does not contain obstacles) with 'x'.
void set_largest_possible_square(const struct board_information *board_info)
{
    struct square largest_square = { 0, 0, 0 };
    int *obstacle_amounts = make_obstacle_amounts(board_info);

    find_largest_square(&largest_square, board_info, obstacle_amounts);
    set_square(board_info, &largest_square);
    free(obstacle_amounts);
}
