cmake_minimum_required(VERSION 3.13...3.22)

project(mem
    DESCRIPTION "Memory Config Utility Functions"
    LANGUAGES   CXX
)

add_library(${PROJECT_NAME} STATIC)

target_sources(${PROJECT_NAME}
    PRIVATE mem.cc
)

target_include_directories(${PROJECT_NAME}
    PUBLIC ${GSYNC_INCLUDE_DIR}
)

install(TARGETS ${PROJECT_NAME}
    ARCHIVE DESTINATION "${GSYNC_BIN_DIR}"
)
