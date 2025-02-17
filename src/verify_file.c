#include "verify_file.h"

#include <string.h>
#if defined(__SSE2__)
#include <x86intrin.h>
#endif
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

__attribute__((hot, always_inline))
static inline bool check_whole_board_is_dots_and_os_except_newlines_no_simd(const struct board_information *board_info)
{
    for (size_t i = 0; i < board_info->num_rows; ++i) {
        char *row = board_info->board + i * board_info->num_cols;
        if (row[board_info->num_cols - 1] != '\n')
            return false;
        if (strspn(row, "o.") != board_info->num_cols - 1)
            return false;
    }
    return true;
}

__attribute__((hot))
static bool check_whole_board_is_dots_and_os_except_newlines(const struct board_information *board_info)
{
    if (board_info->num_cols < 17)
        return check_whole_board_is_dots_and_os_except_newlines_no_simd(board_info);
#if !defined(__SSE2__)
    return check_whole_board_is_dots_and_os_except_newlines_no_simd(board_info);
#else
    size_t i = 0;

#if !defined(__SSE4_1__)
    const __m128i dots = _mm_set1_epi8('.');
    const __m128i os = _mm_set1_epi8('o');

    for (; i < (board_info->num_rows * board_info->num_cols) - 16; i += 16) {
        __m128i row = _mm_loadu_si128((__m128i *)(board_info->board + i));
        __m128i row_dots = _mm_cmpeq_epi8(row, dots);
        __m128i row_os = _mm_cmpeq_epi8(row, os);
        __m128i row_dots_or_os = _mm_or_si128(row_dots, row_os);
        uint16_t mask = _mm_movemask_epi8(row_dots_or_os);
        if (mask != 0xffff) {
            // Make sure the non o/dot characters are newlines, and that they are at the right position
            size_t non_dot_i_pos = i + __builtin_ctz(~mask);
            if (board_info->board[non_dot_i_pos] != '\n')
                return false;
            if (non_dot_i_pos % board_info->num_cols != board_info->num_cols - 1)
                return false;
            i = non_dot_i_pos + 1 - 16;
        }
    }
#else
    const __m128i cmpistri_mask = _mm_set_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '.', 'o');

    for (; i < (board_info->num_rows * board_info->num_cols) - 16; i += 16) {
        __m128i row = _mm_loadu_si128((__m128i *)(board_info->board + i));
        uint32_t cmpstri_result = _mm_cmpistri(cmpistri_mask, row, _SIDD_NEGATIVE_POLARITY | _SIDD_UBYTE_OPS | _SIDD_CMP_EQUAL_ANY | _SIDD_LEAST_SIGNIFICANT);
        bool carry = _mm_cmpistrc(cmpistri_mask, row, _SIDD_NEGATIVE_POLARITY | _SIDD_UBYTE_OPS | _SIDD_CMP_EQUAL_ANY | _SIDD_LEAST_SIGNIFICANT);
        if (carry) {
            // Make sure the non o/dot characters are newlines, and that they are at the right position
            size_t non_dot_i_pos = i + cmpstri_result;
            if (board_info->board[non_dot_i_pos] != '\n')
                return false;
            if (non_dot_i_pos % board_info->num_cols != board_info->num_cols - 1)
                return false;
            i = non_dot_i_pos + 1 - 16;
        }
    }
#endif

    for (i = (board_info->num_rows * board_info->num_cols) - 16; i < board_info->num_rows * board_info->num_cols; ++i)
        if (board_info->board[i] != 'o' && board_info->board[i] != '.' &&
            (board_info->board[i] != '\n' || i % board_info->num_cols != board_info->num_cols - 1))
               return false;

    for (i = 0; i < board_info->num_rows; ++i)
        if (board_info->board[i * board_info->num_cols + board_info->num_cols - 1] != '\n')
            return false;
    return true;

#endif
}

__attribute__((hot))
bool verify_file(struct loaded_file *file_as_buffer,
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

    size_t expected_size = board_info->num_rows * board_info->num_cols;
    if (expected_size != (file_as_buffer->size - (board_info->board - file_as_buffer->data)))
        return false;

    return check_whole_board_is_dots_and_os_except_newlines_no_simd(board_info); // The SIMD version is still slower than strspn 😭
}
