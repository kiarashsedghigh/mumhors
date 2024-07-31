cmake_minimum_required(VERSION 3.22)
project(mumhors)

set(CMAKE_CXX_STANDARD 23)

include_directories(src src/utils src/crypto)

add_compile_options(-O3 -w -DJOURNAL -DBITMAP_LIST -DROW_THRESHOLD=12)
# DBITMAP_ARRAY DBITMAP_LIST

link_libraries(tomcrypt m)

# Add executable
add_executable(mumhors
        README.md
        test/main.c
        src/utils/bitmap.c
        src/utils/bitmap.h
        src/utils/debug.c
        src/utils/debug.h
        src/utils/sort.c
        src/utils/sort.h
        src/utils/math.c
        src/utils/math.h
        src/mumhors.c
        src/mumhors.h
        src/crypto/sha2.c
        src/crypto/hash.h
        src/utils/bits.c
        src/utils/bits.h
        )

# Define the test folder relative to the build directory
set(TEST_FOLDER "${CMAKE_BINARY_DIR}/../test")

# Ensure the test folder exists
file(MAKE_DIRECTORY ${TEST_FOLDER})

# Custom command to copy the executable to the test folder
add_custom_command(TARGET mumhors POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:mumhors> ${TEST_FOLDER}
        COMMENT "Copying mumhors executable to test folder"
)

# Optionally add a custom target to explicitly copy the executable
add_custom_target(test ALL DEPENDS mumhors
        COMMENT "Building and copying mumhors executable to test folder"
)