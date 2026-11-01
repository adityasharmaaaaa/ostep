#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void) {
    pid_t rc = fork();

    if (rc < 0) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    } else if (rc == 0) {
        // --- CHILD PROCESS ---
        // Close standard output (file descriptor 1)
        close(STDOUT_FILENO);

        // Attempt to print after STDOUT is closed
        int ret = printf("Can you see this message from the child?\n");

        // Flush standard output buffer to trigger the underlying write() system call immediately
        int flush_ret = fflush(stdout);

        // Report results via stderr (file descriptor 2 is still open!)
        fprintf(stderr, "Child: printf returned %d, fflush returned %d\n", ret, flush_ret);
        if (flush_ret != 0) {
            perror("Child fflush/write error");
        }

        exit(EXIT_SUCCESS);
    } else {
        // --- PARENT PROCESS ---
        wait(NULL);
    }

    return 0;
}

/*
 * ============================================================================
 * ANSWERS TO QUESTION 7:
 * ============================================================================
 *
 * Question:
 *   Write a program that creates a child process, and then in the child
 *   closes standard output (STDOUT_FILENO). What happens if the child calls
 *   printf() to print some output after closing the descriptor?
 *
 * 1. What happens when printf() is called?
 *    The output NEVER appears on the terminal. The message is completely lost.
 *
 * 2. Why does this happen? The two-step buffering process:
 *    - Step 1: User-space libc buffering
 *      printf() does not immediately perform an OS system call. Instead, it
 *      formats the string and writes it into a user-space memory buffer maintained
 *      by the C standard library for the FILE * stream `stdout`.
 *      Because printf() successfully copies the formatted characters into this
 *      memory buffer, printf() initially returns a positive number (the number
 *      of characters formatted, e.g. 41).
 *
 *    - Step 2: Kernel system call write(1, ...)
 *      When the buffer is flushed (either automatically due to a newline in
 *      line-buffered mode, explicitly via fflush(stdout), or upon process exit),
 *      the C library attempts to send the buffered bytes to the operating system
 *      kernel by executing:
 *          write(STDOUT_FILENO, buffer, count);  // write(1, ...)
 *
 *      Because file descriptor 1 was closed via close(STDOUT_FILENO), the kernel
 *      rejects the system call and returns -1, setting `errno` to:
 *          EBADF (Bad file descriptor)
 *
 *    - In our program:
 *      stderr (descriptor 2) remains open, so the child's error diagnostic:
 *          "Child fflush/write error: Bad file descriptor"
 *      is visible on the terminal, confirming that the underlying write failed.
 * ============================================================================
 */
