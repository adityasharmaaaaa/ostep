#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void) {
    // We create a pipe before forking to synchronize parent and child
    // without calling wait() or waitpid()!
    // pfd[0] is the read end; pfd[1] is the write end.
    int pfd[2];
    if (pipe(pfd) == -1) {
        perror("pipe failed");
        exit(1);
    }

    int rc = fork();
    if (rc < 0) {
        perror("fork failed");
        exit(1);
    } else if (rc == 0) {
        // --- CHILD PROCESS ---
        // Close unused read end of the pipe
        close(pfd[0]);

        // Print "hello" first
        printf("hello\n");

        // Notify the parent by closing the write end of the pipe.
        // When all write ends of a pipe are closed, any read() on the
        // read end returns 0 (End-Of-File / EOF), immediately unblocking it!
        close(pfd[1]);
    } else {
        // --- PARENT PROCESS ---
        // Close unused write end of the pipe
        close(pfd[1]);

        // Attempt to read 1 byte from the pipe.
        // Because the pipe is empty, read() BLOCKS the parent.
        // The parent will not proceed until the child writes to the pipe
        // or closes its write end after printing "hello".
        char ch;
        read(pfd[0], &ch, 1);
        close(pfd[0]);

        // Now we are 100% guaranteed that child has already printed "hello"
        printf("goodbye\n");
    }

    return 0;
}

/*
 * ============================================================================
 * ANSWERS TO QUESTION 3:
 * ============================================================================
 *
 * Question:
 *   Write another program using fork(). The child process should print "hello";
 *   the parent process should print "goodbye". You should try to ensure that
 *   the child process always prints first; can you do this without calling
 *   wait() in the parent?
 *
 * 1. Can you do this without calling wait() in the parent?
 *    YES! There are several ways to synchronize processes in Unix without
 *    calling wait() or waitpid():
 *
 *    Method A: Using an Anonymous Pipe (Implemented in this program)
 *      - We create a pipe with pipe(pfd) prior to fork().
 *      - In the parent, we attempt a blocking read() from pfd[0].
 *      - The child executes, prints "hello", and closes pfd[1].
 *      - The parent's read() immediately unblocks upon receiving EOF (or a byte),
 *        and the parent prints "goodbye".
 *      - This guarantees that "hello" is always printed before "goodbye",
 *        regardless of how the CPU scheduler decides to run the two processes.
 *
 *    Method B: Using Unix Signals
 *      - The parent registers a signal handler for a user signal (e.g., SIGUSR1)
 *        and calls pause() or sigsuspend() to suspend execution.
 *      - The child prints "hello" and then sends the signal to the parent using
 *        kill(getppid(), SIGUSR1).
 *      - The signal interrupts pause(), and the parent proceeds to print "goodbye".
 *
 *    Method C: Using sleep() (Heuristic / Not guaranteed)
 *      - The parent calls sleep(1) before printing "goodbye".
 *      - In practice, 1 second is almost always enough for the child to print "hello".
 *      - However, sleep() is NOT a true synchronization primitive. Under extreme
 *        system load or thread starvation, the child could be delayed for more
 *        than the sleep duration, causing "goodbye" to print first.
 * ============================================================================
 */
