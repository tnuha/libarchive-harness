#!/bin/env bash

echo "remember to run this script from the top-level directory"
sleep 0.5s

AFLCC=afl-cc
AFLCXX=afl-c++

./build/autogen.sh

CC=$AFLCC CXX=$AFLCXX MAKEFLAGS='-fsanitize=fuzzer' \
  ./configure --disable-shared

make -j$(nproc)

afl-cc ./fuzzing/harness.c -I./libarchive ./.libs/libarchive.a

