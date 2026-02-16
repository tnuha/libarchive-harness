#!/bin/env bash

echo "remember to run this script from the top-level directory"
sleep 0.5s

AFLCC=afl-clang-fast
AFLCXX=afl-c++
FLAGS=-fsanitize=fuzzer
# AFLCC=clang
# AFLCXX=clang++
# FLAGS=-g

./build/autogen.sh

CC=$AFLCC CXX=$AFLCXX MAKEFLAGS=$FLAGS \
    ./configure --disable-shared

make -j$(nproc)

$AFLCC ./fuzzing/harness.c \
    $FLAGS \
    -I./libarchive ./.libs/libarchive.a \
    -o ./fuzzing/harness

