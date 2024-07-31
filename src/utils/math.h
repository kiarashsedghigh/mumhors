#ifndef MUMHORS_MATH_H
#define MUMHORS_MATH_H


/// Returns the minimum of two integers
/// \param x First integer
/// \param y Second integer
/// \return Minimum of two integers
int min(int x, int y);

/// Counts number of bits set in the given number
/// \param num Given number
/// \return Number of set bits
int count_num_set_bits(int num);

/// Returns the bit index of the nth set bit of the given byte
/// \param byte A given byte
/// \param nth Nth set bit
/// \return Bit index of the nth set bit of the given byte
int byte_get_index_nth_set(unsigned char byte, int nth);

/// Computes the floor mod
/// @param a First number
/// @param b Second number
/// @return Floor mod
#define floor_add_mod(a,b) \
    ((a % b) + b) % b\

#endif