/*
** EPITECH PROJECT, 2020
** BSQ
** File description:
** Implements the main function
*/

#include "load_file_in_mem.h"
#include "my/stdio.h"
#include "my/stdlib.h"
#include "my/string.h"
#include "my/macros.h"
#include <unistd.h>
#include <stdbool.h>

// board is a pointer to the board
// num_rows is the amount of rows in the board
// num_cols is the amount of columns in every row, including the newline
// character
struct board_information
{
    char *board;
    size_t num_rows;
    size_t num_cols;
};

static const int ERROR_EXIT_CODE = 84;

static bool verify_file(struct my_string *file_as_string,
    struct board_information *board_info)
{
    char *after_number;
    board_info->num_rows = my_strtol_base_str(file_as_string->string,
        &after_number, "0123456789");
    if (*after_number != '\n' || after_number == file_as_string->string)
        return false;
    board_info->board = after_number + 1;
    board_info->num_cols = my_strcspn(board_info->board, "\n") + 1;
    for (int i = 0; i < board_info->num_rows * board_info->num_cols; ++i)
        if (((i + 1) % board_info->num_cols) == 0 && (i != 0)) {
            if (board_info->board[i] != '\n')
                return false;
        }
        else
            if (board_info->board[i] != 'o' && board_info->board[i] != '.')
                return false;
    if (board_info->board[board_info->num_rows * board_info->num_cols] != '\0')
        return false;
    return true;
}

int main(int argc, char *argv[])
{
    struct my_string *file_as_string;
    struct board_information board_info;

    if (argc != 2) {
        my_dprintf(STDERR_FILENO,
            "Incorrect amount of arguments (should be 2, was %d)\n", argc);
        return ERROR_EXIT_CODE;
    }
    file_as_string = load_file_in_mem(argv[1]);
    if (!file_as_string) {
        my_dprintf(STDERR_FILENO, "Failure to read file '%s'\n", argv[1]);
        return ERROR_EXIT_CODE;
    }
    if (!verify_file(file_as_string, &board_info)) {
        my_dprintf(STDERR_FILENO, "Invalid board in '%s'\n", argv[1]);
        my_string_free(file_as_string);
        return ERROR_EXIT_CODE;
    }
}
