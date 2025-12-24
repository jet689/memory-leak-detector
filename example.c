#include <stdio.h>
#include "leak.h"

int main() {
    char *a = malloc(100);
    free(a);

    char *b = malloc(120);
    // forget to free         //--> LEAK (120)

    char *c = calloc(10, 8);
    // refrence to memory lost
    c = malloc(10);           //--> LEAK (10 * 8)

    char *d = realloc(c, 15); // Here memory will be free first
    free(d);                  // then allocated so 1 free will add here also

    char *e;
    for(int i = 0; i < 5; i++) {
        e = malloc(100);
    } // --> LEAK (5 * 100)
    // TOTAL --> 120 + 10 * 8 +5*100 = 700 bytes Leaked
    //       --> 10 allocations
    //       --> 3 free

    return EXIT_SUCCESS;
}
