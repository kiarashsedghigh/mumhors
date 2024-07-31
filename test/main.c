#include "debug.h"
#include "mumhors.h"
#include "hash.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <assert.h>


int main(int argc, char **argv) {
    if (argc < 8) {
        printf("|HELP|\n\tRun:\n");
        printf("\t\t mumhors T K L R RT TESTS SEED_FILE\n");
        exit(1);
    }

    /*
     * Reading the seed
     */
    FILE *fp;
    assert((fp=fopen(argv[7], "r")) != NULL);
    fseek(fp, 0L, SEEK_END);
    int seed_len = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    unsigned char *seed = malloc(seed_len);
    fread(seed, seed_len, 1, fp);

    const int t = atoi(argv[1]);
    const int k = atoi(argv[2]);
    const int l = atoi(argv[3]);
    const int r = atoi(argv[4]);
    const int rt = atoi(argv[5]);
    const int tests = atoi(argv[6]);


    /*
     *
     *  Key generation
     *
     */
    struct timeval start_time, end_time;

    /* Generating the public key from the seed to be provisioned to the verifier.
     * The signer only needs to have access to the seed as not precomputing the private key
     * is the exact goal of this program. */

    debug("Generating the public keys ...", DEBUG_INF);
    public_key_matrix_t pk_matrix;

    gettimeofday(&start_time, NULL);
    mumhors_pk_gen(&pk_matrix, seed, seed_len, r, t);
    gettimeofday(&end_time, NULL);
    double keygen_time = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_usec - start_time.tv_usec) / 1.0e6;
    printf("KEYGEN: %0.12f\n", keygen_time);


    /*
     *
     *  Signing and Verifying
     *
     */
    double sign_time = 0;
    double verify_time = 0;

    /* Create and initialize the signer */
    mumhors_signer_t signer;
    mumhors_init_signer(&signer, seed, seed_len, t, k, l, rt, r);

    /* Create and initialize the verifier */
    mumhors_verifier_t verifier;
    mumhors_init_verifier(&verifier, pk_matrix, t, k, l, r, t, rt, t);

        /* Running the tests */
        debug("Running the test cases ...", DEBUG_INF);

        /* Generating random messages from a message seed by hashing it and using it as a new message */
        unsigned char buffer1[SHA256_OUTPUT_LEN];
        unsigned char buffer2[SHA256_OUTPUT_LEN];
        unsigned char *message = buffer1;
        unsigned char *hash = buffer2;
        ltc_hash_sha2_256(message, seed, seed_len);

        int *message_indices = malloc(sizeof(int) * k);

        /* Creating randomized messages */
        int message_index;
        for (message_index = 0; message_index < tests; message_index++) {
            printf("\r[%d/%d]", message_index, tests);
            fflush(stdout);
            gettimeofday(&start_time, NULL);
            if (mumhors_sign_message(&signer, message, SHA256_OUTPUT_LEN) == SIGN_NO_MORE_ROW_FAILED) {
                debug("\n[Signer] No more rows are left to sign", DEBUG_INF);
                break;
            }
            gettimeofday(&end_time, NULL);
            sign_time += (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_usec - start_time.tv_usec) / 1.0e6;
            gettimeofday(&start_time, NULL);
            if (mumhors_verify_signature(&verifier, &signer.signature, message, SHA256_OUTPUT_LEN) ==
                VERIFY_SIGNATURE_INVALID) {
                printf("\n[Verifier] Signature verification invalid %d\n", message_index);
                break;
            }
            gettimeofday(&end_time, NULL);
            verify_time += (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_usec - start_time.tv_usec) / 1.0e6;

            /* Now, use the hash as the next message by swapping the pointers */
            ltc_hash_sha2_256(hash, message, SHA256_OUTPUT_LEN);
            unsigned char *swap = hash;
            hash = message;
            message = swap;
        }

        printf("\n================ Signing Report ================\n");
        printf("Signed message: %d/%d\n", message_index, tests);
        printf("Sign time: %0.12f\n", sign_time);
        printf("Verify time: %0.12f\n", verify_time);

    #ifdef JOURNAL
        bitmap_report(&signer.bm);
    #endif

    mumhors_delete_verifier(&verifier);
    mumhors_delete_signer(&signer);
    free(message_indices);
}
