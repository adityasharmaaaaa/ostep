# Chapter 5 — Interlude: Process API

> These are my personal notes, dictated from my own understanding and refined with the help of an LLM. The ideas and understanding are mine — AI just helped with the wording. Purely for my own learning through *OSTEP*.

---

## What even is a "Process API"?

Last chapter we understood *what* a process is. This chapter is about *how you interact with one* — how you create them, wait for them, and replace them with something else entirely. The three main system calls are `fork()`, `wait()`, and `exec()`. They're short, they're weird, and between them they explain how basically everything in Unix works.

Let's go one by one.

---

## `fork()` — The Weird Cloning Machine 🧬

```c
int rc = fork();
```

`fork()` creates a new process. That's the simple version. The weird version: **it creates an exact copy of the calling process.** Not a new process running from `main()`. A clone. Mid-execution. Right at the point where `fork()` was called.

So after `fork()` returns, there are **two processes** running the same code, both having just returned from `fork()`. How do you tell which one is which? By the return value:

| Who are you? | What `fork()` returns |
|---|---|
| Parent | PID of the newly created child (a positive number) |
| Child | `0` |
| Error | `-1` (fork failed) |

The child is not a complete photocopy — it has its own **address space, registers, program counter, open files**. It starts with the same values as the parent (copied at the moment of fork), but from that point on they're independent. Changing a variable in the child does not affect the parent. They're separate people now.

### `p1.c` — The simplest fork

```c
printf("hello world (pid:%d)\n", getpid());
int rc = fork();
if (rc < 0)  { /* error */ }
else if (rc == 0) { printf("hello i am child (pid:%d)\n", getpid()); }
else          { printf("hello i am parent of %d (pid:%d)\n", rc, getpid()); }
```

**Output:**
```
hello world (pid:59875)
hello i am parent of 59886 (pid:59875)
hello i am child (pid:59886)
```

Notice: "hello world" printed **once** — because only the parent ran that line. The child came to life *after* `fork()`, inheriting the parent's memory state but not going back to rerun `printf`. It wakes up as if it had just called `fork()` itself.

### The non-determinism problem 🎲

Who runs first — parent or child? **Nobody knows.** The CPU scheduler decides. On a single CPU, it's one or the other. Both are valid. This means the output of P1 can be in either order:

```
# Could be:
hello world
hello i am parent...
hello i am child...

# Or:
hello world
hello i am child...
hello i am parent...
```

This non-determinism is fine in a simple print example, but in multithreaded programs with shared data? That's where bugs are born. We'll revisit this in the concurrency section (fun times ahead).

---

## `wait()` — The Patience Call ⏳

```c
int wc = wait(NULL);
```

Sometimes the parent needs to wait for the child to finish before doing something. `wait()` does exactly that — it **blocks the parent** until one of its children exits. When the child finishes, `wait()` returns the child's PID.

### `p2.c` — Adding wait

```c
int rc = fork();
if (rc == 0) {
    printf("hello i am child (pid:%d)\n", getpid());
} else {
    int wc = wait(NULL);
    printf("hello i am parent of %d (wc:%d) (pid:%d)\n", rc, wc, getpid());
}
```

**Output:**
```
hello world (pid:63374)
hello i am child (pid:63378)
hello i am parent of 63378 (wc:63378) (pid:63374)
```

Now it's **deterministic**. The child always prints first because the parent called `wait()` and blocked itself. The parent can only resume after the child is done. `wc` holds the child's PID — confirming which child finished.

### `waitpid()` — The Precise Version

`wait()` waits for *any* child. What if you have multiple children and only care about one specific one? That's what `waitpid()` is for.

```c
pid_t waitpid(pid_t pid, int *status, int options);
```

- `pid` — which child to wait for (pass `-1` to wait for any, same as `wait`)
- `status` — a pointer to an int where the exit status is stored (can decode with macros like `WIFEXITED`, `WEXITSTATUS`)
- `options` — behaviour tweaks (e.g., `WNOHANG` = don't block, return immediately if child hasn't finished yet)

**Example:**
```c
int status;
pid_t child_pid = fork();
if (child_pid == 0) {
    // child does stuff
    exit(42);
} else {
    waitpid(child_pid, &status, 0);  // wait for this specific child
    if (WIFEXITED(status)) {
        printf("child exited with status %d\n", WEXITSTATUS(status));
        // prints: child exited with status 42
    }
}
```

`WNOHANG` is useful when you want to check if a child is done without getting stuck — like a non-blocking poll.

---

## `exec()` — The Body-Snatcher 👾

```c
execvp(myargs[0], myargs);
```

`fork()` makes a copy of your process. `exec()` **replaces** your process with a completely different program. Not a new process — the same process, but now running different code.

What `exec()` does:
1. Takes the name of an executable and its arguments
2. Loads that executable's code into the current process's memory
3. **Overwrites** the code segment, resets the heap and stack
4. Starts running the new program from its `main()`
5. **Never returns** (if successful) — because the old program is gone

If `exec()` returns, something went wrong.

### `p3.c` — fork + exec to run `wc`

```c
if (rc == 0) {  // child
    char *myargs[3];
    myargs[0] = strdup("wc");
    myargs[1] = strdup("p3.c");
    myargs[2] = NULL;
    execvp(myargs[0], myargs);
    printf("this shouldn't print out");  // never reached if exec succeeds
}
```

**Output:**
```
hello world (pid:68135)
hello i am child (pid:68146)
      27      85     798 p3.c
hello i am parent of 68146 (wc:68146) (pid:68135)
```

The child forked, then replaced itself with `wc`. The child is gone — `wc` is running in its body. It counts lines, words, and bytes in `p3.c`, prints the result, and exits. The parent's `wait()` picks this up and resumes.

### The `exec` family

There's not just one exec, there's a whole family:

| Function | Key behaviour |
|---|---|
| `execvp(file, argv)` | Search PATH for the program, pass argv array |
| `execv(path, argv)` | Full path required, pass argv array |
| `execl(path, arg0, ...)` | Full path, arguments listed out individually |
| `execlp(file, arg0, ...)` | Search PATH, arguments listed individually |
| `execle(path, arg0, ..., envp)` | Full path, individual args, custom environment |

For most uses, `execvp` is the one you want.

---

## Why Are `fork()` and `exec()` Separate? — The Shell Secret 🐚

This is the key insight of the chapter. Other operating systems have a single "spawn" call that creates a new process running a new program in one step. Unix splits it into two: `fork` then `exec`. That seems more complicated — why?

**Because the gap between `fork` and `exec` is where shells do their magic.**

Here's what a shell like `bash` does when you type `wc p3.c > newfile.txt`:

```
1. Shell forks → creates a child
2. Child (before calling exec):
     - closes stdout (fd 1)
     - opens newfile.txt → it gets fd 1 (lowest available)
3. Child calls exec("wc", ...)
4. wc runs — thinks it's writing to stdout, but it's actually writing to newfile.txt
5. Shell's parent waited, now prints a new prompt
```

The child had a window — between `fork` and `exec` — to rewire its file descriptors. Then `exec` replaced it with `wc`, which inherited those rewired descriptors. This is how **output redirection** (`>`) works. And since every process inherits open file descriptors from its parent, you get this for free.

### `p4.c` — Redirect output to a file

```c
if (rc == 0) {  // child
    close(STDOUT_FILENO);                              // close stdout
    open("./p4.output", O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU);  // opens as fd 1
    execvp("wc", myargs);                             // wc writes to p4.output
}
```

**Output on terminal:**
```
hello world (pid:71097)
(nothing else — wc's output went to p4.output)
```

`p4.output` now contains: `      27      85     798 p3.c`

The parent printed "hello world" before forking. The child silently replaced stdout and ran `wc`. The terminal saw nothing from `wc` because it was redirected.

### Unix Pipes work the same way

`ls | wc -l`

This is implemented as:
1. Fork two children
2. Create a pipe (a kernel buffer connecting them): `pipe(fds)`
3. Child 1 (`ls`): close stdout, point it to the write end of the pipe
4. Child 2 (`wc -l`): close stdin, point it to the read end of the pipe
5. exec `ls` in child 1, exec `wc` in child 2
6. `ls` output flows into the pipe → `wc` reads from the pipe

Long chains of commands (`cat file | grep foo | sort | uniq -c`) are just this pattern repeated. The shell builds the pipeline before exec-ing anything.

---

## Other Useful Process Tools

### `kill` — Send Signals to Processes

```sh
kill -9 <pid>    # SIGKILL — unblockable, process dies immediately
kill -15 <pid>   # SIGTERM — polite request to terminate (default)
kill -STOP <pid> # SIGSTOP — pause a process
kill -CONT <pid> # SIGCONT — resume a paused process
```

In C, you can send signals programmatically with `kill(pid, SIGTERM)`. Signals are how the OS (and you) communicate asynchronous events to running processes. The process can catch most signals and handle them (e.g., clean up before dying), except `SIGKILL` which is instant and uncatchable.

### `ps` — See What's Running

```sh
ps aux           # all processes, user-friendly format
ps aux | grep p1 # find your specific process
```

Shows PID, CPU%, memory%, command name, state. Your go-to for "wait, what is running on this machine?"

### `top` — The Live Dashboard

```sh
top
```

Continuously updated view of all processes, sorted by CPU usage. Good for finding what's hogging your CPU. As the book notes — `top` itself often shows up near the top of its own list. Bit of a narcissist.

---

## The Mental Model — Putting It Together

```
You type: ./myprogram arg1

Shell does:
  fork()
    └─ child: exec("./myprogram", ["./myprogram", "arg1"])
         └─ myprogram replaces the child, runs
              └─ exits with some code
    └─ parent: wait()
         └─ waits for child to finish
              └─ child done, wait() returns
                   └─ shell prints prompt again
```

And if you add a `>` redirect or a `|` pipe:
```
  fork()
    └─ child: rewire file descriptors
         └─ exec(...)
```

That's it. That's how `bash`, `zsh`, and every Unix shell works. Three system calls — `fork`, `exec`, `wait` — plus the ability to mess with file descriptors in between. All the power of a Unix shell falls out of this simple design.

---

## Programs in This Chapter

| File | What it shows |
|------|--------------|
| [`p1.c`](p1.c) | Basic `fork()` — non-deterministic output, two processes diverge |
| [`p2.c`](p2.c) | `fork()` + `wait()` — parent waits, output becomes deterministic |
| [`p3.c`](p3.c) | `fork()` + `exec()` — child replaces itself with `wc` |
| [`p4.c`](p4.c) | Output redirection — child rewires stdout before `exec` |

### Build commands

```sh
clang -Wall -Wextra p1.c -o p1
clang -Wall -Wextra p2.c -o p2
clang -Wall -Wextra p3.c -o p3
clang -Wall -Wextra p4.c -o p4
```

---

## Chapter 5 Coding Homework

The homework exercises for this chapter (`hw1.c` through `hw8.c`) are located in [`homework/`](homework/README.md).
- [`hw1.c`](homework/hw1.c) — Address space and variable isolation on `fork()`.
- [`hw2.c`](homework/hw2.c) — Shared open file descriptors and concurrent writes.
- [`hw3.c`](homework/hw3.c) — Synchronizing child-first output without `wait()`.
- [`hw4.c`](homework/hw4.c) — Testing all 6 `exec()` variants (`execl`, `execle`, `execlp`, `execv`, `execvp`, `execvP`).
- [`hw5.c`](homework/hw5.c) — `wait()` return values and calling `wait()` inside child.
- [`hw6.c`](homework/hw6.c) — `waitpid()` advantages and `WNOHANG` non-blocking checks.
- [`hw7.c`](homework/hw7.c) — Closing `STDOUT_FILENO` and libc `printf()` buffering behavior.
- [`hw8.c`](homework/hw8.c) — Connecting two children with `pipe()` and `dup2()`.

Read the comprehensive walkthrough in the [Homework README](homework/README.md).

---

## Self-Check Before Next Chapter

1. `fork()` returns different values to parent and child. What does each get? Why?
2. Why is the output of `p1.c` non-deterministic? What decides the order?
3. What does `wait()` do? How does adding it to `p2.c` make output deterministic?
4. What's the difference between `wait()` and `waitpid()`? When would you use `waitpid`?
5. `exec()` doesn't create a new process. What does it do instead?
6. Why does `exec()` never return on success?
7. Explain in your own words why `fork` and `exec` are two separate calls. What would you lose if they were one?
8. How does output redirection (`>`) work at the system call level? What happens to file descriptor 1?
9. How do pipes (`|`) work? (Hint: two forks, a kernel buffer, and some fd rewiring)
10. What does `kill -9` do? Why can't the process ignore it?

---

*Next: Chapter 6 — Mechanism: Limited Direct Execution. How does the OS stay in control while your program is running?*
