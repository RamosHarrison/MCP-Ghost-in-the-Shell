#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_LINE 256
#define MAX_ARGS 32

void parse_line(char* line, char** args) {
    int i = 0;
    args[i] = strtok(line, " \t\n");
    while (args[i] && i < MAX_ARGS - 1) {
        args[++i] = strtok(NULL, " \t\n");
    }
    args[i] = NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <workload_file>\n", argv[0]);
        return 1;
    }

    FILE *file = fopen(argv[1], "r");
    if (!file) {
        perror("fopen");
        return 1;
    }

    char line[MAX_LINE];
    pid_t pid;
    while (fgets(line, sizeof(line), file)) {
        char *args[MAX_ARGS];
        parse_line(line, args);

        pid = fork();
        if (pid == 0) {
            execvp(args[0], args);
            perror("execvp");
            exit(EXIT_FAILURE);
        } else if (pid < 0) {
            perror("fork");
        }
    }

    fclose(file);

    while (wait(NULL) > 0);
    return 0;
}
