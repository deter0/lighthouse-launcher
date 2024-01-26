#!/bin/sh
#

set -xe

SCRIPT=`dirname "$0"`

mkdir -p $SCRIPT/../test_out

CFLAGS="-Wall -Wextra -Werror -std=c11 -pedantic -ggdb -DTRIE_TESTING"
cc $CFLAGS $SCRIPT/../src/common/trie.c -o $SCRIPT/../test_out/trie_run_tests

if $SCRIPT/../test_out/trie_run_tests; then
	echo "Passed." ;
else
	echo "Failed."
fi

