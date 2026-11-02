# Homework Notes — Chapter 6: Measurement (Limited Direct Execution)

> These are my personal notes as I work through the measurement homework for *Operating Systems: Three Easy Pieces* (OSTEP). I dictate my understanding after writing and running the code on my machine, and use an LLM to organize, explain, and format the notes. Purely for my own learning and future revision.

---

## What is this homework about?

In Chapter 6, we learned the theoretical mechanics of **Limited Direct Execution (LDE)**:
- How dual CPU modes (User vs. Kernel mode) protect the hardware.
- How user processes use the hardware `trap` instruction to perform system calls.
- How the timer interrupt and OS scheduler perform **context switches** to time-share the CPU.

This homework is the practical counterpart: **measuring the real-world cost of these low-level OS operations in nanoseconds.**

We wrote three benchmark programs:
1. [`timer_precision.c`](timer_precision.c) — Measures the resolution of the system timer.
2. [`syscall_benchmark.c`](syscall_benchmark.c) — Measures the exact cost of a system call trap into the kernel.
3. [`context_switch.c`](context_switch.c) — Measures the cost of a full process context switch using the `lmbench` pipe ping-pong method.

---

## Benchmark Results at a Glance

All benchmarks were compiled with `clang -Wall -Wextra` and executed natively on an **Apple Silicon MacBook Air (M1, arm64, macOS Darwin)**:

| Benchmark | What it Measures | Iterations | Total Time | Average Cost |
|---|---|---|---|---|
| [`timer_precision.c`](timer_precision.c) | Timer Resolution (`gettimeofday`) | 100,000 | ~0.005 s | **1 μs resolution** (99.3% measured 0 μs) |
| [`syscall_benchmark.c`](syscall_benchmark.c) | Null System Call (`read` 0 bytes) | 10,000,000 | 3.42 s | **~341.74 ns** (~0.34 μs) |
| [`context_switch.c`](context_switch.c) | Process Context Switch (2 pipes) | 1,000,000 | 3.44 s | **~1,718.99 ns** (~1.72 μs) |

> 💡 **The Big Insight:** A context switch (~1,719 ns) is roughly **5 times more expensive** than a simple system call (~342 ns). This quantitatively proves why context switches are heavy operations that the OS scheduler tries to minimize!

---

## 1. Timer Precision (`timer_precision.c`)

### Why are we doing this?
Before measuring operations that take hundreds of nanoseconds, you need to know if your stopwatch is precise enough. If your stopwatch only ticks in whole seconds, you can't time a 100-meter sprint.

In C, older Unix code frequently uses `gettimeofday()`. This program calls `gettimeofday()` back-to-back in a loop 100,000 times to see the smallest non-zero time difference it can detect.

### Terminal Output
```
Minimum positive difference: 1 us
Zero-difference measurements: 99330 / 100000
```

### What does this mean?
- **Microsecond granularity:** `gettimeofday()` stores time in seconds and microseconds (`tv_sec`, `tv_usec`). Its smallest measurable step is 1 microsecond (1,000 ns).
- **99.3% of measurements were 0:** Modern CPU cores execute multiple instructions per nanosecond. Calling two functions back-to-back takes only a few dozen CPU clock cycles—far faster than 1 microsecond. Therefore, 99,330 times out of 100,000, `gettimeofday()` returned the exact same timestamp!
- **Key Takeaway:** For sub-microsecond measurements (like system calls), `gettimeofday()` is too coarse. We must use **`clock_gettime(CLOCK_MONOTONIC_RAW)`**, which queries the hardware tick counter with nanosecond precision and is immune to NTP network time adjustments.

---

## 2. Measuring System Call Cost (`syscall_benchmark.c`)

### How do we measure a system call?
If you measure `read()` from a real file on disk, you aren't measuring system call overhead—you're measuring the speed of your SSD flash memory.

To measure *pure* OS privilege-transition overhead, we need a **null system call** that does as little actual work as humanly possible:
```c
read(fd, &buffer, 0); // Read 0 bytes from /dev/null
```
- `read()` is a genuine system call.
- `/dev/null` is a kernel virtual device driver (no disk access, no file system lookups).
- Asking to read `0` bytes forces the kernel to validate the descriptor, check arguments, transition privileges, and return `0` immediately.

### The Warmup Loop
Notice this block before the timer starts:
```c
// Warmup
for (int i = 0; i < 10000; i++) {
    read(fd, &buffer, 0);
}
```
Why warm up? The first few executions of any code encounter cold-start penalties: CPU Instruction Cache (I-cache) misses, Data Cache (D-cache) misses, Translation Lookaside Buffer (TLB) misses, and untrained branch predictors. Running 10,000 throwaway iterations primes the CPU caches so our timing measures steady-state performance.

### Terminal Output
```
Iterations: 10000000
Total time: 3417405500 ns
Average system call time: 341.74 ns
```

### What happened during those 341.74 nanoseconds?
Every single iteration did the entire Limited Direct Execution dance:
1. `libc` loaded the syscall number for `read` into register `%rax` / `%x0`.
2. The CPU executed the hardware `trap` instruction (`syscall` or `svc`).
3. Hardware elevated the CPU from **User Mode (Ring 3)** to **Kernel Mode (Ring 0)**.
4. Hardware swapped the stack pointer to the process's **Kernel Stack** and pushed user registers.
5. CPU jumped to the Trap Table handler.
6. Kernel validated file descriptor `fd` and verified the memory buffer pointer.
7. Kernel invoked the `/dev/null` driver read routine, which returned 0.
8. Kernel executed `return-from-trap`: restored user registers from the kernel stack, dropped privilege back to **User Mode**, and resumed user code.

**All of that in ~0.34 microseconds!** Modern hardware and operating systems are absurdly fast.

---

## 3. Measuring Context Switch Cost (`context_switch.c`)

### The Challenge
A system call is easy to time because your program calls it directly. But a **context switch** happens inside the kernel scheduler when switching between *two separate processes*. How can a user-space program measure that?

### The Solution: The `lmbench` 2-Pipe Ping-Pong Method
We use the classic benchmark design invented by Larry McVoy in `lmbench`:

```
               ┌─────────────────────────────────────────┐
               │              Parent Process             │
               └──────────┬───────────────────▲──────────┘
                          │ writes 1 byte     │ reads 1 byte
                          ▼                   │
                     ═══════════════     ═══════════════
                         Pipe 1              Pipe 2
                     ═══════════════     ═══════════════
                          │                   ▲
                          ▼ reads 1 byte      │ writes 1 byte
               ┌──────────┴───────────────────┴──────────┐
               │              Child Process              │
               └─────────────────────────────────────────┘
```

1. We create **two pipes**: `pipe1` (Parent → Child) and `pipe2` (Child → Parent).
   - *Why two pipes instead of one?* If we used one pipe, the parent could write a byte and immediately read its own byte back before the OS ever had a chance to switch to the child! Two unidirectional pipes enforce a strict ping-pong dependency.
2. **The Ping-Pong Loop:**
   - Parent writes 1 byte to `pipe1`.
   - Parent immediately calls `read()` on `pipe2`. Because `pipe2` is empty, **the parent BLOCKS**.
   - The OS scheduler pauses the parent and **context-switches to the child**.
   - Child unblocks, reads the byte from `pipe1`, and writes 1 byte to `pipe2`.
   - Child immediately calls `read()` on `pipe1`. Because `pipe1` is empty, **the child BLOCKS**.
   - The OS scheduler pauses the child and **context-switches back to the parent**.
3. **The Math:**
   Each round trip requires **two context switches** (Parent → Child, Child → Parent).
   $$\text{Estimated Context Switch Cost} = \frac{\text{Total Time}}{2 \times \text{ITERATIONS}}$$

### The CPU Affinity Factor
On a multi-core machine (like an 8-core M1 Mac), if the OS places the Parent on Core 0 and the Child on Core 1, both processes could run simultaneously in parallel without forcing either core to swap CPU registers!
- On **Linux**, we use `sched_setaffinity()` to bind both processes to the exact same CPU core, guaranteeing genuine context switches on a single physical core.
- On **macOS (Darwin)**, Apple's XNU kernel does not expose `sched_setaffinity()` to user space because it dynamically manages asymmetric scheduling across Performance and Efficiency cores. The benchmark runs using the kernel's native scheduler.

### Terminal Output
```
Running on macOS (Darwin): sched_setaffinity not supported, using OS thread scheduler.
Iterations: 1000000
Total time: 3437983750 ns
Average round-trip time: 3437.98 ns
Estimated context-switch cost: 1718.99 ns
```

### What does this mean?
- **One Round-Trip:** ~3,437.98 ns (~3.44 μs).
- **One Context Switch:** **~1,718.99 ns (~1.72 μs)**.
- **Why is a context switch 5x slower than a system call?**
  A system call only switches privilege mode within the *same* process (using the same address space and memory mappings).
  A context switch must:
  1. Save all general-purpose registers of Process A.
  2. Save Process A's kernel stack pointer.
  3. Switch page tables / address space mappings (reloading `%cr3` on x86 or `TTBR0` on ARM).
  4. Flush or tag Translation Lookaside Buffer (TLB) entries.
  5. Restore Process B's kernel stack pointer and CPU registers.
  6. Suffer indirect cache misses because Process B's code and data are not yet in the L1 CPU cache!

---

## How to Build and Run

To run the benchmarks yourself from this directory:

```sh
# 1. Test timer precision
clang -Wall -Wextra timer_precision.c -o timer_precision
./timer_precision

# 2. Benchmark system call cost
clang -Wall -Wextra syscall_benchmark.c -o syscall_benchmark
./syscall_benchmark

# 3. Benchmark context switch cost
clang -Wall -Wextra context_switch.c -o context_switch
./context_switch
```

---

*Done with Chapter 6 homework! Next up: Chapter 7 — Scheduling: Introduction.*
