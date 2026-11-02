#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#define ITERATIONS 10000000

long long get_time_ns(){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);

    return (long long)ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

int main(){
    int fd = open("/dev/null", O_RDONLY);

    if(fd < 0){
        perror("open");
        return 1;
    }

    char buffer;

    //warmup
    for(int i=0; i<10000; i++){
        read(fd,&buffer,0);
    }

    long long start = get_time_ns();

    for(int i=0; i<ITERATIONS; i++){
        if(read(fd,&buffer,0)<0){
            perror("read");
            return 1;
        }
    }

    long long end = get_time_ns();

    long long total_time = end - start;

    printf("Iterations: %d\n", ITERATIONS);
    printf("Total time: %lld ns\n", total_time);

    printf("Average system call time: %.2f ns\n",
           (double)total_time / ITERATIONS);

    close(fd);

    return 0;
}