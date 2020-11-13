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

static int *make_obstable_amounts(const struct board_information *board_info)
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

static bool check_square(size_t i, size_t j, int largest_square_size,
    const int *obstacle_amounts, const struct board_information *board_info)
{
    int obstacles_bottom_right = obstacle_amounts[(i + largest_square_size) *
        board_info->num_cols + (j + largest_square_size)];
    int obstacles_bottom_left = (j == 0 ? 0 : obstacle_amounts[
        (i + largest_square_size) * board_info->num_cols + (j - 1)]);
    int obstacles_top_right = (i == 0 ? 0 : obstacle_amounts[(i - 1) *
        board_info->num_cols + (j + largest_square_size)]);
    int obstacles_top_left = ((i == 0 || j == 0) ? 0 : obstacle_amounts[
        (i - 1) * board_info->num_cols + (j - 1)]);
    
    return (obstacles_bottom_right - obstacles_bottom_left -
        obstacles_top_right + obstacles_top_left) <= 0;
}

void set_largest_possible_square(const struct board_information *board_info)
{
    size_t largest_square_i = 0;
    size_t largest_square_j = 0;
    int largest_square_size = 0;
    int *obstacle_amounts = make_obstable_amounts(board_info);

    for (size_t i = 0; i < board_info->num_rows; ++i) {
        if (largest_square_size >= (board_info->num_rows - i))
            break;
        for (size_t j = 0; j < (board_info->num_cols - 1); ++j) {
            while ((largest_square_size < (board_info->num_rows - i)) &&
                (largest_square_size < (board_info->num_cols - 1 - j)) &&
                check_square(i, j, largest_square_size, obstacle_amounts,
                    board_info)) {
                largest_square_i = i;
                largest_square_j = j;
                ++largest_square_size;
            }
        }
    }
    for (size_t i = 0; i < largest_square_size; ++i)
        my_memset(&board_info->board[(largest_square_i + i) *
            board_info->num_cols + largest_square_j], 'x', largest_square_size);
    free(obstacle_amounts);
}
