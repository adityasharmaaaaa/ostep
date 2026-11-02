#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

#if defined(__linux__)
#include <sched.h>
#endif

#define ITERATIONS 1000000

long long get_time_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);

    return (long long)ts.tv_sec * 1000000000LL
           + ts.tv_nsec;
}

void read_byte(int fd, char *c) {
    if (read(fd, c, 1) != 1) {
        perror("read");
        exit(1);
    }
}

void write_byte(int fd, char c) {
    if (write(fd, &c, 1) != 1) {
        perror("write");
        exit(1);
    }
}

int main() {

    int pipe1[2], pipe2[2];

    if (pipe(pipe1) < 0 || pipe(pipe2) < 0) {
        perror("pipe");
        return 1;
    }

#if defined(__linux__)
    // Find one CPU on which this process is allowed to run (Linux only).
    cpu_set_t allowed, single_cpu;

    CPU_ZERO(&allowed);

    if (sched_getaffinity(0, sizeof(allowed), &allowed) < 0) {
        perror("sched_getaffinity");
        return 1;
    }

    int cpu = -1;

    for (int i = 0; i < CPU_SETSIZE; i++) {
        if (CPU_ISSET(i, &allowed)) {
            cpu = i;
            break;
        }
    }

    if (cpu == -1) {
        fprintf(stderr, "No available CPU\n");
        return 1;
    }

    CPU_ZERO(&single_cpu);
    CPU_SET(cpu, &single_cpu);

    if (sched_setaffinity(0, sizeof(single_cpu),
                          &single_cpu) < 0) {
        perror("sched_setaffinity");
        return 1;
    }

    printf("Using CPU: %d (bound via sched_setaffinity)\n", cpu);
#elif defined(__APPLE__)
    // macOS / Darwin does not provide sched_setaffinity(2) because its XNU
    // kernel dynamically manages thread scheduling across heterogeneous cores (P/E cores).
    printf("Running on macOS (Darwin): sched_setaffinity not supported, using OS thread scheduler.\n");
#else
    printf("Running on non-Linux OS: CPU affinity binding skipped.\n");
#endif

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return 1;
    }

    char token = 'A';

    if (pid == 0) {

        // CHILD PROCESS

        close(pipe1[1]);
        close(pipe2[0]);

        for (int i = 0; i < ITERATIONS; i++) {

            read_byte(pipe1[0], &token);

            write_byte(pipe2[1], token);
        }

        close(pipe1[0]);
        close(pipe2[1]);

        exit(0);
    }

    else {

        // PARENT PROCESS

        close(pipe1[0]);
        close(pipe2[1]);

        long long start = get_time_ns();

        for (int i = 0; i < ITERATIONS; i++) {

            write_byte(pipe1[1], token);

            read_byte(pipe2[0], &token);
        }

        long long end = get_time_ns();

        close(pipe1[1]);
        close(pipe2[0]);

        waitpid(pid, NULL, 0);

        long long total_time = end - start;

        printf("Iterations: %d\n", ITERATIONS);
        printf("Total time: %lld ns\n", total_time);

        printf("Average round-trip time: %.2f ns\n",
               (double)total_time / ITERATIONS);

        printf("Estimated context-switch cost: %.2f ns\n",
               (double)total_time / (2.0 * ITERATIONS));
    }

    return 0;
}
