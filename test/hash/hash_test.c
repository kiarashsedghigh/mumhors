// // #include "hash.h"
// // #include <stdio.h>
// // #include <stddef.h>
// // #include <sys/time.h>
// //
// // #define ITER 1000000
// //
// // int main() {
// //     double time = 0;
// //     struct timeval start_time, end_time;
// //     unsigned char data[256];
// //     unsigned char hash[SHA256_OUTPUT_LEN];
// //
// //     for(int i=0; i<ITER; i++) {
// //         gettimeofday(&start_time, NULL);
// //         ltc_hash_sha2_256(hash, data, 256);
// //         gettimeofday(&end_time, NULL);
// //         time += (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_usec - start_time.tv_usec) / 1.0e6;
// //     }
// //     printf("Time: %0.12f\n", time / ITER * 1000000);
// // }
// // / #include "hash.h"
// #include <stdio.h>
// #include <stddef.h>
// #include <sys/time.h>
// #include <openssl/sha.h>
// // #include "types.h"
// #define SHA256_OUTPUT_LEN 32
// #define ITER 100
// #include <tomcrypt.h>
// // u32 openssl_hash_sha2_256(u8 * hash_output, const u8 * input , u64 length){
// //     SHA256(input, length, hash_output);
// //     return SHA256_OUTPUT_LEN;
// // }
// #define SHA256_OUTPUT_LEN 32
//
// int main() {
//     double time = 0;
//     struct timeval start_time, end_time;
//     unsigned char data[256];
//     unsigned char hash[SHA256_OUTPUT_LEN];
//     for(int i=0; i<ITER; i++) {
//         gettimeofday(&start_time, NULL);
//         // ltc_hash_sha2_256(hash, data, 256);
//         SHA256(data, 256, hash);
//         gettimeofday(&end_time, NULL);
//         time += (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_usec - start_time.tv_usec) / 1.0e6;
//     }
//     printf("Time: %0.12f\n", time / ITER * 1000000);
// }