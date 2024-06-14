#include <muhors/bitmap.h>
#include <muhors/cqueue.h>
#include <muhors/sort.h>
#include <debug.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

/// Returns the minimum of two integers
/// \param x First integer
/// \param y Second integer
/// \return Minimum of two integers
static int min(int x, int y){
    if(x<y)
        return x;
    return y;
}

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
/// \param row Pointer to the row
/// \param row_size Length of the row in terms of bytes
/// \return True if all bits are unset, false, otherwise
static int is_row_all_unset(row_t row, int row_size){
    for (int i=0; i< row_size; i++){
        if (row->data[i]!=0)
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

void bitmap_rowcompressor_removenode(bitmap_rowcom_t * bmrcom, int index){
    bitobj_t * temp = bmrcom->head;

    if (index == 0) {
        bmrcom->head = bmrcom->head->next;
        temp->next = NULL;

        printf("DELETED NODE: i: %d j: %d\n", temp->row, temp->col);
        /* Deallocate the node */
        free(temp);
    }
    else {
        for(int i=0; i<index-1; i++)
            temp=temp->next;
        bitobj_t * target = temp->next;
        temp->next = temp->next->next;
        target->next = NULL;

        printf("DELETED NODE: i: %d j: %d\n", target->row, target->col);


        /* Deallocate the node */
        free(target);
    }
    bmrcom->current_nodes--;

}


void bitmap_rowcompressor_display(bitmap_rowcom_t * bmrcom){
    bitobj_t * temp = bmrcom->head;
    while(temp){
        printf("r: %d c: %d\n", temp->row, temp->col);
        temp = temp->next;
    }
}

static void bitmap_matrix_list_add_row(bitmap_matrix_t * matrix, row_t row){
    if (!matrix->head){
        matrix->head = row;
        matrix->tail = row;
    }else{
        matrix->tail->next = row;
        matrix->tail = row;
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

//    /* Initializing the matrix of rows (the circular queue). The number of rows it supports, is
//     * defined by the threshold row number */
//    cq_init(&bm->bitmap_matrix, bm->rt);

    /* Initializing the matrix of rows (the linked list). */
    bm->bitmap_matrix.head=NULL;
    bm->bitmap_matrix.tail=NULL;

    /* Creating the rows */
    for (int i = 0; i < bm->ir; i++) {
        row_t new_row = malloc(sizeof(row_t));
        new_row->data = malloc(sizeof(unsigned char *) * bm->cB);
        new_row->row_number = i;
        new_row->next = NULL;

        /* Initializing the vector to all 1s */
        for (int j = 0; j < bm->cB; j++) new_row->data[j] = 0xff;

        /* Adding the row to the matrix */
//        cq_enqueue(&bm->bitmap_matrix, (void *) r);
        bitmap_matrix_list_add_row(&bm->bitmap_matrix, new_row);
    }

    bitmap_rowcompressor_init(&bm->compressed_rows, row_compressed_list_length);
}

void bitmap_delete(bitmap_t *bm) {
    /* Deleting the rows data */
//    cq_iter_next(NULL);
//    row_t row;
//    while ((row = cq_iter_next(&bm->bitmap_matrix))) { bitmap_free_row(row); }
    row_t curr = bm->bitmap_matrix.head;
    while(curr){
        row_t target = curr;
        curr=curr->next;
        bitmap_free_row(target);
    }
    bm->bitmap_matrix.head=NULL;
    bm->bitmap_matrix.tail=NULL;

    /* Deleting compressed rows */
    bitobj_t * start = bm->compressed_rows.head;
    while(start){
        bitobj_t * target = start;
        start = start->next;
        free(target);
    }
    bm->compressed_rows.head=NULL;
    bm->compressed_rows.tail=NULL;
    bm->compressed_rows.total_size=0;

//    cq_delete(&bm->bitmap_matrix);
}

void bitmap_display(bitmap_t *bm) {
//    cq_iter_next(NULL);
//
//    for (int i = 0; i < bm->active_rows; i++) {
//        row_t r = cq_iter_next(&bm->bitmap_matrix);
//        for (int j = 0; j < bm->cB; j++)
//            printf("%d ", r->data[j]);
//        printf("\n");
//    }
    row_t row = bm->bitmap_matrix.head;
    while(row){
        for (int j = 0; j < bm->cB; j++)
            printf("%d ", row->data[j]);
        printf("\n");
        row = row->next;
    }
}



void bitmap_remove_row(bitmap_t *bm, int index) {
    assert(index >= 0 && index < bm->active_rows);

    /* Deallocate the row */
//    row_t row = cq_get_row_by_index(&bm->bitmap_matrix, index);
//    bitmap_free_row(row);

    /* Removing the row from the circular queue */
//    cq_remove_row_by_index(&bm->bitmap_matrix, index);


    row_t temp = bm->bitmap_matrix.head;

    if (index == 0) {
        bm->bitmap_matrix.head = bm->bitmap_matrix.head->next;
        temp->next = NULL;

        /* Deallocate the row */
        bitmap_free_row(temp);
    }
    else {
        for(int i=0; i<index-1; i++)
            temp=temp->next;
        row_t target = temp->next;
        temp->next = temp->next->next;
        target->next = NULL;

        /* Deallocate the row */
        bitmap_free_row(target);
    }

    /* Updating the hyperparameters */
    bm->active_rows--;
}

static int bitmap_row_cleanup(bitmap_t *bm){
    int cleaned_rows = 0;
    int row_index = 0;
    row_t r = bm->bitmap_matrix.head;
    while(r){
        if(is_row_all_unset(r, bm->cB)) {
            bitmap_remove_row(bm, row_index);
            bm->active_rows--;
            cleaned_rows++;
        }

        row_index++;
        r=r->next;
    }
    return cleaned_rows;
}

void bitmap_allocate_more_row(bitmap_t *bm) {
    //TODO do not compress all the rows?

    if (bm->next_row_number == bm->r) {
        debug("No more rows to allocate\n", DEBUG_ERR);
        exit(-1);
    }

    /* Check if allocating a new row will pass the threshold of active rows */
    if (bm->active_rows + 1 > bm->rt) {
        debug("Threshold Reached\n", DEBUG_WARNING);

        /* Perform a cleanup. Compress rows if no row was deleted */
        if (!bitmap_row_cleanup(bm)){
            printf("NO clean up");

            /* Compress the rows into a list of nodes */
            if (bm->num_ones_in_active_rows > (bm->compressed_rows.total_size - bm->compressed_rows.current_nodes)){
                debug("Cannot compress as we don't have enough space in the compressor list\n", DEBUG_ERR);
                exit(-1);
            }

            /* Looping to find the set bits. As we pass the rows, we remove them as well */
            row_t curr_row = bm->bitmap_matrix.head;
            while(curr_row) {
                int column_idx = 0;
                for (int j = 0; j < bm->cB; j++) {
                    if (curr_row->data[j] != 0) { // Skip 0 bytes
                        /* Loop around bytes and add the row and column of the bit indices that are set */
                        unsigned char cur_byte = curr_row->data[j];
                        while (cur_byte) {
                            if ((cur_byte & 128) == 128)
                                bitmap_rowcompressor_addnode(&bm->compressed_rows, curr_row->row_number, column_idx);
                            cur_byte <<= 1;
                            column_idx++;
                        }
                    } else
                        column_idx += 8;
                }
                row_t target = curr_row;
                curr_row = curr_row->next;
                bitmap_free_row(target);
                bm->active_rows--;
            }

            bm->bitmap_matrix.head=NULL;
            bm->bitmap_matrix.tail=NULL;

//            cq_iter_next(NULL);
//            row_t row;
//            while ((row = cq_iter_next(&bm->bitmap_matrix))) {
//                int column_idx = 0;
//                for (int j = 0; j < bm->cB; j++) {
//                    if (row->data[j] != 0) { // Skip 0 bytes
//                        /* Loop around bytes and add the row and column of the bit indices that are set */
//                        unsigned char cur_byte = row->data[j];
//                        while(cur_byte){
//                            if ((cur_byte & 128) == 128)
//                                bitmap_rowcompressor_addnode(&bm->compressed_rows, row->row_number, column_idx);
//                            cur_byte <<= 1;
//                            column_idx++;
//                        }
//                    }else
//                        column_idx += 8;
//                }
//            }
            bitmap_rowcompressor_display(&bm->compressed_rows);

            /* Empty the current list */


        }

    }

    /* Possible number of rows to allocate */
    int possible_number_of_rows = min(bm->rt-bm->active_rows, bm->r - bm->next_row_number);


    /* Fill the matrix with more rows */
    for(int i=0;i<possible_number_of_rows;i++) {

        row_t new_row = malloc(sizeof(row_t));
        new_row->data = malloc(sizeof(unsigned char *) * bm->cB);
        new_row->row_number = bm->next_row_number;
        new_row->next = NULL;

        /* Initializing the vector to all 1s */
        for (int j = 0; j < bm->cB; j++)
            new_row->data[j] = 0xff;

        /* Updating the hyperparameters */
        bm->active_rows++;
        bm->next_row_number++;

        /* Add the row to the matrix */
        bitmap_matrix_list_add_row(&bm->bitmap_matrix, new_row);
//    cq_enqueue(&bm->bitmap_matrix, new_row);
    }
    /* Reset number of ones in active rows */
    bm->num_ones_in_active_rows = possible_number_of_rows * bm->cB * 8;
}


void bitmap_unset_index_in_window(bitmap_t *bm, int * indices, int num_index, int windows_size) {

    /* If there are not enough ones in the current window, add a new row. The active ones are
     * the all nodes in the compressed linked list and the active ones in the matrix */
    if (windows_size > bm->num_ones_in_active_rows + bm->compressed_rows.current_nodes)
        bitmap_allocate_more_row(bm);


    /* Sort the indices.
     * Description:
     * */
    merge_sort(indices, 0, num_index-1);

    int index_diff = 0;
    for(int i=0; i<num_index; i++){
        int target_index = indices[i] - index_diff;
        index_diff++;

        /* Check if the target index belongs to the compressed rows */
        if (target_index < bm->compressed_rows.current_nodes){
            /* Remove the node from the list */
            bitmap_rowcompressor_removenode(&bm->compressed_rows, target_index);
        }else{

            /* Update the target index */
            target_index -= bm->compressed_rows.current_nodes;

            /* The target index belongs to the bitmap matrix. Loop over the rows, and unset them. */

            row_t row = bm->bitmap_matrix.head;
            while(row){
                for (int j = 0; j < bm->cB; j++) {
                    if (row->data[j] != 0) { // Skip 0 bytes
                        int cnt_ones = count_num_set_bits(row->data[j]);
                        if (cnt_ones > target_index) {
                            int bit_idx = byte_get_index_nth_set(row->data[j], target_index + 1);
                            row->data[j] &= 0xff - (1 << (8 - bit_idx - 1));
                            goto end;   //TODO
                        } else
                            target_index -= cnt_ones;
                    }
                }
                row=row->next;
            }
            end:
        }
    }


//    cq_iter_next(NULL);
//    row_t row;
//    int row_index = 0;
//
//    while ((row = cq_iter_next(&bm->bitmap_matrix))) {
//        for (int j = 0; j < bm->cB; j++) {
//            if (row->data[j] != 0) { // Skip 0 bytes
//                int cnt_ones = count_num_set_bits(row->data[j]);
//                if (cnt_ones > index) {
//                    int bit_idx = byte_get_index_nth_set(row->data[j], index + 1);
//                    row->data[j] &= 0xff - (1 << (8 - bit_idx - 1));
//                    goto end;
//                } else
//                    index -= cnt_ones;
//            }
//        }
//        row_index++;
//    }
//    end:
//    bm->num_ones_in_active_rows--;
}

