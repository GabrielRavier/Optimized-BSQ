#!/usr/bin/env bash
set -uo pipefail

# Invoked as `./run_tests.sh` or `./run_tests.sh /path/to/bsq`
# When run without arguments, the script assumes that the executable is located at ../../bsq (as relative to the script, not the current working directory)
EXE_BSQ=${1:-$(dirname "$0")/../../bsq}
EXE_BSQ=$(realpath "$EXE_BSQ")

# Execute tests from the directory that which contains the script
cd "$(dirname "$0")"

do_mouli_maps()
{
    tar --extract --directory=. --file=./maps-intermediate.tgz

    for i in mouli_maps/*
    do
        # A bit of a cludge but it works, so... ¯\_(ツ)_/¯
        solved_filename=mouli_maps_solved/$(basename $i)

        # Use & to parallelize the tests
        ( ( diff -u <("$EXE_BSQ" "$i") "$solved_filename" | head -n30 ) || printf 'Failed for %s (note: above are the first 30 lines of the diff with the expected output)\n' "$i" ) &
    done

    wait
}

do_mouli_maps &

./run_tests_10000x10000_maps.sh "$EXE_BSQ" &

# Test command line stuff
test_multiple_args()
{
    temp_file=`mktemp`
    "$EXE_BSQ" a b c d e f g 2>$temp_file
    if [ $? -ne 84 ]; then
        echo 'Wrong return value for multiple arguments'
    fi
    diff -u "$temp_file" <(echo 'error: Incorrect amount of arguments (should be 2, was 8)')
    rm "$temp_file"
}

test_no_such_file()
{
    temp_file=`mktemp`
    temp_file_removed=`mktemp`
    rm "$temp_file_removed"
    "$EXE_BSQ" "$temp_file_removed" 2>"$temp_file"
    if [ $? -ne 84 ]; then
        echo 'Wrong return value for non such file'
    fi
    diff -u "$temp_file" <(echo "error: Failure to read file '$temp_file_removed'")
    rm "$temp_file"
}

test_empty_file()
{
    temp_file=`mktemp`
    input_filename=`mktemp`
    "$EXE_BSQ" "$input_filename" 2>"$temp_file"
    if [ $? -ne 84 ]; then
        echo 'Wrong return value for invalid file'
    fi
    diff -u "$temp_file" <(echo "error: Failure to read file '$input_filename'")
    rm "$temp_file"
    rm "$input_filename"
}

test_invalid_file()
{
    temp_file=`mktemp`
    input_filename=`mktemp`
    echo -ne "$1" >"$input_filename"
    "$EXE_BSQ" "$input_filename" 2>"$temp_file"
    if [ $? -ne 84 ]; then
        echo 'Wrong return value for invalid file'
    fi
    diff -u "$temp_file" <(echo "error: Invalid board in '$input_filename'")
    rm "$temp_file"
    rm "$input_filename"
}

test_multiple_args &
test_no_such_file &
test_empty_file &
test_invalid_file "1\no" &
test_invalid_file "1\na\n" &
test_invalid_file "1\no\no\n" &

# Don't exit until all the child processes have done so
wait

printf 'If no other output (except the message saying the 10000x10000 tests passed) was produced, all tests passed\n'
