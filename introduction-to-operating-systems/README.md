# Introduction to Operating Systems — Revision Notes

These are my notes from today's reading and experiments. 

## Big ideas

### 1. Virtualising the CPU

A computer has a limited number of CPU cores. The OS can switch between programs so each gets some time to run. This makes it feel like many programs are running at once.

If several programs are ready to run, the OS needs a **scheduling policy** to decide which one runs next. Different policies make different trade-offs, such as fairness or response time.

**What I tried:** `cpu.c` runs forever. It calls `Spin(1)` and prints the string I pass on the command line. `Spin()` keeps checking the time, so it uses CPU time instead of sleeping.

```sh
clang -Wall -Wextra cpu.c common.c -o cpu
./cpu hello
```

Stop it with `Ctrl+C`. Try running two copies and watch their output mix together.

### 2. Virtualising memory

Each process gets its own **virtual address space**. The addresses used by a program are virtual addresses; the OS and hardware translate them to physical memory.

This gives processes a private view of memory. Two programs can use the same code but keep their own data separate.

**What I tried:** `mem.c` allocates an integer with `malloc()`, prints the process ID and the pointer address, then increments the integer forever.

```sh
clang -Wall -Wextra mem.c common.c -o mem
./mem
./mem &; ./mem &
```

I ran two copies and saw different process IDs and different pointer addresses. Each process had its own counter. The printed address is a virtual address, not a physical RAM address. The exact addresses can change between runs; macOS also randomises parts of a process's memory layout.

Remember:
- `malloc(sizeof(int))` asks for space for one integer.
- `p` holds the address; `*p` accesses the integer at that address.
- `getpid()` returns the process ID.
- Separate processes normally cannot change each other's private memory directly.

### 3. Concurrency

Concurrency means that multiple tasks make progress over the same period of time. The OS has to juggle many things. Threads inside one process can also overlap in their execution.

Threads in the same process share global variables and heap memory, but each thread has its own stack and local variables.

**What I tried:** `threads.c` starts two threads. Each runs `worker()` and increments the same global `counter` `loops` times.

```sh
clang -Wall -Wextra -pthread threads.c common.c -o threads
./threads 1000000
```

I expected the final value to be `2 * loops`, but the code does not guarantee that. `counter++` looks like one step, but it involves reading the value, adding one, and writing it back. The two threads can overlap and lose an update. This is a **race condition**.

`volatile` does not make an operation thread-safe. In standard C, unsynchronised concurrent access to this ordinary variable is a data race and has undefined behaviour. A mutex or an atomic counter is needed to fix it. A mutex makes the increment safe, but locking every time also has a cost.

### 4. File I/O

The OS gives a process a small integer called a **file descriptor** when it opens a file. The program uses that number to read or write the file.

**What I tried:** `io.c` opens `/tmp/file`, writes bytes, and closes it.

```sh
clang -Wall -Wextra io.c -o io
./io
od -An -t x1 /tmp/file
```

The flags mean:
- `O_WRONLY`: open for writing.
- `O_CREAT`: create the file if it does not exist.
- `O_TRUNC`: clear an existing file when opening it for writing.
- `S_IRWXU`: request read, write, and execute permissions for the owner when creating it. The `umask` can remove permissions.

One small detail I found: `"hello world\n"` has 12 bytes of text, including the newline. The C string also has a terminating `\0` byte in memory. If `write()` is asked to write 13 bytes, it writes that null byte too. Use 12 if I only want the text and newline.

`write()` takes a byte count; it does not automatically stop at the string's null terminator. System calls can fail, so real programs should check their return values instead of relying only on `assert()`.

### 5. The helper files

- `common.h` declares functions used by more than one program.
- `common.c` contains their implementations.
- `Spin(seconds)` busy-waits for the requested time. It is useful for this experiment, not a good way to wait in normal applications.
- `Pthread_create()` and `Pthread_join()` wrap the POSIX thread calls and stop the program if a call fails.

## Commands I want to remember

Run these from this directory:

```sh
clang -Wall -Wextra cpu.c common.c -o cpu
clang -Wall -Wextra mem.c common.c -o mem
clang -Wall -Wextra -pthread threads.c common.c -o threads
clang -Wall -Wextra io.c -o io
```

## Quick self-test before the next session

1. What does it mean to virtualise the CPU?
2. What is the difference between a virtual address and a physical address?
3. Why do two `mem` processes have separate counters?
4. Why can two threads lose an update to `counter`?
5. Why does `volatile` not fix that race?
6. What is a file descriptor?
7. Why does writing 13 bytes from `"hello world\n"` also write `\0`?

If I can answer these in my own words, I have the main ideas from today's session.
