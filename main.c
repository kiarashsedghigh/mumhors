#include <muhors/bitmap.h>
#include <muhors/sort.h>
#include <muhors/debug.h>

#include <time.h>
#include <stdlib.h>
#include <stdio.h>

void generate_rands(int * arr, int k, int t){
    for (int i = 0; i < k; i++)
        arr[i]=rand()%t;
}


int main(int argc, char ** argv){
    int t, k , l , ir, rt , tests;
    if (argc<7){
        printf("|HELP|\n\tRun:\n");
        printf("\t\t muhors T K L IR RT TESTS\n");
        exit(1);
    }

    //TODO atoi is deprecated
    t= atoi(argv[1]);
    k= atoi(argv[2]);
    l= atoi(argv[3]);
    ir= atoi(argv[4]);
    rt= atoi(argv[5]);
    tests= atoi(argv[6]);


    bitmap_t bm;
    bitmap_init(&bm, l, t, ir, rt);

    int * indices = malloc(sizeof(int) * k);
    srand(time(NULL));

    for(int i=0;i< tests; i++){
        generate_rands(indices, k, t);
        if(bitmap_unset_index_in_window(&bm, indices, k, t) == BITMAP_UNSET_BITS_FAILED){
            printf("---> Last covered message: %d\n", i);

            debug("No more rows to allocate", DEBUG_ERR);
            bitmap_report(&bm);
            bitmap_delete(&bm);
            exit(1);
        }
    }
    debug("Successfully covered all the test cases", DEBUG_INF);
    bitmap_delete(&bm);
}