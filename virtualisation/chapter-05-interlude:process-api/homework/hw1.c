#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void) {
    int x = 100;
    printf("Hello world (pid:%d)\n", (int)getpid());

    int rc = fork();
    if (rc < 0) {
        fprintf(stderr, "fork failed\n");
        exit(1);
    } else if (rc == 0) {
        // Child process
        printf("hello i am child (pid:%d)\n", (int)getpid());
        printf("child sees x = %d\n", x);
        x += 1;
        printf("child modified x to %d\n", x);
    } else {
        // Parent process
        printf("hello i am parent of %d (pid:%d)\n", rc, (int)getpid());
        printf("parent sees x = %d\n", x);
        x += 1;
        printf("parent modified x to %d\n", x);
    }

    return 0;
}

/*
 * ============================================================================
 * ANSWERS TO QUESTION 1:
 * ============================================================================
 *
 * Question:
 *   Write a program that calls fork(). Before calling fork(), have the main
 *   process access a variable (e.g., x) and set its value to something (e.g., 100).
 *   What value is the variable in the child process? What happens to the variable
 *   when both the child and parent change the value of x?
 *
 * 1. What value is the variable in the child process?
 *    The child process sees x = 100 initially.
 *    When fork() is called, the OS makes an exact copy of the parent process's
 *    virtual address space for the child. This means all memory contents,
 *    including global variables, heap allocations, and local stack variables
 *    like `x`, are replicated with their current values at the time of the fork.
 *
 * 2. What happens to the variable when both child and parent change x?
 *    Both processes maintain their OWN PRIVATE copies of `x`.
 *    When the child changes x (x += 1), it only alters the variable in its own
 *    isolated address space (x becomes 101 for the child).
 *    When the parent changes x (x += 1), it only alters the variable in the
 *    parent's address space (x becomes 101 for the parent).
 *    Neither process can observe or interfere with the other's modification.
 *    Under the hood, modern OSes use Copy-on-Write (COW) optimization: the
 *    physical memory page holding `x` is shared read-only until one process
 *    attempts to write to it, at which point the kernel creates a separate
 *    physical copy for that process.
 *
 * Sample Output:
 *   Hello world (pid:49177)
 *   hello i am parent of 49188 (pid:49177)
 *   parent sees x = 100
 *   parent modified x to 101
 *   hello i am child (pid:49188)
 *   child sees x = 100
 *   child modified x to 101
 * ============================================================================
 */
