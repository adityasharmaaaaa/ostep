#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

/* Busy-wait for about the requested number of seconds. */
void Spin(int seconds)
{
    struct timeval start, now;

    gettimeofday(&start, NULL);

    while (1) {
        gettimeofday(&now, NULL);

        double elapsed =
            (double)(now.tv_sec - start.tv_sec) +
            (double)(now.tv_usec - start.tv_usec) / 1000000.0;

        if (elapsed >= seconds) {
            break;
        }
    }
}

/* Create a thread; stop with an error message if it fails. */
void Pthread_create(pthread_t *thread,
                    const pthread_attr_t *attr,
                    void *(*start_routine)(void *),
                    void *arg)
{
    int rc = pthread_create(thread, attr, start_routine, arg);

    if (rc != 0) {
        fprintf(stderr, "pthread_create failed: %d\n", rc);
        exit(EXIT_FAILURE);
    }
}

/* Wait for a thread to finish; stop if joining fails. */
void Pthread_join(pthread_t thread, void **retval)
{
    int rc = pthread_join(thread, retval);

    if (rc != 0) {
        fprintf(stderr, "pthread_join failed: %d\n", rc);
        exit(EXIT_FAILURE);
    }
}
