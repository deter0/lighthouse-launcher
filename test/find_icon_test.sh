#!/bin/sh
#

set -xe

SCRIPT=`dirname "$0"`

mkdir -p $SCRIPT/../test_out

CFLAGS="-Wall -Wextra -Wno-format-truncation -Wno-unused-parameter -pedantic -ggdb -DICON_FINDER_TEST"
cc $CFLAGS $SCRIPT/../src/common/icon_finder.c -o $SCRIPT/../test_out/icon_finder_tests

if $SCRIPT/../test_out/icon_finder_tests; then
	echo "Passed." ;
else
	echo "Failed."
fi

