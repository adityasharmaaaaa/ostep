#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

// On macOS / BSD systems, execvP is provided in the standard C library
extern int execvP(const char *, const char *, char *const []);

int main(void) {
    for (int i = 0; i < 6; i++) {
        pid_t rc = fork();

        if (rc < 0) {
            perror("fork");
            exit(1);
        }

        if (rc == 0) {
            printf("\n--- Child executing variant %d ---\n", i);
            fflush(stdout);

            char *args[] = {"ls", "-l", NULL};
            char *env[] = {"PATH=/bin:/usr/bin", NULL};

            switch (i) {
                case 0:
                    // execl: List of arguments, full path, uses current environment
                    printf("Running execl(\"/bin/ls\", \"ls\", \"-l\", NULL):\n");
                    execl("/bin/ls", "ls", "-l", NULL);
                    break;

                case 1:
                    // execle: List of arguments, full path, explicit environment
                    printf("Running execle(\"/bin/ls\", \"ls\", \"-l\", NULL, env):\n");
                    execle("/bin/ls", "ls", "-l", NULL, env);
                    break;

                case 2:
                    // execlp: List of arguments, file name searched via PATH, uses current environment
                    printf("Running execlp(\"ls\", \"ls\", \"-l\", NULL):\n");
                    execlp("ls", "ls", "-l", NULL);
                    break;

                case 3:
                    // execv: Vector (array) of arguments, full path, uses current environment
                    printf("Running execv(\"/bin/ls\", args):\n");
                    execv("/bin/ls", args);
                    break;

                case 4:
                    // execvp: Vector of arguments, file name searched via PATH, uses current environment
                    printf("Running execvp(\"ls\", args):\n");
                    execvp("ls", args);
                    break;

                case 5:
                    // execvP: Vector of arguments, searches a custom search-path parameter (BSD/macOS)
                    printf("Running execvP(\"ls\", \"/bin:/usr/bin\", args):\n");
                    execvP("ls", "/bin:/usr/bin", args);
                    break;
            }

            // If exec succeeds, it never returns. If we reach here, an error occurred.
            perror("exec failed");
            exit(1);
        } else {
            // Parent waits for this child to complete before launching the next variant
            waitpid(rc, NULL, 0);
        }
    }

    return 0;
}

/*
 * ============================================================================
 * ANSWERS TO QUESTION 4:
 * ============================================================================
 *
 * Question:
 *   Write a program that calls fork() and then calls some form of exec() to run
 *   the program /bin/ls. See if you can try all of the variants of exec(),
 *   including execl(), execle(), execlp(), execv(), execvp(), and execvP().
 *   Why do you think there are so many variants of the same basic call?
 *
 * 1. How the exec() naming scheme works:
 *    Every variant starts with "exec" followed by letters encoding its interface:
 *
 *    - 'l' vs 'v' (Argument passing mechanism):
 *      * 'l' (List): Arguments are passed as individual function parameters,
 *        terminated with a NULL pointer (e.g., execl(path, arg0, arg1, ..., NULL)).
 *      * 'v' (Vector): Arguments are passed as an array of string pointers
 *        (char *argv[]), ending with a NULL pointer (e.g., execv(path, argv)).
 *
 *    - 'p' (Path search):
 *      * With 'p' (execlp, execvp): You provide a simple program name (like "ls"),
 *        and the OS automatically searches directories listed in the $PATH
 *        environment variable.
 *      * Without 'p': You must supply an exact absolute or relative path
 *        (e.g., "/bin/ls").
 *
 *    - 'e' (Environment control):
 *      * With 'e' (execle, execve): You explicitly pass a custom environment
 *        array (char *envp[]).
 *      * Without 'e': The child process automatically inherits the calling
 *        process's environment (the global `environ` variable).
 *
 *    - 'P' (Explicit search path - BSD/macOS extension):
 *      * Takes an explicit search path string (e.g., "/bin:/usr/bin") as a
 *        separate parameter rather than relying on the caller's $PATH.
 *
 * 2. Summary Table of Variants:
 *    +-----------+---------------------+-------------------+---------------------+
 *    | Function  | Arg Presentation    | Path Resolution   | Environment Source  |
 *    +-----------+---------------------+-------------------+---------------------+
 *    | execl     | List (varargs)      | Exact path only   | Inherited (environ) |
 *    | execle    | List (varargs)      | Exact path only   | Explicit (envp)     |
 *    | execlp    | List (varargs)      | $PATH search      | Inherited (environ) |
 *    | execv     | Vector (char *[])   | Exact path only   | Inherited (environ) |
 *    | execvp    | Vector (char *[])   | $PATH search      | Inherited (environ) |
 *    | execvP    | Vector (char *[])   | Custom path string| Inherited (environ) |
 *    +-----------+---------------------+-------------------+---------------------+
 *
 * 3. Why are there so many variants of the same basic call?
 *    - Convenience for the C programmer:
 *      C lacks function overloading, optional arguments, and default parameters.
 *      Depending on the context, a programmer might already have arguments in an
 *      array (like a shell parsing user input into tokens -> `execvp`), or have a
 *      fixed set of compile-time arguments (like calling `execl("/bin/ls", ...)`).
 *
 *    - Architectural reality:
 *      In Unix/Linux/POSIX, there is actually only ONE true system call in the
 *      kernel: `execve()` (vector + explicit environment).
 *      All other variants (`execl`, `execlp`, `execle`, `execv`, `execvp`, `execvP`)
 *      are lightweight library wrapper functions in libc that package arguments,
 *      inspect $PATH, and eventually delegate down to `execve()`.
 * ============================================================================
 */
