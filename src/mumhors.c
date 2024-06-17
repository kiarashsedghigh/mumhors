#include <mumhors/mumhors.h>
#include <mumhors/bitmap.h>
#include <mumhors/sort.h>
#include <mumhors/math.h>
#include <stdlib.h>
#include <string.h>

void mumhors_pk_gen(public_key_matrix_t *pk_matrix, unsigned char *seed, int row, int col) {
    /* Initialize the linked list variables */
    pk_matrix->head = NULL;
    pk_matrix->tail = NULL;

    for (int i = 0; i < row; i++) {
        /* Create a new public key node and allocating given number of public keys in each row */
        public_key_t *pk_node = malloc(sizeof(public_key_t));
        pk_node->pks = malloc(sizeof(unsigned char *) * col);

        /* Initialized the public keys in the current row with a dummy public key */
        for (int i = 0; i < col; i++) {
            pk_node->pks[i] = malloc(10);
            strcpy(pk_node->pks[i], "Dummy"); //TODO use random generator
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

void mumhors_init_signer(mumhors_signer_t *signer, unsigned char *seed, int t, int k, int ir, int rt, int r) {
    /* Setting the signer hyperparameters */
    signer->seed = seed;
    signer->t = t;
    signer->k = k;
    signer->t = t;
    signer->rt = rt;
    signer->r = r;

    /* Initializing the underlying bitmap data structure */
    bitmap_init(&signer->bm, signer->r, signer->t, signer->rt, signer->t);
}

void mumhors_delete_signer(mumhors_signer_t *signer) {
    /* Only bitmap will be deallocated */
    bitmap_delete(&signer->bm);
}

void mumhors_init_verifier(mumhors_verifier_t *verifier, public_key_matrix_t pk_matrix, int r, int c,
                           int rt, int window_size) {
    /* Setting the hyperparameters of the verifier */
    verifier->r = r;
    verifier->c = c;
    verifier->rt = rt;
    verifier->active_pks = verifier->rt * verifier->c;
    verifier->windows_size = window_size;
    verifier->nxt_row_number = verifier->rt;    /* We consider rt number of rows in our window initially */
    verifier->pk_matrix = pk_matrix;
}

void mumhors_delete_verifier(mumhors_verifier_t *verifier) {
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
    public_key_t *prev;

    /* Loop and remove the row with 0 remained public key */
    while (pk_row) {
        if (!pk_row->available_pks) {
            if (pk_row == verifier->pk_matrix.head)
                verifier->pk_matrix.head = verifier->pk_matrix.head->next;
            else {
                prev = verifier->pk_matrix.head;
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
        while (temp->next != pk_row) { temp = temp->next; };
        temp->next = pk_row->next;
        if (pk_row == verifier->pk_matrix.tail)
            verifier->pk_matrix.tail = temp;
    }
    verifier->active_pks -= pk_row->available_pks;

    /* Deallocating the public key node */
    free(pk_row->pks);
    free(pk_row);
}


static int mumhors_verifier_alloc_row(mumhors_verifier_t *verifier) {
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
    if (!(cnt_removed_rows= mumhors_verifier_cleanup_rows(verifier))) {
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

        /* We only remove one row */
        cnt_removed_rows=1;
    }

    /* Add more rows virtually to the window. They are virtual as
     * rows are already in the memory. So we only add them to the list of active rows.*/
    int possible_rows_to_add = min(cnt_removed_rows, verifier->r - verifier->nxt_row_number);
    verifier->active_pks += possible_rows_to_add * verifier->c;
    verifier->nxt_row_number += possible_rows_to_add;

    return PKMATRIX_MORE_ROW_ALLOCATION_SUCCESS;
}


int mumhors_verify(mumhors_verifier_t *verifier, int *indices, int num_indices) {
    if (verifier->windows_size > verifier->active_pks) {
        if (mumhors_verifier_alloc_row(verifier) == PKMATRIX_NO_MORE_ROWS_TO_ALLOCATE)
            return VERIFY_NO_MORE_ROW_FAILED;
    }

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

        /* The current row contains the public keys */
        for (int j = 0; j < verifier->c; j++) {
            if (pk_row->pks[j]) { // If the PK is not NULL
                if (target_index == 0) {
                    printf("R: %d C: %d", pk_row->number, j);
                    break;
                }
                target_index--;
            }
        }
        printf("\n");
    }


    /*
     *  Unsetting the indices.
     *  //TODO Can we do better than this?
     * */
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
    merge_sort(indices, 0, num_indices - 1);

    int index_diff = 0;
    for (int i = 0; i < num_indices; i++) {
        if (i > 0 && indices[i] == indices[i - 1]) continue;
        int target_index = indices[i] - index_diff;
        index_diff++;

        /* Invalidate the target index */
        public_key_t *pk_row = verifier->pk_matrix.head;

        while (pk_row) {
            if (target_index < pk_row->available_pks)
                break;
            target_index -= pk_row->available_pks;
            pk_row = pk_row->next;
        }

        for (int j = 0; j < verifier->c; j++) {
            if (pk_row->pks[j]) { // If the PK is not NULL
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
    return VERIFY_SUCCESS;
}


void pk_display(mumhors_verifier_t *verifier) {
    public_key_t *pk_row = verifier->pk_matrix.head;

    int cnt_rows_to_show = verifier->rt;

    while (pk_row && cnt_rows_to_show) {
        printf("#:%d (%d) ", pk_row->number, pk_row->available_pks);
        for (int i = 0; i < verifier->c; i++) {
            if (pk_row->pks[i])
                printf("1 ");
            else
                printf("0 ");
        }
        cnt_rows_to_show--;
        pk_row = pk_row->next;
        printf("\n");
    }
    printf("-------%d-------\n", verifier->active_pks);
}

