#!/usr/bin/env bash

# Execute tests from the directory that which contains the script
cd "$(dirname "$0")"

for i in mouli_maps/*
do
    # A bit of a cludge but it works, so... ¯\_(ツ)_/¯
    solved_filename=mouli_maps_solved/$(basename $i)

    # Use & to parallelize the tests
    cmp <(../../bsq "$i") "$solved_filename"&
done

# Test command line stuff
test_multiple_args()
{
    temp_file=`mktemp`
    ../../bsq a b c d e f g 2>$temp_file
    if [ $? -ne 84 ]; then
        echo "Wrong return value for multiple arguments"
    fi
    cmp $temp_file <(echo "error: Incorrect amount of arguments (should be 2, was 8)")
}

test_no_such_file()
{
    temp_file=`mktemp`
    temp_file_removed=`mktemp`
    rm $temp_file_removed
    ../../bsq $temp_file_removed 2>"$temp_file"
    if [ $? -ne 84 ]; then
        echo "Wrong return value for non such file"
    fi
    cmp $temp_file <(echo "error: Failure to read file '$temp_file_removed'")
}

test_invalid_file()
{
    temp_file=`mktemp`
    input_filename=`mktemp`
    echo -ne "$1" >$input_filename
    ../../bsq $input_filename 2>$temp_file
    if [ $? -ne 84 ]; then
        echo "Wrong return value for invalid file"
    fi
    cmp $temp_file <(echo "error: Invalid board in '$input_filename'")
}

test_multiple_args&
test_no_such_file&
test_invalid_file&
test_invalid_file "1\no"&
test_invalid_file "1\na\n"&
test_invalid_file "1\no\no\n"&

# Don't exit until all the child processes have done so
wait
