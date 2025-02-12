#include "set_largest_possible_square.h"
#include "load_file_in_mem.h"
#include "verify_file.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sched.h>

#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>

static const int ERROR_EXIT_CODE = 84;
static const int NO_ERROR_EXIT_CODE = 0;

__attribute__((format(printf, 1, 2))) static int error(const char *format, ...)
{
    va_list args;

    fputs("error: ", stderr);
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    return ERROR_EXIT_CODE;
}

// Function to pin ourselves to a single thread (to avoid cache invalidation from context switches)
// Pin ourselves to the core we're running on - this is logically likely to be the best core to run on
static void pin_to_single_cpu(void)
{
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(sched_getcpu(), &cpuset);

    sched_setaffinity(0, sizeof(cpuset), &cpuset);
}

int main(int argc, char *argv[])
{
    struct loaded_file file_as_buffer;
    struct board_information board_info;

    //pin_to_single_cpu(); // Has not been shown to be beneficial, so turn it off for now (benchmarks are inconclusive and might even be negative)
    if (argc != 2)
        return error("Incorrect amount of arguments (should be 2, was %d)\n",
            argc);
    if (!load_file_in_mem(&file_as_buffer, argv[1]))
        return error("Failure to read file '%s'\n", argv[1]);
    madvise(file_as_buffer.data, file_as_buffer.size, MADV_WILLNEED);
    if (!verify_file(&file_as_buffer, &board_info)) {
        munmap(file_as_buffer.data, file_as_buffer.size);
        return error("Invalid board in '%s'\n", argv[1]);
    }
    set_largest_possible_square(&board_info);
    write(STDOUT_FILENO, board_info.board,
        board_info.num_cols * board_info.num_rows);
    munmap(file_as_buffer.data, file_as_buffer.size);
    return NO_ERROR_EXIT_CODE;
}
