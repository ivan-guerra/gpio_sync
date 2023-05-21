cmake_minimum_required(VERSION 3.13...3.22)

project(gtimer
    DESCRIPTION "GPIO Signal Time Recorder"
    LANGUAGES   CXX
)

add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME}
    PRIVATE gtimer.cc
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE gpio
            mem
            shmem
)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION "${GSYNC_BIN_DIR}"
)
