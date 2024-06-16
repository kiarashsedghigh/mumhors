#include <mumhors/debug.h>
#include <stdio.h>

void debug(unsigned char message[], int debug_level){
    if (debug_level == DEBUG_INF)
        printf("\e[1;34m[INF]\e[0m %s\n", message);
    else if (debug_level == DEBUG_ERR)
        printf("\e[1;31m[ERR]\e[0m %s\n", message);
    else if (debug_level == DEBUG_WARNING)
        printf("\e[1;38;5;208m[WAR]\e[0m %s\n", message);
}
