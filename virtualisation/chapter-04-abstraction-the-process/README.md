# Chapter 4 — Abstraction: The Process

> **Personal note:** These notes are my own understanding of the material, dictated and refined with the help of an LLM. The ideas, explanations, and understanding are mine — the AI just helped clean up the wording and structure. This is purely for my own learning as I work through *Operating Systems: Three Easy Pieces* (OSTEP). Not for any project, assignment, or publication.

---

## What is a Process?

Informally, dead simple: **a process is a running program.**

A program on disk is just bytes — dead, inert, doing nothing. The moment the OS picks it up and starts executing it, it becomes a **process**. Think of a program as a recipe and a process as actually cooking the dish. Same recipe, different processes can cook it at the same time without interfering.

The OS creates the illusion of many CPUs by running one process for a bit, pausing it, running another, and so on. This is **CPU virtualisation**. The mechanism that makes it work is called **time sharing** — giving each process a slice of CPU time, one at a time, cycling through so fast that it feels simultaneous to everyone.

> The cost? Each program runs a little slower because it's sharing. But the benefit — running dozens of programs at once — is absolutely worth it.

---

## Mechanism vs. Policy — Know the Difference

Before going deeper, there's a key distinction the OS world loves to make:

| | Question it answers | Example |
|--|--|--|
| **Mechanism** | *How?* | How does the OS switch from one process to another? (Context switching) |
| **Policy** | *Which?* | Which process should run next? (Scheduling policy) |

They're kept separate on purpose. That's **modularity** — you can swap out a scheduling policy without touching the switching mechanism. Smart design.

---

## Time Sharing vs. Space Sharing

The CPU is time-shared. But not everything works this way:

- **Time sharing**: The resource is used by one entity at a time, then passed around. (CPU → processes take turns)
- **Space sharing**: The resource is divided and each user gets their portion. (Disk → your files are in your blocks, not mine)

Both are tools the OS uses depending on what resource it's managing.

---

## What Makes Up a Process? — Machine State 🧠

To understand a process fully, you need to understand what it *consists of*. This is its **machine state** — everything the process can read or update while running.

### 1. Memory (Address Space)
- All the **instructions** of your program live in memory
- All the **data** your program works with lives in memory
- The chunk of memory that belongs to a process is called its **address space**
- Address space = the memory that is *part of* the process

### 2. Registers
The CPU has a small set of super-fast storage slots. Several are critical:

| Register | What it does |
|----------|-------------|
| **Program Counter (PC)** / Instruction Pointer | Points to the *next instruction* to execute — the bookmark in your program |
| **Stack Pointer** | Tracks the top of the stack (function calls, local variables) |
| **Frame Pointer** | Tracks the base of the current function's stack frame |

If you freeze a process mid-execution and later want to resume it, you need to save and restore all of these exactly. This is the essence of **context switching**.

### 3. I/O Information
A running process often has files open, network connections active, etc. This I/O state is also part of the process — specifically, the list of **file descriptors** it currently has open.

---

## Process API — What the OS Exposes

Every modern OS provides a set of operations you can perform on processes. The essentials:

| Operation | What it does |
|-----------|-------------|
| **Create** | Launch a new process (happens when you double-click an app or type a command in terminal) |
| **Destroy** | Forcefully kill a process (like `kill -9` in the terminal) |
| **Wait** | Pause and wait for a process to finish (parent waiting for child) |
| **Miscellaneous Control** | Suspend, resume, send signals |
| **Status** | Query a process — how long has it run? what state is it in? |

We'll dig into the actual system calls (`fork`, `exec`, `wait`) in the next chapter.

---

## How Does the OS Actually Start a Process?

When you launch a program, the OS does a surprisingly involved sequence of steps:

```
Program on disk
      │
      ▼
1. LOAD code + static data → memory (into the process's address space)
      │
      ▼
2. ALLOCATE stack memory
   (for local variables, function parameters, return addresses)
   Initialize it: pass argc and argv to main()
      │
      ▼
3. ALLOCATE heap memory
   (small at first; grows when program calls malloc())
      │
      ▼
4. Setup I/O
   (typically open stdin, stdout, stderr by default)
      │
      ▼
5. START executing at main()
   → Process is now running!
```

### Eager vs. Lazy Loading

- **Early OSes — Eager loading**: Load *everything* into memory before starting. Simple but wasteful.
- **Modern OSes — Lazy loading**: Load only the pieces of code/data actually needed, on demand. (This is how **paging** and **swapping** work — topics for later chapters.)

---

## Process States — Where Can a Process Be?

At any given moment, a process is in one of three states:

```
         ┌─────────────────────────────────────────┐
         │                                         │
    Scheduled                               Descheduled
         │                                         │
         ▼                                         │
   ┌──────────┐    I/O initiated    ┌───────────┐  │
   │ RUNNING  │ ─────────────────▶  │  BLOCKED  │  │
   └──────────┘                     └───────────┘  │
         ▲                                │        │
         │                    I/O done    │        │
         │                                ▼        │
         │                          ┌──────────┐   │
         └──────────────────────────│  READY   │───┘
                                    └──────────┘
```

| State | What it means |
|-------|--------------|
| **Running** | Actively executing instructions on the CPU right now |
| **Ready** | Could run, but the OS hasn't scheduled it yet |
| **Blocked** | Waiting for something (I/O, timer, event) — can't run even if CPU is free |

- Moving from **Ready → Running** = **Scheduled**
- Moving from **Running → Ready** = **Descheduled**
- Moving from **Running → Blocked** = process initiated I/O (e.g., disk read)
- Moving from **Blocked → Ready** = I/O completed, process can run again

---

## Worked Examples from the Book

### Example 1 — Two Processes, No I/O (Simple Case)

| Time | Process 0 | Process 1 | Notes |
|------|-----------|-----------|-------|
| 1    | Running   | Ready     | |
| 2    | Running   | Ready     | |
| 3    | Running   | Ready     | |
| 4    | Running   | Ready     | Process 0 finishes at end of time 4 |
| 5    | —         | Running   | OS switches to Process 1 |
| 6    | —         | Running   | |
| 7    | —         | Running   | |
| 8    | —         | Running   | Process 1 done |

No drama. Process 0 ran to completion, then Process 1 got its turn.

---

### Example 2 — Process Issues I/O (Interesting Case)

| Time | Process 0 | Process 1 | Notes |
|------|-----------|-----------|-------|
| 1    | Running   | Ready     | |
| 2    | Running   | Ready     | |
| 3    | Running   | Ready     | Process 0 issues I/O request |
| 4    | **Blocked**| Running  | OS sees P0 is blocked → schedules P1 |
| 5    | Blocked   | Running   | CPU stays busy! |
| 6    | Blocked   | Running   | I/O completes — P0 moves to Ready |
| 7    | **Ready** | Running   | OS decides to let P1 finish first |
| 8    | Running   | —         | P1 done, now P0 gets CPU |

Two things to notice:
1. **While P0 is blocked on I/O, the CPU isn't wasted** — the OS runs P1 instead. This is the whole point. Keep the CPU busy.
2. **When I/O completes, the OS doesn't immediately switch back to P0.** It decides to let P1 finish first. Was that the right call? Maybe, maybe not. That's what the **scheduler** figures out — topic for later.

---

## The OS Needs Data Structures Too

The OS is a program. Programs need data structures. So the OS keeps:

### Process List (PCB — Process Control Block)

The OS maintains a list of all processes. For each one, it stores a **Process Control Block (PCB)** — a data structure with everything about that process:

```
PCB for Process X
┌─────────────────────────────┐
│ Process ID (PID)            │
│ State: Running/Ready/Blocked│
│ Register context            │  ← saved CPU registers when not running
│   - Program Counter         │
│   - Stack Pointer           │
│   - etc.                    │
│ Memory info (address space) │
│ Open file descriptors       │
│ Parent process info         │
│ ...and more                 │
└─────────────────────────────┘
```

### Register Context — The Pause Button

When the OS **pauses a process**, it saves all its register values into memory (into the PCB). When it **resumes the process**, it loads those registers back. The process has no idea it was ever paused. This is the magic of **context switching**.

### One More State — Zombie 🧟

After a process finishes, it doesn't immediately disappear. It enters a **zombie** state — dead, but its exit info is preserved so the parent can read it. The parent calls `wait()` to collect the exit code, and *then* the OS cleans up the PCB. 

This matters because: if the parent never calls `wait()`, the zombie hangs around wasting resources. (These are called zombie processes and they're as annoying as they sound.)

---

## Key Takeaways

- A **process** = a running program. Program is the recipe; process is the cooking.
- The OS uses **time sharing** (cycling the CPU between processes) to virtualise the CPU.
- A process's **machine state** = its address space + register values + open file descriptors.
- **Mechanism** answers *how* (context switching). **Policy** answers *which* (scheduling).
- Process states: **Running → Blocked** (on I/O) → **Ready** (I/O done) → **Running** again.
- The OS tracks all this via the **process list** / **PCB**.
- The OS keeps the CPU busy during I/O by running another process — key to performance.

---

## Self-Check Before Next Chapter

1. What's the difference between a *program* and a *process*?
2. What does the machine state of a process consist of?
3. What is the Program Counter and why does it matter for context switching?
4. Name the three process states and draw the transitions between them.
5. What's the difference between a mechanism and a policy? Give one example of each.
6. What is a zombie process? Why does it happen?
7. Why does the OS run another process while the first is doing I/O? What's the benefit?

---

*Next: Chapter 5 — Process API. We'll get our hands dirty with `fork()`, `exec()`, and `wait()`.*
