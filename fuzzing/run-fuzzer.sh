#!/bin/env bash

echo "remember to run this script from the top-level directory"
sleep 0.5s

afl-fuzz -i ./fuzzing/corpus \
    -o ./fuzzing/out \
    ./fuzzing/harness @@

