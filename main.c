#include <muhors/bitmap.h>
#include <muhors/sort.h>

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

//#define T 16
//#define K 4
//#define L 5
//#define IR 3
//#define RT 3
//#define RCSIZE 12
//#define TESTS 20


#define T 1024
#define K 16
#define L 5
#define IR 3
#define RT 3
#define RCSIZE T/2
#define TESTS 230


void generate_rands(int * arr, int n){
    for (int i = 0; i < K; i++)
        arr[i]=rand()%T;
}


int main(){

    bitmap_t bm;
    bitmap_init(&bm, L, T, IR, RT, RCSIZE);


    int indices[K];
    srand(time(NULL));

    for(int i=0;i< TESTS; i++){
        generate_rands(indices, K);

//        bitmap_display(&bm);
        printf("-----%d----\n", i);
        for(int i=0;i<K;i++)
            printf("%d ", indices[i]);
        printf("\n");
        printf("NODES: %d,   MatrixONES: %d\n", bm.compressed_rows.current_nodes, bm.num_ones_in_active_rows);
        bitmap_display(&bm);
        bitmap_unset_index_in_window(&bm, indices, K, T);


    }

    bitmap_delete(&bm);


////    (bm.bitmap_matrix.head->next->data[0])= 0x12;
//    bitmap_allocate_more_row(&bm);
//    printf("%d\n", bm.compressed_rows.current_nodes);
//
//    int index[]={384,0};
//    int index2[]={384};
//
//    bitmap_unset_index_in_window(&bm, index, 2, 128);
//    bitmap_display(&bm);
//
//    printf("------------------\n");
//
//    bitmap_unset_index_in_window(&bm, index2, 1, 128);
//    bitmap_unset_index_in_window(&bm, index2, 1, 128);
//
//
//    printf("\n\n");
////    bitmap_rowcompressor_display(&bm.compressed_rows);
//    bitmap_display(&bm);
//
//
//    bitmap_delete(&bm);


//    bitobj_t x = { 12,4};



//        bitmap_delete(&bm);


//    cqueue_t cq;
//    cq_init(&cq, 3);
//    int x = 12;
//    int t = 15;
//    int d = 99;
//    int rr = 10;
//    cq_enqueue(&cq, &x);
//    cq_enqueue(&cq, &t);
//    cq_enqueue(&cq, &d);
//    cq_dequeue(&cq);
//    cq_enqueue(&cq, &rr);
//
////    int * r = cq_get_row_by_index(&cq, 3);
//    cq_display(&cq);
//    cq_remove_row_by_index(&cq,2);
//    printf("\n");
//    cq_display(&cq);

}