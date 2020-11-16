/*
** EPITECH PROJECT, 2020
** BSQ
** File description:
** Implements the main function
*/

#include "set_largest_possible_square.h"
#include "load_file_in_mem.h"
#include "my/stdio.h"
#include "my/stdlib.h"
#include "my/string.h"
#include "my/macros.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

static const int ERROR_EXIT_CODE = 84;
static const int NO_ERROR_EXIT_CODE = 0;

static bool verify_file(struct my_string *file_as_string,
    struct board_information *board_info)
{
    char *after_number;
    board_info->num_rows =
        my_strtol_base_str(file_as_string->string, &after_number, "0123456789");
    if (*after_number != '\n' || after_number == file_as_string->string)
        return false;
    board_info->board = after_number + 1;
    board_info->num_cols = my_strcspn(board_info->board, "\n") + 1;
    for (size_t i = 0; i < board_info->num_rows; ++i)
        for (size_t j = 0; j < board_info->num_cols; ++j)
            if (j == (board_info->num_cols - 1)) {
                if (board_info->board[i * board_info->num_cols + j] != '\n')
                    return false;
            } else if (board_info->board[i * board_info->num_cols + j] != 'o' &&
                board_info->board[i * board_info->num_cols + j] != '.')
                return false;
    if (board_info->board[board_info->num_rows * board_info->num_cols] != '\0')
        return false;
    return true;
}

MY_ATTRIBUTE((format(printf, 1, 2))) static int error(const char *format, ...)
{
    va_list args;

    my_dputs("error: ", STDERR_FILENO);
    va_start(args, format);
    my_vdprintf(STDERR_FILENO, format, args);
    va_end(args);
    return ERROR_EXIT_CODE;
}

int main(int argc, char *argv[])
{
    struct my_string *file_as_string;
    struct board_information board_info;

    if (argc != 2)
        return error("Incorrect amount of arguments (should be 2, was %d)\n",
            argc);
    file_as_string = load_file_in_mem(argv[1]);
    if (!file_as_string)
        return error("Failure to read file '%s'\n", argv[1]);
    if (!verify_file(file_as_string, &board_info)) {
        my_string_free(file_as_string);
        return error("Invalid board in '%s'\n", argv[1]);
    }
    set_largest_possible_square(&board_info);
    write(STDOUT_FILENO, board_info.board,
        board_info.num_cols * board_info.num_rows);
    my_string_free(file_as_string);
    return NO_ERROR_EXIT_CODE;
}
