#!/usr/bin/env bats
# File: student_tests.sh
# 
# Create your unit tests suite in this file

@test "Example: check ls runs without errors" {
    run ./dsh <<EOF
ls
exit
EOF
    [ "$status" -eq 0 ]
}

@test "Pipeline: ls | grep dshlib.c" {
    run ./dsh <<EOF
ls | grep dshlib.c
exit
EOF
    stripped_output=$(echo "$output" | tr -d '[:space:]')
    [[ "$stripped_output" == *"dshlib.c"* ]]
    [ "$status" -eq 0 ]
}

@test "Invalid command returns error message" {
    run ./dsh <<EOF
nonexistentcommand
exit
EOF
    [[ "$output" == *"error:"* ]]
    [ "$status" -eq 0 ]
}
