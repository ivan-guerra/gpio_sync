cmake_minimum_required(VERSION 3.13...3.26)

# This toolchain was dervied from the examples given in
# https://kubasejdak.com/how-to-cross-compile-for-embedded-with-cmake-like-a-champ

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Without that flag CMake is not able to pass test compilation check.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(TOOLCHAIN_PATH "/usr/bin/")
set(CMAKE_AR            ${TOOLCHAIN_PATH}arm-linux-gnueabihf-ar${CMAKE_EXECUTABLE_SUFFIX})
set(CMAKE_ASM_COMPILER  ${TOOLCHAIN_PATH}arm-linux-gnueabihf-as${CMAKE_EXECUTABLE_SUFFIX})
set(CMAKE_C_COMPILER    ${TOOLCHAIN_PATH}arm-linux-gnueabihf-gcc${CMAKE_EXECUTABLE_SUFFIX})
set(CMAKE_CXX_COMPILER  ${TOOLCHAIN_PATH}arm-linux-gnueabihf-g++${CMAKE_EXECUTABLE_SUFFIX})
set(CMAKE_LINKER        ${TOOLCHAIN_PATH}arm-linux-gnueabihf-ld${CMAKE_EXECUTABLE_SUFFIX})
set(CMAKE_OBJCOPY       ${TOOLCHAIN_PATH}arm-linux-gnueabihf-objcopy${CMAKE_EXECUTABLE_SUFFIX} CACHE INTERNAL "")
set(CMAKE_RANLIB        ${TOOLCHAIN_PATH}arm-linux-gnueabihf-ranlib${CMAKE_EXECUTABLE_SUFFIX} CACHE INTERNAL "")
set(CMAKE_SIZE          ${TOOLCHAIN_PATH}arm-linux-gnueabihf-size${CMAKE_EXECUTABLE_SUFFIX} CACHE INTERNAL "")
set(CMAKE_STRIP         ${TOOLCHAIN_PATH}arm-linux-gnueabihf-strip${CMAKE_EXECUTABLE_SUFFIX} CACHE INTERNAL "")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
