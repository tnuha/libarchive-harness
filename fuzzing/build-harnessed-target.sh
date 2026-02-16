#!/bin/env bash

echo "- remember to run this script from the top-level directory"
sleep 0.5s

AFLCC=afl-clang-fast
AFLCXX=afl-clang-fast++
# FLAGS='-fsanitize=address -g --afl-llvm'
FLAGS=

# -CC=$AFLCC CXX=$AFLCXX MAKEFLAGS=$FLAGS \
# -    ./configure --disable-shared
# +./configure CC=afl-clang-fast CXX=afl-clang-fast \
# +    CFLAGS='$FLAGS' CXXFLAGS='$FLAGS' \
# +    --enable-static
#
# ./configure CC=$AFLCC CXX=$AFLCXX \
#     CFLAGS='$FLAGS' CXXFLAGS='$FLAGS' \
#     --enable-static

./build/autogen.sh
echo "- configuring build system"
export CC=$AFLCC
export CXX=$AFLCXX

./configure --enable-static

export CFLAGS=$FLAGS
export CXXFLAGS=$FLAGS

echo "- building libarchive"
make -j$(nproc)

echo "- building harness with target"
$AFLCC -static ./fuzzing/harness.c ./.libs/libarchive.a \
    $FLAGS -fsanitize=fuzzer \
    -I ./libarchive \
    -o ./fuzzing/harness

