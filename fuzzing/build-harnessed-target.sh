#!/bin/env bash

echo "- remember to run this script from the top-level directory"
sleep 0.5s

AFLCC=afl-clang-fast
AFLCXX=afl-clang-fast++
# FLAGS='-g -fsanitize=address'
# FLAGS='fsanitize=undefined'
# FLAGS='-fsanitize=address,undefined -ggdb'
FLAGS=
export CFLAGS=$FLAGS
export CXXFLAGS=$FLAGS

if [ ! -f ./.libs/libarchive.a ]; then
    ./build/autogen.sh
    echo "- configuring build system"
    export CC=$AFLCC
    export CXX=$AFLCXX

    ./configure --enable-static

    echo "- building libarchive"
    make -j$(nproc)
fi

echo "- building harness with target"
$AFLCC ./fuzzing/harness.c ./.libs/libarchive.a \
    -fsanitize=fuzzer $FLAGS \
    -I ./libarchive \
    -o ./fuzzing/harness

