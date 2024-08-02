#ifndef MUMHORS_HASH_H
#define MUMHORS_HASH_H

#define SHA256_OUTPUT_LEN 32

/// Computes the hash value based on the Tomcrypt SHA2-256
/// \param hash_output Pointer to buffer that the hash will be stored
/// \param input Pointer to the input that we want the hash value
/// \param length The length of the input
/// \return The size of the hash
int ltc_hash_sha2_256(unsigned char *hash_output, const unsigned char *input, long length);


/// Computes the hash value based on the openssl SHA2-256
/// \param hash_output Pointer to buffer that the hash will be stored
/// \param input Pointer to the input that we want the hash value
/// \param length The length of the input
/// \return The size of the hash
int openssl_hash_sha2_256(unsigned char * hash_output, const unsigned char * input , long length);


/// Computes the hash value based on the Blake2b-256 by (https://github.com/rurban/smhasher?tab=readme-ov-file)
/// \param hash_output Pointer to buffer that the hash will be stored
/// \param input Pointer to the input that we want the hash value
/// \param length The length of the input
/// \return The size of the hash
int blake2b_256(unsigned char * hash_output, const unsigned char * input , long length);


/// Computes the hash value based on the Blake2b-384 by (https://github.com/rurban/smhasher?tab=readme-ov-file)
/// \param hash_output Pointer to buffer that the hash will be stored
/// \param input Pointer to the input that we want the hash value
/// \param length The length of the input
/// \return The size of the hash
int blake2b_384(unsigned char * hash_output, const unsigned char * input , long length);


/// Computes the hash value based on the Blake2b-512 by (https://github.com/rurban/smhasher?tab=readme-ov-file)
/// \param hash_output Pointer to buffer that the hash will be stored
/// \param input Pointer to the input that we want the hash value
/// \param length The length of the input
/// \return The size of the hash
int blake2b_512(unsigned char * hash_output, const unsigned char * input , long length);


/// Computes the hash value based on the Blake2s-128 by (https://github.com/rurban/smhasher?tab=readme-ov-file)
/// \param hash_output Pointer to buffer that the hash will be stored
/// \param input Pointer to the input that we want the hash value
/// \param length The length of the input
/// \return The size of the hash
int blake2s_128(unsigned char * hash_output, const unsigned char * input , long length);


/// Computes the hash value based on the Blake2s-160 by (https://github.com/rurban/smhasher?tab=readme-ov-file)
/// \param hash_output Pointer to buffer that the hash will be stored
/// \param input Pointer to the input that we want the hash value
/// \param length The length of the input
/// \return The size of the hash
int blake2s_160(unsigned char * hash_output, const unsigned char * input , long length);


/// Computes the hash value based on the Blake2s-224 by (https://github.com/rurban/smhasher?tab=readme-ov-file)
/// \param hash_output Pointer to buffer that the hash will be stored
/// \param input Pointer to the input that we want the hash value
/// \param length The length of the input
/// \return The size of the hash
int blake2s_224(unsigned char * hash_output, const unsigned char * input , long length);


/// Computes the hash value based on the Blake2s-256 by (https://github.com/rurban/smhasher?tab=readme-ov-file)
/// \param hash_output Pointer to buffer that the hash will be stored
/// \param input Pointer to the input that we want the hash value
/// \param length The length of the input
/// \return The size of the hash
int blake2s_256(unsigned char * hash_output, const unsigned char * input , long length);



#endif