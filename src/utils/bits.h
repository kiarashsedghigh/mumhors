#ifndef MUMHORS_BITS_H
#define MUMHORS_BITS_H

/// Read slice of bits as a 4-byte unsigned integer
/// \param input Pointer to the byte array
/// \param nth N'th slice of bits
/// \param bit_slice_len Size of the bit slice
/// \return 4-byte unsigned integer
int read_bits_as_4bytes(const unsigned char* input, int nth, int bit_slice_len);

#endif