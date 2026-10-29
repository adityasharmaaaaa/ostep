#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "common.h"

int main(void)
{
    int *p = malloc(sizeof(int));
    assert(p != NULL);

    printf("(%d) memory address of p: %p\n", (int)getpid(), (void *)p);
    *p = 0;

    while (1) {
        Spin(1);
        *p = *p + 1;
        printf("(%d) p: %d\n", (int)getpid(), *p);
    }

    free(p); /* Unreachable unless the loop is changed. */
    return 0;
}
