#!/bin/sh
#

set -xe

CFLAGS="-Wall -Wextra -Werror -std=c11 -pedantic -ggdb"
cc $CFLAGS ./src/trie.c -o trie

ps ./trie # > trie.dot
# dot -Tsvg trie.dot > trie.svg

