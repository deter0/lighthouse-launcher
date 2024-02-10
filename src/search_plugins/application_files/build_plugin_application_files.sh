#!/bin/sh
#

set -e ;

SCRIPT=`dirname "$0"` ;
cd $SCRIPT ;
ROOT_DIR="../../../" ;

CFLAGS="-O2 -Wall -Wextra -ggdb -DSV_IMPLEMENTATION" ;
INC="-I$ROOT_DIR/include -I$ROOT_DIR/lib/raylib/src" ;
DEPS="$ROOT_DIR/src/common/trie.c $ROOT_DIR/src/common/slurp.c -lm $ROOT_DIR/lib/raylib/libraylib.so" ;

SRC="./plugin_application_files.c $ROOT_DIR/src/common/trie.c $ROOT_DIR/src/common/slurp.c ./desktop_file_parser.c $ROOT_DIR/src/common/icon_finder.c" ;

cc -o plugin_application_files.so -shared -fPIC $SRC $CFLAGS $INC $COMMON_DEPS $ROOT_DIR/include/sv/sv.h ;

mv ./plugin_application_files.so $ROOT_DIR/plugins

if [ "$1" = "-t" ]; then
  mkdir -p $ROOT_DIR/test_out

  cc -DPLUGIN_APPLICATION_FILES_TEST plugin_application_files.c desktop_file_parser.c $INC $COMMON_DEPS $CFLAGS -o $ROOT_DIR/test_out/plugin_application_files_test -lm
fi


