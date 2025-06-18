

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>


#define MAX_PROCESSES 128
#define MAX_LINE 1024


#define IO_BOUND_THRESHOLD 1        
#define CPU_TIME_THRESHOLD 10        
#define IO_SLICE 1                  
#define CPU_SLICE 2                  
#define DEFAULT_SLICE 1


pid_t pids[MAX_PROCESSES];
char *commands[MAX_PROCESSES][64];
int num_processes = 0;
int current = 0;
int alive_flags[MAX_PROCESSES];


int time_slices[MAX_PROCESSES];
char *process_types[MAX_PROCESSES];
int last_ctx_switches[MAX_PROCESSES];
unsigned long last_utime[MAX_PROCESSES];
unsigned long last_stime[MAX_PROCESSES];

void free_commands() {
    for (int i = 0; i < num_processes; i++) {
        int j = 0;
        while (commands[i][j] != NULL) {
            free(commands[i][j]);
            commands[i][j] = NULL;
            j++;
        }
    }
}




void parse_workload(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Failed to open workload file");
        exit(1);
    }
    char line[MAX_LINE];
    while (fgets(line, sizeof(line), fp)) {
        char *token = strtok(line, " \t\n");
        int arg_idx = 0;
        while (token) {
            commands[num_processes][arg_idx++] = strdup(token);
            token = strtok(NULL, " \t\n");
        }
        commands[num_processes][arg_idx] = NULL;
        num_processes++;
    }
    fclose(fp);
}


void analyze_proc(int index) {
    pid_t pid = pids[index];
    char path[128], line[256];
    FILE *fp;


    int ctx = 0;
    unsigned long utime = 0, stime = 0;


   
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    fp = fopen(path, "r");
    if (!fp) return;
    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "voluntary_ctxt_switches: %d", &ctx) == 1)
            break;
    }
    fclose(fp);


   
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    fp = fopen(path, "r");
    if (!fp) return;
    fscanf(fp,
        "%*d %*s %*c %*d %*d %*d %*d %*d "
        "%*u %*u %*u %*u %*u %lu %lu",
        &utime, &stime);
    fclose(fp);


    int ctx_delta = ctx - last_ctx_switches[index];
    unsigned long cpu_delta = (utime + stime) - (last_utime[index] + last_stime[index]);


    last_ctx_switches[index] = ctx;
    last_utime[index] = utime;
    last_stime[index] = stime;


    if (ctx_delta > IO_BOUND_THRESHOLD) {
        process_types[index] = "I/O-bound";
        time_slices[index] = IO_SLICE;
    } else if (cpu_delta > CPU_TIME_THRESHOLD) {
        process_types[index] = "CPU-bound";
        time_slices[index] = CPU_SLICE;
    } else {
       
        process_types[index] = "I/O-bound";
        time_slices[index] = IO_SLICE;
    }
}


void print_proc_info(int index) {
    pid_t pid = pids[index];
    char path[128], line[256];
    FILE *fp;


    char mem[64] = "N/A";
    unsigned long utime = 0, stime = 0;


   
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    fp = fopen(path, "r");
    if (!fp) return;
    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "VmRSS: %63[^\n]", mem) == 1)
            break;
    }
    fclose(fp);


   
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    fp = fopen(path, "r");
    if (!fp) return;
    fscanf(fp,
        "%*d %*s %*c %*d %*d %*d %*d %*d "
        "%*u %*u %*u %*u %*u %lu %lu",
        &utime, &stime);
    fclose(fp);


    printf("%5d | %10s | %10lu | %10lu | %4ds | %-10s\n",
           pid, mem, utime, stime, time_slices[index], process_types[index]);
}


void alarm_handler(int sig) {
    (void)sig;
    if (alive_flags[current])
        kill(pids[current], SIGSTOP);


   
    int found = 0;
    for (int i = 1; i <= num_processes; i++) {
        int next = (current + i) % num_processes;
        if (alive_flags[next]) {
            current = next;
            found = 1;
            break;
        }
    }


    if (!found) {
        alarm(0);
        return;
    }


    analyze_proc(current);
    kill(pids[current], SIGCONT);


    printf("\n=== Scheduler Cycle Completed ===\n");
    printf(" PID  |   Memory    |   utime   |   stime   | Time | Type\n");
    for (int i = 0; i < num_processes; i++) {
        if (alive_flags[i])
            print_proc_info(i);
    }


    alarm(time_slices[current]);
}


int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s workload.txt\n", argv[0]);
        exit(1);
    }


    parse_workload(argv[1]);
    signal(SIGALRM, alarm_handler);


    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigprocmask(SIG_BLOCK, &mask, NULL);


    for (int i = 0; i < num_processes; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork failed");
            exit(1);
        }
        if (pid == 0) {
            int sig;
            sigwait(&mask, &sig);
            execvp(commands[i][0], commands[i]);
            perror("exec failed");
            exit(1);
        } else {
            pids[i] = pid;
            alive_flags[i] = 1;
            last_ctx_switches[i] = 0;
            last_utime[i] = 0;
            last_stime[i] = 0;
            process_types[i] = "unknown";
            time_slices[i] = DEFAULT_SLICE;
        }
    }


    sleep(1);
    for (int i = 0; i < num_processes; i++)
        kill(pids[i], SIGUSR1);


    sleep(1);
    for (int i = 0; i < num_processes; i++)
        kill(pids[i], SIGSTOP);


    current = 0;
    analyze_proc(current);
    kill(pids[current], SIGCONT);
    alarm(time_slices[current]);


    while (1) {
        pid_t done;
        int status;
        while ((done = waitpid(-1, &status, WNOHANG)) > 0) {
            for (int i = 0; i < num_processes; i++) {
                if (pids[i] == done) {
                    alive_flags[i] = 0;
                }
            }
        }


        int still_alive = 0;
        for (int i = 0; i < num_processes; i++)
            if (alive_flags[i]) still_alive = 1;


        if (!still_alive) break;
        pause();
    }


    alarm(0);
    printf("All processes finished.\n");
    free_commands();
    return 0;
}



