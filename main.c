#include <muhors/cqueue.h>
#include <muhors/bitmap.h>
#include <stdlib.h>
#include <stdio.h>

int main(){

    bitmap_t bm;
    bitmap_init(&bm, 3, 128);
    bitmap_display(&bm);

}