#!/bin/bash

source config.sh

# Remove the binary directory.
if [ -d $GSYNC_BIN_DIR ]
then
    echo "removing '$GSYNC_BIN_DIR'"
    rm -r $GSYNC_BIN_DIR
fi

# Remove the CMake build directory.
if [ -d $GSYNC_BUILD_DIR ]
then
    echo "removing '$GSYNC_BUILD_DIR'"
    rm -r $GSYNC_BUILD_DIR
fi
