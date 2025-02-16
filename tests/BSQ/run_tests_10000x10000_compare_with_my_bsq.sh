#!/usr/bin/env bash
set -uo pipefail

# Invoked as `./run_tests_10000x10000_compare_with_my_bsq.sh /path/to/other/bsq`
EXE_OTHER_BSQ=$(realpath "$1")

# Execute tests from the directory that which contains the script
cd "$(dirname "$0")"

EXE_MY_BSQ="$(realpath ../../bsq)"
EXE_MAP_GENERATOR="$(realpath ../../map_generator)"

handle_failure()
{
    local temp_generated_map=$1

    # Store the generated map in a file for later inspection
    mkdir -p failed_maps
    <"$temp_generated_map" bzip2 -9 >failed_maps/$(basename "$temp_generated_map").bz2

    printf 'Failed for %s\n' "$temp_generated_map"
    printf 'Generated map stored in failed_maps/%s.bz2\n' $(basename "$temp_generated_map")
}

do_one_run()
{
    # Put the generated map in a temporary file
    local temp_generated_map=`mktemp`

    local random_number_between_0_and_1=$(printf 0; ( printf 'scale=19; '; shuf -i1-10000000000000000000 -n1 | tr -d '\n'; echo ' / 10000000000000000000'; ) | bc)

    echo "Generating map..."

    # Generate a map
    "$EXE_MAP_GENERATOR" 10000 "$random_number_between_0_and_1" $(shuf -i0-2147483647 -n1) >"$temp_generated_map"

    echo "Comparing outputs..."

    diff --brief <("$EXE_MY_BSQ" "$temp_generated_map") <("$EXE_OTHER_BSQ" "$temp_generated_map") || handle_failure "$temp_generated_map"

    # Clean up
    rm "$temp_generated_map"
}

export -f do_one_run handle_failure # Export the functions so that parallel can use them
export EXE_MY_BSQ EXE_MAP_GENERATOR EXE_OTHER_BSQ # Export the variables so that parallel can use them
yes do_one_run | parallel -j 50% # Run tests forever in parallel
