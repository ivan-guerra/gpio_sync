cmake_minimum_required(VERSION 3.13...3.22)

project(gpio
    DESCRIPTION "GPIO Helper Class"
    LANGUAGES   CXX
)

add_library(${PROJECT_NAME} STATIC)

target_sources(${PROJECT_NAME}
    PRIVATE gpio.cc
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE gpiodcxx
)

target_include_directories(${PROJECT_NAME}
    PUBLIC ${GSYNC_INCLUDE_DIR}
)
