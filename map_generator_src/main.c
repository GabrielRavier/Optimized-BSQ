/*
** EPITECH PROJECT, 2023
** SettingUp
** File description:
** generator.c
*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// xorshift128 is from Wikipedia (https://en.wikipedia.org/wiki/Xorshift)
/* struct xorshift128_state can alternatively be defined as a pair
   of uint64_t or a uint128_t where supported */
struct xorshift128_state {
    uint32_t x[4];
};

/* The state must be initialized to non-zero */
uint32_t xorshift128(struct xorshift128_state *state)
{
    /* Algorithm "xor128" from p. 5 of Marsaglia, "Xorshift RNGs" */
    uint32_t t  = state->x[3];

    uint32_t s  = state->x[0];  /* Perform a contrived 32-bit shift. */
    state->x[3] = state->x[2];
    state->x[2] = state->x[1];
    state->x[1] = s;

    t ^= t << 11;
    t ^= t >> 8;
    return state->x[0] = t ^ s ^ (s >> 19);
}

static struct xorshift128_state rand_state = {{0x123, 0x234, 0x345, 0x456}};

const uintmax_t LINES = 500;

void fill_buffer(char *buffer, uintmax_t width, float density)
{
    uintmax_t buffsize = (width + 1) * LINES;

    for (uintmax_t j = 0; j < buffsize; j++)
        buffer[j] = "o."[xorshift128(&rand_state) > ((float)UINT32_MAX * density)];
    for (uintmax_t j = width; j < buffsize; j += width + 1)
        buffer[j] = '\n';
}

void fill_map(char *buffer, uintmax_t size, float density)
{
    uintmax_t area = size * (size + 1);
    uintmax_t buffsize = (size + 1) * LINES;
    uintmax_t cycles = area / buffsize;

    setbuf(stdout, NULL);
    printf("%ju\n", size);
    for (uintmax_t i = 0; i < cycles; i++) {
        fill_buffer(buffer, size, density);
        write(STDOUT_FILENO, buffer, buffsize);
        fprintf(stderr, "%5ju / %5ju\r", i + 1, cycles);
    }
    fill_buffer(buffer, size, density);
    write(STDOUT_FILENO, buffer, area - (buffsize * cycles));
}

void generate_random_map(int size, float density)
{
    char *buffer;

    if (size <= 0 || density <= 0)
        return;
    buffer = malloc((LINES * (size + 1)) * sizeof(char));
    if (buffer == NULL) {
        fprintf(stderr, "%s\n", "Allocation failure.");
        return;
    }
    fill_map(buffer, size, density);
    free(buffer);
}

int main(int argc, char **argv)
{
    int seed;
    if (argc != 4) {
        fprintf(stderr, "%s\n", "Usage ./gen [size] [density] [seed]");
        return 84;
    }
    seed = atoi(argv[3]);
    if (seed < 0)
        return 84;
    rand_state.x[0] = seed;
    generate_random_map(atoi(argv[1]), atof(argv[2]));
    return 0;
}
