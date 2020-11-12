/*
** EPITECH PROJECT, 2020
** BSQ
** File description:
** Loads a file in memory
*/

#include "load_file_in_mem.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

struct my_string *load_file_in_mem(const char *filename)
{
    int fd = open(filename, O_RDONLY);
    struct my_string *result = my_string_new();
    struct stat file_info;

    if (fd < 0)
        return NULL;
    if (fstat(fd, &file_info) < 0) {
        close(fd);
        my_string_free(result);
        return NULL;
    }
    my_string_resize(result, file_info.st_size);
    if (read(fd, result->string, file_info.st_size) < file_info.st_size) {
        close(fd);
        my_string_free(result);
        return NULL;
    }
    close(fd);
    return result;
}
