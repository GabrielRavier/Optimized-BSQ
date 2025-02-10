#include "load_file_in_mem.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>

bool load_file_in_mem(struct loaded_file *result, const char *filename)
{
    int fd = open(filename, O_RDONLY);
    struct stat file_info;

    if (fd < 0)
        return NULL;
    if (fstat(fd, &file_info) < 0) {
        close(fd);
        return NULL;
    }
    result->data = malloc(file_info.st_size);
    result->size = file_info.st_size;
    assert(result->data != 0);
    if (read(fd, result->data, file_info.st_size) < file_info.st_size) {
        close(fd);
	    free(result->data);
        return NULL;
    }
    close(fd);
    return result;
}
