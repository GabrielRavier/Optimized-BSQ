#!/usr/bin/env bash
set -uo pipefail

# Execute tests from the directory that which contains the script
cd "$(dirname "$0")"

EXE_BSQ=$(realpath ../../bsq)

for i in mouli_maps/*
do
    # A bit of a cludge but it works, so... ¯\_(ツ)_/¯
    solved_filename=mouli_maps_solved/$(basename $i)

    # Use & to parallelize the tests
    diff -u <("$EXE_BSQ" "$i") "$solved_filename" &
done

process_single_my_map()
{
    # A bit of a cludge but it works, so... ¯\_(ツ)_/¯
    solved_filename=my_maps_solved/$(basename $1)

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

for i in my_maps/*.bz2
do
    # Use & to parallelize the tests
    process_single_my_map "$i" &
done


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
