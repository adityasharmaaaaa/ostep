# Chapter 6 — Mechanism: Limited Direct Execution

> These are my personal notes as I work through *Operating Systems: Three Easy Pieces* (OSTEP). I dictate my understanding after reading each chapter and use an LLM to help organize, illustrate, and polish the explanations. Purely for my own learning and future revision.

---

## The Great CPU Dilemma: Performance vs. Control

In the previous chapters, we saw that the OS achieves the illusion of many virtual CPUs through **time-sharing** — running one process for a little while, stopping it, running another, and so on.

Sounds easy on paper. But when you actually try to build it, you run head-first into the two central engineering challenges of operating systems:

```
                  ┌─────────────────────────────────────────┐
                  │      The Central OS Trade-off           │
                  └────────────────────┬────────────────────┘
                                       │
                 ┌─────────────────────┴─────────────────────┐
                 ▼                                           ▼
         🚀 PERFORMANCE                              🛡️ CONTROL
   How do we virtualize the CPU                 How do we keep user processes
   WITHOUT adding massive, painful              from running forever, hogging
   overhead to every single instruction?        resources, or trashing the OS?
```

1. **Performance:** If the OS had to inspect, validate, or emulate every single machine instruction a program runs, your computer would crawl at 1% of its normal speed. We need programs to run as close to raw hardware speed as possible.
2. **Control:** But if you give a program raw access to the machine, what stops it from writing directly to the hard disk, reading private data from other apps, or running in an infinite loop forever? Without control, the OS ceases to be an operating system — it's just a glorified library.

The solution invented by OS designers is called **Limited Direct Execution (LDE)**.

---

## What is "Direct Execution"?

The "Direct Execution" part of the name is hilariously straightforward:

> **Just run the program directly on the physical CPU.**

When the OS wants to start a process:
1. It creates an entry in the process list (PCB).
2. Allocates memory for the process (code, stack, heap).
3. Loads the program binary from disk into RAM.
4. Locates `main()` (the entry point).
5. Jumps to `main()` and lets the CPU rip.

```
       OS                         Hardware / CPU                    Program
  ────────────                   ────────────────                 ───────────
  Create PCB
  Allocate RAM
  Load binary from disk
  Jump to main() ───────────────> [Execute native instructions] ──> runs at 100% speed!
```

Fast? Yes. Pure native hardware speed. Zero emulation overhead.

**So why can't we stop here?**

Because two massive problems immediately pop up:

1. **Problem 1: Restricted Operations:** What if the program wants to read a file from disk, allocate physical memory, or talk to the network? If we let it do that directly, any buggy or malicious program could wipe the entire drive.
2. **Problem 2: Regaining Control:** What if the program enters an infinite loop (`while (1);`) and refuses to stop? How does the OS stop it and switch to another process if the program is hogging the CPU?

This is where the **"Limited"** part of Limited Direct Execution comes in.

---

## Problem 1: Restricted Operations & Dual CPU Modes

If we can't let user programs do whatever they want with hardware, how do we stop them without slowing them down?

The hardware and OS cooperate by introducing **Dual Processor Modes**:

| Mode | Who runs here? | Privilege Level | What is allowed? | What happens on violation? |
|---|---|---|---|---|
| **User Mode** | User applications (browsers, games, your C programs) | **Restricted (Ring 3)** | Normal math, logic, function calls, accessing its own memory. Cannot issue disk I/O, touch device registers, or alter page tables. | Hardware raises an **exception/fault**; OS promptly kills the process. |
| **Kernel Mode** | The Operating System kernel | **Privileged (Ring 0)** | Complete, unrestricted access to the entire machine, all hardware devices, and all CPU instructions. | Kernel can do anything. (If it crashes, the whole machine panics.) |

---

### The Gateway: System Calls and the `trap` Instruction

If a program running in User Mode is physically forbidden from accessing the disk, how does it ever open a file?

It must ask the OS nicely through a **System Call**.

To perform a system call, the hardware provides a special CPU instruction called **`trap`** (on x86: `sysenter`, `syscall`, or `int 0x80`; on ARM: `svc`):

```
User Mode                                         Kernel Mode
─────────                                         ───────────
User program runs...
Calls open("foo.txt")
   │
   ▼
[trap instruction] ─────────────────────────────> 1. Hardware raises privilege to Kernel Mode
                                                  2. Saves user registers to KERNEL STACK
                                                  3. Jumps to OS Trap Handler via Trap Table
                                                                  │
                                                                  ▼
                                                      OS inspects permissions,
                                                      performs I/O securely...
                                                                  │
                                                                  ▼
User program resumes <─────────────────────────── [return-from-trap instruction]
at next instruction                               1. Restores registers from KERNEL STACK
                                                  2. Hardware drops privilege to User Mode
```

1. **The `trap` instruction:** Simultaneously:
   - Raises the CPU privilege level from **User Mode** to **Kernel Mode**.
   - Saves the process's current register state onto the process's **Kernel Stack**.
   - Jumps into the kernel to execute the designated trap handler.
2. **The `return-from-trap` instruction:** When the OS finishes the request:
   - Pops the saved register state off the kernel stack.
   - Drops the privilege level back down to **User Mode**.
   - Jumps back to the calling user program right where it left off.

---

### Deep Dive: The Kernel Stack 🥞

*(Why does the OS need its own separate stack?)*

In user mode, every program already has its own stack for local variables and function calls. So why can't the CPU just push the trap register state onto the user's stack?

There are **two critical reasons**:

```
                       ┌──────────────────────────────────────┐
                       │  Why NOT use the User Stack for OS?  │
                       └──────────────────┬───────────────────┘
                                          │
             ┌────────────────────────────┴────────────────────────────┐
             ▼                                                         ▼
     🚨 SECURITY LEAK                                          💥 RELIABILITY COLLAPSE
If kernel return addresses, kernel pointers,              What if the user program corrupted its
or confidential register values are pushed onto           stack pointer (e.g. %rsp = 0x0 or invalid)?
the user stack, malicious user code could inspect         The CPU would fail to push trap data, trigger
or overwrite them before the kernel returns!              a double-fault, and crash the entire computer.
```

**The Solution:**
Every single process in the system is given **two stacks**:
1. **User Stack:** In user address space, used for ordinary function calls and local variables while running in user mode.
2. **Kernel Stack:** In secure kernel memory, accessible *only* when the CPU is in kernel mode.

When a trap or interrupt occurs, the hardware automatically switches the CPU's stack pointer (`%rsp` or `%sp`) to point to that process's **Kernel Stack**. The hardware then pushes the user's program counter, flags, and registers onto the kernel stack. When returning via `return-from-trap`, hardware pops these values back into the CPU registers, switching back to the user stack!

---

### Why does a System Call look like a normal C function?

When you write `open("file.txt", O_RDONLY)` or `read(fd, buf, size)`, it looks identical to calling any regular C function. How does that translate into a hardware `trap`?

**Because `open()` in C is just a wrapper function provided by the C standard library (`libc`).**

Behind the scenes:
1. `libc` places the arguments (`"file.txt"`, `O_RDONLY`) into specific hardware registers (or onto the stack) according to the system's calling convention.
2. It places the **System Call Number** (e.g. System Call #5 for `open`) into a designated register (such as `%eax` or `%rax` on x86).
3. It executes the assembly `trap` instruction.
4. When the kernel finishes and returns, the `libc` wrapper inspects the return register. If negative, it sets `errno` and returns `-1`.

```c
// Simplified assembly of what libc does when you call a syscall:
mov $5, %eax         // System call number for open()
mov %rdi, ...        // Argument 1: filename pointer
mov %rsi, ...        // Argument 2: flags
syscall              // Hardware trap into OS kernel!
// When return-from-trap happens, execution lands right here:
cmp $0, %rax
jl handle_error      // Set errno if rax < 0
ret                  // Return to your C code
```

> **Fun Fact:** You never had to hand-write assembly code to invoke a system call because the authors of `libc` already wrote that hand-tuned assembly for you decades ago!

---

### The Trap Table (Interrupt Descriptor Table / IDT)

One crucial question: **How does the CPU know which code to run when a trap occurs?**

Why can't the user program simply tell the CPU: *"Hey, trap into kernel mode and jump to address `0x80045000`"*?

Because if user programs could specify the jump address, a clever attacker could jump right *past* the permission checks in the OS file system code, directly to the raw disk write routine!

**The Fix: The Trap Table**

```
At Boot Time (Kernel Mode):
  OS configures hardware Trap Table:
  ┌──────────────────────────────────────────────────────────┐
  │ Slot 0:  Divide-by-zero Handler                          │
  │ Slot 13: General Protection Fault Handler                │
  │ Slot 14: Page Fault Handler                              │
  │ Slot 32: Timer Interrupt Handler                         │
  │ Slot 128 (0x80) / SYSCALL: OS System Call Dispatcher    │
  └──────────────────────────────────────────────────────────┘
  Hardware registers (e.g., IDTR) locked with this address!
```

1. **At Boot Time:** When the machine powers on, it starts in privileged **Kernel Mode**. Before any user program is allowed to run, the OS initializes the **Trap Table** and tells the CPU hardware the exact memory addresses of its trap handlers.
2. Once set, this configuration is locked. User programs can only supply a **System Call Number**; the hardware uses the table to jump to the pre-authorized OS dispatcher. User code has zero say in where the CPU jumps.

---

## Complete Limited Direct Execution Protocol (Phase by Phase)

Here is the full lifecycle of Limited Direct Execution:

| Phase | Operating System (Kernel Mode) | Hardware | User Application (User Mode) |
|---|---|---|---|
| **Boot Phase** | 1. Initialize Trap Table (set up handler addresses).<br>2. Program hardware timer.<br>3. Hand control to hardware. | Remembers trap table location and starts timer ticking. | *(Not running yet)* |
| **Process Startup** | 4. Allocate PCB, allocate RAM.<br>5. Load binary from disk.<br>6. Set up user stack with `argc`/`argv`.<br>7. Fill kernel stack with fake trap frame (entry point `main()`).<br>8. Call **`return-from-trap`**. | Restores registers from kernel stack, switches CPU to **User Mode**, jumps to `main()`. | *(Starts executing)* |
| **Normal Execution** | *(Idle / Waiting)* | Executes instructions natively at full hardware speed. | 9. Runs code in `main()`. |
| **System Call** | *(Idle / Waiting)* | 10. `trap` instruction executed: saves registers to kernel stack, switches to **Kernel Mode**, jumps to OS trap handler. | Calls `read()` (executes `trap`). |
| **Syscall Servicing** | 11. Handles syscall request (e.g. read disk).<br>12. Calls **`return-from-trap`**. | Restores registers from kernel stack, switches to **User Mode**, jumps to PC after trap. | *(Suspended in trap)* |
| **Process Exit** | 13. Frees memory, cleans up PCB, removes from process list. | CPU remains in Kernel Mode to schedule next process. | 14. Program returns from `main()` (calls `exit()`). |

---

## Problem 2: Regaining Control & Switching Processes

Now comes the second, almost philosophical problem of CPU virtualization:

> **If a process is actively running on the CPU, by definition the OS is NOT running on the CPU.**
> 
> **If the OS is not running on the CPU, how can it do anything at all to stop the process?**

Think about that! A single CPU core can only execute one instruction at a time. If user program A is executing instructions, the OS code is literally sitting idle in RAM. How does the OS wrest control back?

Historically, there have been two approaches:

---

### Approach 1: The Cooperative Approach (A Naive Hope)

In early operating systems (like early Macintosh OS 9 or Windows 3.1), the OS relied on processes to be polite citizens.

- Processes periodically surrendered the CPU by making **system calls** (like `read()` or `write()`).
- Or they called an explicit system call like `yield()`, saying: *"I'm done for now, let someone else run."*
- What if an application misbehaved (e.g. divided by zero or accessed invalid memory)? The hardware triggered an exception, trapping into the OS, allowing the OS to terminate the offending program ("one strike and you're out!").

```
             ┌────────────────────────────────────────────────────────┐
             │ The Fatal Flaw of Cooperative Multitasking:             │
             │                                                        │
             │   int main() {                                         │
             │       while (1) {                                      │
             │           // Ha! No syscall, no yield, no exceptions!  │
             │       }                                                │
             │   }                                                    │
             └────────────────────────────────────────────────────────┘
```

If a program gets stuck in an infinite loop without making a system call, **the OS never regains control**. The entire computer freezes indefinitely.

Your only recourse? **The oldest troubleshooting tool in computer history:** reach around the back and hit the physical power button to reboot the machine.

---

### Approach 2: The Non-Cooperative Approach (Timer Interrupt)

Modern OSes do not trust user applications. Instead, they use hardware to take control by force: **The Timer Interrupt**.

```
  User Process A is running...
       │
       │  (10 ms pass)
       ▼
  ⏰ [TIMER HARDWARE INTERRUPT FIRES]
       │
       ├─> 1. Hardware forcefully pauses Process A
       ├─> 2. Hardware saves A's registers to A's KERNEL STACK
       ├─> 3. Hardware switches CPU to KERNEL MODE
       └─> 4. Hardware jumps to OS Timer Interrupt Handler in Trap Table
               │
               ▼
       🛡️ THE OS IS SUDDENLY ALIVE AND RUNNING ON THE CPU!
```

1. **Boot time setup:** During boot, the OS programs a hardware timer chip to pulse every $N$ milliseconds.
2. **Periodic interruption:** Every few milliseconds, the hardware timer sends an electrical interrupt signal to the CPU.
3. **Hardware intervention:** The CPU hardware forcefully suspends the currently running process, pushes its registers onto its kernel stack, elevates to Kernel Mode, and jumps straight into the **OS Timer Interrupt Handler**.
4. **Regained Control:** The OS is now executing on the CPU! It can decide whether to let the current process keep running or switch to a different one.

---

## Saving and Restoring Context: The Context Switch

Once the OS regains control via the timer interrupt, its **Scheduler** makes a policy decision:
*"Should we continue running Process A, or switch to Process B?"*

If the scheduler decides to switch, it executes a low-level assembly sequence known as a **Context Switch**:

```
Process A (Running)                                                     Process B (Ready)
───────────────────                                                     ─────────────────
1. Timer interrupt fires!
2. Hardware saves A's user registers
   onto A's Kernel Stack.
3. CPU enters OS timer handler.
                                ┌────────────────────────┐
                                │ OS Scheduler Decides:  │
                                │ Switch from A to B!    │
                                └───────────┬────────────┘
                                            │
4. OS saves A's kernel registers            │
   onto A's Process Control Block / Stack.  │
5. OS switches Stack Pointer (%rsp) ────────┴─────────────────────────> 6. %rsp now points to
   from A's Kernel Stack to B's Kernel Stack.                               B's Kernel Stack!
                                                                        7. OS restores B's kernel
                                                                           registers from B's stack.
                                                                        8. OS executes
                                                                           return-from-trap.
                                                                        9. Hardware restores B's user
                                                                           registers from B's stack,
                                                                           drops to User Mode,
                                                                           and resumes Process B!
```

Notice the subtle beauty here:
- **Two different sets of registers are saved during a switch:**
  1. **User registers** of the interrupted process (saved by *hardware* onto the kernel stack upon interrupt).
  2. **Kernel registers** of the OS execution context (saved by *OS software assembly* when switching between kernel threads).
- By switching the stack pointer from Process A's kernel stack to Process B's kernel stack, when the OS finally calls `return-from-trap`, the CPU pops the register state of **Process B**, completely resuming B as if nothing had ever happened!

---

## The Ultimate Summary: The Baby-Proofing Analogy 👶🏠

To wrap all these mechanisms together in one intuitive picture, think of Limited Direct Execution as **baby-proofing a house for an energetic toddler**:

```
       Baby / Toddler          ===>   User Application
       Parent                  ===>   Operating System Kernel
       The House / Furniture   ===>   Hardware (CPU, RAM, Disk)
```

1. **Direct Execution (Letting the baby walk):**
   You want your toddler to learn to explore and walk on their own two feet because it's fast and natural. If you physically carry the baby every single inch (software emulation), you'll both move at an excruciating crawl.
2. **User Mode (Baby-Proofing):**
   You don't let the toddler roam completely unchecked. You put plastic safety caps on electrical sockets and put baby gates on staircases. If the baby tries to stick a crayon into an electrical socket, the plastic shield stops them cold (**hardware privilege check fails -> exception**).
3. **System Calls & Trap (Asking for a snack):**
   The toddler can't reach the cookie jar on top of the high shelf. They have to walk up to you and say *"cookie please!"* (**system call**). You (the Parent in **Kernel Mode**) check if it's dinner time, safely retrieve the cookie from the cupboard, hand it to the child, and let them go back to playing (**return-from-trap**).
4. **The Kitchen Timer (Timer Interrupt):**
   Toddlers have zero sense of time. If you tell them *"share the toy truck when you're done"* (**cooperative multitasking**), they will play with it until they pass out on the carpet. So you set a loud **kitchen timer for 10 minutes** (**timer interrupt**).
5. **Context Switch (Switching turns):**
   When the timer rings `DING!`, you gently step in, pause toddler A, hand the toy truck to toddler B, and reset the timer!

---

## Quick Self-Check Before Next Chapter

1. What is the fundamental difference between **User Mode** and **Kernel Mode**?
2. What three actions does the CPU hardware take simultaneously when a `trap` instruction executes?
3. Why does the OS maintain a separate **Kernel Stack** for each process instead of pushing trap state onto the user stack?
4. What prevents a user process from jumping directly to arbitrary code inside the kernel?
5. Why did early **Cooperative Multitasking** fail when an application entered an infinite loop?
6. How does a **Timer Interrupt** solve the problem of the OS regaining control of the CPU?
7. During a context switch, which registers are saved by hardware, and which are saved by software?

---

*Next up: Chapter 7 — Scheduling: Introduction. Now that the OS can switch processes at will, how does it decide WHICH process to run next?*
