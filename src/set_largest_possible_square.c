/*
** EPITECH PROJECT, 2020
** BSQ
** File description:
** Implements set_largest_possible_square
*/

#include "set_largest_possible_square.h"
#include "my/string.h"
#include <stdlib.h>
#include <stdbool.h>

struct square {
    size_t i;
    size_t j;
    size_t size;
};

static int *make_obstacle_amounts(const struct board_information *board_info)
{
    int *result = malloc(sizeof(int) *
         ((board_info->num_cols - 1) * board_info->num_rows));
    for (size_t i = 0; i < board_info->num_rows; ++i)
        for (size_t j = 0; j < (board_info->num_cols - 1); ++j)
            result[i * (board_info->num_cols - 1) + j] =
                ((i == 0) ? 0 :
                    result[(i - 1) * (board_info->num_cols - 1) + j]) +
                ((j == 0) ? 0 :
                    result[i * (board_info->num_cols - 1) + (j - 1)]) -
                ((i == 0 || j == 0) ? 0 :
                    result[(i - 1) * (board_info->num_cols - 1) + (j - 1)]) +
                (board_info->board[i * board_info->num_cols + j] == 'o');
    return result;
}

static bool check_valid_square(const struct square *square,
    const int *obstacle_amounts, const struct board_information *board_info)
{
    int obstacles_bottom_right = obstacle_amounts[(square->i + square->size) *
        (board_info->num_cols - 1) + (square->j + square->size)];
    int obstacles_bottom_left = (square->j == 0 ? 0 : obstacle_amounts[
        (square->i + square->size) * (board_info->num_cols - 1) +
        (square->j - 1)]);
    int obstacles_top_right = (square->i == 0 ? 0 : obstacle_amounts[
        (square->i - 1) * (board_info->num_cols - 1) +
        (square->j + square->size)]);
    int obstacles_top_left = ((square->i == 0 || square->j == 0) ? 0 :
        obstacle_amounts[(square->i - 1) * (board_info->num_cols - 1) +
        (square->j - 1)]);
    
    return (obstacles_bottom_right - obstacles_bottom_left -
        obstacles_top_right + obstacles_top_left) <= 0;
}

static void set_square(const struct board_information *board_info,
    const struct square *square)
{
    for (size_t i = 0; i < square->size; ++i)
        my_memset(&board_info->board[(square->i + i) *
            board_info->num_cols + square->j], 'x', square->size);
}

static void do_i_iteration(struct square *largest_square,
    const struct board_information *board_info, const int *obstacle_amounts,
    size_t i)
{
    for (size_t j = 0; j < (board_info->num_cols - 1); ++j) {
        while ((largest_square->size < (board_info->num_rows - i)) &&
               (largest_square->size < (board_info->num_cols - 1 - j)) &&
               check_valid_square(&((const struct square){i, j,
                           largest_square->size}),
                   obstacle_amounts, board_info)) {
            largest_square->i = i;
            largest_square->j = j;
            ++largest_square->size;
        }
    }
}

void set_largest_possible_square(const struct board_information *board_info)
{
    struct square largest_square = {0, 0, 0};
    int *obstacle_amounts = make_obstacle_amounts(board_info);

    for (size_t i = 0; i < board_info->num_rows; ++i) {
        if (largest_square.size >= (board_info->num_rows - i))
            break;
        do_i_iteration(&largest_square, board_info, obstacle_amounts, i);
    }
    set_square(board_info, &largest_square);
    free(obstacle_amounts);
}
