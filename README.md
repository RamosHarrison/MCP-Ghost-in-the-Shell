# MCP-Ghost-in-the-Shell
This project involves creating a simulated operating system scheduler named MCP (Master Control Program). The goal of the project is to implement a system that reads commands from a workload file, schedules them, and executes them as subprocesses. The project follows a step-by-step approach with five stages, each adding more features and complexity. The project also includes an extra part for adaptive scheduling based on workload behavior.

Features
MCP v1.0 (Part 1): Parses commands from a workload file, launches them as subprocesses using fork() and execvp(), and waits for their termination.

MCP v2.0 (Part 2): Implements signal-based process control. Uses SIGUSR1 to start processes, SIGSTOP to suspend them, and SIGCONT to resume them.

MCP v3.0 (Part 3): Implements Round Robin Scheduling using SIGALRM and alarm(). Each process is time-sliced, with processes suspended and resumed according to the round robin scheduling algorithm.

MCP v4.0 (Part 4): Adds Process Monitoring. MCP gathers real-time data from /proc/pid/status, including memory usage, CPU time, and context switches, and outputs the data after each scheduling cycle.

MCP v5.0 (Part 5): Implements Adaptive Scheduling based on I/O and CPU-bound process characteristics. Processes are dynamically categorized as CPU-bound or I/O-bound based on their usage patterns and receive different time slices.

Installation:
Prerequisites:
A Linux or Unix-like environment (macOS, WSL, etc.)
GCC compiler

Steps to Compile and Run:
Clone the repository:
git clone https://github.com/RamosHarrison/mcp-ghost-in-the-shell.git
cd mcp-ghost-in-the-shell

Compile:
gcc part1.c -o mcp_part1
gcc part2.c -o mcp_part2
gcc part3.c -o mcp_part3
gcc part4.c -o mcp_part4
gcc part5.c -o mcp_part5

Run:
./mcp_part1 < workload.txt
./mcp_part2 < workload.txt
./mcp_part3 < workload.txt
./mcp_part4 < workload.txt
./mcp_part5 < workload.txt

Workload format:
command1 arg1 arg2
command2 arg1 arg2

Usage
Part 1: Executes commands from a file by creating subprocesses using fork() and execvp().

Part 2: Adds signal-based process control, where each process is controlled using SIGUSR1, SIGSTOP, and SIGCONT.

Part 3: Implements Round Robin Scheduling. Processes are time-sliced using SIGALRM.

Part 4: Monitors process statistics (memory usage, CPU time, etc.) using /proc/pid/status.

Part 5: Implements adaptive scheduling, where processes are categorized as CPU-bound or I/O-bound, and receive different time slices based on their workload characteristics.

Performance Results
Valgrind: Memory usage was checked with Valgrind and no memory leaks were found in any part of the project.

Interjecting Messages: In Part 4 and Part 5, there were instances where the termination messages of child processes interjected with the MCP’s output due to concurrent stdout usage.

