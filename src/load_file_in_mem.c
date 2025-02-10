#include "load_file_in_mem.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>

bool load_file_in_mem(struct loaded_file *result, const char *filename)
{
    int fd = open(filename, O_RDONLY);
    struct stat file_info;

    if (fd < 0)
        return false;
    if (fstat(fd, &file_info) < 0) {
        close(fd);
        return false;
    }
    result->data = mmap(NULL, file_info.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (result->data == MAP_FAILED) {
        close(fd);
        return false;
    }
    result->size = file_info.st_size;
    close(fd);
    return true;
}
