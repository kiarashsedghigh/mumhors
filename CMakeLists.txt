cmake_minimum_required(VERSION 3.29)
project(muhors)

set(CMAKE_CXX_STANDARD 14)

include_directories(src/utils)

add_executable(muhors
        src/utils/cqueue.c
        src/utils/cqueue.h
        README.md
        main.c
        src/utils/bitmap.c
        src/utils/bitmap.h)
