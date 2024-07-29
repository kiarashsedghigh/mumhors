#include "bitmap.h"
#include "sort.h"
#include "math.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#define BYTES2BITS(x) (x*8)

/// Freeing a row of the Bitmap
/// \param row Pointer to the row
static void bitmap_free_row(row_t *row) {
    free(row->data);
    free(row);
}

/// Adds a row to the matrix of rows
/// \param bm Pointer to the bitmap structure
/// \param row Pointer to the new row
static void bitmap_matrix_list_add_row(bitmap_t *bm, row_t *row) {
    assert(bm!=NULL);

    if (!bm->bitmap_matrix.head) {
        bm->bitmap_matrix.head = row;
        bm->bitmap_matrix.tail = row;
    } else {
        bm->bitmap_matrix.tail->next = row;
        bm->bitmap_matrix.tail = row;
    }
}


void bitmap_init(bitmap_t *bm, int rows, int cols, int row_threshold, int window_size) {
    /* Simple parameter check. This check has been done in this way for simplicity!! */
    assert(cols % 8 == 0);
    assert(row_threshold <= rows);

    /* Setting the hyperparameters */
    bm->r = rows;
    bm->cB = cols / 8;
    bm->rt = row_threshold;
    bm->window_size = window_size;

    /* Allocate the full capacity of the matrix */
    bm->nxt_row_number = bm->rt;
    bm->active_rows = bm->rt;
    bm->set_bits = bm->rt * BYTES2BITS(bm->cB);

#ifdef JOURNAL
    /* If journaling is enabled, initialize the variables to 0 */
    bm->bitmap_report.cnt_cleanup_call = 0;
    bm->bitmap_report.cnt_alloc_more_rows = 0;
    bm->bitmap_report.cnt_count_unset = 0;
    bm->bitmap_report.cnt_discarded_bits = 0;
#endif
    /* Initializing the matrix of rows */
    bm->bitmap_matrix.head = NULL;
    bm->bitmap_matrix.tail = NULL;

    /* Creating the rows and adding them to the matrix */
    for (int i = 0; i < bm->rt; i++) {
        row_t *new_row = malloc(sizeof(row_t));
        new_row->data = malloc(sizeof(unsigned char *) * bm->cB);
        new_row->number = i;
        new_row->set_bits = BYTES2BITS(bm->cB);
        new_row->next = NULL;

        /* Initializing the vector to all 1s */
        for (int j = 0; j < bm->cB; j++) new_row->data[j] = 0xff;

        /* Adding the row to the matrix */
        bitmap_matrix_list_add_row(bm, new_row);
    }
}

void bitmap_delete(bitmap_t *bm) {
    row_t *curr = bm->bitmap_matrix.head;
    while (curr) {
        row_t *target = curr;
        curr = curr->next;
        /* Deleting the rows data */
        bitmap_free_row(target);
    }
    bm->bitmap_matrix.head = NULL;
    bm->bitmap_matrix.tail = NULL;
}

/// Cleanup the matrix by removing the rows that have no set bits left to use
/// \param bm Pointer to the bitmap structure
/// \return Returns number of cleaned(removed) rows
static int bitmap_row_cleanup(bitmap_t *bm) {
#ifdef JOURNAL
    bm->bitmap_report.cnt_cleanup_call++;
#endif
    /* Count the number of cleaned rows */
    int cleaned_rows = 0;

    row_t *row = bm->bitmap_matrix.head;
    while (row) {
        if (!row->set_bits) {
            /* If the head node is going to be removed */
            if (row == bm->bitmap_matrix.head) {
                bm->bitmap_matrix.head = bm->bitmap_matrix.head->next;
            } else {
                // Optimization with prev complicates the process.
                // Moreover, the list's length is not too long in this application.
                row_t *temp = bm->bitmap_matrix.head;
                while (temp->next != row) { temp = temp->next; };
                temp->next = row->next;

                /* Update the tail pointer */
                if (row == bm->bitmap_matrix.tail)
                    bm->bitmap_matrix.tail = temp;
            }
            /* Deallocate the row */
            row_t *target_to_delete = row;
            row = row->next;
            bitmap_free_row(target_to_delete);
            bm->active_rows--;
            cleaned_rows++;
        } else
            row = row->next;
    }
    return cleaned_rows;
}


/// Remove a row from the bitmap matrix based on its index
/// \param bm Pointer to the bitmap structure
/// \param row Pointer to the row
static void bitmap_remove_row(bitmap_t *bm, row_t *row) {
    if (row == bm->bitmap_matrix.head)
        bm->bitmap_matrix.head = bm->bitmap_matrix.head->next;
    else {
        row_t *temp = bm->bitmap_matrix.head;
        while (temp->next != row) { temp = temp->next; };
        temp->next = row->next;
        /* Updating the tail pointer */
        if (row == bm->bitmap_matrix.tail)
            bm->bitmap_matrix.tail = temp;
    }

    /* Updating the hyperparameters */
    bm->active_rows--;
    bm->set_bits -= row->set_bits;
#ifdef JOURNAL
    bm->bitmap_report.cnt_discarded_bits += row->set_bits;
#endif
    bitmap_free_row(row);
}


/// Allocate more new rows
/// \param bm Pointer to the bitmap structure
/// \return BITMAP_MORE_ROW_ALLOCATION_SUCCESS or BITMAP_NO_MORE_ROWS_TO_ALLOCATE
static int bitmap_allocate_more_row(bitmap_t *bm) {
#ifdef JOURNAL
    bm->bitmap_report.cnt_alloc_more_rows++;
#endif
    /* Check if we have any row left to allocate */
    if (bm->nxt_row_number >= bm->r)
        return BITMAP_NO_MORE_ROWS_TO_ALLOCATE;

    /* Check if allocating a new row will pass the threshold of active rows */
    if (bm->active_rows + 1 > bm->rt) {
        /* Perform a cleanup to remove rows that have no set bits.
         * If the cleanup is not successful, remove the row with
         * the least number of set bits if no row was deleted */
        if (!bitmap_row_cleanup(bm)) {
            /* Clean up did not clean anything. So, finding the row with the fewest number of set bits */
            row_t *row = bm->bitmap_matrix.head;
            row_t *target_row;
            int max_set_bits = BYTES2BITS(bm->cB);
            while (row) {
                if (row->set_bits < max_set_bits) {
                    target_row = row;
                    max_set_bits = row->set_bits;
                }
                row = row->next;
            }
            bitmap_remove_row(bm, target_row);
        }
    }

    /* Possible number of rows to allocate */
    int possible_number_of_rows = min(bm->rt - bm->active_rows, bm->r - bm->nxt_row_number);

    /* Fill the matrix with more rows */
    for (int i = 0; i < possible_number_of_rows; i++) {
        row_t *new_row = malloc(sizeof(row_t));
        new_row->data = malloc(sizeof(unsigned char *) * bm->cB);
        new_row->number = bm->nxt_row_number;
        new_row->set_bits = BYTES2BITS(bm->cB);
        new_row->next = NULL;

        /* Initializing the vector to all 1s */
        for (int j = 0; j < bm->cB; j++)
            new_row->data[j] = 0xff;

        /* Updating the hyperparameters */
        bm->active_rows++;
        bm->nxt_row_number++;
        bm->set_bits += BYTES2BITS(bm->cB);

        /* Add the row to the matrix */
        bitmap_matrix_list_add_row(bm, new_row);
    }
    return BITMAP_MORE_ROW_ALLOCATION_SUCCESS;
}


int bitmap_extend_matrix(bitmap_t *bm) {
    /* If there are not enough 1s in the current window, extend the matrix */
    if (bm->window_size > bm->set_bits) {
        if (bitmap_allocate_more_row(bm) == BITMAP_NO_MORE_ROWS_TO_ALLOCATE)
            return BITMAP_EXTENSION_FAILED;
    }
    return BITMAP_EXTENSION_SUCCESS;
}


void bitmap_get_row_colum_with_index(bitmap_t *bm, int target_index, int *row_num, int *col_num) {
    /* Find the row containing our desired index */
    row_t *row = bm->bitmap_matrix.head;
    while (row) {
        if (target_index < row->set_bits)
            break;
        target_index -= row->set_bits;
        row = row->next;
    }

    /* The current row contains the desired index */
    for (int j = 0; j < bm->cB; j++) {
        if (row->data[j]) {
            // Skip 0 bytes
            int cnt_ones = count_num_set_bits(row->data[j]);
            if (target_index < cnt_ones) {
                /* Find the real index of the target_index'th bit in the current byte */
                int bit_idx = byte_get_index_nth_set(row->data[j], target_index + 1);

                /* Return the row number and column number */
                *row_num = row->number;
                *col_num = j * 8 + bit_idx;
                break;
            }
            target_index -= cnt_ones;
        }
    }
}


void bitmap_unset_indices_in_window(bitmap_t *bm, int *indices, int num_index) {
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
    merge_sort(indices, 0, num_index - 1);

    int index_diff = 0;
    for (int i = 0; i < num_index; i++) {
        if (i > 0 && indices[i] == indices[i - 1]) continue;
        int target_index = indices[i] - index_diff;
        index_diff++;

        /* Find the row containing our desired index */
        row_t *row = bm->bitmap_matrix.head;
        while (row) {
            if (target_index < row->set_bits)
                break;
            target_index -= row->set_bits;
            row = row->next;
        }

        /* The current row contains the desired index */
        for (int j = 0; j < bm->cB; j++) {
            if (row->data[j]) {
                // Skip 0 bytes
                int cnt_ones = count_num_set_bits(row->data[j]);
                if (target_index < cnt_ones) {
                    /* Find the real index of the target_index'th bit in the current byte */
                    int bit_idx = byte_get_index_nth_set(row->data[j], target_index + 1);
                    row->data[j] &= 0xff - (1 << (8 - bit_idx - 1));
                    bm->set_bits--;
                    row->set_bits--;
                    break;
                } else
                    target_index -= cnt_ones;
            }
        }
    }
#ifdef JOURNAL
    bm->bitmap_report.cnt_count_unset++;
#endif
}


#ifdef JOURNAL

void bitmap_report(bitmap_t *bm) {
    printf("\n----------Bitmap Report--------\n");
    printf("Row cleanup calls: %d\n", bm->bitmap_report.cnt_cleanup_call);
    printf("Row alloc calls: %d\n", bm->bitmap_report.cnt_alloc_more_rows);
    printf("Index unsetting calls: %d\n", bm->bitmap_report.cnt_count_unset);
    printf("Discarded bits: %d/%d\n", bm->bitmap_report.cnt_discarded_bits, bm->r * BYTES2BITS(bm->cB));
}

#endif

