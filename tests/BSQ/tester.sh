#!/usr/bin/env bash

# Execute tests from the directory that which contains the script
cd "$(dirname "$0")"

for i in mouli_maps/*
do
    solved_filename=mouli_maps_solved/$(basename $i)
    # Use & to parallelize the tests
    cmp <(../../bsq "$i") "$solved_filename"&
done

# Don't exit until all the child processes have done so
wait
