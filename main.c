#include <muhors/bitmap.h>
#include <muhors/sort.h>

int main(){
//    int arr[]={4,1,0,12,199};
//    merge_sort(arr, 0, 4);
//    for(int i=0;i<5;i++)
//        printf("%d\n", arr[i]);
//

    bitmap_t bm;
    bitmap_init(&bm, 6, 128, 3, 3, 1000);
//    (bm.bitmap_matrix.head->next->data[0])= 0x12;
    bitmap_allocate_more_row(&bm);
    printf("%d\n", bm.compressed_rows.current_nodes);

    int index[]={384,0};
    int index2[]={384};

    bitmap_unset_index_in_window(&bm, index, 2, 128);
    bitmap_display(&bm);

    printf("------------------\n");

    bitmap_unset_index_in_window(&bm, index2, 1, 128);
    bitmap_unset_index_in_window(&bm, index2, 1, 128);


    printf("\n\n");
//    bitmap_rowcompressor_display(&bm.compressed_rows);
    bitmap_display(&bm);


    bitmap_delete(&bm);


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