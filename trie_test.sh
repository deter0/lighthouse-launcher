#!/bin/sh
#

set -xe

CFLAGS="-Wall -Wextra -Werror -std=c11 -pedantic -ggdb -DTRIE_TESTING"
cc $CFLAGS ./src/trie.c -o trie

./trie # > trie.dot
# dot -Tsvg trie.dot > trie.svg

