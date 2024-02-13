#!/bin/sh
#

set -xe

SCRIPT=`dirname "$0"`

mkdir -p $SCRIPT/../test_out

CFLAGS="-Wall -Wextra -pedantic -Wno-format-truncation -ggdb -DSV_IMPLEMENTATION -DICON_FINDER_TEST -I./include"
SRC="$SCRIPT/../src/common/icon_finder.c $SCRIPT/../src/common/slurp.c $SCRIPT/../src/common/desktop_file_parser.c"
cc $CFLAGS $SRC -o $SCRIPT/../test_out/icon_finder_tests

if $SCRIPT/../test_out/icon_finder_tests; then
	echo "Passed." ;
else
	echo "Failed."
fi

