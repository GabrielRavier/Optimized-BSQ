#include "set_largest_possible_square.h"
#include <string.h>
#include <assert.h>
#include <x86intrin.h>
#include <stdint.h>
#include <stdbool.h>

// Note: Parts of this code (the basic algorithm used in this file, basically - though much of the optimization is mine) are based on code by Yohann Boniface,
// at https://gist.github.com/Sigmanificient/05387fa40bf9f4e38cc1da7727ac382b
// (that code doesn't actually work even on the 10000x10000 boards it's supposed to work on, but it's a good starting point :p)

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

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
static inline uint32_t check_square_low_o_count(const char processed_square, uint32_t *restrict square_size_values, uint32_t *restrict prev_row_square_size_values, struct solver *solver, uint32_t prev_square_size_value)
{
    register uint32_t square_size;

    if (unlikely(processed_square == 'o')) {
        square_size_values[solver->x] = 0;
        return 0;
    }
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

// Returns the resulting square - so that it can be passed back as the previous square size value for the next square
__attribute__((hot, always_inline))
static inline uint32_t check_square_high_o_count(const char processed_square, uint32_t *restrict square_size_values, uint32_t *restrict prev_row_square_size_values, struct solver *solver, uint32_t prev_square_size_value)
{
    register uint32_t square_size;

    if (likely(processed_square == 'o')) {
        square_size_values[solver->x] = 0;
        return 0;
    }
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

// Returns the resulting square - so that it can be passed back as the previous square size value for the next square
// Branchless version of check_square for when we have an inconvenient number of 'o's (amount of 'o's vaguely equal to amount of '.'s) strewn around in random manner
__attribute__((hot, always_inline))
static inline uint32_t check_square_branchless(const char processed_square, uint32_t *restrict square_size_values, uint32_t *restrict prev_row_square_size_values, struct solver *solver, uint32_t prev_square_size_value)
{
    register uint32_t square_size;
    bool is_o = processed_square == 'o';

    // We have already precalculated min(prev_row_square_size_values[solver->x - 1], prev_row_square_size_values[solver->x]) for every valid x in set_each_val_to_min_of_itself_and_previous_val,
    // so we can just use that precalculated value here instead of recalculating it
    // (We do that because we can precalculate it using SIMD instructions, which is faster than doing it here)
    square_size = 1 + min(
        prev_row_square_size_values[solver->x],
        prev_square_size_value
    );

    square_size *= !is_o; // GCC doesn't know how to convert this into a branch, so we use this to make the function branchless
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

// From https://gist.github.com/powturbo/456edcae788a61ebe2fc and https://gist.github.com/powturbo/2b06a84b6008dfffef11e53edba297d3
#if defined(__AVX2__)
size_t count_val_in_mem(const void *s, int c, size_t n) {
    __m256i cv = _mm256_set1_epi8(c), zv = _mm256_setzero_si256(), sum = zv, acr0,acr1,acr2,acr3;
    const char *p,*pe;
	for(p = s; p != (char *)s+(n- (n % (252*32)));) {
	  for(acr0 = acr1 = acr2 = acr3 = zv,pe = p+252*32; p != pe; p += 128) {
		acr0 = _mm256_add_epi8(acr0, _mm256_cmpeq_epi8(cv, _mm256_lddqu_si256((const __m256i *)p)));
		acr1 = _mm256_add_epi8(acr1, _mm256_cmpeq_epi8(cv, _mm256_lddqu_si256((const __m256i *)(p+32))));
		acr2 = _mm256_add_epi8(acr2, _mm256_cmpeq_epi8(cv, _mm256_lddqu_si256((const __m256i *)(p+64))));
		acr3 = _mm256_add_epi8(acr3, _mm256_cmpeq_epi8(cv, _mm256_lddqu_si256((const __m256i *)(p+96)))); __builtin_prefetch(p+1024);
	  }
      sum = _mm256_add_epi64(sum, _mm256_sad_epu8(_mm256_sub_epi8(zv, acr0), zv));
      sum = _mm256_add_epi64(sum, _mm256_sad_epu8(_mm256_sub_epi8(zv, acr1), zv));
      sum = _mm256_add_epi64(sum, _mm256_sad_epu8(_mm256_sub_epi8(zv, acr2), zv));
      sum = _mm256_add_epi64(sum, _mm256_sad_epu8(_mm256_sub_epi8(zv, acr3), zv));
    }
    for(acr0=zv; p+32 < (char *)s + n; p += 32)
      acr0 = _mm256_add_epi8(acr0, _mm256_cmpeq_epi8(cv, _mm256_lddqu_si256((const __m256i *)p)));
    sum = _mm256_add_epi64(sum, _mm256_sad_epu8(_mm256_sub_epi8(zv, acr0), zv));
    size_t count = _mm256_extract_epi64(sum, 0) + _mm256_extract_epi64(sum, 1) + _mm256_extract_epi64(sum, 2) + _mm256_extract_epi64(sum, 3);
    while(p != (char *)s + n) count += *p++ == c;
    return count;
}
#elif defined(__SSE2__)
size_t count_val_in_mem(const void *s, int c, size_t n) {
    __m128i cv = _mm_set1_epi8(c), sum = _mm_setzero_si128(), acr0,acr1,acr2,acr3;
    const char *p,*pe;
	for(p = s; p != (char *)s+(n- (n % (252*16)));) {
	  for(acr0 = acr1 = acr2 = acr3 = _mm_setzero_si128(),pe = p+252*16; p != pe; p += 64) {
		acr0 = _mm_add_epi8(acr0, _mm_cmpeq_epi8(cv, _mm_loadu_si128((const __m128i *)p)));
		acr1 = _mm_add_epi8(acr1, _mm_cmpeq_epi8(cv, _mm_loadu_si128((const __m128i *)(p+16))));
		acr2 = _mm_add_epi8(acr2, _mm_cmpeq_epi8(cv, _mm_loadu_si128((const __m128i *)(p+32))));
		acr3 = _mm_add_epi8(acr3, _mm_cmpeq_epi8(cv, _mm_loadu_si128((const __m128i *)(p+48)))); __builtin_prefetch(p+1024);
	  }
      sum = _mm_add_epi64(sum, _mm_sad_epu8(_mm_sub_epi8(_mm_setzero_si128(), acr0), _mm_setzero_si128()));
      sum = _mm_add_epi64(sum, _mm_sad_epu8(_mm_sub_epi8(_mm_setzero_si128(), acr1), _mm_setzero_si128()));
      sum = _mm_add_epi64(sum, _mm_sad_epu8(_mm_sub_epi8(_mm_setzero_si128(), acr2), _mm_setzero_si128()));
      sum = _mm_add_epi64(sum, _mm_sad_epu8(_mm_sub_epi8(_mm_setzero_si128(), acr3), _mm_setzero_si128()));
    }
    size_t count = _mm_extract_epi64(sum, 0) + _mm_extract_epi64(sum, 1);
    while(p != (char *)s + n) count += *p++ == c;
    return count;
}
#endif

__attribute__((hot, always_inline))
static inline void check_line(const struct board_information *board_info, uint32_t square_size_values[2][board_info->num_cols + 1], struct solver *solver)
{
    if (solver->y != 0)
        set_each_val_to_min_of_itself_and_previous_val(square_size_values[(solver->y + 1) & 1], board_info->num_cols); // See check_square min() call as for why we do this


    enum {
        CHECK_SQUARE_BRANCHLESS,
        CHECK_SQUARE_LOW_O_COUNT,
        CHECK_SQUARE_HIGH_O_COUNT,
    } which_check_square = CHECK_SQUARE_BRANCHLESS;
#if defined(__SSE2__)
    if (board_info->num_cols > 500) {
        size_t o_count = count_val_in_mem(board_info->board + solver->y * board_info->num_cols, 'o', board_info->num_cols / 8) * 8;
        if (o_count < board_info->num_cols / 10)
            which_check_square = CHECK_SQUARE_LOW_O_COUNT;
        else if (o_count > (board_info->num_cols - (board_info->num_cols / 10)))
            which_check_square = CHECK_SQUARE_HIGH_O_COUNT;
    }
#endif

    uint32_t latest_square_size_value = 0;
    if (which_check_square == CHECK_SQUARE_BRANCHLESS)
#pragma GCC unroll 8
        for (solver->x = 0; solver->x < board_info->num_cols - 1; ++solver->x)
            latest_square_size_value = check_square_branchless(board_info->board[solver->y * board_info->num_cols + solver->x], square_size_values[solver->y & 1], square_size_values[(solver->y + 1) & 1], solver, latest_square_size_value);
    else if (which_check_square == CHECK_SQUARE_LOW_O_COUNT)
#pragma GCC unroll 8
        for (solver->x = 0; solver->x < board_info->num_cols - 1; ++solver->x)
            latest_square_size_value = check_square_low_o_count(board_info->board[solver->y * board_info->num_cols + solver->x], square_size_values[solver->y & 1], square_size_values[(solver->y + 1) & 1], solver, latest_square_size_value);
    else if (which_check_square == CHECK_SQUARE_HIGH_O_COUNT) {
        for (solver->x = 0; solver->x < board_info->num_cols - 1; ++solver->x) {
            size_t o_count = 0;
            while (board_info->board[solver->y * board_info->num_cols + solver->x + o_count] == 'o') {
                square_size_values[solver->y & 1][solver->x + o_count] = 0;
                ++o_count;
            }
            solver->x += o_count;
            if (o_count != 0)
                latest_square_size_value = 0;
            latest_square_size_value = check_square_high_o_count(board_info->board[solver->y * board_info->num_cols + solver->x], square_size_values[solver->y & 1], square_size_values[(solver->y + 1) & 1], solver, latest_square_size_value);
        }
    }

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
