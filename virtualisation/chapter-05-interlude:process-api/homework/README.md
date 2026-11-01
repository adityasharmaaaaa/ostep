# Homework Notes — Chapter 5: Process API

> These are my personal notes as I work through the coding homework for *Operating Systems: Three Easy Pieces* (OSTEP). I dictate my understanding after solving the exercises and use an LLM to help organize and refine the explanations into clear, readable notes. Purely for my own learning and future revision.

---

## What is this homework about?

Chapter 5 gave us the theory of the Unix Process API: `fork()`, `exec()`, `wait()`, redirection, and pipes. This homework is the hands-on verification: **eight small C programs** that probe how the operating system actually behaves when processes duplicate, share descriptors, synchronize, overwrite their address spaces, and talk to each other.

Here is the master roadmap of what we are testing:

| File | Core Question / Concept | Key Takeaway |
|---|---|---|
| [`hw1.c`](hw1.c) | What happens to variables on `fork()`? | Memory is isolated; child gets a copy, writes do not affect parent. |
| [`hw2.c`](hw2.c) | What happens to open file descriptors on `fork()`? | Descriptors are shared via the same kernel open file table entry (shared offset). |
| [`hw3.c`](hw3.c) | Can child print before parent without `wait()`? | Yes! Using pipe or signal synchronization instead of `wait()`. |
| [`hw4.c`](hw4.c) | Exploring all 6 `exec()` variants (`l`, `v`, `p`, `e`, `P`). | They are libc wrappers around the single kernel system call `execve()`. |
| [`hw5.c`](hw5.c) | What does `wait()` return? What if child calls `wait()`? | Returns terminated child's PID. Child calling `wait()` fails with `ECHILD`. |
| [`hw6.c`](hw6.c) | When is `waitpid()` better than `wait()`? | Targets a specific PID, supports non-blocking polling (`WNOHANG`), and job control. |
| [`hw7.c`](hw7.c) | What happens if child closes `STDOUT_FILENO` and calls `printf()`? | `printf` buffers in user space, but underlying `write(1, ...)` fails with `EBADF`. |
| [`hw8.c`](hw8.c) | Connecting two children via `pipe()` and `dup2()`. | The core building block of shell pipelines (`cmd1 | cmd2`). |

---

## Question 1: Variable Isolation on `fork()` (`hw1.c`)

### The Experiment
We set a local integer variable `x = 100` in `main()` before calling `fork()`. Both parent and child then inspect `x`, increment it (`x += 1`), and inspect it again.

### What Happens?
```
Hello world (pid:59798)
hello i am parent of 59799 (pid:59798)
parent sees x = 100
parent modified x to 101
hello i am child (pid:59799)
child sees x = 100
child modified x to 101
```

### The Concept Behind It
- **Copy-on-Fork / Virtual Memory Isolation:**
  When `fork()` is called, the OS duplicates the parent's virtual address space for the child. The child gets an exact copy of the parent's stack, heap, and data segments at that instant. Therefore, the child sees `x = 100`.
- **Private Address Spaces:**
  When either process modifies `x`, it only alters memory in its own private address space. The parent's `x` becomes 101; the child's `x` becomes 101. Neither process can see or modify the other's variables.
- **Kernel Optimization (Copy-on-Write / COW):**
  Instead of wasting time copying all physical RAM pages immediately, modern OSes (macOS, Linux) mark the pages as read-only and share them. Only when a process actually attempts to write to a page does the OS allocate a new physical page for it. Fast and memory-efficient.

---

## Question 2: Shared File Descriptors (`hw2.c`)

### The Experiment
We open a file `output.txt` with `open()` before calling `fork()`. Both parent and child write messages to the same file descriptor concurrently.

### What Happens?
`output.txt` contains:
```
parent writing
child writing
```
(or `child writing` followed by `parent writing` depending on CPU scheduling).

### The Concept Behind It
- **Shared Kernel Open File Description:**
  In Unix, file descriptors inside a process table are simply indices into an OS-wide **Open File Table**. When `fork()` runs, the child gets a copy of the parent's descriptor table, but both descriptors point to the **same open file table entry**.
- **Shared File Offset:**
  Because the open file table entry contains the current file offset pointer, the offset is **shared**. When the parent writes 15 bytes, the offset advances by 15 bytes. When the child writes, its text is appended right after the parent's text—no characters are overwritten!
- **Race Condition on Write Order:**
  While the file content is not overwritten, which message appears first is non-deterministic. It depends on whether the CPU scheduler runs the parent or child first.

---

## Question 3: Child Prints First Without `wait()` (`hw3.c`)

### The Problem
Child prints `"hello"`, parent prints `"goodbye"`. We must guarantee that `"hello"` always prints first—**without calling `wait()` or `waitpid()` in the parent**.

### How We Solved It
We use an **anonymous pipe** (`pipe()`) as a synchronization gate:

```
Child:  [print "hello"]  ──>  [close write-end of pipe]
                                       │ (EOF unblocks read)
                                       ▼
Parent: [block on read()] ──> [unblocks] ──> [print "goodbye"]
```

```c
int pfd[2];
pipe(pfd); // pfd[0] = read end, pfd[1] = write end

if (fork() == 0) {
    close(pfd[0]);
    printf("hello\n");
    close(pfd[1]); // signals the parent by closing the write end
} else {
    close(pfd[1]);
    char ch;
    read(pfd[0], &ch, 1); // BLOCKS until child closes write end or sends data
    close(pfd[0]);
    printf("goodbye\n");
}
```

### Why it Works
`read()` on an empty pipe is a blocking system call. The parent cannot proceed past `read()` until the child has executed `printf("hello\n")` and closed the pipe. This guarantees deterministic ordering without touching `wait()`.

*(Alternative methods include sending a `SIGUSR1` signal with `kill()` and catching it with `sigsuspend()`, or using a dirty `sleep(1)` hack which is not formally guaranteed).*

---

## Question 4: All 6 Variants of `exec()` (`hw4.c`)

### The Question
Run `/bin/ls` using all standard `exec()` variants: `execl()`, `execle()`, `execlp()`, `execv()`, `execvp()`, and BSD/macOS `execvP()`. Why are there so many?

### The Secret Decoder Ring

| Letter | Meaning | What it does |
|---|---|---|
| **`l`** | **List** | Arguments are passed as individual comma-separated strings ending in `NULL`. |
| **`v`** | **Vector** | Arguments are passed as an array of string pointers (`char *argv[]`). |
| **`p`** | **PATH** | Searches directories in the `$PATH` environment variable for the binary name. |
| **`e`** | **Environment** | Allows passing an explicit environment array (`char *envp[]`). |
| **`P`** | **Explicit Path** | (BSD/macOS) Takes a custom search-path string argument (e.g. `"/bin:/usr/bin"`). |

### Comparison Matrix

| Function | Arg Format | Binary Location | Environment |
|---|---|---|---|
| `execl("/bin/ls", "ls", "-l", NULL)` | List | Exact path | Inherited (`environ`) |
| `execle("/bin/ls", "ls", "-l", NULL, env)` | List | Exact path | Custom (`env`) |
| `execlp("ls", "ls", "-l", NULL)` | List | `$PATH` lookup | Inherited (`environ`) |
| `execv("/bin/ls", args)` | Vector | Exact path | Inherited (`environ`) |
| `execvp("ls", args)` | Vector | `$PATH` lookup | Inherited (`environ`) |
| `execvP("ls", "/bin:/usr/bin", args)` | Vector | Custom path string | Inherited (`environ`) |

### Why so many variants?
Because C doesn't support default parameters, method overloading, or polymorphic argument types. Programmers need different interfaces depending on how they have their arguments structured.

**Under the hood:** All of these functions are convenience wrappers in standard libc around a single true operating system system call: **`execve(2)`**.

---

## Question 5: `wait()` in Parent vs `wait()` in Child (`hw5.c`)

### The Experiment
Parent creates a child and calls `wait(&status)`. Inside the child, before doing anything else, the child also calls `wait()`.

### Output
```
Parent process (PID: 59835) waiting for child (PID: 59836)...
Child process (PID: 59836) started.
Child called wait(): returned -1
Child wait error: No child processes
Child process finished, exiting with status 42.
Parent wait() returned PID: 59836
Child exited normally with exit code: 42
```

### The Takeaway
1. **In the parent:** `wait()` suspends the parent until the child terminates, returning the child's PID (`59836`) and filling the status integer (decoded with `WIFEXITED` and `WEXITSTATUS`).
2. **In the child:** Since the child has spawned no children of its own, `wait()` immediately returns `-1` and sets `errno = ECHILD` ("No child processes"). It does not block.

---

## Question 6: `waitpid()` and Targeted Waiting (`hw6.c`)

### The Question
Modify the wait program to use `waitpid()`. When is `waitpid()` useful compared to plain `wait()`?

### Key Advantages of `waitpid()`
Plain `wait(&status)` is really just `waitpid(-1, &status, 0)`. `waitpid()` gives us four critical superpowers:

1. **Targeting a specific child PID:** If a server spawns 10 worker children, `wait()` returns whichever finishes first. `waitpid(worker_3_pid, &status, 0)` blocks specifically for Worker 3.
2. **Non-blocking check (`WNOHANG`):**
   `waitpid(pid, &status, WNOHANG)` returns `0` immediately if the child is still running, without putting the parent to sleep. Perfect for event loops and polling.
3. **Process groups:** Passing negative PID values allows waiting on any child within a specific process group.
4. **Shell Job Control:** Flags like `WUNTRACED` and `WCONTINUED` let interactive shells detect when background or foreground jobs are paused (Ctrl+Z / `SIGSTOP`) or resumed (`SIGCONT`).

---

## Question 7: Closing `STDOUT_FILENO` and Calling `printf()` (`hw7.c`)

### The Experiment
Child closes standard output (`close(STDOUT_FILENO)`) and calls `printf("Can you see this message?\n")`.

### Output
```
Child: printf returned 41, fflush returned -1
Child fflush/write error: Bad file descriptor
```

### Why did `printf` return 41 if nothing printed?
This demonstrates the distinction between the **C standard library** and the **OS kernel**:
1. `printf()` formats the string into a user-space memory buffer inside `libc`. Copying 41 characters into this memory buffer succeeds, so `printf()` returns 41.
2. When `fflush(stdout)` or newline flushing attempts to actually flush the buffer to the OS via `write(1, ...)`, the kernel checks file descriptor 1, finds it closed, and returns `-1` with `errno = EBADF` ("Bad file descriptor").
3. The message never appears on the screen.

---

## Question 8: Connecting Two Children with `pipe()` and `dup2()` (`hw8.c`)

### The Architecture
This program implements the exact plumbing used by the Unix command `cmd1 | cmd2`:

```
               ┌──────────┐
               │  Parent  │
               └────┬─────┘
                    │ creates pipe(pipefd)
         ┌──────────┴──────────┐
         ▼                     ▼
   ┌───────────┐         ┌───────────┐
   │  Child 1  │         │  Child 2  │
   │ (Writer)  │         │ (Reader)  │
   └─────┬─────┘         └─────▲─────┘
         │ stdout              │ stdin
         │ (dup2 pipefd[1])    │ (dup2 pipefd[0])
         ▼                     │
    ════════════════════════════════ kernel pipe buffer
```

### The Code Breakdown
1. **`pipe(pipefd)`:** Allocates a kernel pipe buffer. `pipefd[0]` is read end, `pipefd[1]` is write end.
2. **Child 1 (Writer):**
   ```c
   dup2(pipefd[1], STDOUT_FILENO); // stdout now points to the pipe's write end
   close(pipefd[0]);
   close(pipefd[1]);
   printf("Hello from Child 1 through the pipe!\n");
   ```
3. **Child 2 (Reader):**
   ```c
   dup2(pipefd[0], STDIN_FILENO);  // stdin now reads from the pipe's read end
   close(pipefd[0]);
   close(pipefd[1]);
   fgets(buffer, sizeof(buffer), stdin);
   ```
4. **Parent Process (The Trap!):**
   ```c
   close(pipefd[0]);
   close(pipefd[1]);
   waitpid(pid1, NULL, 0);
   waitpid(pid2, NULL, 0);
   ```

### ⚠️ The Critical Lesson: Closing Pipe Descriptors in the Parent
A reader reading from a pipe only receives **EOF (End-Of-File)** when **all** open write descriptors across **all** processes pointing to that pipe are closed.
If the parent forgets to close `pipefd[1]`, Child 2 will hang forever waiting for more input even after Child 1 has exited!

---

## Compiling and Running All Homework

To build and run any homework from this directory:

```sh
# Compile any exercise
clang -Wall -Wextra hw1.c -o hw1
./hw1

clang -Wall -Wextra hw2.c -o hw2
./hw2 && cat output.txt

clang -Wall -Wextra hw3.c -o hw3
./hw3

clang -Wall -Wextra hw4.c -o hw4
./hw4

clang -Wall -Wextra hw5.c -o hw5
./hw5

clang -Wall -Wextra hw6.c -o hw6
./hw6

clang -Wall -Wextra hw7.c -o hw7
./hw7

clang -Wall -Wextra hw8.c -o hw8
./hw8
```

---

*Done with Chapter 5 homework! Next up: Chapter 6 — Mechanism: Limited Direct Execution.*
