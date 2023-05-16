#!/bin/bash

CWD=$(pwd)

# Root directory.
GSYNC_PROJECT_PATH=$(dirname ${CWD})

# Scripts directory.
GSYNC_SCRIPTS_PATH="${GSYNC_PROJECT_PATH}/scripts"

# Binary directory.
GSYNC_BIN_DIR="${GSYNC_PROJECT_PATH}/bin"

# CMake build files and cache.
GSYNC_BUILD_DIR="${GSYNC_PROJECT_PATH}/build"
