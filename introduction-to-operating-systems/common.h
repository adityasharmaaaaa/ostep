#ifndef COMMON_H
#define COMMON_H

#include <pthread.h>

void Spin(int seconds);

void Pthread_create(pthread_t *thread,
                    const pthread_attr_t *attr,
                    void *(*start_routine)(void *),
                    void *arg);

void Pthread_join(pthread_t thread, void **retval);

#endif
