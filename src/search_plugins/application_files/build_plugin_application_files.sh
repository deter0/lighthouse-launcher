#!/bin/sh
#

set -xe

SCRIPT=`dirname "$0"`
CFLAGS="-O2 -Wall -Wextra -ggdb" ;
INC="-I$SCRIPT/../../../include -I$SCRIPT../../common"
COMMON_DEPS="$SCRIPT/../../common/trie.c $SCRIPT/../../common/slurp.c"
cc -shared -fPIC $SCRIPT/plugin_application_files.c $SCRIPT/desktop_file_parser.c $INC $COMMON_DEPS $CFLAGS -o $SCRIPT/plugin_application_files.so

mkdir -p $SCRIPT/../../../test_out

ls $SCRIPT/../../../include

cc -DPLUGIN_APPLICATION_FILES_TEST $SCRIPT/plugin_application_files.c $SCRIPT/desktop_file_parser.c $INC $COMMON_DEPS $CFLAGS -o $SCRIPT/../../../test_out/plugin_application_files_test

