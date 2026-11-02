# Chapter 7 — Scheduling: Introduction

> These are my personal notes as I work through *Operating Systems: Three Easy Pieces* (OSTEP). I dictate my understanding after each reading session and use an LLM to help organize, explain, and polish the concepts. Purely for my own learning and future revision.

---

## The Big Question: Who Gets the CPU Next?

In Chapter 6, we mastered the low-level **machinery** of CPU virtualization:
- The OS can trap into kernel mode.
- The timer interrupt can wrench control back from a runaway user process.
- The OS can execute a **context switch** to swap one process for another.

Now we face the **policy** question:
> **Given a set of runnable jobs sitting in the ready queue, WHICH one should the OS run next?**

This problem is called **scheduling**.

Interestingly, scheduling didn't originate in computer science. Decades before computers existed, factory managers in **Operations Management** were trying to optimize how assembly lines processed orders. Computer scientists basically looked at manufacturing assembly lines, copied their homework, and applied it to CPU cores!

---

## Workload & The 5 "Absurd" Assumptions

To evaluate and design schedulers, we need to talk about the **workload** — the collection of processes running on the machine.

To build our intuition from scratch, we start by making **five blatantly unrealistic assumptions** about our workload:

```
               ┌────────────────────────────────────────────────────────┐
               │         The 5 "Toy World" Assumptions                  │
               ├────────────────────────────────────────────────────────┤
               │ 1. Each job runs for the exact same amount of time.    │
               │ 2. All jobs arrive at the exact same time.             │
               │ 3. Once started, each job runs to completion.          │
               │ 4. All jobs only use the CPU (no I/O operations).      │
               │ 5. The exact run-time of each job is known in advance. │
               └────────────────────────────────────────────────────────┘
```

*(Cue dramatic pause 🎭)*

Yes, these assumptions are completely detached from the real world. Real jobs have wildly different lengths, arrive unpredictably, make disk and network I/O calls, and the OS has zero psychic ability to know how long a program will run.

**The plan:** We start with these five assumptions, explore the simplest schedulers, and then **systematically relax each assumption one by one** until we arrive at a real, fully operational scheduling policy.

---

## Scheduling Metrics: How Do We Keep Score?

To compare scheduling algorithms objectively, we need **metrics**.

### Metric 1: Turnaround Time (A Performance Metric)

Turnaround time measures how quickly a job gets done from the moment it arrived in the system:

$$T_{\text{turnaround}} = T_{\text{completion}} - T_{\text{arrival}}$$

- If Job A arrives at $t = 0$ and finishes at $t = 20$, its turnaround time is $20 - 0 = 20$.
- **Average Turnaround Time** is the sum of turnaround times divided by the number of jobs.

### The Inherent Tension: Performance vs. Fairness
Turnaround time is purely a **performance metric**. It doesn't care whether individual processes were treated "fairly" or evenly—it only cares about finishing work as fast as possible.

As we'll soon see: **Performance and Fairness are almost always at war in operating systems.** Optimizing for turnaround time often means starving longer jobs; optimizing for fairness often ruins turnaround time. Life isn't always fair, and neither are schedulers.

---

## Policy 1: FIFO (First In, First Out)

The most natural, intuitive policy in human civilization: **First In, First Out (FIFO)**, also known as **First Come, First Served (FCFS)**. Whoever shows up first gets the CPU first.

### Scenario A: Under all 5 toy assumptions
Suppose three jobs, **A, B, and C**, arrive at $t = 0$. Each job runs for **10 seconds**.

```
Time:   0          10          20          30
        ┌───────────┬───────────┬───────────┐
CPU:    │     A     │     B     │     C     │
        └───────────┴───────────┴───────────┘
```

- A completes at $t = 10 \implies T_{\text{turnaround}} = 10 - 0 = 10$
- B completes at $t = 20 \implies T_{\text{turnaround}} = 20 - 0 = 20$
- C completes at $t = 30 \implies T_{\text{turnaround}} = 30 - 0 = 30$

$$\text{Average Turnaround Time} = \frac{10 + 20 + 30}{3} = \mathbf{20\text{ seconds}}$$

Works great! Simple, predictable, clean.

---

### Relaxing Assumption 1: Jobs have different run times
Now let's break Assumption 1: **jobs do not run for the same amount of time.**

Suppose **Job A runs for 100 seconds**, while **B and C only run for 10 seconds each**. All three arrive at $t = 0$.

Under FIFO, A got there first, so A runs first:

```
Time:   0                                                100    110    120
        ┌─────────────────────────────────────────────────┬──────┬──────┐
CPU:    │                        A                        │  B   │  C   │
        └─────────────────────────────────────────────────┴──────┴──────┘
```

- A completes at $100 \implies T_{\text{turnaround}} = 100$
- B completes at $110 \implies T_{\text{turnaround}} = 110$
- C completes at $120 \implies T_{\text{turnaround}} = 120$

$$\text{Average Turnaround Time} = \frac{100 + 110 + 120}{3} = \mathbf{110\text{ seconds}}$$

Ouch. Jobs B and C only needed 10 tiny seconds of CPU time, but were forced to wait 100 seconds just for A to finish!

### The Convoy Effect 🚛🚗🚗
This disaster is called the **Convoy Effect**:
> When a number of relatively short consumers of a resource get queued up behind one massive, heavyweight resource hog.

**Real-world analogy:** You run into a grocery store to buy a single pack of chewing gum (10-second job). You get in line at the register, only to find yourself stuck behind someone buying 4 shopping carts full of canned goods for a zombie apocalypse (100-second job). You stand there fuming for 20 minutes just to spend 5 seconds at the scanner. That is FIFO suffering from the convoy effect.

---

## Policy 2: Shortest Job First (SJF)

How would a smart human solve the grocery store problem?
*"Hey, you only have chewing gum? Go ahead of me!"*

That is **Shortest Job First (SJF)**:
> Run the shortest job first, then the next shortest, and so on.

Let's re-run our previous example (A = 100s, B = 10s, C = 10s, all arriving at $t = 0$) using SJF:

```
Time:   0     10     20                                                120
        ┌──────┬──────┬─────────────────────────────────────────────────┐
CPU:    │  B   │  C   │                        A                        │
        └──────┴──────┴─────────────────────────────────────────────────┘
```

- B completes at $10 \implies T_{\text{turnaround}} = 10 - 0 = 10$
- C completes at $20 \implies T_{\text{turnaround}} = 20 - 0 = 20$
- A completes at $120 \implies T_{\text{turnaround}} = 120 - 0 = 120$

$$\text{Average Turnaround Time} = \frac{10 + 20 + 120}{3} = \mathbf{50\text{ seconds}}$$

Average turnaround plummeted from **110s down to 50s**—more than a 2x speedup!

In fact, mathematically: **SJF is provably optimal** if all jobs arrive at the same time and run to completion. No other non-preemptive algorithm can achieve a lower average turnaround time.

---

### Relaxing Assumption 2: Jobs arrive at different times
Now let's break Assumption 2: **jobs arrive at different times.**

Suppose:
- **Job A** arrives at $t = 0$ and takes **100s**.
- **Jobs B and C** arrive shortly after, at $t = 10s$, and take **10s** each.

Because SJF is currently **non-preemptive** (Assumption 3: once started, a job runs to completion), when B and C show up at $t = 10s$, Job A is already on the CPU. The scheduler can't stop A!

```
Time:   0      10                                        100    110    120
               │ B, C arrive!
        ┌──────┴──────────────────────────────────────────┬──────┬──────┐
CPU:    │                        A                        │  B   │  C   │
        └─────────────────────────────────────────────────┴──────┴──────┘
```

- A completes at $100 \implies T_{\text{turnaround}} = 100 - 0 = 100$
- B completes at $110 \implies T_{\text{turnaround}} = 110 - 10 = 100$
- C completes at $120 \implies T_{\text{turnaround}} = 120 - 10 = 110$

$$\text{Average Turnaround Time} = \frac{100 + 100 + 110}{3} = \mathbf{103.33\text{ seconds}}$$

The convoy effect is right back! Even though B and C arrived almost immediately, they were held hostage for 90 seconds while A hogged the processor.

---

## Policy 3: Shortest Time-to-Completion First (STCF)

How do we fix this? We relax Assumption 3: **jobs do NOT have to run to completion!**

Thanks to Chapter 6, we have the hardware machinery to stop a running program: the **timer interrupt** and **context switch**.

When Jobs B and C arrive at $t = 10s$, the scheduler doesn't have to sit there twiddling its thumbs. It can **preempt** Job A, save A's registers, and run someone else!

This gives us **Shortest Time-to-Completion First (STCF)** (also known as **Preemptive Shortest Job First / PSJF**):
> Whenever a new job enters the system, the scheduler evaluates which of all remaining jobs (including the currently running one) has the shortest remaining time, and runs that one.

Let's re-run the scenario:
- At $t = 0$: Only A is present. A runs for 10s. Remaining time for A = **90s**.
- At $t = 10$: B and C arrive (10s each).
  - Scheduler compares remaining times: B (10s), C (10s), A (90s).
  - B has the shortest remaining time! Scheduler **preempts A** and runs B.
- At $t = 20$: B finishes. Scheduler chooses C (10s left vs A's 90s).
- At $t = 30$: C finishes. Only A is left (90s remaining). A resumes until completion at $t = 120$.

```
Time:   0      10     20     30                                        120
        ┌──────┬──────┬──────┬──────────────────────────────────────────┐
CPU:    │  A   │  B   │  C   │                    A                     │
        └──────┴──────┴──────┴──────────────────────────────────────────┘
```

- B completes at $20 \implies T_{\text{turnaround}} = 20 - 10 = 10$
- C completes at $30 \implies T_{\text{turnaround}} = 30 - 10 = 20$
- A completes at $120 \implies T_{\text{turnaround}} = 120 - 0 = 120$

$$\text{Average Turnaround Time} = \frac{10 + 20 + 120}{3} = \mathbf{50\text{ seconds}}$$

STCF crushes non-preemptive SJF (50s vs 103.33s).

In fact: **STCF is provably optimal.** Given our assumptions, no algorithm can produce a better average turnaround time when jobs arrive dynamically.

---

## Metric 2: Response Time (The Human Factor)

If STCF is provably optimal for turnaround time, why didn't operating system designers declare victory and go home in 1970?

Because in the 1970s, **interactive computing** arrived. People stopped submitting batch decks of punch cards and started typing at terminals!

Imagine sitting at a terminal, typing a command, and waiting **10 seconds** just to see a letter appear on screen because a long background compile job was occupying the CPU. You'd throw the terminal out the window.

Turnaround time only measures when a job *finishes*. It says nothing about how long a user had to wait to get the *first sign of life*.

This brings us to our second critical metric: **Response Time**.

$$T_{\text{response}} = T_{\text{first\_run}} - T_{\text{arrival}}$$

Under STCF:
- Job C arrived at $t = 10s$, but didn't even get scheduled for the first time until $t = 20s$.
- If you have 5 medium jobs ahead of you, you won't see your job run *at all* for a long time.
- STCF is fantastic for batch jobs; it is **terrible for interactive responsiveness**.

---

## Policy 4: Round Robin (RR) / Time Slicing

To make a computer feel interactive and responsive to humans, we need a scheduler that doesn't wait for jobs to finish before letting someone else have a turn.

Enter **Round Robin (RR)**:
> Instead of running jobs to completion, Round Robin runs a job for a short **time slice** (sometimes called a **scheduling quantum**), then switches to the next job in the ready queue. It cycles through jobs repeatedly until they are all done.

*(For this reason, Round Robin is also called **time slicing**.)*

> ⚠️ **Rule:** The length of the time slice must be an integer multiple of the timer interrupt period (e.g., if the timer ticks every 10ms, your time slice could be 10ms, 20ms, 50ms, etc.).

---

### Round Robin in Action: Response Time Heaven

Suppose Jobs **A, B, and C** arrive at $t = 0$, and each needs **5 seconds** of CPU time.

#### 1. Under SJF (Non-preemptive):
```
Time:   0          5          10         15
        ┌──────────┬──────────┬──────────┐
CPU:    │    A     │    B     │    C     │
        └──────────┴──────────┴──────────┘
```
- A first runs at $t = 0 \implies T_{\text{response}} = 0 - 0 = 0$
- B first runs at $t = 5 \implies T_{\text{response}} = 5 - 0 = 5$
- C first runs at $t = 10 \implies T_{\text{response}} = 10 - 0 = 10$

$$\text{Average Response Time (SJF)} = \frac{0 + 5 + 10}{3} = \mathbf{5\text{ seconds}}$$

#### 2. Under Round Robin (Time Slice = 1 second):
```
Time:   0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15
        ┌───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┐
CPU:    │ A │ B │ C │ A │ B │ C │ A │ B │ C │ A │ B │ C │ A │ B │ C │
        └───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┘
```
- A first runs at $t = 0 \implies T_{\text{response}} = 0 - 0 = 0$
- B first runs at $t = 1 \implies T_{\text{response}} = 1 - 0 = 1$
- C first runs at $t = 2 \implies T_{\text{response}} = 2 - 0 = 2$

$$\text{Average Response Time (RR)} = \frac{0 + 1 + 2}{3} = \mathbf{1\text{ second}}!$$

Round Robin crushed SJF on response time (**1s vs. 5s**). Every process got CPU attention almost immediately!

---

### The Time Slice Tuning Dilemma ⚖️

What should the length of the time slice be?

```
                 ┌───────────────────────────────────────────────┐
                 │       The Time Slice Engineering Trade-off    │
                 └───────────────────────┬───────────────────────┘
                                         │
                 ┌───────────────────────┴───────────────────────┐
                 ▼                                               ▼
         ⏱️ TOO SHORT (< 1 ms)                           ⏳ TOO LONG (> 500 ms)
   Context switch overhead dominates!              System feels sluggish and laggy.
   If time slice is 10 μs and context              Keystrokes stutter; interactive apps
   switch takes 1 μs, 10% of CPU is                lose that smooth, immediate feel.
   wasted just doing OS bookkeeping!               Degrades back toward FIFO.
```

**The Goal:** Make the time slice long enough to **amortize** the cost of context switching (so context switching accounts for less than 1% of CPU time), but short enough that users never perceive lag ($< 20$–$50\text{ ms}$).

---

### The Dark Side of Round Robin: Turnaround Time Hell 💀

Round Robin is the king of response time. But what happens to **Turnaround Time**?

Let's look back at our 1-second time slice example:
- Job A finishes at $t = 13 \implies T_{\text{turnaround}} = 13$
- Job B finishes at $t = 14 \implies T_{\text{turnaround}} = 14$
- Job C finishes at $t = 15 \implies T_{\text{turnaround}} = 15$

$$\text{Average Turnaround Time (RR)} = \frac{13 + 14 + 15}{3} = \mathbf{14\text{ seconds}}$$

Under simple FIFO or SJF:
- A finished at 5, B at 10, C at 15.
- Average Turnaround was $(5 + 10 + 15)/3 = \mathbf{10\text{ seconds}}$.

**Round Robin is nearly pessimal for turnaround time!** (14s vs 10s).

Why? Because Round Robin stretches every process out like chewing gum. Instead of knocking jobs out one by one, it keeps *all* jobs lingering until the very end.

---

## The Great Trade-Off: You Can't Have Your Cake and Eat It Too 🍰

We have now reached the fundamental philosophical fork in the road of scheduling:

| Scheduler Family | Primary Champion | Optimizes For | Sucks At | Strategy |
|---|---|---|---|---|
| **SJF / STCF** | Shortest Job First | **Turnaround Time** | Response Time | Unfair: Favors short jobs, finishes them fast |
| **Round Robin (RR)** | Time Slicing | **Response Time** | Turnaround Time | Fair: Shares CPU evenly across all jobs |

> **The Inherent Trade-off:**
> If you are willing to be **unfair**, you can run short jobs to completion and achieve optimal turnaround time.
> If you insist on being **fair**, you give everyone a slice, achieving great response time at the expense of terrible turnaround time.

---

## Relaxing Assumption 4: Incorporating I/O

Real programs don't just do CPU math—they perform **I/O** (disk reads, database queries, network sockets).

A program with no input produces the exact same output every run. A program with no output is like a tree falling in a deserted forest: nobody knows or cares. Programs need I/O.

When a running process issues an I/O request:
1. The process does not use the CPU while waiting; it transitions to the **BLOCKED** state.
2. The I/O request goes to the hard drive or network, which can take several milliseconds.
3. If the scheduler leaves the CPU idle during this wait, hardware utilization drops through the floor!

### Overlapping CPU and I/O
The solution: **treat each CPU burst between I/O requests as an independent sub-job.**

Suppose:
- **Job A:** Needs 50ms total CPU, but broken into five 10ms bursts with disk I/O in between.
- **Job B:** Needs 50ms of pure continuous CPU work (no I/O).

Under STCF:
```
Time (ms):  0         10        20        30        40        50
            ┌─────────┬─────────┬─────────┬─────────┬─────────┐
CPU:        │ A (CPU) │ B (CPU) │ A (CPU) │ B (CPU) │ A (CPU) │ ...
            └─────────┴─────────┴─────────┴─────────┴─────────┘
Disk:                 │ A (I/O) │         │ A (I/O) │         │
                      └─────────┘         └─────────┘
```

1. Job A starts running its first 10ms CPU burst.
2. At $t = 10\text{ ms}$, A issues a disk read and enters the `BLOCKED` state.
3. The scheduler immediately grabs **Job B** and runs it on the CPU while A's data is loading from disk!
4. At $t = 20\text{ ms}$, A's disk I/O completes. An interrupt fires. The OS marks A as `READY`.
5. Under STCF, A's next CPU burst (10ms) is shorter than B's remaining time (40ms), so the OS preempts B and resumes A!

**Result:** Both the CPU and the disk remain 100% busy simultaneously. Maximum overlap = maximum throughput!

---

## The Final Frontier: Relaxing Assumption 5

We have relaxed 4 out of our 5 original assumptions:
- [x] ~~Jobs run for the same time~~ $\implies$ Handled by SJF.
- [x] ~~All jobs arrive at the same time~~ $\implies$ Handled by dynamic scheduling.
- [x] ~~Jobs must run to completion~~ $\implies$ Handled by preemption (STCF).
- [x] ~~Jobs do not perform I/O~~ $\implies$ Handled by sub-job overlap.
- [ ] **The run-time of each job is known in advance.**

Assumption 5 is the final, toughest obstacle:
> **In a general-purpose operating system, the OS has NO idea how long a process is going to run!**

When you double-click Photoshop, run `gcc`, or launch a Python script, the OS does not possess a crystal ball. It cannot know whether the program will finish in 5 milliseconds or run for 3 weeks.

So how can we build a scheduler that mimics the optimal turnaround time of SJF/STCF without knowing the future?

**Answer:** We build a scheduler that looks at the **past** to predict the **future**.

This is the legendary **Multi-Level Feedback Queue (MLFQ)**—the subject of Chapter 8!

---

## Master Comparison of Early Schedulers

| Policy | Preemptive? | Optimizes For | Weakness | Best Used For |
|---|---|---|---|---|
| **FIFO / FCFS** | No | Simplicity | Severe Convoy Effect | Simple batch queues with equal-length jobs |
| **SJF** | No | Turnaround Time (optimal if simultaneous arrival) | Convoy effect if long job arrives first | Non-preemptive batch systems |
| **STCF / PSJF** | Yes | Turnaround Time (provably optimal) | Terrible Response Time; starves long jobs | Batch processing where turnaround matters most |
| **Round Robin (RR)** | Yes | Response Time (interactive fairness) | Terrible Turnaround Time for equal jobs | Interactive systems, desktop OS, terminal shells |

---

## Quick Self-Check Before Next Chapter

1. What is the mathematical definition of **Turnaround Time**? What about **Response Time**?
2. Explain the **Convoy Effect** using a real-world example. Which policy suffers from it?
3. Under what conditions is **SJF** provably optimal? Why does it fail when jobs arrive at different times?
4. How does **STCF** differ from SJF? What OS mechanism from Chapter 6 enables STCF?
5. Why is Round Robin nearly **pessimal** for turnaround time when all jobs have equal lengths?
6. If making the Round Robin time slice shorter improves response time, why can't we make it 10 nanoseconds?
7. Why is Assumption 5 (knowing job lengths beforehand) impossible in real operating systems?

---

*Next up: Chapter 8 — Multi-Level Feedback Queue (MLFQ). How to build an intelligent, self-learning scheduler that needs zero psychic powers!*
