

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>


#define MAX_PROCESSES 128
#define TIME_SLICE 1
#define MAX_LINE 1024


pid_t pids[MAX_PROCESSES];
char *commands[MAX_PROCESSES][64];
int num_processes = 0;
int current = 0;
int alive_flags[MAX_PROCESSES];

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


void print_proc_info(pid_t pid) {
    char path[128], line[256];
    FILE *fp;


   
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    fp = fopen(path, "r");
    if (!fp) return;


    char mem[64] = "N/A", ctxt[64] = "N/A";
    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "VmRSS: %63[^\n]", mem) == 1) continue;
        if (sscanf(line, "voluntary_ctxt_switches: %63[^\n]", ctxt) == 1) continue;
    }
    fclose(fp);


   
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    fp = fopen(path, "r");
    if (!fp) return;


    unsigned long utime = 0, stime = 0;
    fscanf(fp,
        "%*d %*s %*c %*d %*d %*d %*d %*d "
        "%*u %*u %*u %*u %*u %lu %lu",
        &utime, &stime);
    fclose(fp);


    printf("%5d | %10s | %10s | utime=%lu | stime=%lu\n",
           pid, mem, ctxt, utime, stime);
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


    kill(pids[current], SIGCONT);


    printf("\n=== Scheduler Cycle Completed ===\n");
    printf(" PID  |   Memory    | ContextSw | Exec Times\n");
    for (int i = 0; i < num_processes; i++) {
        if (alive_flags[i])
            print_proc_info(pids[i]);
    }


    alarm(TIME_SLICE);
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
        }
    }


    sleep(1);
    for (int i = 0; i < num_processes; i++)
        kill(pids[i], SIGUSR1);


    sleep(1);
    for (int i = 0; i < num_processes; i++)
        kill(pids[i], SIGSTOP);


    current = 0;
    kill(pids[current], SIGCONT);
    alarm(TIME_SLICE);


   
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



