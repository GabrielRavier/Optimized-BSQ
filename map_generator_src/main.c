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

const uintmax_t LINES = 500;

void fill_buffer(char *buffer, uintmax_t width, float density)
{
    uintmax_t buffsize = (width + 1) * LINES;

    for (uintmax_t j = 0; j < buffsize; j++)
        buffer[j] = "o."[rand() > ((float)RAND_MAX * density)];
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
    srand(seed);
    generate_random_map(atoi(argv[1]), atof(argv[2]));
    return 0;
}
