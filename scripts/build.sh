#!/bin/bash

BUILD_TYPE="Release"
TOOLCHAIN_FILE=""

source config.sh

Help()
{
    echo "Build GPIO Sync"
    echo
    echo "usage: build.sh [OPTION]..."
    echo "options:"
    echo -e "\tg    enable debug info"
    echo -e "\tc    cross compile for the beaglebone black"
    echo -e "\th    print this help message"
}

Main()
{
    # Create the build directory if it does not already exist.
    mkdir -pv $GSYNC_BUILD_DIR

    pushd $GSYNC_BUILD_DIR
        cmake ../                               \
              -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
              -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN_FILE \
              -DCMAKE_BUILD_TYPE=$BUILD_TYPE && \
        make -j$(nproc) all                  && \
        make install

        # Exit if any of the above commands fails.
        if [ $? -ne 0 ];
        then
            exit 1
        fi
    popd
}

while getopts ":hgc" flag
do
    case "$flag" in
        g) BUILD_TYPE="Debug";;
        c) TOOLCHAIN_FILE=${GSYNC_PROJECT_PATH}/cmake/arm-linux-gnueabihf-gcc.cmake;;
        h) Help
           exit;;
       \?) echo "error: invalid option '$OPTARG'"
           Help
           exit;;
    esac
done

Main
