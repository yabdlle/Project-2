#define _GNU_SOURCE

#include "swish_funcs.h"

#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "job_list.h"
#include "string_vector.h"

#define MAX_ARGS 10

int tokenize(char *s, strvec_t *tokens) {
    // TODO Task 0: Tokenize string s
    // Assume each token is separated by a single space (" ")
    // Use the strtok() function to accomplish this
    // Add each token to the 'tokens' parameter (a string vector)
    // Return 0 on success, -1 on error

    if (s == NULL || tokens == NULL) {
        return -1;
    }
    char *tok = strtok(s, " ");    // Split string s into spaces
    while (tok != NULL) {
        if (strvec_add(tokens, tok) != 0) {    // Add token
            perror("Failed to add token");
            return -1;
        }
        tok = strtok(NULL, " ");    // Get the next token
    }

    return 0;
}

int run_command(strvec_t *tokens) {
    if (tokens == NULL || tokens->length == 0) {
        fprintf(stderr, "run_command: No command provided\n");
        return -1;
    }

    char *argv[MAX_ARGS];
    int i, in_fd = -1, out_fd = -1;
    int argc = 0;

    for (i = 0; i < tokens->length && argc < MAX_ARGS - 1; i++) {
        char *token = strvec_get(tokens, i);

        if (strcmp(token, "<") == 0) {
            if (i + 1 < tokens->length) {
                in_fd = open(strvec_get(tokens, i + 1), O_RDONLY);
                if (in_fd == -1) {
                    perror("Failed to open input file");
                    return -1;
                }
                i++;
            } else {
                fprintf(stderr, "Missing file");
                return -1;
            }
        } else if (strcmp(token, ">") == 0) {
            if (i + 1 < tokens->length) {
                out_fd = open(strvec_get(tokens, i + 1), O_WRONLY | O_CREAT | O_TRUNC,
                              S_IRUSR | S_IWUSR);
                if (out_fd == -1) {
                    perror("Failed to open output file");
                    return -1;
                }
                i++;
            } else {
                fprintf(stderr, "Missing file");
                return -1;
            }
        } else if (strcmp(token, ">>") == 0) {
            if (i + 1 < tokens->length) {
                out_fd = open(strvec_get(tokens, i + 1),
                              O_WRONLY | O_CREAT | O_APPEND | S_IRUSR | S_IWUSR);
                if (out_fd == -1) {
                    perror("Failed to open output file");
                    return -1;
                }
                i++;
            } else {
                fprintf(stderr, "Missing file");
                return -1;
            }
        } else {
            argv[argc++] = token;
        }
    }
    argv[argc] = NULL;

    if (in_fd != -1) {
        if (dup2(in_fd, STDIN_FILENO) == -1) {
            perror("dup2 input redirection failed");
            close(in_fd);
            return -1;
        }
        close(in_fd);
    }

    if (out_fd != -1) {
        if (dup2(out_fd, STDOUT_FILENO) == -1) {
            perror("dup2 output redirection failed");
            close(out_fd);
            return -1;
        }
        close(out_fd);
    }

    execvp(argv[0], argv);
    perror("exec");
    exit(EXIT_FAILURE);
}

int resume_job(strvec_t *tokens, job_list_t *jobs, int is_foreground) {
    // TODO Task 5: Implement the ability to resume stopped jobs in the foreground
    // 1. Look up the relevant job information (in a job_t) from the jobs list
    //    using the index supplied by the user (in tokens index 1)
    //    Feel free to use sscanf() or atoi() to convert this string to an int
    // 2. Call tcsetpgrp(STDIN_FILENO, <job_pid>) where job_pid is the job's process ID
    // 3. Send the process the SIGCONT signal with the kill() system call
    // 4. Use the same waitpid() logic as in main -- don't forget WUNTRACED
    // 5. If the job has terminated (not stopped), remove it from the 'jobs' list
    // 6. Call tcsetpgrp(STDIN_FILENO, <shell_pid>). shell_pid is the *current*
    //    process's pid, since we call this function from the main shell process

    // TODO Task 6: Implement the ability to resume stopped jobs in the background.
    // This really just means omitting some of the steps used to resume a job in the foreground:
    // 1. DO NOT call tcsetpgrp() to manipulate foreground/background terminal process group
    // 2. DO NOT call waitpid() to wait on the job
    // 3. Make sure to modify the 'status' field of the relevant job list entry to BACKGROUND
    //    (as it was STOPPED before this)

    return 0;
}

int await_background_job(strvec_t *tokens, job_list_t *jobs) {
    // TODO Task 6: Wait for a specific job to stop or terminate
    // 1. Look up the relevant job information (in a job_t) from the jobs list
    //    using the index supplied by the user (in tokens index 1)
    // 2. Make sure the job's status is BACKGROUND (no sense waiting for a stopped job)
    // 3. Use waitpid() to wait for the job to terminate, as you have in resume_job() and main().
    // 4. If the process terminates (is not stopped by a signal) remove it from the jobs list

    return 0;
}

int await_all_background_jobs(job_list_t *jobs) {
    // TODO Task 6: Wait for all background jobs to stop or terminate
    // 1. Iterate through the jobs list, ignoring any stopped jobs
    // 2. For a background job, call waitpid() with WUNTRACED.
    // 3. If the job has stopped (check with WIFSTOPPED), change its
    //    status to STOPPED. If the job has terminated, do nothing until the
    //    next step (don't attempt to remove it while iterating through the list).
    // 4. Remove all background jobs (which have all just terminated) from jobs list.
    //    Use the job_list_remove_by_status() function.

    return 0;
}
