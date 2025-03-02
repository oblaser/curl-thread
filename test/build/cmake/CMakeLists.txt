
# author        Oliver Blaser
# date          01.03.2025
# copyright     MIT - Copyright (c) 2025 Oliver Blaser

cmake_minimum_required(VERSION 3.13)

project(curl-thread-test)

#set(CMAKE_CXX_STANDARD 17)
#set(CMAKE_CXX_STANDARD_REQUIRED true)



set(BINNAME curl-thread-test)

if(_DEBUG)
    add_definitions(-D_DEBUG)
    add_definitions(-DCONFIG_LOG_LEVEL=4)
    add_definitions(-DCURLTHREAD_DEBUG_print_queueId_vector)
else()
    add_definitions(-DCONFIG_LOG_LEVEL=3)
endif()

include_directories(../../src/)

set(SOURCES
../../src/main.cpp
../../src/middleware/util.cpp
../../../curl.cpp
)

add_executable(${BINNAME} ${SOURCES})
target_link_libraries(${BINNAME} curl pthread)
target_compile_options(${BINNAME} PRIVATE -pedantic -Wall -Werror=return-type -Werror=switch -Werror=reorder -Werror=format -Wdouble-promotion)
