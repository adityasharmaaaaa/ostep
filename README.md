# Operating Systems — My Notes

These are my personal notes while I work through *Operating Systems: Three Easy Pieces* (OSTEP). I am writing them to help me revise what I read and what I try on my Mac.

AI was used to help make some wording clearer and easier to revise. The notes are for my own learning, and I will keep updating them as I work through the book.

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

## Build and run

Go into the chapter directory and compile the program you want to try. Commands are in the chapter notes.

I keep the `.c` and `.h` files in Git. Compiled programs are local outputs and are ignored by `.gitignore`.
