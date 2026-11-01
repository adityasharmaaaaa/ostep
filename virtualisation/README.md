# Part I — Virtualisation

> **Personal note:** These notes are my own understanding of the material, dictated and refined with the help of an LLM. The ideas, explanations, and understanding are mine — the AI just helped clean up the wording and structure. This is purely for my own learning as I work through *Operating Systems: Three Easy Pieces* (OSTEP). Not for any project, assignment, or publication.

---

## What is Virtualisation? — The Peach Analogy 🍑

Imagine you have **one peach**. But five people want to eat it. What do you do?

You could:
- Give the whole peach to one person (unfair)
- Cut it into five slices and give each person their slice simultaneously (space sharing)
- Let each person take a bite, then pass it to the next person (time sharing)

The OS does exactly this — but with physical hardware (CPU, memory, disk) instead of fruit.

**The peach is the CPU.** There's only one (or a few), but dozens of programs want to run. The OS takes turns giving each program a bite of the CPU — run it for a bit, pause it, run the next one, repeat. So fast that everyone *thinks* they have the whole peach to themselves. That illusion? That's **virtualisation**.

The beautiful trick is: each program doesn't know it's sharing. It thinks it has an entire CPU to itself, its own private chunk of memory, its own everything. The OS is behind the curtain, juggling furiously, making the magic happen.

> This is the OS's main superpower: **take one physical thing, make it appear as many virtual things**, each private and isolated.

---

## The Two Flavours of Sharing

| Type | What it means | Example |
|------|--------------|---------|
| **Time sharing** | Everyone gets the resource for a short time, then it moves to the next | CPU — programs take turns |
| **Space sharing** | The resource is physically divided and each user gets their piece | Disk — your files don't overwrite mine |

The CPU is time-shared. Memory is (mostly) space-shared (each process gets its own chunk of RAM). Disk is space-shared too.

---

## What's Covered in This Section

The virtualisation section of OSTEP breaks into two big parts:

### 🖥️ CPU Virtualisation
How the OS makes many programs feel like they each have a dedicated CPU.

- [Chapter 4 — Abstraction: The Process](chapter-04-abstraction-the-process/README.md)
- [Chapter 5 — Process API (`fork`, `exec`, `wait`)](chapter-05-interlude:process-api/README.md)
- [Chapter 6 — Mechanism: Limited Direct Execution](chapter-06-mechanism-limited-direct-execution/README.md)
- Chapter 7 — CPU Scheduling: Introduction
- Chapter 8 — Multi-Level Feedback Queue (MLFQ)
- Chapter 9 — Lottery Scheduling
- Chapter 10 — Multi-CPU Scheduling

### 🧠 Memory Virtualisation
How the OS makes each process feel like it owns all of memory.

- Chapter 13 — The Abstraction: Address Spaces
- Chapter 14 — Memory API (`malloc`, `free`)
- Chapter 15 — Mechanism: Address Translation
- Chapter 16 — Segmentation
- Chapter 17 — Free-Space Management
- Chapter 18-20 — Paging (Introduction, TLBs, Advanced)
- Chapter 21-23 — Swapping (Mechanisms, Policies, Complete VM)

---

## The Golden Rule of Virtualisation

> **Make programs think they're alone, even when they're not.**

Every technique in this section is in service of that one idea.

---

*Let's start with the first abstraction the OS provides: the **Process**.*
