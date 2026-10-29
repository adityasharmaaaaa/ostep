#include <stdio.h>
#include <stdlib.h>
#include "common.h"

/* Shared by both threads. volatile does not make this thread-safe. */
volatile int counter = 0;
int loops;

void *worker(void *arg)
{
    (void)arg;

    for (int i = 0; i < loops; i++) {
        counter++;
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "usage: threads <value>\n");
        return EXIT_FAILURE;
    }

    loops = atoi(argv[1]);
    if (loops < 0) {
        fprintf(stderr, "value must be non-negative\n");
        return EXIT_FAILURE;
    }

    pthread_t p1, p2;
    printf("Initial value : %d\n", counter);

    Pthread_create(&p1, NULL, worker, NULL);
    Pthread_create(&p2, NULL, worker, NULL);
    Pthread_join(p1, NULL);
    Pthread_join(p2, NULL);

    printf("Final value : %d\n", counter);
    return 0;
}
