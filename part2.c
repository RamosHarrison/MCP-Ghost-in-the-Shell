#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_PROCS 128

void signaler(pid_t* pid_array, int count, int signal);

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

    char line[256];
    pid_t pid_array[MAX_PROCS];
    int proc_count = 0;

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
            
            printf("Child Process: %d - Waiting for SIGUSR1…\n", getpid());

            int sig;
            sigwait(&sigset, &sig);

            printf("Child Process: %d - Received signal: SIGUSR1 - Calling exec().\n", getpid());
            execvp(args[0], args);
            perror("execvp"); 
            exit(EXIT_FAILURE);
        } else if (pid > 0) {
            
            pid_array[proc_count++] = pid;
        } else {
            perror("fork");
            exit(EXIT_FAILURE);
        }
    }

    fclose(fp);

    
    signaler(pid_array, proc_count, SIGUSR1);
    sleep(1);
    signaler(pid_array, proc_count, SIGSTOP);
    sleep(1);
    signaler(pid_array, proc_count, SIGCONT);

    
    for (int i = 0; i < proc_count; i++) {
        waitpid(pid_array[i], NULL, 0);
    }

    return 0;
}

void signaler(pid_t* pid_array, int count, int signal) {
    for (int i = 0; i < count; i++) {
        sleep(1);
        printf("Parent process: %d - Sending signal: %s to child process: %d\n",
               getpid(), strsignal(signal), pid_array[i]);
        kill(pid_array[i], signal);
    }
}
