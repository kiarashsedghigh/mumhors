cmake_minimum_required(VERSION 3.22)
project(mumhors)

set(CMAKE_CXX_STANDARD 23)

include_directories(src src/utils src/crypto)

#if(NOT CMAKE_CXX_FLAGS MATCHES "-BITMAP_ARRAY]")
#    message(STATUS "No optimization flags (-O2 or -O3) have been passed.")
#endif()

add_compile_options(-O3 -w -DJOURNAL -DBITMAP_ARRAY -DROW_THRESHOLD=11 -DBIT_VECTOR=1024)
#add_compile_options(-O3 -w -DJOURNAL -DBITMAP_LIST)

#set(CMAKE_CXX_FLAGS_RELEASE "-Os -ffunction-sections -fdata-sections -flto -DNDEBUG")
#set(CMAKE_C_FLAGS_RELEASE "-Os -ffunction-sections -fdata-sections -flto -DNDEBUG")
#set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-s -Wl,--gc-sections")

link_libraries(tomcrypt m ssl crypto)

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
        src/crypto/blake/blake2b.c
        src/crypto/blake/blake2s.c
        src/crypto/blake/tomcrypt.h
        src/crypto/blake/tomcrypt_argchk.h
        src/crypto/blake/tomcrypt_cfg.h
        src/crypto/blake/tomcrypt_cipher.h
        src/crypto/blake/tomcrypt_custom.h
        src/crypto/blake/tomcrypt_hash.h
        src/crypto/blake/tomcrypt_mac.h
        src/crypto/blake/tomcrypt_macros.h
        src/crypto/blake/tomcrypt_math.h
        src/crypto/blake/tomcrypt_misc.h
        src/crypto/blake/tomcrypt_pk.h
        src/crypto/blake/tomcrypt_pkcs.h
        src/crypto/blake/tomcrypt_prng.h
)
