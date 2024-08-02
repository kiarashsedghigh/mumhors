#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "bitmap.h"
#include "sort.h"
#include "math.h"
#include <sys/time.h>

#define BYTES2BITS(x) (x*8)

struct timeval start_time, end_time;


/// Freeing a row of the Bitmap
/// \param row Pointer to the row
static void bitmap_free_row(row_t *row) {
#ifdef BITMAP_LIST
    free(row->data);
    free(row); /* Only in the linked list representation, rows are allocated from the heap */
#endif
}


/// Macro function for adding a row to the linked list of rows
/// @param row Pointer to the row to be added to the list
#define BITMAP_LIST_ADD_ROW(row) \
    if (!bm->bitmap_matrix.head) {\
        bm->bitmap_matrix.head = row;\
        bm->bitmap_matrix.tail = row;\
    } else {\
        bm->bitmap_matrix.tail->next = row;\
        bm->bitmap_matrix.tail = row;\
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
#ifdef BITMAP_LIST
    bm->bitmap_matrix.head = NULL;
    bm->bitmap_matrix.tail = NULL;
#elif BITMAP_ARRAY
    bm->bitmap_matrix.head = -1;
    bm->bitmap_matrix.tail = -1;
    bm->bitmap_matrix.size = bm->rt;
#endif

    /* Allocate the full capacity of the bitmap */
    bm->nxt_row_number = bm->rt;
    bm->active_rows = bm->rt;
    bm->set_bits = bm->rt * BYTES2BITS(bm->cB);

#ifdef JOURNAL
    /* If journaling is enabled, initialize the variables to 0 */
    bm->bitmap_report.cnt_call_direct_remove_row = 0;
    bm->bitmap_report.cnt_call_cleanup_call = 0;
    bm->bitmap_report.cnt_call_alloc_more_rows_call = 0;
    bm->bitmap_report.cnt_cnt_unset_call = 0;
    bm->bitmap_report.cnt_discarded_bits = 0;
    bm->bitmap_report.cnt_call_cleanup_rows_removed = 0;
    bm->bitmap_report.cnt_discarded_rows = 0;
    bm->bitmap_report.cnt_cnt_get_row_col_call = 0;

    bm->bitmap_report.total_time_cleanup = 0;
    bm->bitmap_report.total_time_remove_row = 0;
    bm->bitmap_report.total_time_get_row_col = 0;
    bm->bitmap_report.total_time_unset_bits = 0;

#endif


#ifdef BITMAP_LIST
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
        BITMAP_LIST_ADD_ROW(new_row);
    }

#elif BITMAP_ARRAY

    for (int i = 0; i < bm->rt; i++) {
        if (bm->bitmap_matrix.head == -1)
            bm->bitmap_matrix.head = bm->bitmap_matrix.tail = 0;
        else if (bm->bitmap_matrix.tail == bm->bitmap_matrix.size - 1)
            bm->bitmap_matrix.tail = 0;
        else
            bm->bitmap_matrix.tail++;

        row_t *new_row = &bm->bitmap_matrix.rows[bm->bitmap_matrix.tail];
        new_row->number = i;
        new_row->set_bits = BYTES2BITS(bm->cB);

        /* Initializing the vector to all 1s */
        for (int j = 0; j < bm->cB; j++) new_row->data[j] = 0xff;
    }


#endif
}

void bitmap_delete(bitmap_t *bm) {
#ifdef BITMAP_LIST
    row_t *curr = bm->bitmap_matrix.head;
    while (curr) {
        row_t *target = curr;
        curr = curr->next;
        /* Deleting the rows data */
        bitmap_free_row(target);
    }
#endif
}


#ifdef BITMAP_ARRAY
#define SHIFT_FROM_HEAD_TO_CURRENT_INDEX_AND_UPDATE_HEAD() \
    for(int row = index - 1; row >= bm->bitmap_matrix.head ; row--) \
        bm->bitmap_matrix.rows[row+1] = bm->bitmap_matrix.rows[row]; \
    bm->bitmap_matrix.head = floor_add_mod(bm->bitmap_matrix.head + 1 , bm->bitmap_matrix.size) ;
#define SHIFT_FROM_TAIL_TO_CURRENT_INDEX_AND_UPDATE_HEAD() \
    for(int i = index + 1; i <= bm->bitmap_matrix.tail ; i++) \
        bm->bitmap_matrix.rows[i-1] = bm->bitmap_matrix.rows[i]; \
    bm->bitmap_matrix.tail = floor_add_mod(bm->bitmap_matrix.tail - 1 , bm->bitmap_matrix.size) ;

#define GET_TIME_BITMAP_ARRAY_REMOVE_ROW_BY_INDEX() \
gettimeofday(&end_time, NULL);\
bm->bitmap_report.total_time_remove_row += (end_time.tv_sec - start_time.tv_sec) + (\
    end_time.tv_usec - start_time.tv_usec) / 1.0e6;

/// Removing a row from the array of rows and return the next index to be used
/// \param bm Pointer to the bitmap structure
/// @param index Index of the target row
/// @return Index of the next row to be used
static int bitmap_remove_row_by_index(bitmap_t *bm, int index) {
#ifdef JOURNAL
    bm->bitmap_report.cnt_discarded_bits += bm->bitmap_matrix.rows[index].set_bits;
    gettimeofday(&start_time, NULL);
#endif

    /* Remove the row */
    bm->set_bits -= bm->bitmap_matrix.rows[index].set_bits;
    bm->active_rows--;

    /* Handling the boundaries */
    /* Removing head */
    if (index == bm->bitmap_matrix.head) {
        bm->bitmap_matrix.head = floor_add_mod(bm->bitmap_matrix.head + 1, bm->bitmap_matrix.size);
#ifdef JOURNAL
        GET_TIME_BITMAP_ARRAY_REMOVE_ROW_BY_INDEX()
#endif
        return bm->bitmap_matrix.head;
    }
    /* Removing tail */
    if (index == bm->bitmap_matrix.tail) {
        bm->bitmap_matrix.tail = floor_add_mod(bm->bitmap_matrix.tail - 1, bm->bitmap_matrix.size);
#ifdef JOURNAL
        GET_TIME_BITMAP_ARRAY_REMOVE_ROW_BY_INDEX()
#endif
        return bm->bitmap_matrix.tail;
    }

    /* If head < tail */
    if (bm->bitmap_matrix.head <= bm->bitmap_matrix.tail) {
        if (index <= (bm->bitmap_matrix.tail - bm->bitmap_matrix.head) / 2) {
            SHIFT_FROM_HEAD_TO_CURRENT_INDEX_AND_UPDATE_HEAD()

#ifdef JOURNAL
            GET_TIME_BITMAP_ARRAY_REMOVE_ROW_BY_INDEX()
#endif
            return floor_add_mod(index + 1, bm->bitmap_matrix.size);
        }
        SHIFT_FROM_TAIL_TO_CURRENT_INDEX_AND_UPDATE_HEAD()
#ifdef JOURNAL
        GET_TIME_BITMAP_ARRAY_REMOVE_ROW_BY_INDEX()
#endif
        return floor_add_mod(index, bm->bitmap_matrix.size);
    }
    /* If tail < head */
    if (index > bm->bitmap_matrix.head && index < bm->bitmap_matrix.size) {
        SHIFT_FROM_HEAD_TO_CURRENT_INDEX_AND_UPDATE_HEAD()

#ifdef JOURNAL
        GET_TIME_BITMAP_ARRAY_REMOVE_ROW_BY_INDEX()
#endif
        return floor_add_mod(index + 1, bm->bitmap_matrix.size);
    }
    SHIFT_FROM_TAIL_TO_CURRENT_INDEX_AND_UPDATE_HEAD()

#ifdef JOURNAL
    GET_TIME_BITMAP_ARRAY_REMOVE_ROW_BY_INDEX()
#endif
    return floor_add_mod(index, bm->bitmap_matrix.size);
}
#endif


/// Cleanup the matrix by removing the rows that have no set bits left to use
/// \param bm Pointer to the bitmap structure
/// \return Returns number of cleaned(removed) rows
static int bitmap_row_cleanup(bitmap_t *bm) {
#ifdef JOURNAL
    bm->bitmap_report.cnt_call_cleanup_call++;
    gettimeofday(&start_time, NULL);
#endif
    /* Count the number of cleaned rows */
    int cleaned_rows = 0;

#ifdef BITMAP_LIST
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
#elif BITMAP_ARRAY
    // todo Delegate the row index boundary change to remove function?
    /* In array version, there is no way to detect a deleted row. Hence, we have breaks */
    if (bm->bitmap_matrix.head <= bm->bitmap_matrix.tail) {
        for (int row_index = bm->bitmap_matrix.head; row_index <= bm->bitmap_matrix.tail; row_index++) {
            if (bm->bitmap_matrix.rows[row_index].set_bits == 0) {
                row_index = bitmap_remove_row_by_index(bm, row_index);
                cleaned_rows++;
                if (row_index == bm->bitmap_matrix.tail) break;
                row_index--;
            }
        }
    } else {
        for (int row_index = bm->bitmap_matrix.head; row_index < bm->bitmap_matrix.size; row_index++) {
            if (bm->bitmap_matrix.rows[row_index].set_bits == 0) {
                row_index = bitmap_remove_row_by_index(bm, row_index);
                cleaned_rows++;
                if (row_index == 0) break; /* If head goes from end of array to the beginning */
                row_index--;
            }
        }
        for (int row_index = 0; row_index <= bm->bitmap_matrix.tail; row_index++) {
            if (bm->bitmap_matrix.rows[row_index].set_bits == 0) {
                row_index = bitmap_remove_row_by_index(bm, row_index);
                cleaned_rows++;

                if (row_index == bm->bitmap_matrix.tail) break;
                row_index--;
            }
        }
    }
#endif
#ifdef JOURNAL
    bm->bitmap_report.cnt_call_cleanup_rows_removed += cleaned_rows;
    gettimeofday(&end_time, NULL);
    bm->bitmap_report.total_time_cleanup += (end_time.tv_sec - start_time.tv_sec) + (
        end_time.tv_usec - start_time.tv_usec) / 1.0e6;
#endif
    return cleaned_rows;
}


/// Remove a row from the bitmap matrix based on its index
/// \param bm Pointer to the bitmap structure
/// \param row Pointer to the row
#ifdef BITMAP_LIST
static void bitmap_list_remove_row(bitmap_t *bm, row_t *row) {
#ifdef JOURNAL
    bm->bitmap_report.cnt_discarded_bits += row->set_bits;
    gettimeofday(&start_time, NULL);
#endif

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
    bitmap_free_row(row);

#ifdef JOURNAL
    gettimeofday(&end_time, NULL);
    bm->bitmap_report.total_time_remove_row += (end_time.tv_sec - start_time.tv_sec) + (
        end_time.tv_usec - start_time.tv_usec) / 1.0e6;
#endif
}
#endif


#define BITMAP_AND_FIND_ROW_WITH_MINIMUM_BITS(start, end) \
    for (int i = start; i <= end; i++) {\
        if (bm->bitmap_matrix.rows[i].set_bits < max_set_bits) {\
            max_set_bits = bm->bitmap_matrix.rows[i].set_bits;\
            target_index = i;\
        }\
    }

/// Allocate more new rows
/// \param bm Pointer to the bitmap structure
/// \return BITMAP_MORE_ROW_ALLOCATION_SUCCESS or BITMAP_NO_MORE_ROWS_TO_ALLOCATE
static int bitmap_allocate_more_row(bitmap_t *bm) {
#ifdef JOURNAL
    bm->bitmap_report.cnt_call_alloc_more_rows_call++;
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

#ifdef JOURNAL
            bm->bitmap_report.cnt_call_direct_remove_row++;
            bm->bitmap_report.cnt_discarded_rows++;
#endif

#ifdef BITMAP_LIST
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
            bitmap_list_remove_row(bm, target_row);
#elif BITMAP_ARRAY
            int max_set_bits = BYTES2BITS(bm->cB);
            int target_index = 0;
            if (bm->bitmap_matrix.head <= bm->bitmap_matrix.tail) {
                BITMAP_AND_FIND_ROW_WITH_MINIMUM_BITS(bm->bitmap_matrix.head, bm->bitmap_matrix.tail)
            } else {
                BITMAP_AND_FIND_ROW_WITH_MINIMUM_BITS(bm->bitmap_matrix.head, bm->bitmap_matrix.size - 1)
                BITMAP_AND_FIND_ROW_WITH_MINIMUM_BITS(0, bm->bitmap_matrix.tail)
            }
            bitmap_remove_row_by_index(bm, target_index);

#endif
        }
    }

    /* Possible number of rows to allocate */
    int possible_number_of_rows = min(bm->rt - bm->active_rows, bm->r - bm->nxt_row_number);
    bm->active_rows += possible_number_of_rows;
    bm->set_bits += BYTES2BITS(bm->cB) * possible_number_of_rows;

#ifdef BITMAP_LIST
    for (int i = 0; i < possible_number_of_rows; i++) {
        row_t *new_row = malloc(sizeof(row_t));
        new_row->data = malloc(sizeof(unsigned char *) * bm->cB);
        new_row->number = bm->nxt_row_number;
        new_row->set_bits = BYTES2BITS(bm->cB);
        new_row->next = NULL;
        /* Initializing the vector to all 1s */
        for (int j = 0; j < bm->cB; j++) new_row->data[j] = 0xff;

        /* Updating the hyperparameters */
        bm->nxt_row_number++;

        /* Add the row to the matrix */
        BITMAP_LIST_ADD_ROW(new_row);
    }

#elif BITMAP_ARRAY
    for (int i = 0; i < possible_number_of_rows; i++) {
        if (bm->bitmap_matrix.head == -1)
            bm->bitmap_matrix.head = bm->bitmap_matrix.tail = 0;
        else if ((bm->bitmap_matrix.tail == bm->bitmap_matrix.size - 1) && bm->bitmap_matrix.head != 0)
            bm->bitmap_matrix.tail = 0;
        else
            bm->bitmap_matrix.tail++;

        row_t *new_row = &bm->bitmap_matrix.rows[bm->bitmap_matrix.tail];
        new_row->number = bm->nxt_row_number;
        new_row->set_bits = BYTES2BITS(bm->cB);

        /* Initializing the vector to all 1s */
        for (int j = 0; j < bm->cB; j++) new_row->data[j] = 0xff;

        /* Updating the hyperparameters */
        bm->nxt_row_number++;
    }

#endif
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


#ifdef BITMAP_ARRAY
#define CHECK_IF_ROW_HAS_DESIRED_BIT(index) \
    {if (target_index < bm->bitmap_matrix.rows[index].set_bits) { \
        row = &bm->bitmap_matrix.rows[index]; \
        goto extract_manipulate_indices; \
    } \
    target_index -= bm->bitmap_matrix.rows[index].set_bits;}
#endif

void bitmap_get_row_colum_with_index(bitmap_t *bm, int target_index, int *row_num, int *col_num) {
#ifdef JOURNAL
    bm->bitmap_report.cnt_cnt_get_row_col_call++;
    gettimeofday(&start_time, NULL);
#endif

    row_t *row;

#ifdef BITMAP_LIST
    /* Find the row containing our desired index */
    row = bm->bitmap_matrix.head;
    while (row) {
        if (target_index < row->set_bits) break;
        target_index -= row->set_bits;
        row = row->next;
    }

#elif BITMAP_ARRAY
    if (bm->bitmap_matrix.head <= bm->bitmap_matrix.tail) {
        for (int index = bm->bitmap_matrix.head; index <= bm->bitmap_matrix.tail; index++)
            CHECK_IF_ROW_HAS_DESIRED_BIT(index)
    } else {
        for (int index = bm->bitmap_matrix.head; index < bm->bitmap_matrix.size; index++)
            CHECK_IF_ROW_HAS_DESIRED_BIT(index)

        for (int index = 0; index <= bm->bitmap_matrix.tail; index++)
            CHECK_IF_ROW_HAS_DESIRED_BIT(index)
    }

#endif

extract_manipulate_indices:
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
#ifdef JOURNAL
    gettimeofday(&end_time, NULL);
    bm->bitmap_report.total_time_get_row_col += (end_time.tv_sec - start_time.tv_sec) + (
        end_time.tv_usec - start_time.tv_usec) / 1.0e6;
#endif
}


void bitmap_unset_indices_in_window(bitmap_t *bm, int *indices, int num_index) {
#ifdef JOURNAL
    bm->bitmap_report.cnt_cnt_unset_call++;
    gettimeofday(&start_time, NULL);
#endif

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


        row_t *row;
#ifdef BITMAP_LIST
        /* Find the row containing our desired index */
        row = bm->bitmap_matrix.head;
        while (row) {
            if (target_index < row->set_bits) break;
            target_index -= row->set_bits;
            row = row->next;
        }
#elif BITMAP_ARRAY
        if (bm->bitmap_matrix.head <= bm->bitmap_matrix.tail) {
            for (int index = bm->bitmap_matrix.head; index <= bm->bitmap_matrix.tail; index++)
                CHECK_IF_ROW_HAS_DESIRED_BIT(index)
        } else {
            for (int index = bm->bitmap_matrix.head; index < bm->bitmap_matrix.size; index++)
                CHECK_IF_ROW_HAS_DESIRED_BIT(index)

            for (int index = 0; index <= bm->bitmap_matrix.tail; index++)
                CHECK_IF_ROW_HAS_DESIRED_BIT(index)
        }

#endif
    extract_manipulate_indices:
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
                }
                target_index -= cnt_ones;
            }
        }
    }

#ifdef JOURNAL
    gettimeofday(&end_time, NULL);
    bm->bitmap_report.total_time_unset_bits += (end_time.tv_sec - start_time.tv_sec) + (
        end_time.tv_usec - start_time.tv_usec) / 1.0e6;
#endif
}


#ifdef JOURNAL
void bitmap_report(const bitmap_t *bm) {
    printf("\n================ Bitmap Report ================\n");
    printf("#ROW_ALLOC(.): %d\n", bm->bitmap_report.cnt_call_alloc_more_rows_call);
    printf("\t#ROW_CLEANUP(.): %d\n", bm->bitmap_report.cnt_call_cleanup_call);
    printf("\t\t--- Empty rows removed: %d\n", bm->bitmap_report.cnt_call_cleanup_rows_removed);
    printf("\t#ROW_DIRECT_REMOVE(.): %d\n", bm->bitmap_report.cnt_call_direct_remove_row);
    printf("#INDEX_GET_ROW_COL(.)/Index: %d\n", bm->bitmap_report.cnt_cnt_get_row_col_call);
    printf("#INDEX_UNSET(.)/Batch: %d\n", bm->bitmap_report.cnt_cnt_unset_call);
    printf("--- Discarded rows: %d/%d\n", bm->bitmap_report.cnt_discarded_rows, bm->r);
    printf("--- Discarded bits: %d/%d\n", bm->bitmap_report.cnt_discarded_bits, bm->r * BYTES2BITS(bm->cB));

    /* Timing */
    printf("\n------- Timings -------\n");
    printf("--- TT Cleanup: %0.12f micros\n", bm->bitmap_report.total_time_cleanup * 1000000);
    printf("--- TT Direct Remove Row: %0.12f micros\n", bm->bitmap_report.total_time_remove_row * 1000000);
    printf("--- TT Get Row Col: %0.12f micros\n", bm->bitmap_report.total_time_get_row_col * 1000000);
    printf("--- TT Unset Bits: %0.12f micros\n", bm->bitmap_report.total_time_unset_bits * 1000000);

    printf("\n");
    printf("--- AVGT Cleanup: %0.12f micros\n", bm->bitmap_report.cnt_call_cleanup_rows_removed != 0
                                                    ? bm->bitmap_report.total_time_cleanup / bm->bitmap_report.
                                                      cnt_call_cleanup_rows_removed * 1000000
                                                    : 0);

    printf("--- AVGT Direct Remove Row: %0.12f micros\n",
           (bm->bitmap_report.cnt_call_cleanup_rows_removed + bm->bitmap_report.cnt_call_direct_remove_row) != 0
               ? bm->bitmap_report.total_time_remove_row / (
                     bm->bitmap_report.cnt_call_direct_remove_row + bm->bitmap_report.cnt_call_cleanup_rows_removed) *
                 1000000
               : 0);

    printf("--- AVGT Get Row Col: %0.12f micros\n", bm->bitmap_report.cnt_cnt_get_row_col_call != 0
                                                        ? bm->bitmap_report.total_time_get_row_col / bm->bitmap_report.
                                                          cnt_cnt_get_row_col_call * 1000000
                                                        : 0);
    printf("--- AVGT Unset Bits: %0.12f micros\n", bm->bitmap_report.cnt_cnt_unset_call != 0
                                                       ? bm->bitmap_report.total_time_unset_bits / bm->bitmap_report.
                                                         cnt_cnt_unset_call * 1000000
                                                       : 0);
}
#endif
