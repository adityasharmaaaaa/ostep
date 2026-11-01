# Operating Systems — My Notes

These are my personal notes as I work through *Operating Systems: Three Easy Pieces* (OSTEP). I dictate my understanding after each reading session and use an LLM to help clean up the wording and structure. The ideas, conclusions, and questions are mine — the AI just helps me not write like a sleep-deprived caveman.

Doing this purely for myself. No assignment, no deadline, no audience. Just trying to genuinely understand operating systems.

## What I am learning

### Virtualisation

The OS takes a physical resource and gives programs a simpler, more useful version of it. For example, the OS and hardware make it look like each process has its own memory, even though the machine has a limited amount of physical RAM.

For the CPU, the OS switches between runnable programs. The machine has a limited number of CPU cores, but each program gets a chance to run. The OS needs a policy to decide which program runs next.

### Concurrency

Concurrency means dealing with more than one thing during the same period of time. The OS itself does this all the time: it handles processes, switches between them, and responds to events. When multiple threads work with the same data, their steps can overlap in unexpected ways. That can cause bugs such as a race condition.

## Repository

### 📖 Introduction
- [`introduction-to-operating-systems/`](introduction-to-operating-systems/README.md) — What is an OS? The three big themes. Code experiments: CPU, memory, threads, I/O.
  - [`chapter-1-notes.md`](introduction-to-operating-systems/chapter-1-notes.md) — Revision notes: virtualisation, concurrency, persistence, design goals, history.

### 🖥️ Part I — Virtualisation
- [`virtualisation/`](virtualisation/README.md) — Overview of CPU and memory virtualisation (the peach analogy, chapter index).
  - [`chapter-04-abstraction-the-process/`](virtualisation/chapter-04-abstraction-the-process/README.md) — What is a process? Machine state, process API, process states, PCB, context switching.
    - [`homework/`](virtualisation/chapter-04-abstraction-the-process/homework/README.md) — `process-run.py` simulator: all 8 questions answered with output, explanation, and key lessons.
  - [`chapter-05-interlude:process-api/`](virtualisation/chapter-05-interlude:process-api/README.md) — `fork`, `exec`, `wait`. How the shell works. Output redirection and pipes explained.
    - [`homework/`](virtualisation/chapter-05-interlude:process-api/homework/README.md) — 8 practical programs on fork isolation, shared descriptors, synchronization without wait, exec variants, wait/waitpid, stdout closing, and pipe chaining.
  - [`chapter-06-mechanism-limited-direct-execution/`](virtualisation/chapter-06-mechanism-limited-direct-execution/README.md) — Limited Direct Execution. Dual CPU modes, system calls, trap table, the kernel stack, timer interrupts, context switching, and the baby-proofing analogy.

## Build and run

Go into the chapter directory and compile the program you want to try. Commands are in the chapter notes.

I keep the `.c` and `.h` files in Git. Compiled programs are local outputs and are ignored by `.gitignore`.
