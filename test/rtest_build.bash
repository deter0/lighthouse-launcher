#!/bin/sh
#

set -xe

SCRIPT=`dirname "$0"`

mkdir -p $SCRIPT/../test_out

CFLAGS="-Wall -Wextra -Werror -std=c11 -ggdb -lm"
cc $CFLAGS $SCRIPT/rtest.c $SCRIPT/../lib/raylib/src/libraylib.a -o $SCRIPT/../test_out/rtest -I$SCRIPT/../lib/raylib/src

echo "Run the rtest (test_out/rtest) to see if a red window appears." ;
 
