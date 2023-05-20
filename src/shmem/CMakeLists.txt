cmake_minimum_required(VERSION 3.13...3.22)

project(shmem LANGUAGES CXX)

add_library(${PROJECT_NAME} INTERFACE)

target_include_directories(${PROJECT_NAME}
    INTERFACE ${GSYNC_INCLUDE_DIR}
)

target_link_libraries(${PROJECT_NAME}
    INTERFACE pthread
)
