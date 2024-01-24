#!/bin/sh
#

set -xe ;

RAYLIB_A=./raylib/src/libraylib.a ;
if test -f "$FILE"; then
    echo "RAYLIB COMPILED." ;
else
    cd ./raylib/src && make && cd ../..;
    echo "RAYLIB BUILT." ;
fi

CFLAGS="-Wall -Wextra -I./include/  -I./raylib/src -ggdb" ;
FILES="./src/main.c ./src/slurp.c ./src/desktop_file_parser.c ./src/trie.c" ;
LIBS="-lm $RAYLIB_A" ;

gcc -O0 $FILES $CFLAGS -o ./lighthouse $LIBS ;

