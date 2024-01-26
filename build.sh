#!/bin/sh
#

set -xe ;

RAYLIB_A=./lib/raylib/src/libraylib.so ;
if test -f "$FILE"; then
    echo "RAYLIB COMPILED." ;
else
    cd ./raylib/src && make && cd ../..;
    echo "RAYLIB BUILT." ;
fi

CFLAGS="-O1 -Wall -Wextra -I./include/  -I./lib/raylib/src -ggdb" ;
FILES="./src/main.c" ;
LIBS="-lm $RAYLIB_A -ldl" ;

#build default theme

cc $CFLAGS -shared -fPIC -o ./plugins/default_ui.so ./src/themes/default_ui.c $RAYLIB_A -lm

cc $FILES $CFLAGS -o ./lighthouse-bin $LIBS ;



DIRNAME=`dirname $RAYLIB_A`
RAYLIB_SRC_DIR=`realpath $DIRNAME`
printf "#!/bin/sh\nLD_LIBRARY_PATH=$RAYLIB_SRC_DIR ./lighthouse-bin\n" > lighthouse
chmod +x ./lighthouse

