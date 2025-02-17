#include "set_largest_possible_square.h"
#include <string.h>
#include <assert.h>
#include <x86intrin.h>
#include <stdint.h>
#include <stdbool.h>

// Note: Parts of this code (the basic algorithm used in this file, basically - though much of the optimization is mine) are based on code by Yohann Boniface,
// at https://gist.github.com/Sigmanificient/05387fa40bf9f4e38cc1da7727ac382b
// (that code doesn't actually work even on the 10000x10000 boards it's supposed to work on, but it's a good starting point :p)

typedef uint32_t uint32_may_alias_t __attribute__((may_alias));

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

__attribute__((hot, always_inline))
static inline uint32_t min(uint32_t x, uint32_t y)
{
    return x < y ? x : y;
}

// Returns the resulting square - so that it can be passed back as the previous square size value for the next square
__attribute__((hot, always_inline))
static inline uint32_t check_square(const char processed_square, uint32_t *restrict square_size_values, uint32_t *restrict prev_row_square_size_values, struct solver *solver, uint32_t prev_square_size_value)
{
    register uint32_t square_size;

    if (processed_square == 'o')
        return 0; // We don't need to store a 0 because memset has already set the entire array to 0. We do things this way because it happens to be faster

    // We have already precalculated min(prev_row_square_size_values[solver->x - 1], prev_row_square_size_values[solver->x]) for every valid x in set_each_val_to_min_of_itself_and_previous_val,
    // so we can just use that precalculated value here instead of recalculating it
    // (We do that because we can precalculate it using SIMD instructions, which is faster than doing it here)
    square_size = 1 + min(
        prev_row_square_size_values[solver->x],
        prev_square_size_value
    );

    square_size_values[solver->x] = square_size;
    return square_size;
}

// If there is a value in the array that is larger than the current maximum, the maximum value is updated to that value
// Note that the position shall always be that of the earliest maximum value in the array - that is, if there are multiple samples of the maximum value, the index of the first one must be returned
__attribute__((hot, always_inline))
static inline void find_u32_arr_larger_with_pos(const uint32_t *arr, size_t size, uint32_t *max, uint32_t *pos)
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
#elif !defined(__AVX2__)
    const __m128i *arr128 = (const __m128i *)arr;
    const __m128i *arr128_end = (const __m128i *)(arr + size - 4);

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
                if (((const uint32_may_alias_t *)&values)[i] > *max) {
                    *max = ((const uint32_may_alias_t *)&values)[i];
                    *pos = (const uint32_t *)arr128 - arr + i;
                }

            // *max has been updated, so max_splatted is now. Update it too.
            max_splatted = _mm_set1_epi32(*max);
        }
    }

    for (size_t i = (const uint32_t *)arr128 - arr; i < size; ++i) {
        if (arr[i] > *max) {
            *max = arr[i];
            *pos = i;
        }
    }
#else
    const __m256i *arr256 = (const __m256i *)arr;
    const __m256i *arr256_end = (const __m256i *)(arr + size - 9);

    // We cache the value of _mm256_set1_epi32(*max) to avoid having to reload it on every loop iteration (we only need to reload it when *max is updated)
    __m256i max_splatted = _mm256_set1_epi32(*max);

    for (; arr256 < arr256_end; ++arr256) {
        // First load the values
        const __m256i values = _mm256_loadu_si256(arr256);

        // Check if any of the values are larger than the current maximum
        __m256i cmp = _mm256_cmpgt_epi32(values, max_splatted);

        if (_mm256_movemask_epi8(cmp)) {
            // Find the largest value in those and update the maximum and position
            for (size_t i = 0; i < 8; ++i)
                if (((const uint32_may_alias_t *)&values)[i] > *max) {
                    *max = ((const uint32_may_alias_t *)&values)[i];
                    *pos = (const uint32_t *)arr256 - arr + i;
                }

            // *max has been updated, so max_splatted is now. Update it too.
            max_splatted = _mm256_set1_epi32(*max);
        }
    }

    for (size_t i = (const uint32_t *)arr256 - arr; i < size; ++i) {
        if (arr[i] > *max) {
            *max = arr[i];
            *pos = i;
        }
    }
#endif
}

#ifdef __SSE2__
__attribute__((hot, always_inline))
static inline __m128i do_m128i_min_epu32(__m128i a, __m128i b)
{
#if !defined(__SSE4_1__)
    __m128i cmp = _mm_cmplt_epi32(a, b); // Note: Technically wrong but we're not working with numbers above 2**31 here so it's fine
    return _mm_or_si128(_mm_and_si128(cmp, a), _mm_andnot_si128(cmp, b));
#else
    return _mm_min_epu32(a, b);
#endif
}
#endif

// Note that we assume the "previous value" is 0 for the first element (this is correct for our purposes)
// arr[0] = min(arr[0], 0)
// arr[1] = min(arr[1], arr[0])
// arr[2] = min(arr[2], arr[1])
// ...
__attribute__((hot, always_inline))
static inline void set_each_val_to_min_of_itself_and_previous_val(uint32_t *arr, size_t size)
{
#if !defined(__SSE2__)

    uint32_t prev_val = 0;
    for (size_t i = 0; i < size; ++i) {
        uint32_t next_val = arr[i];
        arr[i] = min(next_val, prev_val);
        prev_val = next_val;
    }

#elif !defined(__AVX2__)

    // Go backwards to avoid having to store the previous value
    uint32_t *arr_iter_backwards = arr + size;
    while (arr_iter_backwards - arr >= 5) {
        arr_iter_backwards -= 4;
        __m128i current_vals = _mm_loadu_si128((__m128i *)arr_iter_backwards);
        __m128i previous_vals = _mm_loadu_si128((__m128i *)(arr_iter_backwards - 1));
        __m128i min_vals = do_m128i_min_epu32(current_vals, previous_vals);
        _mm_storeu_si128((__m128i *)arr_iter_backwards, min_vals);
    }

    for (size_t i = arr_iter_backwards - arr - 1; i > 0; --i)
        arr[i] = min(arr[i], arr[i - 1]);
    arr[0] = min(arr[0], 0);

#else

    // Go backwards to avoid having to store the previous value
    uint32_t *arr_iter_backwards = arr + size;
    while (arr_iter_backwards - arr >= 9) {
        arr_iter_backwards -= 8;
        __m256i current_vals = _mm256_loadu_si256((__m256i *)arr_iter_backwards);
        __m256i previous_vals = _mm256_loadu_si256((__m256i *)(arr_iter_backwards - 1));
        __m256i min_vals = _mm256_min_epu32(current_vals, previous_vals);
        _mm256_storeu_si256((__m256i *)arr_iter_backwards, min_vals);
    }

    for (size_t i = arr_iter_backwards - arr - 1; i > 0; --i)
        arr[i] = min(arr[i], arr[i - 1]);
    arr[0] = min(arr[0], 0);

#endif
}

__attribute__((hot, always_inline))
static inline void check_line(const struct board_information *board_info, uint32_t square_size_values[2][board_info->num_cols + 1], struct solver *solver)
{
    if (solver->y != 0)
        set_each_val_to_min_of_itself_and_previous_val(square_size_values[(solver->y + 1) & 1], board_info->num_cols); // See check_square min() call as for why we do this

    // Set the entire values array to 0 to handle 'o's (check_square specifically doesn't do so)
    memset(square_size_values[solver->y & 1], 0, sizeof(square_size_values[solver->y & 1]));

    uint32_t latest_square_size_value = 0;
    for (solver->x = 0; solver->x < board_info->num_cols - 1; ++solver->x)
        latest_square_size_value = check_square(board_info->board[solver->y * board_info->num_cols + solver->x], square_size_values[solver->y & 1], square_size_values[(solver->y + 1) & 1], solver, latest_square_size_value);

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
    uint32_t square_size_values[2][board_info->num_cols + 1]; // vla moment (maybe revise this later lol)
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
