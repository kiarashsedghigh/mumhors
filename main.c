#include "bitmap.h"
#include "debug.h"
#include "sort.h"
#include "mumhors.h"
#include "hash.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>


void generate_rands(int *arr, int k, int t) {
    int in, im;
    int N = t - 1;
    int M = k;
    im = 0;

    for (in = 0; in < N && im < M; ++in) {
        int rn = N - in;
        int rm = M - im;
        if (rand() % rn < rm)
            /* Take it */
            arr[im++] = in + 1; /* +1 since your range begins from 1 */
    }

    merge_sort(arr, 0, k - 1);
    for (int i = 1; i < k; i++) {
        if (arr[i] == arr[i - 1]) {
            printf("SAME\n");
            exit(0);
        }
    }

}


int main(int argc, char **argv) {
    int t, k, l, ir, rt, tests, r;
//    if (argc<7){
//        printf("|HELP|\n\tRun:\n");
//        printf("\t\t mumhors T K L IR RT TESTS\n");
//        exit(1);
//    }
//
//    //TODO atoi is deprecated
//    t= atoi(argv[1]);
//    k= atoi(argv[2]);
//    l= atoi(argv[3]);
//    ir= atoi(argv[4]);
//    rt= atoi(argv[5]);
//    tests= atoi(argv[6]);

    double keygen_time;
    double sign_and_verify = 0;
    struct timeval start_time, end_time;

    t = 1024;
    k = 16;
    r = 16385;
    l = 256;
    rt = 12;
    tests = 1048576;
    /* Generating the public key from the seed to be provisioned to the verifier.
     * The signer only needs to have access to the seed as not precomputing the private key
     * is the exact goal of this program. */
    printf("Generating the public keys\n");
    public_key_matrix_t pk_matrix;

    gettimeofday(&start_time, NULL);

    mumhors_pk_gen(&pk_matrix, "seed", 4, r, t);

    gettimeofday(&end_time, NULL);
    keygen_time = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_usec - start_time.tv_usec) / 1.0e6;
    printf("KEYGEN: %0.12f\n", keygen_time);


    /* Create and initialize the signer */
    mumhors_signer_t signer;
    mumhors_init_signer(&signer, "seed", 4, t, k, l, rt, r);

    /* Create and initialize the verifier */
    mumhors_verifier_t verifier;
    mumhors_init_verifier(&verifier, pk_matrix, t, k, l, r, t, rt, t);

    /* Running the tests */
    printf("Running the test cases\n");
    /* Generating random messages from a message seed by hashing it and using it as a new message */
    unsigned char *message_seed = "seed";
    unsigned char buffer1[SHA256_OUTPUT_LEN];
    unsigned char buffer2[SHA256_OUTPUT_LEN];
    unsigned char *message = buffer1;
    unsigned char *hash = buffer2;
    ltc_hash_sha2_256(message, message_seed, 4);

    int *message_indices = malloc(sizeof(int) * k);

    /* Creating randomized messages */
    for (int i = 0; i < tests; i++) {
        gettimeofday(&start_time, NULL);
        if (mumhors_sign_message(&signer, message, SHA256_OUTPUT_LEN) == SIGN_NO_MORE_ROW_FAILED) {
            debug("[Signer] No more rows are left to sign", DEBUG_INF);
            break;
        }
        gettimeofday(&end_time, NULL);
        sign_and_verify += (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_usec - start_time.tv_usec) / 1.0e6;

        gettimeofday(&start_time, NULL);

        if (mumhors_verify_signature(&verifier, signer.signature, message, SHA256_OUTPUT_LEN) ==
            VERIFY_SIGNATURE_INVALID) {
            printf("[Verifier] Signature verification invalid %d\n", i);
            break;
        }
        gettimeofday(&end_time, NULL);
        sign_and_verify += (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_usec - start_time.tv_usec) / 1.0e6;

        ltc_hash_sha2_256(hash, message, SHA256_OUTPUT_LEN);

        /* Now, use the hash as the next message by swapping the pointers */
        unsigned char *t = hash;
        hash = message;
        message = t;
    }

    printf("SIGN & VERIFY: %0.12f\n", sign_and_verify);


#ifdef JOURNAL
    bitmap_report(&signer.bm);
#endif

    mumhors_delete_verifier(&verifier);
    mumhors_delete_signer(&signer);
    free(message_indices);
}