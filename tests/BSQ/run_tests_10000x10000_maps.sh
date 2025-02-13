#!/usr/bin/env bash
set -uo pipefail

# Invoked as `./run_tests.sh /path/to/bsq`
EXE_BSQ=$(realpath "$1")

# Execute tests from the directory that which contains the script
cd "$(dirname "$0")"

process_single_my_map()
{
    # A bit of a cludge but it works, so... ¯\_(ツ)_/¯
    solved_filename=my_maps_10000x10000_solved/$(basename $1)

    # Make temporary files for both
    temp_file_input=`mktemp`
    temp_file_output=`mktemp`

    # Uncompress the files
    bzip2 -d -c "$1" >"$temp_file_input"
    bzip2 -d -c "$solved_filename" >"$temp_file_output"

    diff -u <("$EXE_BSQ" "$temp_file_input") "$temp_file_output"

    # Clean up
    rm "$temp_file_input"
    rm "$temp_file_output"
}

for i in my_maps_10000x10000/*.bz2
do
    # Use & to parallelize the tests
    process_single_my_map "$i" &
done

wait