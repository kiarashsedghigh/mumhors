#include <muhors/bitmap.h>
#include <muhors/sort.h>
#include <muhors/math.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

#define BYTES2BITS(x) (x*8)

/// Freeing a row of bitmap
/// \param row Pointer to the row
static void bitmap_free_row(row_t *row) {
    free(row->data);
    free(row);
}

int GLOB = 0;
int total_bits;

/// Adds a row to the matrix of rows
/// \param bm Pointer to the bitmap structure
/// \param row Pointer to the new row
static void bitmap_matrix_list_add_row(bitmap_t *bm, row_t *row){
    if (!bm->bitmap_matrix.head){
        bm->bitmap_matrix.head = row;
        bm->bitmap_matrix.tail = row;
    }else{
        bm->bitmap_matrix.tail->next = row;
        bm->bitmap_matrix.tail = row;
    }
}


void bitmap_init(bitmap_t *bm, int rows, int cols, int init_rows, int row_threshold) {
    /* Simple parameter check. This check has been done in this way for simplicity!! */
    assert(cols % 8 == 0);
    assert(init_rows <= rows);
    assert(init_rows <= row_threshold);
    assert(row_threshold <= rows);

    /* Setting the hyperparameters */
    bm->r = rows;
    bm->cB = cols / 8;
    bm->ir = init_rows;
    bm->rt = row_threshold;
    bm->next_row_number = bm->ir;
    bm->active_rows = bm->ir;
    bm->set_bits = bm->ir * BYTES2BITS(bm->cB);

    total_bits= bm->r * bm->cB * 8;

#ifdef JOURNAL
    /* If journaling is enabled, initialize the variables to 0 */
    bm->bitmap_report.cnt_cleanup_call = 0;
    bm->bitmap_report.cnt_alloc_more_rows = 0;
    bm->bitmap_report.cnt_row_compression = 0;
    bm->bitmap_report.cnt_count_unset = 0;
    bm->bitmap_report.cnt_discarded_bits = 0;
    bm->bitmap_report.cnt_discarded_rows=0;
#endif

    /* Initializing the matrix of rows (the linked list). */
    bm->bitmap_matrix.head=NULL;
    bm->bitmap_matrix.tail=NULL;

    /* Creating the rows and adding them to the matrix */
    for (int i = 0; i < bm->ir; i++) {
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
    while(curr){
        row_t *target = curr;
        curr=curr->next;

        /* Deleting the rows data */
        bitmap_free_row(target);
    }
    bm->bitmap_matrix.head=NULL;
    bm->bitmap_matrix.tail=NULL;
}

/// Cleanup the matrix by removing the rows that have no set bits left to use
/// \param bm Pointer to the bitmap structure
/// \return Returns number of cleaned(removed) rows
static int bitmap_row_cleanup(bitmap_t *bm){
    //TODO Better writing

    int cleaned_rows = 0;
    row_t *r = bm->bitmap_matrix.head;
    row_t *prev = NULL;

    while(r){
        if(!r->set_bits) {
            row_t *target_to_delete;

            /* If the head node is going to be removed */
            if (r==bm->bitmap_matrix.head){
                bm->bitmap_matrix.head = bm->bitmap_matrix.head->next;
                target_to_delete = r;
                r=r->next;
            }else {
                target_to_delete = r;
                prev->next = r->next;
                if (r == bm->bitmap_matrix.tail) {
                    bm->bitmap_matrix.tail = prev;
                    bm->bitmap_matrix.tail->next=NULL;
                }
                r = r->next;
            }
            /* Deallocate the row */
            bitmap_free_row(target_to_delete);
            bm->active_rows--;
            cleaned_rows++;

        }else {
//            if(!prev)
//                prev=r;
            prev=r;
            r = r->next;
        }

    }
//    if(cleaned_rows){
//        bitmap_display(bm);
//        printf(">> %d\n", bm->set_bits);
//        exit(0);

    return cleaned_rows;
}


/// Remove a row from the bitmap matrix based on its index
/// \param bm Pointer to the bitmap structure
/// \param index The index of the row
static void bitmap_remove_row(bitmap_t *bm, int index) {
    assert(index >= 0 && index < bm->active_rows);

    /* Deallocate the row */
    row_t *temp = bm->bitmap_matrix.head;
    int set_bits_in_row;

    if (index == 0) {
        bm->bitmap_matrix.head = bm->bitmap_matrix.head->next;
        temp->next = NULL;

        /* Deallocate the row */
        set_bits_in_row = temp->set_bits;
        bitmap_free_row(temp);
    }
    else {
        for(int i=0; i<index-1; i++)
            temp=temp->next;
        row_t *target = temp->next;

        /* Updating the tail node */
        if(target==bm->bitmap_matrix.tail) {
            bm->bitmap_matrix.tail = temp;
            bm->bitmap_matrix.tail->next=NULL;
        }else {
            temp->next = temp->next->next;
            target->next = NULL;
        }
        /* Deallocate the row */
        set_bits_in_row = target->set_bits;
        bitmap_free_row(target);
    }

    /* Updating the hyperparameters */
    bm->active_rows--;
    bm->set_bits-=set_bits_in_row;
#ifdef JOURNAL
    bm->bitmap_report.cnt_discarded_bits+=set_bits_in_row;
    bm->bitmap_report.cnt_discarded_rows++;
#endif
}


/// Allocate more new rows
/// \param bm Pointer to the bitmap structure
/// \return BITMAP_MORE_ROW_ALLOCATION_SUCCESS or BITMAP_NO_MORE_ROWS_TO_ALLOCATE
static int bitmap_allocate_more_row(bitmap_t *bm) {
//    printf("%d %d\n",bm->next_row_number, bm->r );

    if (bm->next_row_number >= bm->r) {
//        printf("NO MORE ROWS\n");
//        exit(0);
        return BITMAP_NO_MORE_ROWS_TO_ALLOCATE;
    }
    /* Check if allocating a new row will pass the threshold of active rows */
    if (bm->active_rows + 1 > bm->rt) {
#ifdef JOURNAL
        bm->bitmap_report.cnt_cleanup_call++;
#endif

        /* Perform a cleanup. Remove the row with the least number of set bits if no row was deleted */
        if (!bitmap_row_cleanup(bm)) {


            /* Finding the row with the fewest number of set bits */
            row_t *row = bm->bitmap_matrix.head;
            int row_index = 0;
            int target_row_index;
            int max_set_bits = bm->cB*8;
            while(row){
                if(row->set_bits<max_set_bits) {
                    target_row_index = row_index;
                    max_set_bits = row->set_bits;
                }
                row_index++;
                row=row->next;
            }
//            printf("AC: %d\n", bm->active_rows);
//            printf("1s: %d\n", bm->set_bits);
//            printf("TARGET: %d with %d bits\n", target_row_index, max_set_bits);
//
////            printf("HERE %d\n" ,GLOB);
//            GLOB++;


            bitmap_remove_row(bm, target_row_index);

//            printf("AC: %d with bits %d\n", bm->active_rows, bm->set_bits);
//            exit(0);
        }
    }

    /* Possible number of rows to allocate */
    int possible_number_of_rows = min(bm->rt-bm->active_rows, bm->r - bm->next_row_number);
//    printf("%d\n", possible_number_of_rows);

    /* Fill the matrix with more rows */
    for(int i=0;i< 1;i++) {
        row_t *new_row = malloc(sizeof(row_t));
        new_row->data = malloc(sizeof(unsigned char *) * bm->cB);
        new_row->number = bm->next_row_number;
        new_row->set_bits = BYTES2BITS(bm->cB);
        new_row->next = NULL;

        /* Initializing the vector to all 1s */
        for (int j = 0; j < bm->cB; j++)
            new_row->data[j] = 0xff;

        /* Updating the hyperparameters */
        bm->active_rows++;
        bm->next_row_number++;
        bm->set_bits += BYTES2BITS(bm->cB);

        /* Add the row to the matrix */
        bitmap_matrix_list_add_row(bm, new_row);
    }



//    printf("AC: %d with bits %d\n", bm->active_rows, bm->set_bits);

    return BITMAP_MORE_ROW_ALLOCATION_SUCCESS;
}


int bitmap_unset_index_in_window(bitmap_t *bm, int * indices, int num_index, int windows_size) {
//    printf("%d\n", total_bits);
    total_bits-=num_index;

    /* If there are not enough ones in the current window, add a new row. The active ones are
     * the all nodes in the compressed linked list and the active ones in the matrix */
    if (windows_size > bm->set_bits){
        if (bitmap_allocate_more_row(bm) == BITMAP_NO_MORE_ROWS_TO_ALLOCATE)
            return BITMAP_UNSET_BITS_FAILED;
#ifdef JOURNAL
        bm->bitmap_report.cnt_alloc_more_rows++;
#endif
    }
    /* Sort the indices.
     * Description:
     * */
    merge_sort(indices, 0, num_index-1);

    int index_diff = 0;
    for(int i=0; i<num_index; i++){
        if (i>0 && indices[i]==indices[i-1]) continue;

        int target_index = indices[i] - index_diff;
        index_diff++;



        /* The target index belongs to the bitmap matrix. Loop over the rows, and unset them. */
        row_t *row = bm->bitmap_matrix.head;
        while(row){
            for (int j = 0; j < bm->cB; j++) {
                if (row->data[j] != 0) { // Skip 0 bytes
                    int cnt_ones = count_num_set_bits(row->data[j]);
                    if (cnt_ones > target_index) {
                        int bit_idx = byte_get_index_nth_set(row->data[j], target_index + 1);
                        row->data[j] &= 0xff - (1 << (8 - bit_idx - 1));
                        bm->set_bits--;
                        row->set_bits--;

                        goto end;   //TODO
                    } else
                        target_index -= cnt_ones;
                }
            }
            row=row->next;
        }
        end:
    }
#ifdef JOURNAL
    bm->bitmap_report.cnt_count_unset++;
#endif
    return BITMAP_UNSET_BITS_SUCCESS;
}

void bitmap_display(bitmap_t *bm) {
    printf("---------ROWs---------\n");

    row_t *row = bm->bitmap_matrix.head;
    while(row){
        printf("> (%d) ",row->set_bits);
        for (int j = 0; j < bm->cB; j++)
            printf("%d ", row->data[j]);
        printf("\n");
        row = row->next;
    }
}

#ifdef JOURNAL
void bitmap_report(bitmap_t *bm){

    printf("REPORT: \n-------------------\n");
    printf("Cleanup calls: %d\n", bm->bitmap_report.cnt_cleanup_call);
    printf("Compression calls: %d\n", bm->bitmap_report.cnt_row_compression);
    printf("Alloc calls: %d\n", bm->bitmap_report.cnt_alloc_more_rows);
    printf("Unset calls: %d\n", bm->bitmap_report.cnt_count_unset);
    printf("Discarded bits: %d/%d\n", bm->bitmap_report.cnt_discarded_bits, bm->r * BYTES2BITS(bm->cB));
    printf("Discarded rows: %d/%d\n", bm->bitmap_report.cnt_discarded_rows, bm->r);

}
#endif