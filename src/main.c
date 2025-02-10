#include "set_largest_possible_square.h"
#include "load_file_in_mem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdbool.h>

static const int ERROR_EXIT_CODE = 84;
static const int NO_ERROR_EXIT_CODE = 0;

__attribute__((hot))
static bool verify_file(struct loaded_file *file_as_buffer,
    struct board_information *board_info)
{
    char *after_number;
    board_info->num_rows = strtol(file_as_buffer->data, &after_number, 0);
    if (*after_number != '\n' || after_number == file_as_buffer->data)
        return false;
    board_info->board = after_number + 1;

    char *memchr_result = memchr(board_info->board, '\n', file_as_buffer->size - (board_info->board - file_as_buffer->data));
    if (memchr_result == NULL)
        return false;
    board_info->num_cols = memchr_result - board_info->board + 1;

    for (size_t i = 0; i < board_info->num_rows; ++i) {
        char *row = board_info->board + i * board_info->num_cols;
        if (strspn(row, ".o") != board_info->num_cols - 1)
            return false;
        if (row[board_info->num_cols - 1] != '\n')
            return false;
    }
    if (board_info->num_rows * board_info->num_cols != (file_as_buffer->size - (board_info->board - file_as_buffer->data)))
        return false;
    return true;
}

__attribute__((format(printf, 1, 2))) static int error(const char *format, ...)
{
    va_list args;

    fputs("error: ", stderr);
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    return ERROR_EXIT_CODE;
}

int main(int argc, char *argv[])
{
    struct loaded_file file_as_buffer;
    struct board_information board_info;

    if (argc != 2)
        return error("Incorrect amount of arguments (should be 2, was %d)\n",
            argc);
    if (!load_file_in_mem(&file_as_buffer, argv[1]))
        return error("Failure to read file '%s'\n", argv[1]);
    if (!verify_file(&file_as_buffer, &board_info)) {
        free(file_as_buffer.data);
        return error("Invalid board in '%s'\n", argv[1]);
    }
    set_largest_possible_square(&board_info);
    write(STDOUT_FILENO, board_info.board,
        board_info.num_cols * board_info.num_rows);
    free(file_as_buffer.data);
    return NO_ERROR_EXIT_CODE;
}
