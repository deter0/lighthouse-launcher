#!/bin/sh
#

set -e ;

SCRIPT=`dirname "$0"` ;
cd $SCRIPT ;
ROOT_DIR="../../../" ;

CFLAGS="-Wall -Wextra -ggdb" ;
INC="-I$ROOT_DIR/include -I$ROOT_DIR/lib/raylib/src" ;
DEPS="" ;

SRC="./binaries.c" ;

cc -o plugin_binaries.so -shared -fPIC $SRC $CFLAGS $INC $DEPS ;
mv ./plugin_binaries.so $ROOT_DIR/plugins



