#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>

#define MAX_PROCS 128

typedef struct {
    pid_t pid;
    int alive;
} procinfo_t;

procinfo_t proc_table[MAX_PROCS];
int proc_count = 0;
int current = 0;

void alarm_handler(int sig);
void child_exit_handler(int sig);

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <workload_file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    FILE *fp = fopen(argv[1], "r");
    if (!fp) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGUSR1);
    sigprocmask(SIG_BLOCK, &sigset, NULL);

    
    signal(SIGALRM, alarm_handler);
    signal(SIGCHLD, child_exit_handler);

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n")] = 0;

        char *args[32];
        int i = 0;
        char *token = strtok(line, " ");
        while (token && i < 31) {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL;

        pid_t pid = fork();
        if (pid == 0) {
            int sig;
            sigwait(&sigset, &sig);
            execvp(args[0], args);
            perror("execvp");
            exit(EXIT_FAILURE);
        } else if (pid > 0) {
            proc_table[proc_count].pid = pid;
            proc_table[proc_count].alive = 1;
            proc_count++;
        } else {
            perror("fork");
            exit(EXIT_FAILURE);
        }
    }

    fclose(fp);

    
    for (int i = 0; i < proc_count; i++) {
        kill(proc_table[i].pid, SIGUSR1);
        usleep(100000); 
        kill(proc_table[i].pid, SIGSTOP);
    }

    
    for (int i = 0; i < proc_count; i++) {
        if (proc_table[i].alive) {
            kill(proc_table[i].pid, SIGCONT);
            current = i;
            break;
        }
    }

    alarm(1); 

    
    while (1) {
        int all_done = 1;
        for (int i = 0; i < proc_count; i++) {
            if (proc_table[i].alive) {
                all_done = 0;
                break;
            }
        }
        if (all_done) break;
        pause(); 
    }

    return 0;
}

void alarm_handler(int sig) {
    (void)sig;

    if (proc_table[current].alive) {
        printf("Scheduler: Stopping PID %d\n", proc_table[current].pid);
        kill(proc_table[current].pid, SIGSTOP);
    }

    int next = current;
    do {
        next = (next + 1) % proc_count;
    } while (!proc_table[next].alive && next != current);

    if (proc_table[next].alive) {
        printf("Scheduler: Continuing PID %d\n", proc_table[next].pid);
        kill(proc_table[next].pid, SIGCONT);
        current = next;
        alarm(1);
    }
}



void child_exit_handler(int sig) {
    (void)sig;
    pid_t pid;
    while ((pid = waitpid(-1, NULL, WNOHANG)) > 0) {
        for (int i = 0; i < proc_count; i++) {
            if (proc_table[i].pid == pid) {
                proc_table[i].alive = 0;
                break;
            }
        }
    }
}
