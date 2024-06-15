cmake_minimum_required(VERSION 3.22)
project(muhors)

set(CMAKE_CXX_STANDARD 14)

include_directories(src/utils)

add_compile_options(-O3 -w)

# Add executable
add_executable(muhors
        src/utils/cqueue.c
        src/utils/cqueue.h
        README.md
        main.c
        src/utils/bitmap.c
        src/utils/bitmap.h
        src/utils/debug.c
        src/utils/debug.h
        src/utils/sort.c
        src/utils/sort.h
        src/utils/math.c
        src/utils/math.h)

# Install header files from src/utils to /usr/include/muhors
install(DIRECTORY src/utils/ DESTINATION /usr/include/muhors FILES_MATCHING PATTERN "*.h")
install(TARGETS muhors DESTINATION /usr/bin)
