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
    // TODO Task 2: Execute the specified program (token 0) with the
    // specified command-line arguments
    // THIS FUNCTION SHOULD BE CALLED FROM A CHILD OF THE MAIN SHELL PROCESS
    // Hint: Build a string array from the 'tokens' vector and pass this into execvp()
    // Another Hint: You have a guarantee of the longest possible needed array, so you
    // won't have to use malloc.

    // TODO Task 3: Extend this function to perform output redirection before exec()'ing
    // Check for '<' (redirect input), '>' (redirect output), '>>' (redirect and append output)
    // entries inside of 'tokens' (the strvec_find() function will do this for you)
    // Open the necessary file for reading (<), writing (>), or appending (>>)
    // Use dup2() to redirect stdin (<), stdout (> or >>)
    // DO NOT pass redirection operators and file names to exec()'d program
    // E.g., "ls -l > out.txt" should be exec()'d with strings "ls", "-l", NULL

    // TODO Task 4: You need to do two items of setup before exec()'ing
    // 1. Restore the signal handlers for SIGTTOU and SIGTTIN to their defaults.
    // The code in main() within swish.c sets these handlers to the SIG_IGN value.
    // Adapt this code to use sigaction() to set the handlers to the SIG_DFL value.
    // 2. Change the process group of this process (a child of the main shell).
    // Call getpid() to get its process ID then call setpgid() and use this process
    // ID as the value for the new process group ID

    return 0;
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

    //checking if the correct number of tokens is provided
    if (tokens->length < 2) {
        fprintf(stderr, "Missing job index.");
        return -1;
    }

    //parsing the job index from token 1
    int job_ind;
    if (sscanf(tokens->data[1], "%d", &job_ind) != 1) {
        fprintf(stderr, "Invalid job index\n");
        return -1;
    }

    //retrieving job from the job list
    job_t *job = job_list_get(jobs, job_ind);
    if (job == NULL) {
        fprintf(stderr, "Job index out of bounds\n");
        return -1;
    }

    //getting process group ID of job
    pid_t job_pgid = job->pid;
    //resume job in the foreground
    if (is_foreground) {
        //I need to move job's process group to forground
        if (tcsetpgrp(STDERR_FILENO, job_pgid) == -1) {
            perror("tcsetpgrp failed.");
            return -1;
        }

        //sending the sigcontinue to job's process group
        if (kill(-job_pgid, SIGCONT) < 0) {
            perror("kill (SIGCONT)");
            return -1;
        }

        //now we wait for job to terminate or stop
        int status;
        if (waitpid(job->pid, &status, WUNTRACED) == -1) {
            perror("waitpid failed.");
            return -1;
        }

        //if job has terminated check
        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            //remove job from job list if has terminated
            if (job_list_remove(jobs, job_ind) == -1) {
                fprintf(stderr, "failed to remove job from list\n");
                return -1;
            }
        }

        //restore shell's process group to foreground
        if (tcsetpgrp(STDERR_FILENO, getpgrp()) == -1) {
            perror("tcsetpgrp failed.");
            return -1;
        }
    }

    //resume job in background
    else {
        //sending sigcontinue to job's process group
        if (kill(-job_pgid, SIGCONT) < 0) {
            perror("kill (SIGCONT) failed.");
            return -1;
        }

        //update the job's status to background
        job->status = BACKGROUND;
    }


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
