#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

int main(void) {
    int pipefd[2]; // pipefd[0] is read end, pipefd[1] is write end

    // 1. Create the pipe before forking
    if (pipe(pipefd) == -1) {
        perror("pipe failed");
        exit(EXIT_FAILURE);
    }

    // 2. Fork first child (Writer)
    pid_t pid1 = fork();
    if (pid1 < 0) {
        perror("fork 1 failed");
        exit(EXIT_FAILURE);
    }

    if (pid1 == 0) {
        // Child 1: Writer
        // Redirect stdout to the pipe's write end
        dup2(pipefd[1], STDOUT_FILENO);

        // Close original pipe descriptors now that stdout points to the write end
        close(pipefd[0]); // Unused read end
        close(pipefd[1]); // Replaced by STDOUT_FILENO

        // Any writes to stdout now flow directly into the pipe!
        printf("Hello from Child 1 (PID: %d) through the pipe!\n", getpid());
        fflush(stdout);

        exit(EXIT_SUCCESS);
    }

    // 3. Fork second child (Reader)
    pid_t pid2 = fork();
    if (pid2 < 0) {
        perror("fork 2 failed");
        exit(EXIT_FAILURE);
    }

    if (pid2 == 0) {
        // Child 2: Reader
        // Redirect stdin to the pipe's read end
        dup2(pipefd[0], STDIN_FILENO);

        // Close original pipe descriptors now that stdin points to the read end
        close(pipefd[0]); // Replaced by STDIN_FILENO
        close(pipefd[1]); // Unused write end

        // Read data coming through stdin (sent by Child 1 through the pipe)
        char buffer[256];
        if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
            fprintf(stderr, "Child 2 (PID: %d) received: %s", getpid(), buffer);
        }

        exit(EXIT_SUCCESS);
    }

    // 4. Parent process
    // CRITICAL: Parent must close both ends of the pipe!
    // If the parent leaves pipefd[1] (write end) open, Child 2's reader will
    // never receive EOF (End-Of-File) when Child 1 exits, because the kernel
    // thinks the parent might still write to the pipe. This would cause Child 2
    // to hang indefinitely!
    close(pipefd[0]);
    close(pipefd[1]);

    // Wait for both children to terminate cleanly
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);

    return 0;
}

/*
 * ============================================================================
 * ANSWERS TO QUESTION 8:
 * ============================================================================
 *
 * Question:
 *   Write a program that creates two children, and connects the standard
 *   output of one to the standard input of the other, using the pipe() system call.
 *
 * 1. How pipe() and dup2() connect two processes:
 *    - pipe(pipefd) creates a unidirectional byte channel allocated in kernel
 *      memory.
 *      * pipefd[0] is the READ end.
 *      * pipefd[1] is the WRITE end.
 *
 *    - dup2(oldfd, newfd) duplicates an open file descriptor onto newfd,
 *      silently closing newfd first if it was open.
 *      * In Child 1: `dup2(pipefd[1], STDOUT_FILENO)` rewires file descriptor 1
 *        (stdout) to point to the pipe's write end.
 *      * In Child 2: `dup2(pipefd[0], STDIN_FILENO)` rewires file descriptor 0
 *        (stdin) to point to the pipe's read end.
 *
 * 2. Why closing unused pipe descriptors is MANDATORY:
 *    - A reader reading from a pipe (such as read() or fgets()) only gets an
 *      EOF indicator (0 bytes read) when ALL write descriptors across all
 *      processes pointing to that pipe have been closed.
 *    - If Child 1 closes pipefd[1] and terminates, but the Parent forgets to
 *      close pipefd[1], the pipe still has one open write descriptor in the parent.
 *      As a result, Child 2 will block forever waiting for EOF and freeze.
 *    - Therefore, the rule in Unix pipe programming is:
 *      EVERY process must close any pipe end it does not actively read from or write to.
 *
 * 3. Connection to the Shell:
 *    This exact pattern is how Unix shells implement the command-line pipe:
 *        cmd1 | cmd2
 *    The shell creates the pipe, forks both processes, uses dup2 to redirect
 *    their standard streams, closes the pipe descriptors in the parent, and
 *    executes the respective binaries in each child.
 * ============================================================================
 */
