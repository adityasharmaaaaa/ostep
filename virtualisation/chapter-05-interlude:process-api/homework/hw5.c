#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

int main(void) {
    pid_t rc = fork();

    if (rc < 0) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    } else if (rc == 0) {
        // --- CHILD PROCESS ---
        printf("Child process (PID: %d) started.\n", getpid());

        // What happens if we call wait() in the child?
        // Since this child has NOT created any children of its own,
        // wait() has no child process to wait for.
        int child_status;
        pid_t child_wait_rc = wait(&child_status);

        printf("Child called wait(): returned %d\n", child_wait_rc);
        if (child_wait_rc == -1) {
            perror("Child wait error");
        }

        printf("Child process finished, exiting with status 42.\n");
        exit(42);
    } else {
        // --- PARENT PROCESS ---
        int status;
        printf("Parent process (PID: %d) waiting for child (PID: %d)...\n", getpid(), rc);

        // wait(&status) blocks until ANY child finishes.
        // It returns the PID of that child.
        pid_t child_pid = wait(&status);

        printf("Parent wait() returned PID: %d\n", child_pid);

        if (WIFEXITED(status)) {
            printf("Child exited normally with exit code: %d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("Child was terminated by signal: %d\n", WTERMSIG(status));
        }
    }

    return 0;
}

/*
 * ============================================================================
 * ANSWERS TO QUESTION 5:
 * ============================================================================
 *
 * Question:
 *   Now write a program that uses wait() to wait for the child process to finish
 *   in the parent. What does wait() return? What happens if you use wait() in
 *   the child?
 *
 * 1. What does wait() return?
 *    - On Success:
 *      wait() blocks the calling process until one of its children terminates.
 *      It returns the Process ID (PID) of the child process that terminated.
 *      In addition, if a non-NULL pointer is passed as `&status`, the kernel
 *      stores the exit status / termination signal information into that integer,
 *      which can be inspected using POSIX macros like WIFEXITED() and WEXITSTATUS().
 *
 *    - On Error:
 *      wait() returns -1 and sets `errno` to describe the failure.
 *
 * 2. What happens if you use wait() in the child?
 *    - If the child process has not spawned any child processes of its own,
 *      calling wait() FAILS immediately.
 *    - It returns -1 without blocking, and sets `errno` to ECHILD
 *      ("No child processes").
 *    - In our program output, running wait() in the child prints:
 *          Child called wait(): returned -1
 *          Child wait error: No child processes
 *    - If the child *had* previously called fork() to spawn its own grandchild,
 *      then calling wait() in the child would successfully wait for that grandchild.
 * ============================================================================
 */
