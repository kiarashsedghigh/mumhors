#include <muhors/bitmap.h>
#include <muhors/cqueue.h>
#include <debug.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

/// Freeing a row of bitmap
/// \param row Pointer to the row
static void bitmap_free_row(row_t row) {
    free(row->data);
    free(row);
}

///// Counts number of bits set in the given number
///// \param num Given number
///// \return Number of set bits
static int count_num_set_bits(int num) {
    int cnt = 0;
    while (num) {
        cnt += num & 1;
        num >>= 1;
    }
    return cnt;
}

/// Returns the bit index of the nth set bit of the given byte
/// \param byte A given byte
/// \param nth Nth set bit
/// \return Bit index of the nth set bit of the given byte
static int byte_get_index_nth_set(unsigned char byte, int nth) {
    int bit_idx = 0;
    nth -= 1;   // Converting nth to 0-based index
    while (nth >= 0) {
        while ((byte & 128) != 128) {
            bit_idx++;
            byte <<= 1;
        }
        byte <<= 1;
        bit_idx++;
        nth--;
    }
    return bit_idx-1;
}

void bitmap_init(bitmap_t *bm, int rows, int cols, int init_rows, int row_threshold) {
    /* Simple parameter check. This check has been done in this way for simplicity!! */
    assert(cols % 8 == 0);
    assert(init_rows <= rows);
    assert(row_threshold < rows);

    /* Setting the hyperparameters */
    bm->r = rows;
    bm->cB = cols / 8;
    bm->ir = init_rows;
    bm->rt = row_threshold;
    bm->next_row_number = bm->ir;
    bm->active_rows = bm->ir;
    bm->num_ones_in_active_rows = bm->ir * bm->cB * 8;

    /* Initializing the matrix of rows (the circular queue) */
    cq_init(&bm->bitmap_matrix, bm->ir);

    /* Creating the rows */
    for (int i = 0; i < bm->ir; i++) {
        row_t r = malloc(sizeof(row_t));
        r->data = malloc(sizeof(unsigned char *) * bm->cB);
        r->row_number = i;

        /* Initializing the vector to all 1s */
        for (int j = 0; j < bm->cB; j++) r->data[j] = 0xff;

        /* Adding the row to the matrix */
        cq_enqueue(&bm->bitmap_matrix, (void *) r);
    }
}

void bitmap_delete(bitmap_t *bm) {
    /* Deleting the rows data */
    cq_iter_next(NULL);
    row_t row;
    while ((row = cq_iter_next(&bm->bitmap_matrix))) { bitmap_free_row(row); }

    cq_delete(&bm->bitmap_matrix);
}

void bitmap_display(bitmap_t *bm) {
    cq_iter_next(NULL);

    for (int i = 0; i < bm->active_rows; i++) {
        row_t r = cq_iter_next(&bm->bitmap_matrix);
        for (int j = 0; j < bm->cB; j++)
            printf("%d ", r->data[j]);
        printf("\n");
    }
}


void bitmap_allocate_new_row(bitmap_t *bm) {
    /* Check if allocating a new row will pass the threshold of active rows */
    if (bm->active_rows + 1 > bm->rt)
        debug("Threshold Reached\n", DEBUG_WARNING);

    if (bm->active_rows + 1 > bm->r) {
        debug("No more rows to allocate\n", DEBUG_ERR);
        exit(-1);
    }

    row_t new_row = malloc(sizeof(row_t));
    new_row->data = malloc(sizeof(unsigned char *) * bm->cB);
    new_row->row_number = bm->next_row_number;

    /* Initializing the vector to all 1s */
    for (int i = 0; i < bm->cB; i++)
        new_row->data[i] = 0xff;

    /* Updating the hyperparameters */
    bm->active_rows++;
    bm->next_row_number++;
    bm->num_ones_in_active_rows += bm->cB * 8;

    /* Add the row to the rows */
    cq_enqueue(&bm->bitmap_matrix, new_row);

}

void bitmap_remove_row(bitmap_t *bm, int index) {
    assert(index >= 0 && index < bm->active_rows);

    /* Deallocate the row */
    row_t row = cq_get_row_by_index(&bm->bitmap_matrix, index);
    bitmap_free_row(row);

    /* Removing the row from the circular queue */
    cq_remove_row_by_index(&bm->bitmap_matrix, index);

    /* Updating the hyperparameters */
    bm->active_rows--;
}


void bitmap_unset_index_in_window(bitmap_t *bm, int index, int windows_size) {
    assert(index<windows_size);

    /* If there are not enough ones in the current window, add a new row */
    if (windows_size > bm->num_ones_in_active_rows) {
        bitmap_allocate_new_row(bm);
    }

    cq_iter_next(NULL);
    row_t row;

    while ((row = cq_iter_next(&bm->bitmap_matrix))) {
        for (int j = 0; j < bm->cB; j++) {
            if (row->data[j] != 0) { // Skip 0 bytes
                int cnt_ones = count_num_set_bits(row->data[j]);
                if (cnt_ones > index) {
                    int bit_idx = byte_get_index_nth_set(row->data[j], index+1);
                    row->data[j] &= 0xff - (1 << (8 - bit_idx - 1));
                    goto end;
                } else
                    index -= cnt_ones;
            }
        }
    }
    end:
    bm->num_ones_in_active_rows--;
}



//
//

//
//
//void bitmap_unset(bitmap_t * bm, int index){
//    cqueue_t rows = bm->bitmap_matrix;
//    row_t row;
//
//    // Checking the windows size and not enough, allocate row, before allocate clean up
//
//
//
//
//    /* Initializing the iterator */
//    cq_iter_next(NULL);
//
//    while ((row = cq_iter_next(&rows))){
//        for (int j = 0; j<bm->cB; j++){
//            if (row[j]!=0){
//                int cnt_ones = count_num_set_bits(row[j]);
//                if (cnt_ones>index){
//                    //masking the byte
//                    row[j] &= 0xff-(1<< (8-index));
//                    return;
//                }else
//                    index-=8;
//            }
//        }
//    }
//}
