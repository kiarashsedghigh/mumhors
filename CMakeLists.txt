cmake_minimum_required(VERSION 3.22)
project(muhors)

set(CMAKE_CXX_STANDARD 14)

include_directories(src/utils)

add_compile_options(-O3 -w -DJOURNAL)


install(DIRECTORY src/utils/ DESTINATION /usr/include/muhors FILES_MATCHING PATTERN "*.h")
add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/install_headers_done
        COMMAND ${CMAKE_COMMAND} -E make_directory /usr/include/muhors
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/src/utils/*.h /usr/include/muhors
        COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_BINARY_DIR}/install_headers_done
        COMMENT "Installing headers to /usr/include/muhors"
)


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

install(TARGETS muhors DESTINATION /usr/bin)


#cmake_minimum_required(VERSION 3.22)
#project(muhors)
#
#set(CMAKE_CXX_STANDARD 14)
#
#include_directories(src/utils)
#
#add_compile_options(-O3 -w)
#
## Custom command to install headers
#add_custom_command(
#        OUTPUT ${CMAKE_BINARY_DIR}/install_headers_done
#        COMMAND ${CMAKE_COMMAND} -E make_directory /usr/include/muhors
#        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/src/utils/*.h /usr/include/muhors
#        COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_BINARY_DIR}/install_headers_done
#        COMMENT "Installing headers to /usr/include/muhors"
#)
#
#
#add_compile_options(${ADDITIONAL_COMPILE_OPTIONS})
#
#
## Ensure headers are installed before building muhors executable
#add_custom_target(install_headers_target DEPENDS ${CMAKE_BINARY_DIR}/install_headers_done)
#
## Add executable
#add_executable(muhors
#        src/utils/cqueue.c
#        src/utils/cqueue.h
#        README.md  # Adjust this as necessary, typically main files should be .cpp or .c
#        main.c
#        src/utils/bitmap.c
#        src/utils/bitmap.h
#        src/utils/debug.c
#        src/utils/debug.h
#        src/utils/sort.c
#        src/utils/sort.h
#        src/utils/math.c
#        src/utils/math.h
#)
#
## Make sure install_headers_target runs before building muhors
#add_dependencies(muhors install_headers_target)
#
## Installation rules
#install(TARGETS muhors DESTINATION /usr/bin)
#cmake -DCMAKE_CXX_FLAGS="-DJOURNAL" ../
