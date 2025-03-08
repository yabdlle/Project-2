#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main() {
    // TODO Fork a child process to run the command "cat sample.txt"
    // The parent should wait and print out the child's exit code
    pid_t child_pid = fork();

    if (child_pid == 0){
        printf("Child Process\n");
        execlp("cat", "cat", "sample.txt", NULL);
    } else if (child_pid > 0){
        int status;
        wait(&status);
        printf("Child exited with status %d\n", WEXITSTATUS(status));
    } else {
        return 1;
    }
    printf("Child exited with status %d\n", -1);
    return 0;
}
