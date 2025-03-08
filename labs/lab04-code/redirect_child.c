// redirect_child.c: starts a child process which will print into a
// file instead of onto the screen. Uses dup2(), fork(), and wait()
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {    
        printf("Usage: %s <outputfile>\n", argv[0]);
        return 1;
    }

    char *output_file = argv[1]; 
    char *child_argv[] = {"wc", "test_cases/resources/nums.txt", NULL}; 

    int file = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (file == -1) {
        perror("file error");
        return 1;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork error");
        close(file);
        return 1;
    }

    if (pid == 0) { 
        if (dup2(file, STDOUT_FILENO) == -1) {
            perror("dup2 error");
            close(file);
            return 1;
        }
        close(file); 

        execvp(child_argv[0], child_argv); 
        perror("exec failed"); 
        return 1;
    }
    
    close(file); 

    int status;
    wait(&status);

    if (WIFEXITED(status)) {
        printf("Child complete, return code %d\n", WEXITSTATUS(status));
    } else {
        printf("Child exited abnormally\n");
    }


    // Uncomment lines below to use specified output file and command-line args in child process
    // output file that child process will print into
    // char *output_file = argv[1];
    // child command/arguments to execute
    // char *child_argv[] = {"wc", "test_cases/resources/nums.txt", NULL};

    // TODO: Spawn a child process, which will exec the "wc" command with the arguments in
    // 'child_argv' Redirect the output of the command to 'output_file'

    // TODO: In the parent, wait for the child and ensure it terminated normally using wait macros
    // Print "Child complete, return code <status_code>" if child terminated normally, replacing
    //    <status_code> with the child's numerical status code
    // Print "Child exited abnormally" if child terminated abnormally

    return 0;
}
