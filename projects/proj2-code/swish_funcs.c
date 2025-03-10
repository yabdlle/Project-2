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
        strvec_add(tokens, tok);    // Add token to the tokens vector
        tok = strtok(NULL, " ");    // Get the next token
    }

    return 0;
}

#define _GNU_SOURCE

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
#include "swish_funcs.h"

#define MAX_ARGS 10

int run_command(strvec_t *tokens) {
    // Error checking for invalid input
    if (tokens == NULL || tokens->length == 0) {
        return -1;
    }

    // Create an array of arguments from the tokens (excluding redirection operators)
    char *args[MAX_ARGS + 1];    // Array for execvp arguments (one extra for NULL)
    int arg_count = 0;
    int input_redirect = -1, output_redirect = -1, append_redirect = -1;

    // Process tokens to separate arguments and handle redirection operators
    for (int i = 0; i < tokens->length && arg_count < MAX_ARGS; i++) {
        if (strcmp(strvec_get(tokens, i), "<") == 0) {
            // Input redirection operator found
            input_redirect = i + 1;
        } else if (strcmp(strvec_get(tokens, i), ">") == 0) {
            // Output redirection operator found
            output_redirect = i + 1;
        } else if (strcmp(strvec_get(tokens, i), ">>") == 0) {
            // Append output redirection operator found
            append_redirect = i + 1;
        } else {
            // Add the token to the arguments array
            args[arg_count++] = strvec_get(tokens, i);
        }
    }

    // Set the last argument to NULL for execvp()
    args[arg_count] = NULL;

    // Handle input redirection (if applicable)
    if (input_redirect != -1) {
        int input_fd = open(strvec_get(tokens, input_redirect), O_RDONLY);
        if (input_fd == -1) {
            perror("Input redirection failed");
            return -1;
        }
        if (dup2(input_fd, STDIN_FILENO) == -1) {
            perror("dup2 for input redirection failed");
            close(input_fd);
            return -1;
        }
        close(input_fd);    // We can close the original file descriptor after redirecting
    }

    // Handle output redirection (if applicable)
    if (output_redirect != -1) {
        int output_fd =
            open(strvec_get(tokens, output_redirect), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (output_fd == -1) {
            perror("Output redirection failed");
            return -1;
        }
        if (dup2(output_fd, STDOUT_FILENO) == -1) {
            perror("dup2 for output redirection failed");
            close(output_fd);
            return -1;
        }
        close(output_fd);
    }

    // Handle append redirection (if applicable)
    if (append_redirect != -1) {
        int append_fd =
            open(strvec_get(tokens, append_redirect), O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (append_fd == -1) {
            perror("Append redirection failed");
            return -1;
        }
        if (dup2(append_fd, STDOUT_FILENO) == -1) {
            perror("dup2 for append redirection failed");
            close(append_fd);
            return -1;
        }
        close(append_fd);
    }

    // Setup the signal handlers to their default (SIG_DFL) values
    struct sigaction sa;
    sa.sa_handler = SIG_DFL;    // Use the default action
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    // Restore signal handlers for SIGTTOU and SIGTTIN
    if (sigaction(SIGTTOU, &sa, NULL) == -1 || sigaction(SIGTTIN, &sa, NULL) == -1) {
        perror("sigaction failed");
        return -1;
    }

    // Set the process group of the child process to its own process group
    if (setpgid(0, 0) == -1) {
        perror("Failed to set process group");
        return -1;
    }

    // Execute the command
    if (execvp(args[0], args) == -1) {
        perror("execvp failed");
        return -1;
    }

    return 0;    // This point is never reached if execvp is successful
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
