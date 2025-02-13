# Optimized-BSQ

This project aims to find the biggest square in a map, avoiding obstacles, as fast as possible.

## How to use

```bash
$ make
$ ./bsq some_map
```

## How to test

```bash
$ make tests_run
```

This repository's tests can also be executed on a different implementation of the BSQ algorithm. To do so, you need to provide the path to the other implementation's executable.

```bash
$ ./tests/BSQ/run_tests.sh path_to_other_bsq_executable
```

If your executable does not do error handling and only handles maps of a size of 10000x10000, you can use an alternative script that only tests maps of that size.

```bash
$ ./tests/BSQ/run_tests_10000x10000_maps.sh path_to_other_bsq_executable no_error_handling
```
