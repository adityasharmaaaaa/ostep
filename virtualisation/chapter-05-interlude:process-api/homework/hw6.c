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
        // Child process
        printf("Child process (PID: %d) running...\n", getpid());
        sleep(1);
        printf("Child process finished.\n");
        exit(42);
    } else {
        // Parent process
        int status;
        printf("Parent process (PID: %d) waiting specifically for child (PID: %d)...\n", getpid(), rc);

        // waitpid(target_pid, &status, options)
        // Passing rc waits specifically for that exact child PID.
        pid_t child_pid = waitpid(rc, &status, 0);

        if (child_pid == -1) {
            perror("waitpid failed");
            exit(EXIT_FAILURE);
        }

        printf("waitpid() returned PID: %d\n", child_pid);

        if (WIFEXITED(status)) {
            printf("Child exited normally with status: %d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("Child terminated by signal: %d\n", WTERMSIG(status));
        }
    }

    return 0;
}

/*
 * ============================================================================
 * ANSWERS TO QUESTION 6:
 * ============================================================================
 *
 * Question:
 *   Write a slight modification of the previous program, this time using
 *   waitpid() instead of wait(). When would waitpid() be useful?
 *
 * 1. How waitpid() works:
 *    Signature: pid_t waitpid(pid_t pid, int *status, int options);
 *    In fact, wait(&status) is just a shorthand wrapper for:
 *        waitpid(-1, &status, 0);
 *
 * 2. When would waitpid() be useful?
 *    waitpid() provides much finer control than plain wait() in several
 *    critical operating system scenarios:
 *
 *    a) Waiting for a SPECIFIC Child:
 *       If a parent creates multiple children (e.g., Child A, Child B, Child C),
 *       calling wait() will return as soon as ANY child exits. If the parent
 *       specifically needs Child B's results before continuing, wait() cannot
 *       guarantee that. With waitpid(pid_B, ...), the parent targets that exact
 *       process.
 *
 *    b) Non-blocking Execution with WNOHANG:
 *       If you pass the `WNOHANG` flag in `options`, waitpid() does not put the
 *       parent to sleep. If the child hasn't finished yet, waitpid() immediately
 *       returns 0. This enables event loops, polling, or doing background work
 *       while occasionally checking if a child process has completed.
 *
 *    c) Process Groups:
 *       - If `pid < -1`: wait for any child process whose process group ID
 *         equals the absolute value of `pid`.
 *       - If `pid == 0`: wait for any child process whose process group ID
 *         equals that of the calling process.
 *
 *    d) Monitoring Stopped and Resumed Children (Shell Job Control):
 *       With flags like `WUNTRACED` and `WCONTINUED`, waitpid() can detect when
 *       a child process is stopped (e.g. user pressed Ctrl+Z / SIGSTOP) or
 *       resumed (SIGCONT). This is how interactive shells implement job control
 *       (e.g., fg, bg, jobs commands).
 * ============================================================================
 */
