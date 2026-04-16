# CSE 314: Operating Systems Sessional

This repository contains various implementations and assignments related to Operating System internals. The projects cover high-level synchronization problems and low-level kernel modifications using the xv6 operating system.

## 📂 Repository Structure

The assignments are divided into several "Offlines," each targeting a specific OS subsystem:

* **/Offline 1: Shell & Scripting** - Proficiency in Linux command-line environments and automated scripting.
* **/Offline 2: System Calls (xv6)** - Extending the xv6 kernel by implementing custom system calls to monitor process statistics.
* **/Offline 3: IPC (Inter-Process Communication)** - Solving classic synchronization problems using semaphores, mutexes, and condition variables.
* **/Offline 4: Threading & Scheduling** - Modifying the xv6 scheduler and implementing kernel-level threading support.
* **/Offline 5: Memory Management** - Implementing paging, swapping, and virtual memory management.

---

## 🛠️ Key Projects

### 1. xv6 System Call Implementation
Modified the **xv6-riscv** kernel to introduce new system calls.
- Implemented `trace` and `sysinfo` calls to track system behavior.
- Gained experience with kernel-space vs. user-space memory boundaries and the RISC-V syscall interface.

### 2. Inter-Process Communication (IPC)
Solved complex synchronization scenarios (e.g., Producer-Consumer, Dining Philosophers, or specific custom scenarios) using:
- **Semaphores** and **Mutexes**.
- Preventing deadlocks and ensuring thread safety in a multi-threaded environment.

### 3. Scheduling & Threading
Analyzed and modified the process scheduler in xv6.
- Implementation of scheduling algorithms like **Multi-Level Feedback Queue (MLFQ)** or **Priority Scheduling**.
- Exploration of context switching and process control blocks (PCB).

### 4. Memory Management & Paging
Deep dive into how operating systems handle memory.
- Implementation of page tables and address translation.
- Managing page faults and implementing page replacement algorithms (e.g., LRU or FIFO).

---

## 🚀 Environment & Tools
- **Operating System:** Linux (Ubuntu)
- **Emulator:** QEMU (for running xv6-riscv)
- **Language:** C, Bash Scripting
- **Compiler:** RISC-V Cross-Compiler (`riscv64-unknown-elf-gcc`)

## 👤 Author
- **Tanvir Hossain**
- **Student ID:** 2005014
- Computer Science and Engineering