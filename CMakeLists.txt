cmake_minimum_required(VERSION 3.22)
project(mumhors)

set(CMAKE_CXX_STANDARD 23)

include_directories(src src/utils src/crypto)
add_compile_options(-O3 -w)

set(CMAKE_CXX_FLAGS_RELEASE "-Os -ffunction-sections -fdata-sections -flto -DNDEBUG")
set(CMAKE_C_FLAGS_RELEASE "-Os -ffunction-sections -fdata-sections -flto -DNDEBUG")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-s -Wl,--gc-sections")



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
