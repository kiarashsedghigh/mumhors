#include <muhors/bitmap.h>
#include <muhors/cqueue.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

void bitmap_init(bitmap_t * bm, int rows, int cols){
    assert(cols%8==0);
    cq_init(&bm->rows, rows);
    for(int i=0; i<rows; i++){
        row_t r = malloc(sizeof (unsigned char) * cols/8);
        for (int i=0; i< cols/8; i++)
            r[i]=0xff;
        cq_enqueue(&bm->rows, (void *)r);
    }
    bm->r = rows;
    bm->cb = cols;
    bm->cB = cols/8;
}

void bitmap_delete(bitmap_t * bm){
    cq_delete(&bm->rows);
}

void bitmap_display(bitmap_t * bm){
    cq_iter_next(NULL);

    for (int i=0; i<bm->r; i++){
        row_t r = cq_iter_next(&bm->rows);
        for (int j = 0; j<bm->cB; j++)
            printf("%d ",r[j]);
        printf("\n");
    }

}

