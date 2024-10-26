#include "mumhors.h"
#include "bitmap.h"
#include "sort.h"
#include "math.h"
#include "bits.h"
#include "hash.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#ifdef JOURNAL
/* Timing variables */
static struct timeval start_time, end_time;
static double mumhors_sign_time = 0;
static double mumhors_verify_time = 0;

void mumhors_report_time(int total_tests) {
    printf("-- Total Sign time (w rejection sampling w.o. bitmap): %0.12f s\n", mumhors_sign_time);
    printf("-- Total Verify time (w rejection checking): %0.12f s \n", mumhors_verify_time);

    printf("-- Each sign time (w rejection sampling w.o. bitmap): %0.12f micros\n", mumhors_sign_time/total_tests * 1000000);
    printf("-- Each verify time (w rejection checking): %0.12f micros\n", mumhors_verify_time/total_tests * 1000000);
}
#endif




void mumhors_pk_gen(public_key_matrix_t *pk_matrix, const unsigned char *seed, int seed_len, int row, int col) {
    /* Initialize the linked list variables */
    pk_matrix->head = NULL;
    pk_matrix->tail = NULL;

    for (int i = 0; i < row; i++) {
        /* Create a new public key node and allocating given number of public keys in each row */
        public_key_t *pk_node = malloc(sizeof(public_key_t));
        pk_node->pks = malloc(sizeof(unsigned char *) * col);

        /* Initialized the public keys in the current row with a dummy public key */
        for (int j = 0; j < col; j++) {
            unsigned char *pk = malloc(SHA256_OUTPUT_LEN);
            unsigned char sk[SHA256_OUTPUT_LEN];
            unsigned char *new_seed = malloc(seed_len + 4 + 4);
            memcpy(new_seed, seed, seed_len);
            memcpy(new_seed + seed_len, &i, 4);
            memcpy(new_seed + seed_len + 4, &j, 4);
            blake2b_256(sk, new_seed, seed_len + 4 + 4);
            blake2b_256(pk, sk, SHA256_OUTPUT_LEN);
            pk_node->pks[j] = pk;
            free(new_seed);
        }
        pk_node->number = i;
        pk_node->available_pks = col;
        pk_node->next = NULL;

        /* Add the new public key to the matrix of public keys */
        if (pk_matrix->head == NULL) {
            pk_matrix->head = pk_node;
            pk_matrix->tail = pk_node;
        } else {
            pk_matrix->tail->next = pk_node;
            pk_matrix->tail = pk_node;
        }
    }
}

void
mumhors_init_signer(mumhors_signer_t *signer, unsigned char *seed, int seed_len, int t, int k, int l, int rt, int r) {
    /* Setting the signer hyperparameters */
    signer->seed = seed;
    signer->seed_len = seed_len;
    signer->t = t;
    signer->k = k;
    signer->t = t;
    signer->rt = rt;
    signer->r = r;
    signer->l = l;
    signer->signature.signature = malloc((signer->k * signer->l) / 8);

    /* Initializing the underlying bitmap data structure */
    bitmap_init(&signer->bm, signer->r, signer->t, signer->rt, signer->t);
}

void mumhors_delete_signer(mumhors_signer_t *signer) {
    /* Deallocate the signature buffer and the bitmap */
    free(signer->signature.signature);
    bitmap_delete(&signer->bm);
}

static int check_if_indices_are_distinct(unsigned char *value, int k, int chunk, int *message_indices,
    int** sorted_indices) {

    int *new_indices = malloc(sizeof(int) * k);
    *sorted_indices = new_indices;

    for (int i = 0; i < k; i++) {
        new_indices[i] = read_bits_as_4bytes(value, i + 1, chunk);
        message_indices[i] = new_indices[i];
    }

    array_sort(new_indices, k);

    for (int i = 1; i < k; i++) {
        if (new_indices[i] == new_indices[i - 1]) {
            return 0;
        }
    }
    return 1;
}

static int perform_rejection_sampling(const unsigned char *message, int message_len, int k, int t,
                                      int* message_indices, int** sorted_indices) {
    unsigned char pads[3][32] = {
        {
            0x6b, 0x8f, 0x34, 0x1a, 0xdf, 0x21, 0x5e, 0xa3, 0x79, 0x2d, 0xe7, 0xc1, 0x5b, 0x6a, 0x1b, 0x3f, 0x5c, 0xe0,
            0x1d, 0x8b, 0x3d, 0xf2, 0x7e, 0x4a, 0xe8, 0xb1, 0x5d, 0x9c, 0x6f, 0x43, 0x84, 0x2e
        },
        {
            0xab, 0xf9, 0x27, 0xcd, 0x12, 0xe3, 0x89, 0x45, 0xd8, 0x66, 0x97, 0xa4, 0xbc, 0x8d, 0x5e, 0xf1, 0x4c, 0x32,
            0x7a, 0x90, 0x8f, 0xb3, 0xd9, 0xe6, 0x1e, 0xac, 0x74, 0x91, 0x5b, 0xdf, 0x2c, 0xe5
        },
        {
            0x59, 0x9f, 0x4b, 0x8a, 0x36, 0xf4, 0xa7, 0x28, 0x91, 0x6e, 0x2b, 0x5d, 0xc9, 0x72, 0xf2, 0x13, 0x46, 0x8e,
            0x93, 0xb4, 0xd7, 0x6a, 0xe1, 0x5f, 0x0b, 0xc4, 0x89, 0x71, 0x3d, 0x2a, 0x94, 0xfc
        },
    };

    /* Hash one time */
    unsigned char hash_ctr_buffer[SHA256_OUTPUT_LEN + 4];
    blake2b_256(hash_ctr_buffer, message, message_len);

    if (check_if_indices_are_distinct(hash_ctr_buffer, k, (int) log2(t), message_indices, sorted_indices))
        return 0;


    /* XOR with pads 1-3 and try again */
    for (int j = 0; j < 3; j++) {

        for (int i = 0; i < 32; i++)
            hash_ctr_buffer[i] ^= pads[j][i];
        if (check_if_indices_are_distinct(hash_ctr_buffer, k, (int) log2(t), message_indices, sorted_indices))
            return 0;
    }

    /* Use Ctr to resolve */
    unsigned int ctr = 0;
    while (1) {
        unsigned char hash_result[SHA256_OUTPUT_LEN];
        mempcpy(hash_ctr_buffer + SHA256_OUTPUT_LEN, &ctr, sizeof(ctr));
        blake2b_256(hash_result, hash_ctr_buffer, SHA256_OUTPUT_LEN + sizeof(ctr));

        if (check_if_indices_are_distinct(hash_result, k, (int) log2(t), message_indices, sorted_indices))
            return ctr;
        ctr++;

        /* Overflow the unsigned counter variable*/
        if (ctr == 0)
            assert(ctr != 0);
    }

    return ctr;
}

static int check_rejection_sampling(const unsigned char *message, int message_len, int k, int t, int *indices,
                                    unsigned int ctr, int** sorted_indices) {
    unsigned char pads[3][32] = {
        {
            0x6b, 0x8f, 0x34, 0x1a, 0xdf, 0x21, 0x5e, 0xa3, 0x79, 0x2d, 0xe7, 0xc1, 0x5b, 0x6a, 0x1b, 0x3f, 0x5c, 0xe0,
            0x1d, 0x8b, 0x3d, 0xf2, 0x7e, 0x4a, 0xe8, 0xb1, 0x5d, 0x9c, 0x6f, 0x43, 0x84, 0x2e
        },
        {
            0xab, 0xf9, 0x27, 0xcd, 0x12, 0xe3, 0x89, 0x45, 0xd8, 0x66, 0x97, 0xa4, 0xbc, 0x8d, 0x5e, 0xf1, 0x4c, 0x32,
            0x7a, 0x90, 0x8f, 0xb3, 0xd9, 0xe6, 0x1e, 0xac, 0x74, 0x91, 0x5b, 0xdf, 0x2c, 0xe5
        },
        {
            0x59, 0x9f, 0x4b, 0x8a, 0x36, 0xf4, 0xa7, 0x28, 0x91, 0x6e, 0x2b, 0x5d, 0xc9, 0x72, 0xf2, 0x13, 0x46, 0x8e,
            0x93, 0xb4, 0xd7, 0x6a, 0xe1, 0x5f, 0x0b, 0xc4, 0x89, 0x71, 0x3d, 0x2a, 0x94, 0xfc
        },
    };

    /* Hash one time */
    unsigned char hash_ctr_buffer[SHA256_OUTPUT_LEN + 4];
    blake2b_256(hash_ctr_buffer, message, message_len);

    if (check_if_indices_are_distinct(hash_ctr_buffer, k, (int) log2(t), indices, sorted_indices))
        return 1;


    /* XOR with pads 1-3 and try again */
    for (int j = 0; j < 3; j++) {
        for (int i = 0; i < 32; i++)
            hash_ctr_buffer[i] ^= pads[j][i];
        if (check_if_indices_are_distinct(hash_ctr_buffer, k, (int) log2(t), indices, sorted_indices))
            return 1;
    }

    /* Use Ctr to resolve */
    unsigned char target_hash[SHA256_OUTPUT_LEN];
    mempcpy(hash_ctr_buffer + SHA256_OUTPUT_LEN, &ctr, sizeof(ctr));
    blake2b_256(target_hash, hash_ctr_buffer, SHA256_OUTPUT_LEN + sizeof(ctr));

    if (check_if_indices_are_distinct(target_hash, k, (int) log2(t), indices, sorted_indices))
        return 1;

    return 0;
}

int mumhors_sign_message(mumhors_signer_t *signer, const unsigned char *message, int message_len) {
    int *message_indices = malloc(sizeof(int) * signer->k);

    /* Hashing the message */
    // unsigned char message_hash[SHA256_OUTPUT_LEN];
    // blake2b_256(message_hash, message, message_len);

    /* Extract the indices from the hash of the message while ensuring they are different
     * through a process known as rejection sampling. */

    unsigned char *new_seed = malloc(signer->seed_len + 4 + 4);

#ifdef JOURNAL
    gettimeofday(&start_time, NULL);
#endif

    int* sorted_indices;
    signer->signature.ctr = perform_rejection_sampling(message, message_len, signer->k, signer->t, message_indices,
        &sorted_indices);


    for (int i = 0; i < signer->k; i++) {
        int row_number, col_number;
        /* Getting the row and colum numbers for the given index */
        bitmap_get_row_colum_with_index(&signer->bm, message_indices[i], &row_number, &col_number);

        /* Create the respective private key and build the signature */
        /* Create the respective private key and build the signature */
        unsigned char sk[SHA256_OUTPUT_LEN];
        memcpy(new_seed, signer->seed, signer->seed_len);
        memcpy(new_seed + signer->seed_len, &row_number, 4);
        memcpy(new_seed + signer->seed_len + 4, &col_number, 4);
        blake2b_256(sk, new_seed, signer->seed_len + 4 + 4);
        memcpy(signer->signature.signature + i * SHA256_OUTPUT_LEN, sk, SHA256_OUTPUT_LEN);
    }

#ifdef JOURNAL
    gettimeofday(&end_time, NULL);
    mumhors_sign_time += (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_usec - start_time.tv_usec) / 1.0e6;
#endif

    free(new_seed);
    /* Unsetting the indices in the bitmap */
    bitmap_unset_indices_in_window(&signer->bm, sorted_indices, signer->k);
    free(sorted_indices);
    free(message_indices);



    /* Extending the bitmap matrix for later usage. This can be optimized to be done every t/k messages */
    if (bitmap_extend_matrix(&signer->bm) == BITMAP_EXTENSION_FAILED)
        return SIGN_NO_MORE_ROW_FAILED;
    return SIGN_SUCCESS;
}


void
mumhors_init_verifier(mumhors_verifier_t *verifier, public_key_matrix_t pk_matrix, int t, int k, int l, int r, int c,
                      int rt, int window_size) {
    /* Setting the hyper parameters of the verifier */
    verifier->t = t;
    verifier->k = k;
    verifier->l = l;
    verifier->r = r;
    verifier->c = c;
    verifier->rt = rt;
    verifier->active_pks = verifier->rt * verifier->c;
    verifier->windows_size = window_size;
    verifier->nxt_row_number = verifier->rt; /* We consider rt number of rows in our window initially */
    verifier->pk_matrix = pk_matrix;
}

void mumhors_delete_verifier(const mumhors_verifier_t *verifier) {
    /* The verifier is public key consumer and hence, it is responsible to deallocate the
     * memory allocated for it in the key generation function. This is required, as in MUM-HORS
     * a huge number of public keys are stored on the verifier side, which need to be removed as they are used
     * to free some storage */
    public_key_t *pk_row = verifier->pk_matrix.head;
    while (pk_row) {
        for (int i = 0; i < verifier->c; i++)
            if (pk_row->pks[i])
                free(pk_row->pks[i]); /* Free each public key */

        /* Free the array of PK addresses */
        free(pk_row->pks);
        public_key_t *target = pk_row;
        pk_row = pk_row->next;

        /* Free the PK node */
        free(target);
    }
}


/// Performs a clean up on the matrix rows by removing rows that have been depleted (0 PKs)
/// \param verifier Pointer to MUMHORS verifier struct
/// \return Number of cleaned rows
static int mumhors_verifier_cleanup_rows(mumhors_verifier_t *verifier) {
    /* Count the number of rows we cleaned */
    int cleaned_rows = 0;
    public_key_t *pk_row = verifier->pk_matrix.head;
    // public_key_t *prev;

    /* Loop and remove the row with 0 remained public key */
    while (pk_row) {
        if (!pk_row->available_pks) {
            if (pk_row == verifier->pk_matrix.head)
                verifier->pk_matrix.head = verifier->pk_matrix.head->next;
            else {
                public_key_t *prev = verifier->pk_matrix.head;
                while (prev->next != pk_row) { prev = prev->next; }
                prev->next = pk_row->next;

                /* Updating the tail pointer */
                if (pk_row == verifier->pk_matrix.tail)
                    verifier->pk_matrix.tail = prev;
            }
            cleaned_rows++;

            public_key_t *target_pk = pk_row;
            pk_row = pk_row->next;

            /* Deallocating the array of PK addresses.
             * Note that the actual PKs have been deallocated during unsetting */
            free(target_pk->pks);
            free(target_pk);
        } else
            pk_row = pk_row->next;
    }
    return cleaned_rows;
}


/// Given a pointer to a row of the matrix, it removes that row from the matrix (linked list)
/// \param verifier Pointer to MUMHORS verifier struct
/// \param pk_row Pointer to the desired row
static void mumhors_verifier_remove_row(mumhors_verifier_t *verifier, public_key_t *pk_row) {
    if (pk_row == verifier->pk_matrix.head)
        verifier->pk_matrix.head = verifier->pk_matrix.head->next;
    else {
        public_key_t *temp = verifier->pk_matrix.head;
        while (temp->next != pk_row) { temp = temp->next; }
        temp->next = pk_row->next;
        if (pk_row == verifier->pk_matrix.tail)
            verifier->pk_matrix.tail = temp;
    }
    verifier->active_pks -= pk_row->available_pks;

    /* Deallocating the public key node */
    free(pk_row->pks);
    free(pk_row);
}

/// This function adds a new row to the verifier's view of the public keys. This function is called virtual, as the
/// verifier virtually follows the signer's approach for verification without storing signer's bitmap data structure.
/// \param verifier Pointer to MUMHORS verifier struct
/// \return PKMATRIX_NO_MORE_ROWS_TO_ALLOCATE or PKMATRIX_MORE_ROW_ALLOCATION_SUCCESS
static int mumhors_verifier_alloc_row_virtually(mumhors_verifier_t *verifier) {
    if (verifier->nxt_row_number >= verifier->r)
        return PKMATRIX_NO_MORE_ROWS_TO_ALLOCATE;

    /* Number of removed rows in this allocation */
    int cnt_removed_rows;

    /* This cleanup always removes rows that have been depleted. It is possible that many of such rows
     * exist in the matrix. Hence, optimizing the code by removing the row with fewest PKs only removes
     * a single row. Moreover, Removing one row each time, can increase the number of calls to clean up process.
     * We note that, considering the row with fewest PKs and remove any row with such number, is not a solution as
     * it might happen that our matrix has no row with zero and many with X PKs. Hence, the number of discard bits will
     * increase and is not desirable. */
    if (!(cnt_removed_rows = mumhors_verifier_cleanup_rows(verifier))) {
        /* No row was cleaned up. We perform the same policy as the signer by removing a/the row with
         * the fewest number of PKs*/

        /* Find the row the fewest number of pks to be deleted */
        public_key_t *pk_row = verifier->pk_matrix.head;
        public_key_t *target_row;
        int max_seen_pks = verifier->c;

        while (pk_row) {
            if (pk_row->available_pks < max_seen_pks) {
                max_seen_pks = pk_row->available_pks;
                target_row = pk_row;
            }
            pk_row = pk_row->next;
        }
        /* The row is found. Delete the row */
        mumhors_verifier_remove_row(verifier, target_row);

        /* We only removed one row */
        cnt_removed_rows = 1;
    }

    /* Add more rows virtually to the window. They are virtual as
     * rows are already in the memory. So we only add them to the list of active rows.*/
    int possible_rows_to_add = min(cnt_removed_rows, verifier->r - verifier->nxt_row_number);
    verifier->active_pks += possible_rows_to_add * verifier->c;
    verifier->nxt_row_number += possible_rows_to_add;

    return PKMATRIX_MORE_ROW_ALLOCATION_SUCCESS;
}

/// This function verifies the received signature with its stored public keys. The intention behind calling this
/// function virtual, is because the verifier virtually follows the signer's approach for verification without storing
/// signer's bitmap data structure.
/// \param verifier Pointer to MUMHORS verifier struct
/// \param indices List of indices to be used for signature verification
/// \param num_indices Number of passed indices
/// \param signature Pointer to the signature
/// \return VERIFY_SIGNATURE_INVALID or VERIFY_SIGNATURE_INVALID, or VERIFY_SIGNATURE_VALID
static int verify_signature_using_virtual_matrix(mumhors_verifier_t *verifier, const int *indices, int num_indices,
                                                 const unsigned char *signature) {



    if (verifier->windows_size > verifier->active_pks) {
        if (mumhors_verifier_alloc_row_virtually(verifier) == PKMATRIX_NO_MORE_ROWS_TO_ALLOCATE)
            return VERIFY_SIGNATURE_INVALID;
    }

    /* A buffer for extracting the private key from the signature */
    unsigned char *sk = malloc(verifier->l / 8);

    /* Verification status */
    int ver_status = 1;

#ifdef JOURNAL
    gettimeofday(&start_time, NULL);
#endif


    /* Retrieving the row and column numbers for the provided indices */
    for (int i = 0; i < num_indices; i++) {
        int target_index = indices[i];

        /* Finding the row containing the target index */
        public_key_t *pk_row = verifier->pk_matrix.head;
        while (pk_row) {
            if (target_index < pk_row->available_pks)
                break;
            target_index -= pk_row->available_pks;
            pk_row = pk_row->next;
        }

        /* The current row contains the public key. Find the public key */
        unsigned char *target_pk;
        for (int j = 0; j < verifier->c; j++) {
            if (pk_row->pks[j]) {
                // If the PK is not NULL
                if (target_index == 0) {
                    target_pk = pk_row->pks[j];
                    break;
                }
                target_index--;
            }
        }

        /* Extract the corresponding private key and hash it */
        memcpy(sk, signature + i * verifier->l / 8, verifier->l / 8);
        unsigned char sk_hash[SHA256_OUTPUT_LEN];
        blake2b_256(sk_hash, sk, verifier->l / 8);

        /* Compare the hash with the current public key*/
        if (strncmp(target_pk, sk_hash, SHA256_OUTPUT_LEN) != 0) {
            free(sk);
            ver_status = 0;
            break;
        }
    }


    /* Sort the indices.
     * Description: The rationale behind first soring and then unsetting is that, when the indices
     * are given in not-ordered fashion, then if we try to unset the first one, we loose the information
     * for the next index. For instance, if the set {5, 7, 10} is given, when we unset 5, the bit at index 7
     * becomes index 7 and the 10 becomes 8. But this is more complicated because if the set is {5, 1, 10}, then
     * unsetting 5, will not impact 1 but impacts 7 and changes it to 9 but not even 8. Sorting the indices can
     * resolve this issue. However, for the usage in MUM-HORS, we shall not provide a sorted value back. So either
     * we should maintain a data structure that returns back the actual order before sorting, or, first perform
     * fetching the values and then remove the indices.
     * */
    /* First sort the indices */
    for (int i = 0; i < num_indices; i++) {
        int target_index = indices[i];

        /* Invalidate the target index */
        public_key_t *pk_row = verifier->pk_matrix.head;

        while (pk_row) {
            if (target_index < pk_row->available_pks)
                break;
            target_index -= pk_row->available_pks;
            pk_row = pk_row->next;
        }

        for (int j = 0; j < verifier->c; j++) {
            if (pk_row->pks[j]) {
                // If the PK is not NULL
                if (target_index == 0) {
                    /* Free up the used public key */
                    free(pk_row->pks[j]);
                    pk_row->pks[j] = NULL;
                    pk_row->available_pks--;
                    verifier->active_pks--;
                    break;
                }
                target_index--;
            }
        }
    }
#ifdef JOURNAL
    gettimeofday(&end_time, NULL);
    mumhors_verify_time += (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_usec - start_time.tv_usec) / 1.0e6;
#endif
    if (!ver_status)
        return VERIFY_SIGNATURE_INVALID;

    return VERIFY_SIGNATURE_VALID;
}


int mumhors_verify_signature(mumhors_verifier_t *verifier, const mumhors_signature_t *signature,
                             const unsigned char *message, int message_len) {
    int *message_indices = malloc(sizeof(int) * verifier->k);

    /* Extract the indices from the hash of the message while ensuring they are different
     * through a process known as rejection sampling. */
    int *sorted_indices;
    int verify_status;


    /* End the time here */
#ifdef JOURNAL
    gettimeofday(&start_time, NULL);
#endif
    if (check_rejection_sampling(message, message_len, verifier->k, verifier->t, message_indices, signature->ctr,
                                 &sorted_indices) == 0) {
        verify_status = VERIFY_SIGNATURE_INVALID;
                                 }
#ifdef JOURNAL
    gettimeofday(&end_time, NULL);
    mumhors_verify_time += (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_usec - start_time.tv_usec) / 1.0e6;
#endif

    verify_status = verify_signature_using_virtual_matrix(verifier, message_indices, verifier->k,
                                                          signature->signature);

    free(message_indices);
    free(sorted_indices);
    return verify_status;
}
