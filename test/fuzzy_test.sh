#!/bin/sh
#

set -xe

SCRIPT=`dirname "$0"`

mkdir -p $SCRIPT/../test_out

cd $SCRIPT/../src/common/fuzzy
./build.sh

cd ../../../

mv ./src/common/fuzzy/fuzzy ./test_out/

if ./test_out/fuzzy; then
	echo "Passed." ;
else
	echo "Failed."
fi


