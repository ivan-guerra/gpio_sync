cmake_minimum_required(VERSION 3.13...3.22)

project(gpio_sync
    DESCRIPTION "High Fidelity Synchronization Using GPIO"
)

set(GSYNC_BIN_DIR "${CMAKE_SOURCE_DIR}/bin"
    CACHE STRING  "${PROJECT_NAME} binary directory.")

add_subdirectory(src)
