#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string.h>

int main(void) {
    int fd = open("output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("open failed");
        exit(1);
    }

    int rc = fork();
    if (rc < 0) {
        perror("fork failed");
        exit(1);
    } else if (rc == 0) {
        // Child process
        const char *msg = "child writing\n";
        write(fd, msg, strlen(msg));
    } else {
        // Parent process
        const char *msg = "parent writing\n";
        write(fd, msg, strlen(msg));
        wait(NULL); // wait for child to finish
    }

    close(fd);
    return 0;
}

/*
 * ============================================================================
 * ANSWERS TO QUESTION 2:
 * ============================================================================
 *
 * Question:
 *   Write a program that opens a file (with the open() system call) and then
 *   calls fork() to create a new process. Can both the child and parent access
 *   the file descriptor returned by open()? What happens when they are writing
 *   to the file concurrently, i.e., at the same time?
 *
 * 1. Can both child and parent access the file descriptor?
 *    YES. When fork() is called, the child inherits duplicate file descriptors
 *    for all open files held by the parent. In both processes, `fd` has the same
 *    integer numeric value and refers to the open file.
 *
 * 2. What happens when they are writing to the file concurrently?
 *    - Shared File Table Entry & Shared Offset:
 *      Unlike user-space memory which is separated into isolated address spaces,
 *      open file descriptors in parent and child point to the SAME underlying
 *      "struct file" (open file description) in the operating system kernel.
 *      Crucially, this struct holds the current file offset pointer.
 *
 *    - No Overwriting of Data:
 *      Because they share the exact same offset pointer, if Parent writes 15 bytes,
 *      the kernel moves the offset forward by 15 bytes. When Child subsequently
 *      writes, its data is written starting from that new offset, rather than
 *      overwriting Parent's data from the beginning.
 *
 *    - Non-deterministic Ordering (Race Condition):
 *      The order of which process writes first depends entirely on the CPU
 *      scheduler. If the parent runs first, `output.txt` contains:
 *          parent writing
 *          child writing
 *      If the child runs first, `output.txt` contains:
 *          child writing
 *          parent writing
 *
 *    - Atomicity:
 *      Individual write() system calls of modest size are executed atomically
 *      by the kernel with respect to file offset updates, meaning bytes from one
 *      write() call are not interleaved with bytes from the other write() call.
 * ============================================================================
 */
