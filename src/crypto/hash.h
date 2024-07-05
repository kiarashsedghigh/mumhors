#ifndef MUMHORS_HASH_H
#define MUMHORS_HASH_H

#define SHA256_OUTPUT_LEN 32

/// Computes the hash value based on the Tomcrypt SHA2-256
/// \param hash_output Pointer to buffer that the hash will be stored
/// \param input Pointer to the input that we want the hash value
/// \param length The length of the input
/// \return The size of the hash
int ltc_hash_sha2_256(unsigned char *hash_output, const unsigned char *input, long length);

#endif