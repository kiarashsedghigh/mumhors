#include "math.h"

int min(int x, int y) {
    if (x < y)
        return x;
    return y;
}


int count_num_set_bits(int num) {
    int cnt = 0;
    while (num) {
        cnt += num & 1;
        num >>= 1;
    }
    return cnt;
}


int byte_get_index_nth_set(unsigned char byte, int nth) {
    int bit_idx = 0;
    nth -= 1; // Converting nth to 0-based index
    while (nth >= 0) {
        while ((byte & 128) != 128) {
            bit_idx++;
            byte <<= 1;
        }
        byte <<= 1;
        bit_idx++;
        nth--;
    }

    return bit_idx - 1;
}
