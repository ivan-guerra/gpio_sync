set(COMMON_FLAGS
    -std=c++2a
)

set(WARNING_FLAGS
    -Wall
    -Wextra
    -Werror
    -pedantic
)

set(DEBUG_FLAGS
    ${COMMON_FLAGS}
    ${WARNING_FLAGS}
    -O0
    -g3
    -ggdb
    -fno-omit-frame-pointer
    -fsanitize=address
)

set(RELEASE_FLAGS
    ${COMMON_FLAGS}
    -O2
)

add_compile_options(
    "$<$<CONFIG:Release>:${RELEASE_FLAGS}>"
    "$<$<CONFIG:Debug>:${DEBUG_FLAGS}>"
)

add_link_options(
    "$<$<CONFIG:Debug>:-fsanitize=address>"
)

add_subdirectory(gpio)
add_subdirectory(gtimer)
add_subdirectory(shmem)
