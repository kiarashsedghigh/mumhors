#include <mumhors/bitmap.h>
#include <mumhors/debug.h>
#include <mumhors/sort.h>
#include <mumhors/mumhors.h>
#include <stdlib.h>
#include <stdio.h>

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
    int t, k, l, ir, rt, tests;
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

    t = 1024; k = 16; l = 16385; rt = 12; tests = 1048576;
    /* Generating the public key from the seed to be provisioned to the verifier.
     * The signer only needs to have access to the seed as not precomputing the private key
     * is the exact goal of this program. */
    public_key_matrix_t pk_matrix;
    mumhors_pk_gen(&pk_matrix, "seed", l, t);

    /* Create and initialize the signer */
    mumhors_signer_t signer;
    mumhors_init_signer(&signer, "seed", t, l, ir, rt, l);

    /* Create and initialize the verifier */
    mumhors_verifier_t verifier;
    mumhors_init_verifier(&verifier, pk_matrix, l, t, rt, t);


    /* Running the tests */
    int *indices = malloc(sizeof(int) * k);

    for (int i = 0; i < tests; i++) {
        generate_rands(indices, k, t);

        /* First sign the message */ //TODO The name of the following functions will be changed
        if (bitmap_unset_index_in_window(&signer.bm, indices, k) == BITMAP_UNSET_BITS_FAILED) {
            printf("---> Last covered message: %d\n", i);
            debug("[SIGNER] No more rows to allocate", DEBUG_ERR);
            break;
        }
        if (mumhors_verify(&verifier, indices, k) == VERIFY_NO_MORE_ROW_FAILED) {
            printf("---> Last covered message: %d\n", i);
            debug("[Verifier] No more rows to allocate", DEBUG_ERR);
            break;
        }
    }

#ifdef JOURNAL
    bitmap_report(&signer.bm);
#endif

    mumhors_delete_verifier(&verifier);
    mumhors_delete_signer(&signer);
    free(indices);
}