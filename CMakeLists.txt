cmake_minimum_required(VERSION 3.13...3.22)

project(gpio_sync
    DESCRIPTION "High Fidelity Synchronization Using GPIO"
)

set(GSYNC_BIN_DIR "${CMAKE_SOURCE_DIR}/bin"
    CACHE STRING  "${PROJECT_NAME} binary directory.")
set(GSYNC_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include"
    CACHE STRING      "${PROJECT_NAME} include directory.")

add_subdirectory(docs)
add_subdirectory(src)
