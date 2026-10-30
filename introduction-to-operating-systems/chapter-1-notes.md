# Chapter 1 — Introduction to Operating Systems

> *These are my revision notes. Light reading, not a textbook. If this gets boring, I wrote it wrong.*

---

## So... what even is an Operating System?

An OS is the software that sits between you and the hardware. It takes all the physical stuff — CPU, RAM, disks — and makes them usable. Without it, you'd be writing assembly to talk to hardware directly. Nobody wants that. 

The fancy term the book uses is: **the OS virtualises the hardware**. It takes a physical resource and presents a simpler, more powerful version of it to programs. It also provides a bunch of **standard library** functions and **system calls** so programs can ask the OS to do things on their behalf (open a file, allocate memory, etc.).

Because many programs use these services, the OS is also sometimes called a **resource manager** — it manages CPU, memory, and disk so everyone gets a fair (ish) slice.

---

## The Three Big Ideas (the "Three Easy Pieces")

The whole book is organised around three themes. Spoiler: they're not that easy.

### 1. 🖥️ Virtualisation

The OS virtualises two main things:

**CPU** — You might have 4 cores, but you can run 50 programs. The OS rapidly switches between them so they *feel* like they're all running at the same time. This illusion is **virtualising the CPU**. The OS needs a **scheduling policy** to decide whose turn it is.

**Memory** — Each process thinks it has the whole memory to itself. It doesn't. The OS (with hardware help) gives each process its own **virtual address space**. When your program says `malloc()`, it gets a virtual address. The hardware translates that to a real physical address. Two programs can even have the same virtual address but different physical memory — completely isolated from each other.

> Think of it like apartments in a building. Everyone has their own "Room 101" but they're all different physical rooms. The building (OS) handles the mapping.

### 2. 🔀 Concurrency

Concurrency means multiple things are happening — or *appearing* to happen — at the same time. The OS juggles many processes. Threads inside a process share memory and can also overlap. This gets hairy fast.

The classic trap: `counter++` looks like one operation. It's actually three: read → add → write. If two threads do this at the same time, they can both read the same value, both add 1, and both write the same result. Net effect: you added 1, not 2. This is a **race condition**.

`volatile` does **not** fix this. You need a **mutex** or **atomic operation**. (More on this in the concurrency chapters — it gets a whole section of the book.)

### 3. 💾 Persistence

DRAM (your RAM) is **volatile** — it forgets everything when the power goes off. Run a program, write some data, pull the plug → gone. That's not great for anything you actually care about.

So we need **persistent storage** — HDDs, SSDs, the stuff that survives reboots. The OS software that manages this is called the **file system**.

#### What does the file system do?

The file system is responsible for storing files reliably and efficiently on disk. Simple to say, non-trivial to do. It has to:

1. **Figure out where to put data** on the disk
2. **Keep track of all the metadata** — what's stored where, what's the file's name, who owns it, etc.
3. **Issue I/O requests** to the storage device — either to read existing structures or to update them

And here's a fun detail: **most file systems delay writes**. Rather than rushing to write every tiny change immediately, they batch multiple writes and flush them together. This is way more efficient (disks hate random small writes). The trade-off? If the system crashes in between, you might lose some recent changes. That's why crash recovery and journaling exist — but that's a later chapter problem.

---

## Design Goals of the OS

Why is building an OS hard? Because it needs to be everything at once:

### 1. Abstraction 🧩

The OS hides the ugly hardware details behind clean, simple interfaces. You call `printf()`, not "set these 47 registers and send interrupt 0x13 to the disk controller." Abstraction makes the system convenient and programmable. It lets you write code in C (or Python) instead of assembly.

### 2. High Performance ⚡

Virtualising stuff isn't free — there's overhead. Every time the OS steps in, there's a cost. The goal is to **minimise these overheads** and not waste CPU cycles doing OS bookkeeping when your program could be running. Virtualisation is worth it, but not at *any* cost.

### 3. Protection 🛡️

Since many programs run at the same time, the OS has to make sure they don't mess with each other. A buggy (or malicious) program shouldn't be able to crash another program or — worse — crash the OS itself.

This is called **isolation**. The OS and hardware enforce it. It's also what separates user mode (where your programs run) from kernel mode (where the OS runs). Your program can't just reach into the kernel and do whatever it wants.

### 4. Reliability 🔋

The OS needs to **run non-stop**. It's the foundation everything else is built on. If the OS crashes, everything crashes with it. (You've seen the Windows blue screen of death. That's what failure looks like.) Reliability means the OS must handle errors gracefully and keep running even when individual programs fail.

### (Bonus) Energy Efficiency, Security, Mobility...

Modern OSes also care about not draining your battery, keeping bad actors out, and running well on phones. These weren't big concerns in the 1960s. Times change.

---

## A Brief History (Only the Good Parts)

*Because context makes things stick.*

### 🦕 The Early Days — Just a Library (1950s)

Early computers didn't really have an OS. Programs ran one at a time, directly on the hardware. The OS was basically a set of helper routines — a **library** of common functions so every programmer didn't have to reinvent I/O from scratch.

One program ran at a time. You'd walk up, load your punch cards, wait, collect your output, and leave. This was called **batch processing**.

### ⚡ Multiprogramming (1960s)

I/O is **slow**. Like, embarrassingly slow compared to the CPU. While one program waited for the disk, the CPU sat there doing nothing. This was wasteful.

The fix: **multiprogramming**. Load multiple jobs into memory and switch to another job when the current one is waiting for I/O. The CPU stays busy. Everyone wins.

This also introduced the need for **memory protection** — if multiple programs are in memory at the same time, you don't want them stepping on each other.

### 🖥️ Time-Sharing (1960s–70s)

Researchers wanted multiple users to interact with the machine at the same time — not just batch jobs. So they built **time-sharing** systems. The OS would rapidly switch between users, giving each one a small slice of CPU time.

The famous early system here is **UNIX** (1969, Bell Labs). Ken Thompson and Dennis Ritchie built it. Ritchie also invented the C programming language to write it. This is why we write system code in C — because the person who made the OS also made the language. Convenient.

UNIX introduced ideas that are still everywhere today: processes, the file system, pipes, the shell. Modern Linux and macOS are direct descendants.

### 💻 The PC Era (1980s)

Computers got cheap enough for regular people. Early home OSes (like MS-DOS) were simple — single user, single task. No protection. If your program crashed, the whole machine crashed.

Eventually, systems got proper memory protection, multitasking, and user interfaces. The modern OS as we know it started taking shape.

### 🌐 Today

Your phone, your laptop, your cloud server — they all run some descendant of these ideas. Linux runs most of the internet. macOS and iOS come from BSD UNIX. Android is Linux. It all traces back to those Bell Labs offices in 1969.

---

## The Code I Ran (Quick Summary)

The code experiments are in [`README.md`](README.md). Here's the one-line version:

| File | What it shows |
|------|---------------|
| `cpu.c` | OS switches between processes — many "run" on one CPU |
| `mem.c` | Each process has its own virtual address space |
| `threads.c` | Race condition: `counter++` is not atomic, results are wrong |
| `io.c` | File descriptor, `open()`/`write()`/`close()` system calls |

---

## Self-Check Before Next Chapter

Answer these in your own words (no peeking):

1. What does it mean to *virtualise* a resource? Give an example for CPU and one for memory.
2. Why does the OS delay some disk writes instead of writing immediately?
3. What are the four design goals of an OS? Which one do you think is hardest to achieve?
4. What was multiprogramming and why did it matter?
5. Who wrote UNIX and why does that matter for C?
6. What's the difference between user mode and kernel mode?
7. What is a race condition? Why doesn't `volatile` fix it?

If you can answer all seven, you're ready for Chapter 2. If not — re-read the section you blanked on.

---

*Next up: Virtualisation — Processes. Time to go deeper.*
