#include <tomcrypt.h>

#define SHA256_OUTPUT_LEN 32

int ltc_hash_sha2_256(unsigned char *hash_output, const unsigned char *input, long length) {
    hash_state md;
    sha256_init(&md);
    sha256_process(&md, input, length);
    sha256_done(&md, hash_output);
    return SHA256_OUTPUT_LEN;
}





