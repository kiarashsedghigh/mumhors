#ifndef MUHORS_BITMAP_H
#define MUHORS_BITMAP_H

#include <muhors/cqueue.h>


typedef unsigned char * row_t;

typedef struct bitmap{
    int r;
    int cb;
    int cB;
    cqueue_t rows;
}bitmap_t;

void bitmap_init(bitmap_t * bm, int rows, int cols);

void bitmap_display(bitmap_t * bm);


#endif