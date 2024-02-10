#!/bin/sh
#

set -e ;

RAYLIB_FILE="./lib/raylib/src/libraylib.so" ;
if test -f "$RAYLIB_FILE"; then
    echo "Raylib is already compiled!\n" ;
else
    echo "Building Raylib\n\n" ;
    cd ./lib/raylib/src && make && cd ../../../;
    echo "Raylib build" ;
fi

CFLAGS="-O3 -Wall -Wextra -ggdb" ;
INCLUDE="-I./include/  -I./lib/raylib/src" ;
FILES="./src/main.c ./src/image_cache.c" ;
LIBS="-lm $RAYLIB_FILE -ldl" ;

echo "COMPILING CFLAGS: '$CFLAGS'" ;
echo "INCLUDE: '$INCLUDE'" ;
echo "LIBS: '$LIBS'" ;

function compile_ui_theme() {
    echo "Compiling default UI theme" ;
    cc $CFLAGS -shared -fPIC -o ./plugins/default_ui.so ./src/themes/default_ui.c $LIBS ;
}

function compile_lighthouse_bin() {
    cc $FILES $CFLAGS $INCLUDE -o ./lighthouse-bin $LIBS ;
}

function generate_lighthouse_wrapper() {
    RAYLIB_SRC_DIR_FULL_PATH=`readlink -f "./lib/raylib/src"` ;
    BINARY_FULL_PATH=`readlink -f ./lighthouse-bin`

    printf "#!/bin/sh\nLD_LIBRARY_PATH=$RAYLIB_SRC_DIR_FULL_PATH $BINARY_FULL_PATH\n" > lighthouse ;
    chmod +x ./lighthouse ;
}

echo "Compiling UI Theme" ;
compile_ui_theme ;

echo "Compiling Lighthouse Binary" ;
compile_lighthouse_bin ;

echo "Generate Lighthouse Wrapper" ;
generate_lighthouse_wrapper ;

echo "Building Applications Search Plugin"
./src/search_plugins/application_files/build_plugin_application_files.sh ; 

echo "Building Binaries Search Plugin"
./src/search_plugins/binaries/build_binaries.sh ; 

