int read_bits_as_4bytes(const unsigned char* input, int nth, int bit_slice_len) {
    /* Getting the starting bit/byte indices of the desired slice */
    int target_slice_start_bit_index = (nth - 1) * bit_slice_len;
    int target_slice_start_byte_index = (target_slice_start_bit_index) / 8;
    int next_8bit_boundary = (target_slice_start_byte_index + 1) * 8;

    int result = 0;

    /* If the bit slice we need is in the target byte */
    if (next_8bit_boundary - target_slice_start_bit_index >= bit_slice_len) {
        int num_shifts_to_right = next_8bit_boundary - target_slice_start_bit_index - bit_slice_len;
        result = input[target_slice_start_byte_index] >> num_shifts_to_right;
        result &= (1 << bit_slice_len) - 1;
    } else {
        /*
         * In this case, we first read any bits in the first byte, so we can reach
         * an index which is 8-bit boundary. Then, we read the remaining bits.
         * */

        /* Read the whole remaining bits of the first byte of the slice */
        result = input[target_slice_start_byte_index];

        /* No shift to the right, only masking the bits on the left side of the slice */
        result &= (1 << (next_8bit_boundary - target_slice_start_bit_index)) - 1;

        /* Reading the remaining bits of the slice */
        bit_slice_len -= next_8bit_boundary - target_slice_start_bit_index;
        int num_of_rem_bytes = bit_slice_len / 8;
        int num_of_rem_bits = bit_slice_len % 8;

        for (int i = 0; i < num_of_rem_bytes; i++) {
            result <<= 8;
            result |= input[next_8bit_boundary / 8 + i];
        }

        /* Adding the remaining bits to the result */
        result <<= num_of_rem_bits;
        result |= input[next_8bit_boundary / 8 + num_of_rem_bytes] >> (8 - num_of_rem_bits);
    }
    return result;
}

