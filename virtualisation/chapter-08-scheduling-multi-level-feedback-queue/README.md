# Chapter 8 — Scheduling: The Multi-Level Feedback Queue (MLFQ)

> These are my personal notes as I work through *Operating Systems: Three Easy Pieces* (OSTEP). I dictate my understanding after each reading session and use an LLM to help organize, explain, and polish the concepts. Purely for my own learning and future revision.

---

## The Holy Grail of Scheduling

In Chapter 7, we hit a massive brick wall. We discovered two scheduling ideals that seemed completely incompatible:

1. **Optimize Turnaround Time:** To minimize turnaround time, the scheduler must run the shortest jobs first (SJF / STCF). But SJF requires knowing *in advance* how long each job will run. The OS has no psychic powers; in a real computer, it has no idea whether a newly launched process will take 5 milliseconds or 5 days!
2. **Minimize Response Time:** To make computers snappy and interactive for humans staring at a monitor, the scheduler should use **Round Robin** (time-slicing). But Round Robin is nearly *pessimal* for turnaround time because it stretches all jobs out simultaneously.

So here is the multi-million-dollar question:
> **How can the OS design a scheduler that achieves low turnaround time (like SJF) WITHOUT knowing job lengths beforehand, while simultaneously providing low response time (like Round Robin) for interactive users?**

The answer is one of the most celebrated algorithms in computer systems: **The Multi-Level Feedback Queue (MLFQ)**.

First developed by Fernando Corbato in 1962 for the Compatible Time-Sharing System (CTSS)—work that contributed to his Turing Award—MLFQ solves this puzzle by using a simple, brilliant insight:

> **Don't guess the future. Learn from the past.**
> 
> As a program runs, the OS observes its behavior (feedback) and dynamically adjusts its priority.

---

## MLFQ: The Basic Setup & Rules 1 & 2

An MLFQ scheduler consists of a collection of **distinct queues**, each assigned a different **priority level**:

```
[High Priority]  Queue 2: ───> [ Job A ] ───> [ Job B ]
                    │
                    ▼
                 Queue 1: ───> [ Job C ]
                    │
                    ▼
[Low Priority]   Queue 0: ───> [ Job D ] ───> [ Job E ]
```

At any given moment, a runnable job sits on **exactly one queue**. MLFQ uses priority to decide who gets the CPU:

- **Rule 1:** If $\text{Priority}(A) > \text{Priority}(B)$, **$A$ runs** ($B$ does not).
- **Rule 2:** If $\text{Priority}(A) == \text{Priority}(B)$, **$A$ and $B$ run in Round Robin** using the time slice of that queue.

```
Example:
- In the diagram above, Job A and Job B are in Queue 2 (highest priority).
- Jobs C, D, and E will NOT run at all while A and B are runnable.
- A and B will share the CPU in Round Robin fashion.
- Only when both A and B are blocked or finished does Job C get a turn.
```

### The Magic Sauce: How Priorities Are Adjusted
The core intelligence of MLFQ isn't Rules 1 and 2—it's **how the scheduler moves jobs between queues over time**.

Instead of assigning a fixed priority to each job, MLFQ observes how a process behaves:
- **Interactive Process (Good citizen):** Frequently yields the CPU before its time slice expires (e.g. waiting for keyboard/mouse input, disk reads, or network packets). MLFQ keeps this job at **high priority** so that when user input arrives, the job runs immediately!
- **CPU-bound Batch Process (Number cruncher):** Hogs the CPU for long continuous stretches of time (e.g. video rendering, scientific simulation). MLFQ demotes this job to **low priority**.

---

## Attempt 1: How to Change Priority (Rules 3, 4A, 4B)

To decide how to adjust priorities, we introduce three new rules:

- **Rule 3:** When a job enters the system, it is placed at the **highest priority** (the topmost queue).
- **Rule 4A:** If a job uses up an entire time slice while running, its priority is **reduced** (it moves down one queue).
- **Rule 4B:** If a job gives up the CPU before the time slice is up, it **stays at the same priority level**.

Let's see how these rules behave across three classic workload scenarios.

---

### Example 1: A Single Long-Running Job

A long compute job arrives at $t = 0$. Each queue has a time slice of 10 ms.

```
Priority
   Q2  [   10 ms   ] ──> (Used entire slice! Demoted to Q1)
   Q1                    [   10 ms   ] ──> (Used entire slice! Demoted to Q0)
   Q0                                      [   10 ms   ][   10 ms   ] ...
       ─────────────────────────────────────────────────────────────────> Time
       0               10              20              30              40 ms
```

1. The job enters at $Q2$ (Rule 3).
2. It runs for 10 ms, consumes its full time slice, and is demoted to $Q1$ (Rule 4A).
3. It runs in $Q1$ for 10 ms, uses its full time slice again, and is demoted to $Q0$.
4. It remains in $Q0$ for the rest of its execution, running whenever higher queues are empty.

---

### Example 2: Along Came a Short Job 🏃💨

Now suppose Job A (a long-running job) is happily crunching away in $Q0$. At $t = 100\text{ ms}$, **Job B** (a short, 20-ms interactive job) arrives!

```
Priority
   Q2                                  [ B: 10ms ]
   Q1                                               [ B: 10ms (DONE!) ]
   Q0  [ A: 10ms ][ A: 10ms ] ...                                        [ A resumes ]
       ──────────────────────────────────────────────────────────────────────────────> Time
       0          10         ...       100          110                  120 ms
```

1. Job B enters at $Q2$ (Rule 3).
2. Because $\text{Priority}(B) > \text{Priority}(A)$, **Job B immediately preempts Job A** (Rule 1).
3. Job B runs for 10 ms at $Q2$, drops to $Q1$, runs for another 10 ms at $Q1$, and **finishes** at $t = 120\text{ ms}$!
4. With $Q2$ and $Q1$ now empty, the scheduler drops back to $Q0$, and Job A resumes.

> 💡 **The Core Philosophical Insight:**
> Because the OS doesn't know whether a new job is short or long, **it assumes it is short!**
> - If it truly is a short job, it finishes quickly at top priority.
> - If it is a long job, it slowly sinks down the queues, proving itself to be a batch process.
> 
> **In this way, MLFQ naturally approximates Shortest Job First (SJF) without knowing the future!**

---

### Example 3: What About I/O? (Interactive Jobs)

What if Job C is an interactive text editor? It runs for 1 ms to handle a keystroke, then calls `read()` to wait for the next key, voluntarily relinquishing the CPU.

```
Priority
   Q2  [C: 1ms]        [C: 1ms]        [C: 1ms]        (C yields before 10ms, stays in Q2!)
   Q1
   Q0          [ A... ]        [ A... ]        [ A... ] (Long job runs while C is waiting)
       ──────────────────────────────────────────────────────────────────────────────> Time
```

Under **Rule 4B**, because Job C gives up the CPU before its 10 ms time slice expires, it **remains at $Q2$**. Whenever the user presses a key, Job C is already at the highest priority queue, preempts Job A instantly, updates the screen in 1 ms, and goes back to sleep.

The user experiences **zero perceptible lag**!

---

## The Three Fatal Flaws of Attempt 1 ⚠️

Our basic MLFQ seems brilliant. But under real-world conditions, it has three catastrophic vulnerabilities:

```
                          ┌──────────────────────────────────────┐
                          │    The Three Flaws of Basic MLFQ     │
                          └──────────────────┬───────────────────┘
                                             │
             ┌───────────────────────────────┼───────────────────────────────┐
             ▼                               ▼                               ▼
       💀 STARVATION                  🦹 GAMING ATTACK               🔄 BEHAVIOR SHIFT
If too many interactive jobs     A sneaky process yields CPU     A batch process that becomes
exist, low-priority batch jobs   at 99% of its quantum to stay   interactive later is stuck in Q0
will NEVER run (starve to death). at top priority forever.       forever with terrible lag.
```

### Flaw 1: Starvation
If you have multiple interactive apps (browser, terminal, music player) constantly doing small bursts of work, the top queues will never stay empty. Batch jobs in $Q0$ will **starve to death**, receiving zero CPU cycles indefinitely.

### Flaw 2: Gaming the Scheduler (The 99% Exploit)
A devious programmer can easily trick Rule 4B:
```c
while (1) {
    // 1. Do CPU work for 9.9 ms (just under the 10 ms time slice)
    do_heavy_computation_for(9.9);

    // 2. Issue a dummy I/O operation to an unimportant file
    write(dummy_fd, "x", 1); 
    // -> Relinquish CPU! Rule 4B keeps the process in Q2 forever!
}
```
By giving up the CPU right before the time slice ends, the process stays in $Q2$ forever. It effectively steals **99% of all CPU time**, starving everyone else.

### Flaw 3: Change in Behavior Over Time
Imagine a program that spends its first 20 seconds doing heavy CPU computation (e.g. parsing a large database file into RAM), but then becomes an interactive GUI waiting for user clicks.
Under Attempt 1, that initial compute phase permanently banished the process down to $Q0$. When it finally becomes interactive, it is trapped in the basement queue, responding to user clicks with agonizing sluggishness!

---

## Attempt 2: Priority Boost (Rule 5)

How do we solve both **Starvation (Flaw 1)** and **Behavioral Shifts (Flaw 3)** in one elegant move?

We periodically hit the reset button!

- **Rule 5:** After some time period $S$, **move all jobs in the system to the topmost queue ($Q2$)**.

```
Priority
   Q2  ─── Boost! ───────────────────────────────────> All jobs moved to Q2!
         ▲                                              │
   Q1    │                                              ▼
   Q0  [ Starved Batch Job ] ─────────────────────────> Gets CPU time in Round Robin!
       ──────────────────────────────────────────────────────────────────────────────> Time
                                     Period S
```

### Why Rule 5 is brilliant:
1. **No process starves:** Even the longest, ugliest batch process will periodically find itself at $Q2$, sharing the CPU in Round Robin with interactive jobs.
2. **Behavioral shifts are forgiven:** If a batch job becomes interactive, the priority boost sends it back to $Q2$. If it now yields frequently, it will stay in the upper queues!

### The "Voodoo Constant" $S$ 🧙‍♂️
What should the boost period $S$ be set to?

John Ousterhout (creator of Tcl and renowned OS researcher) famously called parameters like this **voodoo constants**, because tuning them correctly seems to require black magic:
- If $S$ is set **too high** (e.g. every 10 minutes), batch processes starve for long stretches.
- If $S$ is set **too low** (e.g. every 10 milliseconds), interactive jobs don't get their fair share, and the system degrades into pure Round Robin.

In practice, OS designers tune $S$ through extensive experimentation (typically somewhere around 100 ms to a few seconds depending on workload).

---

## Attempt 3: Better Accounting (Defeating the 99% Gamer)

Now let's crush **Flaw 2 (Gaming the Scheduler)**.

The real flaw in Attempt 1 was that Rules 4A and 4B **forgot how much CPU time a process had already consumed** whenever it performed I/O. The process got a brand-new, clean time slice every time it yielded!

The fix is **accurate accounting**:
> Instead of measuring time slice *per continuous burst*, the scheduler tracks the **cumulative time allotment** spent at a given priority level.

We rewrite Rule 4:

- **Rule 4 (Revised):** Once a job uses up its **time allotment** at a given level (regardless of how many times it has given up the CPU), its priority is **reduced** (it moves down one queue).

```
Attempt 1 (Vulnerable to Gaming):
   Q2  [ 9.9ms ][I/O] [ 9.9ms ][I/O] [ 9.9ms ][I/O] ... (Stays in Q2 forever!)

Attempt 3 (Better Accounting):
   Q2  [ 9.9ms ][I/O] [ 0.1ms ] ──> Allotment (10ms) exhausted! Demoted to Q1!
   Q1                               [ 9.9ms ][I/O] [ 0.1ms ] ──> Demoted to Q0!
   Q0                                                            [ Runs here... ]
```

Now it doesn't matter if you run one continuous 10 ms block or take a hundred tiny 0.1 ms bites with fake I/O in between. Once your total CPU time at $Q2$ hits 10 ms, the scheduler says: *"Nice try, buddy. Down to $Q1$ you go."*

Gaming defeated.

---

## Tuning MLFQ & Real-World Implementations

Building an MLFQ in the real world requires making several practical design choices:

### 1. Varying Time Slices Across Queues
Should all queues have the same time slice length? **No!**
- **High-priority queues ($Q2$):** Interactive jobs want snappy, fast context switches. Give them **short time slices** (e.g. 10 ms).
- **Low-priority queues ($Q0$):** Batch jobs don't care about millisecond responsiveness; they want raw throughput. Give them **long time slices** (e.g. 100 ms to 200 ms) to amortize context-switch overhead and cache misses!

```
[High Priority]  Queue 2: Time Slice = 10 ms   (Short & snappy for interactive jobs)
                    │
                    ▼
                 Queue 1: Time Slice = 40 ms   (Medium)
                    │
                    ▼
[Low Priority]   Queue 0: Time Slice = 200 ms  (Long slices for heavy compute batch jobs)
```

### 2. Solaris Time-Sharing Table (`ts_dptbl`)
Solaris configured its MLFQ via a clean, user-configurable kernel dispatch table:
- It explicitly defined how many priority levels existed (typically 60 levels!).
- Specified the time slice length for each level (e.g. 20 ms at priority 59, 200 ms at priority 0).
- Explicitly defined where a process was demoted if it used its full quantum, and how often priority was boosted.

### 3. User-Directed Priority (`nice`)
Sometimes the user knows more about a program than the OS does. Most Unix systems provide the **`nice`** command (`nice -n 10 ./my_heavy_script.py`), allowing you to voluntarily lower your process's priority so it doesn't interfere with your interactive desktop experience.

---

## The Master Summary: All 5 Final Rules of MLFQ

Here is the complete, modern specification of the Multi-Level Feedback Queue:

```
┌────────────────────────────────────────────────────────────────────────────────────────┐
│                              THE 5 FINAL RULES OF MLFQ                                 │
├────────────────────────────────────────────────────────────────────────────────────────┤
│ Rule 1: If Priority(A) > Priority(B), A runs (B does not).                             │
│                                                                                        │
│ Rule 2: If Priority(A) == Priority(B), A and B run in Round Robin using the time        │
│         slice of that queue.                                                           │
│                                                                                        │
│ Rule 3: When a job enters the system, it is placed at the highest priority             │
│         (the topmost queue).                                                           │
│                                                                                        │
│ Rule 4: Once a job uses up its time allotment at a given level (regardless of          │
│         how many times it has given up the CPU), its priority is reduced               │
│         (it moves down one queue).                                                     │
│                                                                                        │
│ Rule 5: After some time period S, move all jobs in the system to the topmost queue.    │
└────────────────────────────────────────────────────────────────────────────────────────┘
```

---

## Why MLFQ is Legendary

MLFQ achieved what seemed impossible:
- **It approximates SJF/STCF:** Short jobs finish almost immediately at high priority.
- **It supports interactive users:** Interactive jobs naturally stay at high priority and enjoy low response times.
- **It prevents starvation:** The periodic priority boost ensures long batch jobs always make progress.
- **It is robust against exploits:** Time allotment tracking ensures devious users cannot game the system with fake I/O.
- **It requires ZERO advance knowledge of job runtimes.**

It is a masterclass in computer systems design: **simple heuristics combined with dynamic feedback to solve an apparently impossible problem.**

---

## Quick Self-Check Before Next Chapter

1. What two conflicting metrics does MLFQ attempt to optimize simultaneously?
2. Why does MLFQ place every new job at the highest priority queue? Which algorithm does this approximate?
3. How did a malicious program game Attempt 1 of MLFQ? How did Attempt 3 (time allotment) fix this?
4. What is a "voodoo constant"? Name the voodoo constant in MLFQ and explain the trade-off of setting it too high vs. too low.
5. Why is it beneficial to give high-priority queues shorter time slices and low-priority queues longer time slices?
6. What two distinct problems does Rule 5 (Priority Boost) solve at the same time?

---

*Next up: Chapter 9 — Scheduling: Proportional Share (Lottery & Stride Scheduling). Instead of optimizing turnaround or response time, how do we guarantee each process gets a guaranteed percentage of the CPU?*
