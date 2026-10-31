# Homework — Chapter 4: Abstraction: The Process

> **Personal note:** These are my personal notes, dictated from my own understanding and refined with the help of an LLM. The ideas, questions, answers and conclusions here are mine — AI only helped with wording and formatting. This is purely for my own learning as I work through *Operating Systems: Three Easy Pieces* (OSTEP). Not for any project, assignment, or publication.

---

## What is `process-run.py`?

`process-run.py` is a Python simulator that lets you play OS scheduler — without actually writing one. You describe a bunch of fake processes, tell it how to behave when I/O happens, and it shows you exactly what the CPU and I/O devices are doing at every clock tick.

It's the perfect sandbox to test your intuitions about scheduling: *"If process A does I/O, will process B run? When does the CPU sit idle? How does the order of processes affect total time?"*

### The flag cheat-sheet

| Flag | What it does |
|------|-------------|
| `-l X:Y` | Define a process: X instructions, Y% chance each is CPU (not I/O). Multiple processes separated by commas. |
| `-c` | **Compute** — actually run the simulation and show the trace |
| `-p` | **Print stats** — show CPU%, IO% at the end (only works with `-c`) |
| `-s N` | Random seed — gives you reproducible random processes |
| `-L N` | How many ticks an I/O takes (default: **5**) |
| `-S SWITCH_ON_IO` | Switch to another process as soon as the current one issues I/O |
| `-S SWITCH_ON_END` | Don't switch — wait until the current process is fully done |
| `-I IO_RUN_LATER` | When I/O finishes, put the process back in the ready queue (don't jump the line) |
| `-I IO_RUN_IMMEDIATE` | When I/O finishes, immediately preempt whatever is running and give CPU back to the process that just finished I/O |

### How to read the output

```
Time        PID: 0        PID: 1           CPU           IOs
  1        RUN:cpu         READY             1          
  2        BLOCKED       RUN:cpu             1             1
```

- **`RUN:cpu`** — this process is using the CPU to compute
- **`RUN:io`** — this process just *issued* an I/O request (1 tick)
- **`RUN:io_done`** — I/O finished, this process is handling completion (1 tick)
- **`BLOCKED`** — waiting for I/O to come back, not using CPU
- **`READY`** — could run, but scheduler hasn't picked it yet
- **`DONE`** — finished
- **`1`** in CPU column — CPU was busy that tick
- **`1`** in IOs column — one I/O was in flight that tick
- **`*`** after time number — an I/O completed this tick

### The hidden detail: I/O takes 5+1 ticks

When a process issues I/O (`RUN:io`), the I/O runs for **5 ticks** in the background (default `-L 5`). Then there's **1 more tick** to handle the completion (`RUN:io_done`). Total: **7 ticks** of wall time from issuing I/O to the process being ready again (1 to issue + 5 blocked + 1 to handle done).

---

## The Questions — Run by Run

---

### Q1 — `./process-run.py -l 5:100,5:100 -c -p`

**The setup:** Two processes. Each has 5 instructions. Both are 100% CPU (no I/O at all).

**What I predicted:** CPU should be 100% busy. Both processes only do compute, zero I/O, so there's nothing to wait for.

**What happened:**
```
Time  PID:0    PID:1     CPU
  1   RUN:cpu  READY      1
  2   RUN:cpu  READY      1
  3   RUN:cpu  READY      1
  4   RUN:cpu  READY      1
  5   RUN:cpu  READY      1
  6   DONE     RUN:cpu    1
  ...
 10   DONE     RUN:cpu    1

Stats: Total Time 10 | CPU 100.00% | IO 0.00%
```

**Why:** P0 runs its 5 instructions. P1 then runs its 5. No gaps, no waiting. Pure sequential CPU work = 100% utilisation. This is the easy case — two people taking turns, no idle time.

**Key lesson:** Pure CPU processes are trivial for the scheduler. Utilisation is 100% because there's never a moment where the CPU has nothing to do.

---

### Q2 — `./process-run.py -l 4:100,1:0 -c -p`

**The setup:** P0 has 4 CPU instructions. P1 has 1 instruction that is 0% CPU = it *only* does I/O.

**⚠️ Important note:** The `-l` flag needs a comma between processes: `-l 4:100,1:0`. Running `4:100 1:0` (space instead of comma) is wrong — the simulator silently ignores the second process. That's exactly what happened in the terminal output above — P1 never showed up! Always use a comma.

**What I predicted:** P0 runs its 4 CPU instructions. Then P1 runs, issues I/O, blocks for 5 ticks, comes back. Total time should be around 4 + 1 (issue) + 5 (wait) + 1 (done) = 11 ticks.

**What actually happened with correct flags:**
```
Time  PID:0     PID:1      CPU   IOs
  1   RUN:cpu   READY        1
  2   RUN:cpu   READY        1
  3   RUN:cpu   READY        1
  4   RUN:cpu   READY        1
  5   DONE      RUN:io       1
  6   DONE      BLOCKED            1
  7   DONE      BLOCKED            1
  8   DONE      BLOCKED            1
  9   DONE      BLOCKED            1
 10   DONE      BLOCKED            1
 11*  DONE      RUN:io_done  1
```
Total: 11 ticks. CPU busy: ~55%. IO busy: ~45%.

**Why:** P0 runs first (4 ticks). Then P1 issues I/O (1 tick). Then P1 blocks for 5 ticks — CPU is completely **idle** during this. Then P1 handles completion (1 tick). The CPU sat around doing nothing for 5 whole ticks. Wasteful!

**Key lesson:** Running CPU-heavy work first, then I/O-heavy work = bad. You end up with CPU idle during I/O wait. The order matters.

---

### Q3 — `./process-run.py -l 1:0,4:100 -c -p`

**The setup:** Same two processes — but now P0 is the I/O-issuing one, and P1 is the CPU one. Order flipped.

**What I predicted:** P0 issues I/O and blocks. While it's blocked, P1 runs its 4 CPU instructions. By the time P0's I/O is done, P1 might already be finished. Total time should be shorter.

**What happened:**
```
Time  PID:0       PID:1      CPU   IOs
  1   RUN:io      READY        1
  2   BLOCKED     RUN:cpu      1      1
  3   BLOCKED     RUN:cpu      1      1
  4   BLOCKED     RUN:cpu      1      1
  5   BLOCKED     RUN:cpu      1      1
  6   BLOCKED     DONE               1
  7*  RUN:io_done DONE         1

Stats: Total Time 7 | CPU 85.71% | IO 71.43%
```

**Why:** P0 issues I/O (1 tick), then blocks. The OS immediately switches to P1 (because `SWITCH_ON_IO` is the default). P1 runs all 4 of its CPU instructions during P0's I/O wait. When I/O finishes at tick 7, P1 is already done. P0 handles its `io_done` and exits. Total: **7 ticks vs 11 ticks** from Q2.

**Does order matter? Yes. Dramatically.**

| Order | Total Time | CPU Util |
|-------|-----------|---------|
| CPU first, then I/O (Q2) | 11 ticks | ~55% |
| I/O first, then CPU (Q3) | 7 ticks | 85.71% |

**Key lesson:** Do your I/O early. While the disk is churning, the CPU can be doing useful work. This is the whole idea behind why blocking I/O and a smart scheduler work together so well.

---

### Q4 — `-l 1:0,4:100 -c -S SWITCH_ON_END`

**The setup:** Same processes as Q3, but now with `SWITCH_ON_END` — the OS will NOT switch to another process while P0 is doing I/O. It stubbornly waits.

**What I predicted:** Bad. The OS waits for P0 to fully finish (including I/O) before touching P1. CPU sits idle during I/O.

**What happened:**
```
Time  PID:0       PID:1     CPU   IOs
  1   RUN:io      READY       1
  2   BLOCKED     READY              1
  3   BLOCKED     READY              1
  4   BLOCKED     READY              1
  5   BLOCKED     READY              1
  6   BLOCKED     READY              1
  7*  RUN:io_done READY       1
  8   DONE        RUN:cpu     1
  9   DONE        RUN:cpu     1
 10   DONE        RUN:cpu     1
 11   DONE        RUN:cpu     1

Stats: Total Time 11 | CPU 54.55% | IO 45.45%
```

**Why:** `SWITCH_ON_END` means: "don't touch P1 until P0 is completely done." So for ticks 2–6, P0 is BLOCKED, P1 is READY, and the CPU does... nothing. Five wasted ticks. Then P0 finishes, P1 finally gets to run.

**Key lesson:** `SWITCH_ON_END` is a terrible policy for I/O-heavy workloads. The CPU stares at the ceiling for 5 ticks while perfectly runnable work is just sitting there. Never use this unless you have a specific reason.

---

### Q5 — `-l 1:0,4:100 -c -S SWITCH_ON_IO`

**The setup:** Same, but now with `SWITCH_ON_IO` — the default, the sensible one.

**What happened:** Exactly the same as Q3. 7 ticks, 85.71% CPU.

The default behaviour (`SWITCH_ON_IO`) switches to P1 as soon as P0 issues I/O. P1 runs during P0's wait. No idle CPU.

**Key lesson:** `SWITCH_ON_IO` is the right call almost always. It's the OS being smart: "this process is going to wait a while, let me give the CPU to someone who can actually use it."

---

### Q6 — `-l 3:0,5:100,5:100,5:100 -S SWITCH_ON_IO -I IO_RUN_LATER -c -p`

**The setup:** P0 issues **3 I/Os** (one after another). P1, P2, P3 each have 5 CPU instructions. `IO_RUN_LATER` = when P0's I/O finishes, it doesn't jump the queue — it waits its turn behind whoever else is running.

**What happened:** Total 31 ticks. CPU 67.74%. IO 48.39%.

```
Ticks 1-6:   P0 issues first I/O, P1 runs all 5 CPU instructions
Ticks 7-11:  P0's I/O done, but IO_RUN_LATER → P0 waits. P2 runs.
Ticks 12-16: P3 runs.
Ticks 17-31: P0 finally gets the CPU. Issues I/O #2, waits. Issues I/O #3, waits.
             During these I/Os, everyone else is already DONE → CPU sits idle.
```

**Are resources being effectively utilised?** No. The problem is clear: by the time P0 gets to run its remaining I/Os (ticks 17+), P1/P2/P3 are all done. So during P0's 2nd and 3rd I/Os, the CPU goes idle again. `IO_RUN_LATER` caused P0's I/Os to be spread out with long gaps, instead of being overlapped with other processes' CPU work.

**Key lesson:** `IO_RUN_LATER` can hurt you if the I/O-issuing process has multiple I/Os. By the time it gets scheduled back, there's no one left to overlap with.

---

### Q7 — Same, but `-I IO_RUN_IMMEDIATE`

**What happened:** Total 21 ticks. CPU 100%. IO 71.43%.

```
Tick 1:     P0 issues I/O #1
Ticks 2-6:  P1 runs. P0's I/O completes at tick 7.
Tick 7:     IO_RUN_IMMEDIATE → P0 jumps in! Handles io_done, immediately issues I/O #2.
Ticks 8-13: P2 runs. P0's I/O #2 finishes at tick 14.
Tick 14:    P0 jumps in again. Issues I/O #3.
Ticks 15-20: P3 runs. P0's I/O #3 finishes at tick 21.
Tick 21:    P0 handles final io_done. Done.
```

CPU is 100% busy the entire time. The I/Os were perfectly pipelined with the CPU work of P1, P2, P3.

**Why is `IO_RUN_IMMEDIATE` better here?**

When P0 finishes an I/O, it probably has more I/Os to issue. If you let it run immediately, it can issue the next I/O right away, keeping the I/O device busy while the CPU goes back to another process. It's like a relay race — the moment the baton (I/O completion) arrives, you want the I/O-issuing process to hand off the next I/O to the disk immediately, not sit in a waiting room.

**Key lesson:** `IO_RUN_IMMEDIATE` is better when you have multiple I/O-heavy processes. It keeps both the CPU and I/O devices maximally busy. The overlap is what makes the system efficient.

---

### Q8 — Random processes: seeds 1, 2, 3 with `-l 3:50,3:50`

**The setup:** `3:50` means 3 instructions, 50% chance each is CPU (coin flip per instruction). The output depends on the seed.

**Seed 1 — process list:**
```
P0: cpu, io, io_done, io, io_done
P1: cpu, cpu, cpu
```

**Seed 2:**
```
P0: io, io_done, io, io_done, cpu
P1: cpu, io, io_done, io, io_done
```

**Seed 3:**
```
P0: cpu, io, io_done, cpu
P1: io, io_done, io, io_done, cpu
```

**What changes with `IO_RUN_IMMEDIATE` vs `IO_RUN_LATER`?**

- `IO_RUN_IMMEDIATE`: Process that just finished I/O jumps back in immediately, issues the next I/O right away. Keeps I/O device busier. Generally better total time when there are multiple I/Os.
- `IO_RUN_LATER`: Let whoever is running keep going. Simpler, but can leave I/O device idle unnecessarily.

**What changes with `SWITCH_ON_IO` vs `SWITCH_ON_END`?**

- `SWITCH_ON_IO`: As soon as a process issues I/O, give CPU to someone else. Smart. Keeps CPU busy.
- `SWITCH_ON_END`: Wait until the current process completely finishes, *then* switch. Dumb when there's I/O involved — CPU sits idle for the entire I/O duration.

**The general rule that emerges:**
> **Best combo: `SWITCH_ON_IO` + `IO_RUN_IMMEDIATE`**
> 
> Switch as soon as I/O is issued. Jump back as soon as I/O finishes. This keeps both the CPU and the I/O device as busy as possible simultaneously. It's the foundation of what makes modern OSes efficient with mixed workloads.

---

## The Big Picture — What I Actually Learned

All eight questions are really building toward one insight:

### Overlap is everything

A CPU and a disk can work at the same time. A smart OS exploits this. A dumb OS doesn't.

```
BAD (SWITCH_ON_END):
Time: [P0 CPU][P0 IO wait (idle CPU)][P0 done][P1 CPU][P1 CPU]...
              ↑ wasted

GOOD (SWITCH_ON_IO):
Time: [P0 IO issue][P0 IO (disk busy)][P0 done]
              [P1 CPU][P1 CPU][P1 CPU][P1 CPU]  ← running at the same time!
```

### Two decisions the scheduler makes (and why they matter)

| Decision | Options | Impact |
|----------|---------|--------|
| When to switch? | `SWITCH_ON_IO` vs `SWITCH_ON_END` | Whether CPU is idle during I/O |
| What to run after I/O completes? | `IO_RUN_IMMEDIATE` vs `IO_RUN_LATER` | How quickly the next I/O gets issued |

These two knobs, together, determine total job completion time and resource utilisation.

### Why does this matter in real life?

Everything you use runs mixed workloads. A web server reads files from disk (I/O) and processes requests (CPU). A browser downloads images (I/O) and renders the page (CPU). Your phone plays music (I/O) and runs the UI (CPU). The scheduler's ability to overlap these is why your computer feels fast even when it's doing many things at once.

---

## Commands Reference

```sh
# Run from: virtualisation/homework/

# Q1 — Pure CPU, two processes
python3 ./process-run.py -l 5:100,5:100 -c -p

# Q2 — CPU first, then I/O (note the comma!)
python3 ./process-run.py -l 4:100,1:0 -c -p

# Q3 — I/O first, then CPU (better!)
python3 ./process-run.py -l 1:0,4:100 -c -p

# Q4 — Dumb switch policy
python3 ./process-run.py -l 1:0,4:100 -S SWITCH_ON_END -c -p

# Q5 — Smart switch policy
python3 ./process-run.py -l 1:0,4:100 -S SWITCH_ON_IO -c -p

# Q6 — Multiple I/Os, late run (suboptimal)
python3 ./process-run.py -l 3:0,5:100,5:100,5:100 -S SWITCH_ON_IO -I IO_RUN_LATER -c -p

# Q7 — Multiple I/Os, immediate run (optimal)
python3 ./process-run.py -l 3:0,5:100,5:100,5:100 -S SWITCH_ON_IO -I IO_RUN_IMMEDIATE -c -p

# Q8 — Random workloads
python3 ./process-run.py -s 1 -l 3:50,3:50 -c -p
python3 ./process-run.py -s 2 -l 3:50,3:50 -c -p
python3 ./process-run.py -s 3 -l 3:50,3:50 -c -p
```

---

*Next: Chapter 5 — Process API. Time to use `fork()`, `exec()`, and `wait()` for real.*
