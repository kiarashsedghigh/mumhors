#include <mumhors/mumhors.h>
#include <mumhors/bitmap.h>
#include <mumhors/sort.h>

#include <stdlib.h>
#include <string.h>

unsigned char *dummy_PK = "DUMMY PK";

void mumhors_init_signer(mumhors_signer_t *signer, unsigned char *seed,
                        int t, int k, int ir, int rt, int l) {
    signer->seed = seed;
    signer->t = t;
    signer->k = k;
    signer->t = t;
    signer->ir = ir;
    signer->rt = rt;
    signer->l = l;

    bitmap_init(&signer->bm, signer->l, signer->t, signer->rt, signer->t);
}

void mumhors_delete_signer(mumhors_signer_t *signer) {
    bitmap_delete(&signer->bm);
}

/// Adds a public key node to the list of public keys
/// \param verifier Pointer to mumhors verifier struct
/// \param pk Pointer to the public key node struct
static void mumhors_verifier_public_key_node_add(mumhors_verifier_t *verifier, public_key_t *pk) {
    if (verifier->pk_matrix.head == NULL) {
        verifier->pk_matrix.head = pk;
        verifier->pk_matrix.tail = pk;
    } else {
        verifier->pk_matrix.tail->next = pk;
        verifier->pk_matrix.tail = pk;
    }
}

void mumhors_init_verifier(mumhors_verifier_t *verifier, int r, int c, int rt, int window_size) {
    verifier->r = r;
    verifier->c = c;
    verifier->rt = rt;
    verifier->active_pks = verifier->rt * verifier->c;
    verifier->windows_size = window_size;
    verifier->pk_matrix.head = NULL;
    verifier->pk_matrix.tail = NULL;
    verifier->next_row_number = 0;

    for (int i = 0; i < verifier->r; i++) {
        /* Create a new public key node */
        public_key_t *pk_node = malloc(sizeof(public_key_t));
        pk_node->pks = malloc(sizeof(unsigned char *) * verifier->c);

        /* Initialized the public keys in the current row with a dummy public key */
        for (int i = 0; i < verifier->c; i++) {
            pk_node->pks[i] = malloc(10);
            strcpy(pk_node->pks[i], "Dummy");
        }
        pk_node->number = verifier->next_row_number;
        verifier->next_row_number++;

        pk_node->available_pks = verifier->c;
        pk_node->next = NULL;

        /* Add the new public key to the matrix of public keys */
        mumhors_verifier_public_key_node_add(verifier, pk_node);
    }

    verifier->next_row_number = verifier->rt;

}

void mumhors_delete_verifier(mumhors_verifier_t *verifier) {
    public_key_t *pk_row = verifier->pk_matrix.head;
    while (pk_row) {
        for (int i = 0; i < verifier->c; i++)
            free(pk_row->pks[i]);
        free(pk_row->pks);
        public_key_t *target = pk_row;
        pk_row = pk_row->next;
        free(target);
    }
}


/// Invalidates the public keys based on the given indices
/// \param verifier Pointer to mumhors verifier struct
/// \param indices List of indices to be invalidated
/// \param num_indices Number of indices
void mumhors_invalidate_public_pks(mumhors_verifier_t *verifier, int * indices, int num_indices) {
    /* First sort the indices */
    merge_sort(indices, 0, num_indices - 1);

    int index_diff = 0;
    for (int i = 0; i < num_indices; i++) {
        if (i > 0 && indices[i] == indices[i - 1]) continue;

        int target_index = indices[i] - index_diff;
        index_diff++;



        /* Invalidate the target index */
        public_key_t *pk_row = verifier->pk_matrix.head;

        while(pk_row){

            if (target_index<pk_row->available_pks)
                break;
            target_index-=pk_row->available_pks;
            pk_row=pk_row->next;
        }
        for(int j=0;j<verifier->c;j++){
            if (pk_row->pks[j]){ // If the PK is not NULL
                if(target_index == 0){
                    /* Free up the public key */
                    free(pk_row->pks[j]);
                    pk_row->pks[j]=NULL;
                    pk_row->available_pks--;
                    verifier->active_pks--;
                    break;
                }
                target_index--;
            }
        }
    }
}

static int mumhors_verifier_cleanup_rows(mumhors_verifier_t *verifier){
    //TODO check tail assignemtn

    int cleaned_rows = 0;
    public_key_t *pk_row = verifier->pk_matrix.head;
    public_key_t *prev = verifier->pk_matrix.head;

    while (pk_row){
        if (!pk_row->available_pks){
            free(pk_row->pks);
            if(pk_row==verifier->pk_matrix.head) {
                verifier->pk_matrix.head = verifier->pk_matrix.head->next;
                free(pk_row);
            }else{
                while(prev->next!=pk_row){prev=prev->next;};
                prev->next=pk_row->next;
                /* Updating the tail pointer */
                if(pk_row==verifier->pk_matrix.tail)
                    verifier->pk_matrix.tail=prev;
                free(pk_row);
            }
            cleaned_rows++;
        }
        pk_row=pk_row->next;
    }
    return cleaned_rows;
}


static void mumhors_verifier_remove_row(mumhors_verifier_t *verifier, public_key_t *pk_row){
    if (pk_row == verifier->pk_matrix.head)
        verifier->pk_matrix.head = verifier->pk_matrix.head->next;
    else{
        public_key_t *temp = verifier->pk_matrix.head;
        while(temp->next!=pk_row){temp=temp->next;};
        temp->next=pk_row->next;
        if (pk_row==verifier->pk_matrix.tail)
            verifier->pk_matrix.tail=temp;
    }
    verifier->active_pks -= pk_row->available_pks;

    //TODO remove
    free(pk_row);
}


static void mumhors_verifier_alloc_row(mumhors_verifier_t *verifier){
    if (verifier->next_row_number >= verifier->r){
        printf("NO MORE ROW TO ALLOCATE\n");
        exit(1);
    }

    if(!mumhors_verifier_cleanup_rows(verifier)){
        /* No row was cleaned up. We perform the same policy as the signer. */

        /* Find the row the fewest number of pks to be deleted */
        public_key_t *pk_row = verifier->pk_matrix.head;
        int max_pks = verifier->c;
        public_key_t *target_row;

        while(pk_row){
            if (pk_row->available_pks<max_pks){
                max_pks=pk_row->available_pks;
                target_row = pk_row;
            }
            pk_row=pk_row->next;
        }
        /* The row is found. Delete the row */
        mumhors_verifier_remove_row(verifier, target_row);
    }

    /* Add another row to the window */
    verifier->active_pks+=verifier->c;
    verifier->next_row_number++;

}



void mumhors_verify(mumhors_verifier_t *verifier, int * indices, int num_indices){
    if (verifier->windows_size > verifier->active_pks) {
        /* Perform a row allocation */
        mumhors_verifier_alloc_row(verifier);
    }

    for(int i=0;i<num_indices;i++){
        public_key_t *pk_row = verifier->pk_matrix.head;
        int target_index = indices[i];

        /* Finding the row containing the target index */
        int row_index = 0;
        while(pk_row){
            if (target_index<pk_row->available_pks)
                break;
            target_index-=pk_row->available_pks;
            row_index++;
            pk_row=pk_row->next;
        }

        /* The current row contains the public keys */
        for(int j=0;j<verifier->c;j++){
            if (pk_row->pks[j]){ // If the PK is not NULL
                if(target_index == 0){
                    printf("R: %d C: %d", row_index, j);
                    break;
                }
                target_index--;
            }
        }
        printf("\n");
    }
    mumhors_invalidate_public_pks(verifier, indices,num_indices);

}


void pk_display(mumhors_verifier_t *verifier){
    public_key_t *pk_row = verifier->pk_matrix.head;

    int cnt_rows_to_show = verifier->rt;

    while (pk_row && cnt_rows_to_show) {
        printf("#:%d (%d) ", pk_row->number,  pk_row->available_pks);
        for (int i = 0; i < verifier->c; i++){
            if (pk_row->pks[i])
                printf("1 ");
            else
                printf("0 ");
        }
        cnt_rows_to_show--;
        pk_row = pk_row->next;
        printf("\n");
    }
    printf("-------%d-------\n",verifier->active_pks);
}

