#include "verify_file.h"

#include <string.h>
#if defined(__SSE2__)
#include <x86intrin.h>
#endif
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

__attribute__((hot))
static bool check_only_two_chars_in_range(char *start, size_t size, char c1, char c2)
{
#if defined(__SSE4_2__)
    __m128i mask = _mm_set_epi8(
        0, 0, 0, 0, 0, 0, 0,  0,
        0, 0, 0, 0, 0, 0, c2, c1
    );

    __m128i *start_m128i = (__m128i *)start;
    __m128i *end_m128i = (__m128i *)(start + size - 16);

    for (; start_m128i < end_m128i; ++start_m128i) {
        __m128i data = _mm_loadu_si128(start_m128i);
        uint32_t result = _mm_cmpistri(mask, data, _SIDD_NEGATIVE_POLARITY | _SIDD_UBYTE_OPS | _SIDD_CMP_EQUAL_ANY | _SIDD_LEAST_SIGNIFICANT);
        bool carry = _mm_cmpistrc(mask, data, _SIDD_NEGATIVE_POLARITY | _SIDD_UBYTE_OPS | _SIDD_CMP_EQUAL_ANY | _SIDD_LEAST_SIGNIFICANT);
        if (carry)
            return false;
    }

    size -= (char *)start_m128i - start;
    start = (char *)start_m128i;
#elif defined(__SSE2__)
    __m128i c1_mask = _mm_set1_epi8(c1);
    __m128i c2_mask = _mm_set1_epi8(c2);
    __m128i *start_m128i = (__m128i *)start;
    __m128i *end_m128i = (__m128i *)(start + size - 16);

    for (; start_m128i < end_m128i; ++start_m128i) {
        __m128i data = _mm_loadu_si128(start_m128i);
        __m128i c1_result = _mm_cmpeq_epi8(data, c1_mask);
        __m128i c2_result = _mm_cmpeq_epi8(data, c2_mask);
        __m128i or_result = _mm_or_si128(c1_result, c2_result);
        if (_mm_movemask_epi8(or_result) != 0xFFFF)
            return false;
    }

    size -= (char *)start_m128i - start;
    start = (char *)start_m128i;
#else
#define CHECK_ONLY_TWO_CHARS_IN_RANGE_SLOWER_THAN_STRSPN
#endif
    for (size_t i = 0; i < size; ++i)
        if (start[i] != c1 && start[i] != c2)
            return false;
    return true;
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

    for (size_t i = 0; i < board_info->num_rows; ++i) {
        char *row = board_info->board + i * board_info->num_cols;
        if (row[board_info->num_cols - 1] != '\n')
            return false;
#if 0 //!defined(CHECK_ONLY_TWO_CHARS_IN_RANGE_SLOWER_THAN_STRSPN) // I've tried pretty hard but it looks like glibc's strspn is just too good
        if (!check_only_two_chars_in_range(row, board_info->num_cols - 1, 'o', '.'))
            return false;
#else
        if (strspn(row, "o.") != board_info->num_cols - 1)
            return false;
#endif
    }
    return true;
}

