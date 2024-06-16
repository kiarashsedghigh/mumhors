#include <muhors/bitmap.h>
#include <muhors/debug.h>
#include <muhors/sort.h>
#include <muhors/muhors.h>

#include <time.h>
#include <stdlib.h>
#include <stdio.h>

void generate_rands(int * arr, int k, int t) {
    for (int i = 0; i < k; i++)
        arr[i] = rand() % t;
//    printf("%d %d\n", arr[0], arr[1]);
//    exit(0);
//    merge_sort(arr, 0, k - 1);
//    for(int i=1;i<k;i++){
//        if(arr[i]==arr[i-1]){
//            printf("SAME\n");
//            exit(0);
//        }
//
//    }
}


int main(int argc, char ** argv){
    int t, k , l , ir, rt , tests;
//    if (argc<7){
//        printf("|HELP|\n\tRun:\n");
//        printf("\t\t muhors T K L IR RT TESTS\n");
//        exit(1);
//    }
//
//    //TODO atoi is deprecated
//    t= atoi(argv[1]);
//    k= atoi(argv[2]);
//    l= atoi(argv[3]);
//    ir= atoi(argv[4]);
//    rt= atoi(argv[5]);
//    tests= atoi(argv[6]);

    t=1024;
    k=16;
    l=16384;
    ir=10;
    rt=10;
    tests=1048576;


//    t=1024;
//    k=16;
//    l=20;
//    ir=8;
//    rt=8;
//    tests=1;

    ////////////////////////////////
    muhors_signer_t signer;
    muhors_verifier_t verifier;

    muhors_init_signer(&signer, "asd", t, l ,ir, rt,l);
    muhors_init_verifier(&verifier, l, t, rt, t);



//    l=5;
//    t=5;
//    rt=3;

    srand(time(NULL));

    int * indices = malloc(sizeof(int) * k);

    for(int i=0;i< tests; i++){
        generate_rands(indices, k, t);
        printf("--T %d--\n", i);
//        if (i>1043810)
//            pk_display(&verifier);
        muhors_verify(&verifier, indices, k);
    }
    free(indices);





    muhors_delete_verifier(&verifier);
    muhors_delete_signer(&signer);



//
//    bitmap_t bm;
//    bitmap_init(&bm, l, t, ir, rt);
//
//    int * indices = malloc(sizeof(int) * k);
//    srand(time(NULL));
//
//    for(int i=0;i< tests; i++){
//        generate_rands(indices, k, t);
////        for(int i=0;i<k;i++)
////            printf("%d ", indices[i]);
////        printf("    ");
////        printf("%d\n", i);
////        if(bitmap_unset_index_in_window(&bm, indices, k, t) == BITMAP_UNSET_BITS_FAILED){
////            printf("---> Last covered message: %d\n", i);
////
////            debug("No more rows to allocate", DEBUG_ERR);
////#ifdef JOURNAL
////            bitmap_report(&bm);
////#endif
////            bitmap_delete(&bm);
////            exit(1);
////        }
//    }
//    debug("Successfully covered all the test cases", DEBUG_INF);
//#ifdef JOURNAL
//    bitmap_report(&bm);
//#endif
//    bitmap_delete(&bm);
}