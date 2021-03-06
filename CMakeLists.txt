cmake_minimum_required(VERSION 3.5)

project(ezio CXX)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

set(CXX_FLAGS
 -g
 -Wall
 -Wextra
 -Werror
 -Wno-unused-parameter
 -Wold-style-cast
 -Woverloaded-virtual
 -Wpointer-arith
 -Wshadow
)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    list(APPEND CXX_FLAGS "-fno-limit-debug-info")
endif()

string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")

set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

set(CMAKE_EXE_LINKER_FLAGS "-rdynamic")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

string(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE)
message(STATUS "BUILD_TYPE = " ${BUILD_TYPE})
message(STATUS "CXX_FLAGS = " ${CMAKE_CXX_FLAGS})
message(STATUS "CXX_FLAGS_${BUILD_TYPE} = " ${CMAKE_CXX_FLAGS_${BUILD_TYPE}})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

add_subdirectory(KBase/src)
add_subdirectory(ezio)

message(STATUS "BUILD_NO_UNITTEST = " ${CMAKE_BUILD_NO_UNITTEST})

if (NOT CMAKE_BUILD_NO_UNITTEST)
    add_subdirectory(tests)
endif()
