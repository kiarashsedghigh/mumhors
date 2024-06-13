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

/// Checks if all the bits of the current vector is unset
/// \param vec Pointer to the vector
/// \param vec_size Length of the vector in terms of bytes
/// \return True if all bits are unset, false, otherwise
static int is_vector_all_unset(vec_t vec, int vec_size){
    for (int i=0; i< vec_size; i++){
        if (vec[i]!=0)
            return 0;
    }
    return 1;
}

static void bitmap_rowcompressor_init(bitmap_rowcom_t * bmrcom, int total_nodes){
    bmrcom->total_size = total_nodes;
    bmrcom->current_nodes = 0;
    bmrcom->head = NULL;
    bmrcom->tail = NULL;
}

void bitmap_rowcompressor_addnode(bitmap_rowcom_t * bmrcom, int row, int col){
    bitobj_t * new_bitobj = malloc(sizeof(bitobj_t));
    new_bitobj->row = row;
    new_bitobj->col = col;
    new_bitobj->next = NULL;

    if (!bmrcom->head){
        bmrcom->head = new_bitobj;
        bmrcom->tail = new_bitobj;
    }else{
        bmrcom->tail->next = new_bitobj;
        bmrcom->tail = new_bitobj;
    }

    bmrcom->current_nodes++;
}

void bitmap_rowcompressor_display(bitmap_rowcom_t * bmrcom){
    bitobj_t * temp = bmrcom->head;
    while(temp){
        printf("r: %d c: %d\n", temp->row, temp->col);
        temp = temp->next;
    }
}




void bitmap_init(bitmap_t *bm, int rows, int cols, int init_rows, int row_threshold, int row_compressed_list_length) {
    /* Simple parameter check. This check has been done in this way for simplicity!! */
    assert(cols % 8 == 0);
    assert(init_rows <= rows);
    assert(row_threshold <= rows);

    /* Setting the hyperparameters */
    bm->r = rows;
    bm->cB = cols / 8;
    bm->ir = init_rows;
    bm->rt = row_threshold;
    bm->next_row_number = bm->ir;
    bm->active_rows = bm->ir;
    bm->num_ones_in_active_rows = bm->ir * bm->cB * 8;

    /* Initializing the matrix of rows (the circular queue). The number of rows it supports, is
     * defined by the threshold row number */
    cq_init(&bm->bitmap_matrix, bm->rt);

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

    bitmap_rowcompressor_init(&bm->compressed_rows, row_compressed_list_length);
}

void bitmap_delete(bitmap_t *bm) {
    /* Deleting the rows data */
    cq_iter_next(NULL);
    row_t row;
    while ((row = cq_iter_next(&bm->bitmap_matrix))) { bitmap_free_row(row); }

    /* Deleting compressed rows */
    //TODO

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
    if (bm->next_row_number == bm->r) {
        debug("No more rows to allocate\n", DEBUG_ERR);
        exit(-1);
    }

    /* Check if allocating a new row will pass the threshold of active rows */
    if (bm->active_rows + 1 > bm->rt) {
        debug("Threshold Reached\n", DEBUG_WARNING);

        /* No need for clean as every empty row was already deleted.
         * Hence, we need for more compression of the active rows. */
        if (bm->num_ones_in_active_rows > (bm->compressed_rows.total_size - bm->compressed_rows.current_nodes)){
            debug("Cannot compress as we don't have enough space in the compressor list\n", DEBUG_ERR);
            exit(-1);
        }

        /* Looping to find the set bits */
        cq_iter_next(NULL);
        row_t row;
        while ((row = cq_iter_next(&bm->bitmap_matrix))) {
            int column_idx = 0;
            for (int j = 0; j < bm->cB; j++) {
                if (row->data[j] != 0) { // Skip 0 bytes
                    /* Loop around bytes and add the row and column of the bit indices that are set */
                    unsigned char cur_byte = row->data[j];
                    while(cur_byte){
                        if ((cur_byte & 128) == 128)
                            bitmap_rowcompressor_addnode(&bm->compressed_rows, row->row_number, column_idx);
                        cur_byte <<= 1;
                        column_idx++;
                    }
                }else
                    column_idx += 8;
            }
        }
        bitmap_rowcompressor_display(&bm->compressed_rows);
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
    int row_index = 0;

    while ((row = cq_iter_next(&bm->bitmap_matrix))) {
        for (int j = 0; j < bm->cB; j++) {
            if (row->data[j] != 0) { // Skip 0 bytes
                int cnt_ones = count_num_set_bits(row->data[j]);
                if (cnt_ones > index) {
                    int bit_idx = byte_get_index_nth_set(row->data[j], index+1);
                    row->data[j] &= 0xff - (1 << (8 - bit_idx - 1));
//                    if (is_vector_all_unset(row->data, bm->cB))
//                        bitmap_remove_row(bm, row_index);
                    goto end;
                } else
                    index -= cnt_ones;
            }
        }
        row_index++;
    }
    end:
    bm->num_ones_in_active_rows--;
}


