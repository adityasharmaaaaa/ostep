# Homework Notes — Chapter 7: Scheduling Simulator (`scheduler.py`)

> These are my personal notes as I work through the scheduling homework for *Operating Systems: Three Easy Pieces* (OSTEP). I dictate my understanding after testing each scenario in the simulator and use an LLM to organize, calculate, and format the explanations into clear, readable notes. Purely for my own learning and future revision.

---

## What is `scheduler.py`?

`scheduler.py` is an event-driven Python simulator provided by OSTEP. It models how different scheduling policies execute a workload of jobs and computes key metrics:
- **Response Time:** $T_{\text{first\_run}} - T_{\text{arrival}}$
- **Turnaround Time:** $T_{\text{completion}} - T_{\text{arrival}}$
- **Wait Time:** Total time a job spent waiting in the ready queue without running.

### Useful CLI Flags
- `-p <policy>`: Scheduling policy to use (`FIFO`, `SJF`, or `RR`).
- `-l <list>`: Comma-separated list of job lengths (e.g. `-l 100,200,300`).
- `-q <quantum>`: Time slice length for Round Robin (default: `1`).
- `-c`: Compute and print the full execution trace and statistics.

---

## Question 1: Equal-Length Jobs Under FIFO and SJF

### The Question
> Compute the response time and turnaround time when running three jobs of length 200 with the SJF and FIFO schedulers.

### Running the Simulator
```sh
python3 scheduler.py -p FIFO -l 200,200,200 -c
python3 scheduler.py -p SJF -l 200,200,200 -c
```

### Execution Trace & Output (Both Schedulers)
```
Execution trace:
  [ time   0 ] Run job 0 for 200.00 secs ( DONE at 200.00 )
  [ time 200 ] Run job 1 for 200.00 secs ( DONE at 400.00 )
  [ time 400 ] Run job 2 for 200.00 secs ( DONE at 600.00 )

Final statistics:
  Job   0 -- Response: 0.00    Turnaround: 200.00   Wait: 0.00
  Job   1 -- Response: 200.00  Turnaround: 400.00   Wait: 200.00
  Job   2 -- Response: 400.00  Turnaround: 600.00   Wait: 400.00

  Average -- Response: 200.00  Turnaround: 400.00   Wait: 200.00
```

### Analysis & Intuition
- **Response Times:**
  - Job 0 starts at $t = 0 \implies 0$
  - Job 1 waits for Job 0 $\implies 200$
  - Job 2 waits for Job 0 and Job 1 $\implies 400$
  - **Average Response Time:** $\frac{0 + 200 + 400}{3} = \mathbf{200.00}$
- **Turnaround Times:**
  - Job 0 finishes at $200 \implies 200$
  - Job 1 finishes at $400 \implies 400$
  - Job 2 finishes at $600 \implies 600$
  - **Average Turnaround Time:** $\frac{200 + 400 + 600}{3} = \mathbf{400.00}$

**Key Takeaway:** When all jobs have identical runtimes ($200, 200, 200$), **FIFO and SJF produce identical results**. Every job is tied for "shortest", so SJF executes them in arrival order, exactly matching FIFO.

---

## Question 2: Different-Length Jobs (100, 200, 300)

### The Question
> Now do the same but with jobs of different lengths: 100, 200, and 300.

### Scenario A: Jobs arrive sorted (100, 200, 300)
```sh
python3 scheduler.py -p FIFO -l 100,200,300 -c
python3 scheduler.py -p SJF -l 100,200,300 -c
```
**Output:**
- Job 0 (len 100): Response = 0, Turnaround = 100
- Job 1 (len 200): Response = 100, Turnaround = 300
- Job 2 (len 300): Response = 300, Turnaround = 600
- **Average Response Time:** $\frac{0 + 100 + 300}{3} = \mathbf{133.33}$
- **Average Turnaround Time:** $\frac{100 + 300 + 600}{3} = \mathbf{333.33}$

### Scenario B: Longest job arrives first under FIFO (300, 200, 100)
To see why SJF actually matters, what if the 300-second job arrived first in FIFO?
```sh
python3 scheduler.py -p FIFO -l 300,200,100 -c
```
**Output under FIFO:**
- Job 0 (len 300): Response = 0, Turnaround = 300
- Job 1 (len 200): Response = 300, Turnaround = 500
- Job 2 (len 100): Response = 500, Turnaround = 600
- **Average Response Time:** $\mathbf{266.67}$ (Double!)
- **Average Turnaround Time:** $\mathbf{466.67}$ (Severely degraded by the Convoy Effect!)

Under **SJF**, regardless of arrival order, the jobs are sorted before execution ($100 \rightarrow 200 \rightarrow 300$), guaranteeing optimal turnaround time of **333.33** every time.

---

## Question 3: Round Robin with Quantum = 1

### The Question
> Now do the same, but also with the RR scheduler and a time-slice of 1.

### 1. Running RR on equal jobs (`200, 200, 200`, $q = 1$)
```sh
python3 scheduler.py -p RR -l 200,200,200 -q 1 -c
```
**Results:**
- Job 0 starts at $t = 0 \implies \text{Response} = 0$, finishes at $t = 598 \implies \text{Turnaround} = 598$
- Job 1 starts at $t = 1 \implies \text{Response} = 1$, finishes at $t = 599 \implies \text{Turnaround} = 599$
- Job 2 starts at $t = 2 \implies \text{Response} = 2$, finishes at $t = 600 \implies \text{Turnaround} = 600$
- **Average Response Time:** $\frac{0 + 1 + 2}{3} = \mathbf{1.00}$ (Incredible improvement from 200.00!)
- **Average Turnaround Time:** $\frac{598 + 599 + 600}{3} = \mathbf{599.00}$ (Terrible degradation from 400.00!)

### 2. Running RR on variable jobs (`100, 200, 300`, $q = 1$)
```sh
python3 scheduler.py -p RR -l 100,200,300 -q 1 -c
```
**Results:**
- Job 0 (len 100) finishes at $t = 298 \implies \text{Turnaround} = 298$
- Job 1 (len 200) finishes at $t = 499 \implies \text{Turnaround} = 499$
- Job 2 (len 300) finishes at $t = 600 \implies \text{Turnaround} = 600$
- **Average Response Time:** $\frac{0 + 1 + 2}{3} = \mathbf{1.00}$
- **Average Turnaround Time:** $\frac{298 + 499 + 600}{3} = \mathbf{465.67}$

### Direct Comparison: SJF vs. Round Robin ($100, 200, 300$)

| Metric | SJF (or sorted FIFO) | Round Robin ($q = 1$) | The Winner |
|---|---|---|---|
| **Average Response Time** | 133.33 | **1.00** | 🏆 **Round Robin** (133x faster response!) |
| **Average Turnaround Time** | **333.33** | 465.67 | 🏆 **SJF** (Finishes jobs 28% faster on average) |

This demonstrates the core lesson of Chapter 7: **Round Robin delivers phenomenal response time at the cost of stretching turnaround time to near-pessimal levels.**

---

## Question 4: When Does SJF Match FIFO Turnaround Time?

### The Question
> For what types of workloads does SJF deliver the same turnaround times as FIFO?

### Answer
SJF and FIFO deliver the exact same turnaround times in two specific workload conditions:

1. **All jobs have the exact same length:**
   If every job in the queue requires $L$ units of CPU time (e.g. `10, 10, 10` or `200, 200, 200`), every job is tied for shortest. Therefore, SJF executes them in their arrival order, matching FIFO identically.
2. **Jobs arrive in non-decreasing order of their runtimes:**
   If the workload is already sorted by length (e.g. `Job 0: 5s, Job 1: 15s, Job 2: 50s`), FIFO naturally executes the shortest job first, matching SJF's reordering.
3. **Trivial case ($N = 1$):** If there is only one job in the system, all non-preemptive schedulers behave identically.

---

## Question 5: When Does SJF Match RR Response Time?

### The Question
> For what types of workloads and quantum lengths does SJF deliver the same response times as RR?

### Answer
SJF delivers the same response times as Round Robin when:

1. **The time slice $q$ is greater than or equal to the longest job in the workload ($q \ge \max(L_i)$), AND the jobs arrive sorted in shortest-to-longest order:**
   - When quantum $q$ is larger than any job's runtime, Round Robin never preempts a running job. Every job runs to completion on its first time slice.
   - When no preemption occurs, Round Robin degrades into FIFO.
   - If the jobs were already sorted from shortest to longest, that FIFO execution order matches SJF exactly!
2. **Trivial cases:**
   - When $N = 1$ job (both have response time = 0).
   - When all jobs have length 1 and $q = 1$.

---

## Question 6: SJF Response Time as Job Lengths Increase

### The Question
> What happens to response time with SJF as job lengths increase? Can you use the simulator to demonstrate the trend?

### Mathematical Derivation
Under non-preemptive SJF with $N$ equal jobs of length $L$ arriving at $t = 0$:
- Job 0 starts at $0 \implies \text{Response} = 0$
- Job 1 starts at $L \implies \text{Response} = L$
- Job 2 starts at $2L \implies \text{Response} = 2L$
- Job $i$ starts at $i \times L \implies \text{Response} = i \times L$

$$\text{Average Response Time} = \frac{1}{N} \sum_{i=0}^{N-1} i \cdot L = \frac{L}{N} \frac{(N - 1)N}{2} = \mathbf{\frac{(N - 1) \cdot L}{2}}$$

Notice that Average Response Time is **strictly linear with respect to job length $L$**!

### Simulator Demonstration ($N = 3$ jobs)

```sh
python3 scheduler.py -p SJF -l 10,10,10 -c     # Avg Response: 10.00
python3 scheduler.py -p SJF -l 100,100,100 -c   # Avg Response: 100.00
python3 scheduler.py -p SJF -l 1000,1000,1000 -c # Avg Response: 1000.00
```

| Job Length ($L$) | Job 0 Response | Job 1 Response | Job 2 Response | Average Response Time |
|---|---|---|---|---|
| **10** | 0.00 | 10.00 | 20.00 | **10.00** |
| **100** | 0.00 | 100.00 | 200.00 | **100.00** |
| **1,000** | 0.00 | 1,000.00 | 2,000.00 | **1,000.00** |

**Conclusion:** As job lengths scale up by $10\times$, the average response time under SJF scales up by exactly $10\times$. For long batch jobs, SJF response time is unacceptably high.

---

## Question 7: Round Robin Response Time vs. Quantum Length

### The Question
> What happens to response time with RR as quantum lengths increase? Can you write an equation that gives the worst-case response time, given N jobs?

### Mathematical Derivation
Suppose $N$ jobs arrive simultaneously at $t = 0$. The Round Robin scheduler allocates a time slice of length $q$ to each job in circular order:
- Job 0 is scheduled from $t = 0$ to $q \implies \text{Response} = 0$
- Job 1 is scheduled from $t = q$ to $2q \implies \text{Response} = q$
- Job 2 is scheduled from $t = 2q$ to $3q \implies \text{Response} = 2q$
- ...
- Job $i$ is scheduled at $t = i \cdot q \implies \text{Response} = i \cdot q$

The very last job in line (Job $N-1$) suffers the **worst-case response time**:

$$\mathbf{T_{\text{worst-case response}} = (N - 1) \times q}$$

*(assuming all jobs have runtime $\ge q$)*

And the **Average Response Time** across all $N$ jobs is:

$$\mathbf{T_{\text{average response}} = \frac{(N - 1) \times q}{2}}$$

### Simulator Verification ($N = 3$ jobs, length 100)

```sh
python3 scheduler.py -p RR -l 100,100,100 -q 1 -c
python3 scheduler.py -p RR -l 100,100,100 -q 5 -c
python3 scheduler.py -p RR -l 100,100,100 -q 20 -c
```

| Quantum ($q$) | Job 0 Response | Job 1 Response | Job 2 Response (Worst Case) | Average Response Time |
|---|---|---|---|---|
| **$q = 1$** | 0.00 | 1.00 | **2.00** (Formula: $(3-1)\times 1 = 2$) | **1.00** |
| **$q = 5$** | 0.00 | 5.00 | **10.00** (Formula: $(3-1)\times 5 = 10$) | **5.00** |
| **$q = 20$** | 0.00 | 20.00 | **40.00** (Formula: $(3-1)\times 20 = 40$) | **20.00** |

**Conclusion:** Both worst-case and average response times increase in **direct linear proportion to the quantum size $q$**. To achieve sub-millisecond response times, the quantum must be kept reasonably small.

---

## Master Summary Table

| Policy | Average Response Time ($N$ equal jobs) | Average Turnaround Time ($N$ equal jobs) | Key Advantage | Major Flaw |
|---|---|---|---|---|
| **FIFO** | $\frac{(N-1)L}{2}$ | $\frac{(N+1)L}{2}$ | Simple, zero scheduling overhead | Severe convoy effect if a long job is first |
| **SJF** | $\frac{(N-1)L}{2}$ | Optimal for batch jobs | Minimizes average turnaround time | Non-preemptive; long jobs delay short ones |
| **Round Robin** | $\frac{(N-1)q}{2}$ | $\approx N \cdot L$ (Pessimal!) | Excellent response time (interactive) | Terrible turnaround time |

---

*Done with Chapter 7 homework! Next up: Chapter 8 — Multi-Level Feedback Queue (MLFQ).*
